var ptX,ptY;
var ptX_drag,ptY_drag;
var timerID_key=0;
var timerID_click=0;
var timerID_move=0;
var txtKeyEvent="";
// Double-click detection state.  msdown records the time of each left-button
// press; when a second press arrives within the double-click window we set
// isDblClickSecond (currently unused — kept for future reference).
var lastMousedownTime=0;
var isDblClickSecond=false;
// Unused since button events are now forwarded immediately in msdown/msup;
// kept to avoid reference errors in any stale timer callbacks.
var wasDrag=false;
// Tracks whether the left mouse button is currently held down.  Set in msdown,
// cleared in msup / _dragUp.  Used to attach document-level mousemove/mouseup
// listeners that continue tracking the cursor even after it leaves the canvas,
// preventing lost drag movement and stuck-button-down on the remote host.
var isLeftDown=false;
// Position of the most recent left-button mousedown, used to decide whether a
// subsequent mousedown within the double-click time window is at the same spot
// (a true double-click) or at a different location (two separate single clicks,
// e.g. selecting different files in Explorer).
var ptX_last_down, ptY_last_down;
// Unused since click events are no longer batched via a timer; kept to avoid
// reference errors in cleanup code inside msup.
var pendingClickParam = null;
// Modifier state (Ctrl/Shift/Alt bits) captured at the most recent left-button
// mousedown.  Used when sending the immediate button-down (act=3) from msdown.
var lastDownAltk = 0;

// Tracks which modifier keys are currently being held as part of key combinations.
// Set when a non-modifier keydown event fires with a modifier held; cleared when
// the modifier's keyup event is sent.  Used to distinguish "held modifier released"
// (sends 0x0800|kc so the server injects only a VK-up) from "bare modifier tap"
// (e.g. Alt to focus the menu bar, sends the normal altk*256+kc encoding).
var heldModifiers = 0;
// True when running on Firefox, which doesn't fire 'beforeinstallprompt' natively.
// PWAsForFirefox (https://github.com/filips423/PWAsForFirefox) adds that support once installed.
var _isFirefox = /Firefox\//.test(navigator.userAgent) && !/Chrome\//.test(navigator.userAgent);

// Dedicated XHR for keyboard events so they don't conflict with pending mouse requests
var xmlHttpKey = false;
function getKeyXHR()
{
if(!xmlHttpKey)
{
if(window.XMLHttpRequest) xmlHttpKey = new XMLHttpRequest();
else if(window.ActiveXObject) xmlHttpKey = new ActiveXObject("Microsoft.XMLHTTP");
}
return xmlHttpKey;
}

// Serial queue for button events (down/up).
// Using a separate fresh XHR per event was the original approach to avoid one
// event aborting another, but it allows out-of-order delivery: if act=3
// (button-down) travels on a new TCP connection that needs a handshake while
// act=6 (button-up) reuses a warm connection, act=6 arrives first.  The server
// then processes button-up on an un-pressed button (no-op) and button-down
// second, leaving the remote host with a stuck LEFTDOWN — unintended dragging.
// The queue serialises sends so each XHR completes before the next is started,
// guaranteeing the arrival order matches the dispatch order, while still using a
// fresh XHR object per event so no event is ever silently dropped.
var _buttonQueue = [];
var _buttonSending = false;
function _sendNextButtonEvent()
{
if(_buttonSending || _buttonQueue.length === 0) return;
_buttonSending = true;
var param = _buttonQueue.shift();
var xhr;
if(window.XMLHttpRequest) xhr = new XMLHttpRequest();
else if(window.ActiveXObject) xhr = new ActiveXObject("Microsoft.XMLHTTP");
if(!xhr) { _buttonSending = false; return; }
xhr.open("POST", "/msevent", true);
xhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
// Safety timeout: if the browser abandons the XHR (e.g. the PWA is
// backgrounded and the connection is suspended), readyState 4 may never
// fire.  After 5 s we treat the request as done and drain the queue so
// subsequent button events are not permanently blocked.
var _btnSafetyTO = setTimeout(function() {
try { xhr.abort(); } catch(e) {}
_buttonSending = false;
_sendNextButtonEvent();
}, 5000);
xhr.onreadystatechange = function() {
if(xhr.readyState === 4) { clearTimeout(_btnSafetyTO); _buttonSending = false; _sendNextButtonEvent(); }
};
xhr.send(param);
}
function sendButtonEvent(param)
{
_buttonQueue.push(param);
_sendNextButtonEvent();
}

// Detect Internet Explorer (pre-Edge) which uses different button/event conventions
var isIE = !!(window.ActiveXObject || (navigator.userAgent.indexOf("Trident") !== -1));

// Normalize mouse button to server-expected values: 1=left, 2=right, 4=middle
// IE already uses these values; modern browsers use 0=left, 1=middle, 2=right
function normalizeButton(b)
{
if(isIE) return b;
if(b===0) return 1;
if(b===1) return 4;
if(b===2) return 2;
return b;
}

// Return true if the mouse event button represents the left button
function isLeftButton(b)
{
return isIE ? (b===1) : (b===0);
}

// Scale display coordinates back to actual screen coordinates when the canvas
// is CSS-resized.  Uses data-nw/data-nh (set by the diff-stream renderer) for
// the logical content size, and getBoundingClientRect() for the rendered size.
function scaleToScreen(x, y)
{
var canvas=document.getElementById("screenimage");
var rect=canvas.getBoundingClientRect();
var rw=rect.width||canvas.clientWidth;
var rh=rect.height||canvas.clientHeight;
var nw=parseInt(canvas.getAttribute("data-nw")||"0")||canvas.width;
var nh=parseInt(canvas.getAttribute("data-nh")||"0")||canvas.height;
if(nw && rw && nw !== rw)
{
x=Math.round(x * nw / rw);
y=Math.round(y * nh / rh);
}
return {x:x, y:y};
}

// --- Cursor sync ---
// Fetch the current server cursor shape and apply it to the screen image element.
// The /getCursor endpoint returns JSON {"cursor":"text"} with a CSS cursor name.
// Only one request is allowed in-flight at a time: if the previous poll has not
// yet completed the new one is skipped, ensuring fetchCursor never holds more
// than one browser HTTP connection slot.
var _cursorPending = false;
function fetchCursor()
{
if(_cursorPending) return;
var xhr;
if(window.XMLHttpRequest) xhr=new XMLHttpRequest();
else if(window.ActiveXObject) xhr=new ActiveXObject("Microsoft.XMLHTTP");
if(!xhr) return;
_cursorPending = true;
xhr.open("GET","/getCursor",true);
xhr.onreadystatechange=function(){
if(xhr.readyState===4)
{
_cursorPending = false;
if(xhr.status===200)
{
try{
var data=JSON.parse(xhr.responseText);
if(data && data.cursor)
{
var img=document.getElementById("screenimage");
if(img) img.style.cursor=data.cursor;
}
}catch(e){}
}
}
};
xhr.send();
}

// Compute the mouse position relative to the screen image using getBoundingClientRect()
// so the result is accurate regardless of scroll, CSS transforms, or frame nesting.
function msPosition(e) 
{ 
var img=document.getElementById("screenimage");
var rect=img.getBoundingClientRect();
var cx=(e.clientX !== undefined ? e.clientX : e.x);
var cy=(e.clientY !== undefined ? e.clientY : e.y);
ptX=cx-Math.round(rect.left);
ptY=cy-Math.round(rect.top);
var coords=scaleToScreen(ptX,ptY);
ptX=coords.x; ptY=coords.y;
document.getElementById("txtHide").focus();
}

function window_onload()
{
if(!xmlHttp) createXMLHttpRequest();
document.getElementById("txtHide").focus();
// Start the binary diff stream (falls back to BMP polling on older browsers)
startScreenStream();
// Poll server cursor shape every 200 ms so the client cursor stays in sync
fetchCursor();
window.setInterval(fetchCursor, 200);
// Resume audio in the left panel when the user interacts with the screen.
// The AudioContext lives in frmLeft (left.htm), but after navigating here
// all user gestures occur in this frame.  Browsers do not propagate user
// activation across sibling frames, so we forward the gesture explicitly.
document.addEventListener('click', function() {
try {
if (parent && parent.frmLeft && typeof parent.frmLeft.tryResumeAudio === 'function')
parent.frmLeft.tryResumeAudio();
} catch(e) {}
}, false);
// If the browser window loses focus while the left button is held (e.g. user
// Alt-Tabs or clicks outside the browser), we will never receive the mouseup.
// Send a button-up immediately so the remote host does not keep dragging.
window.addEventListener('blur', function() {
if(!isLeftDown) return;
document.removeEventListener('mousemove', _dragMove, true);
document.removeEventListener('mouseup', _dragUp, true);
isLeftDown=false;
if(timerID_move!=0){window.clearTimeout(timerID_move);timerID_move=0;}
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk=0&button=1&act=6");
}, false);
// Listen for the browser's PWA install prompt so the Install button can be shown.
window.addEventListener('beforeinstallprompt', function(e) {
e.preventDefault();
_installPrompt = e;
var btn = document.getElementById("btnInstall");
if(btn) btn.style.display = '';
console.log("[viewCtrl] Install prompt available");
});
// Hide the Install button once the app has been installed.
window.addEventListener('appinstalled', function() {
_installPrompt = null;
var btn = document.getElementById("btnInstall");
if(btn) btn.style.display = 'none';
console.log("[viewCtrl] App installed");
});
// On Firefox, beforeinstallprompt never fires unless PWAsForFirefox is installed.
// Show the button immediately so users can follow the link to install the extension.
if(_isFirefox) {
var btn = document.getElementById("btnInstall");
if(btn) btn.style.display = '';
}
// When a PWA (or any browser) comes back to the foreground after being
// backgrounded, in-flight XHRs may have been silently abandoned by the
// browser without ever reaching readyState 4.  This leaves _buttonSending
// permanently true, blocking all subsequent button events (the "hang").
// On visibility restore we reset the button-queue state so input becomes
// responsive again.  The screen stream is reconnected by the listener in
// common.js startScreenStream().
document.addEventListener('visibilitychange', function() {
if(document.visibilityState === 'visible') {
_buttonSending = false;
_buttonQueue = [];
}
}, false);
// Chrome's Page Lifecycle API: when Chrome freezes the page (standby/idle
// mode triggered by its "this page is slowing down the browser" warning),
// any in-flight button XHR will be abandoned.  Reset the queue state on
// freeze so that when the page resumes, input is immediately responsive
// without waiting for the 5 s safety timeout to expire.
document.addEventListener('freeze', function() {
_buttonSending = false;
_buttonQueue = [];
}, false);
}

// Stored deferred BeforeInstallPromptEvent, set by the 'beforeinstallprompt' listener.
var _installPrompt = null;

// Show the browser's "Add to Home Screen / Install app" prompt.
// On Firefox without PWAsForFirefox installed, opens the extension's page instead.
function installAsApp()
{
if(_installPrompt) {
_installPrompt.prompt();
_installPrompt.userChoice.then(function(result) {
console.log("[viewCtrl] Install prompt result: " + result.outcome);
_installPrompt = null;
var btn = document.getElementById("btnInstall");
if(btn) btn.style.display = 'none';
});
} else if(_isFirefox) {
window.open('https://addons.mozilla.org/firefox/addon/pwas-for-firefox/', '_blank');
}
}

function processRequest() 
{
// Event sent; the binary diff stream updates the display automatically
}
function sendEvent(strurl,param)
{
if(timerID_move!=0)
{
window.clearTimeout(timerID_move);
timerID_move=0;
}
xmlHttp.open("POST", strurl, true);
xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
xmlHttp.onreadystatechange = processRequest;
xmlHttp.send(param);
}

function msmove(e)
{
e=e||window.event;
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
// Capture position and modifier state now (they are current at mousemove time),
// but evaluate isLeftDown lazily inside the timer callback so that if the
// button is released before the 50 ms debounce fires the server receives
// button=0.  This lets the server-side stuck-button sync (s_injectedMouseButtons)
// detect and release any LEFTDOWN that was injected out-of-order and heal the
// phantom-drag state within the same session — no page reload required.
var paramBase="x="+ptX+"&y="+ptY+"&altk="+altk+"&act=0";
if(timerID_move!=0)
{
window.clearTimeout(timerID_move);
timerID_move=0;
}
timerID_move=window.setTimeout(function(){ sendEvent("/msevent",paramBase+"&button="+(isLeftDown?1:0)); },50);
}

function msclick(e)
{
// Left-button down and up are now forwarded immediately from msdown and msup
// respectively, so no separate click event needs to be sent from here.
// The remote OS registers a click when it receives the button-down followed
// by the button-up that was already delivered.
}
function msdblclick(e)
{
// Double-click is recognised by the remote OS when it sees two consecutive
// button-down/button-up sequences within its double-click time window.
// Both sequences are already sent via msdown/msup, so nothing extra is
// needed here.
}

// Document-level handlers added when the left button goes down on the canvas so that
// mouse movement and button-release are tracked even after the cursor leaves the canvas.
// Without these, a drag that moves outside the canvas would stop sending move events
// and the remote host would never receive the button-up, leaving its left button stuck.
function _dragMove(e)
{
// If the browser missed the mouseup (e.g. button released outside the window),
// e.buttons will be 0 even though isLeftDown is still true.  Detect this and
// synthesize a button-up so the remote host doesn't keep dragging.
if(isLeftDown && e.buttons !== undefined && !(e.buttons & 1))
{
document.removeEventListener('mousemove', _dragMove, true);
document.removeEventListener('mouseup', _dragUp, true);
isLeftDown=false;
msPosition(e);
var altk2=(e.ctrlKey?1:0)|(e.shiftKey?2:0)|(e.altKey?4:0);
if(timerID_move!=0){window.clearTimeout(timerID_move);timerID_move=0;}
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+altk2+"&button=1&act=6");
return;
}
msmove(e);
}
function _dragUp(e)
{
if(!isLeftButton(e.button)) return;
// Always remove the document listeners regardless of isLeftDown state to ensure clean-up.
document.removeEventListener('mousemove', _dragMove, true);
document.removeEventListener('mouseup', _dragUp, true);
if(!isLeftDown) return; // defensive guard: listeners are removed on first call so this is unreachable in normal usage
isLeftDown=false;
msPosition(e);
var altk=(e.ctrlKey?1:0)|(e.shiftKey?2:0)|(e.altKey?4:0);
if(timerID_move!=0){window.clearTimeout(timerID_move);timerID_move=0;}
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=6");
}

// Get mouse down event, record drag start point. Prevents browser native drag/text-selection.
// For all buttons a button-down event (act=3) is forwarded immediately to the server.
// Left-button button-up is sent from msup; the remote OS detects double-clicks when it
// receives two button-down/up sequences within its double-click time window.
// Each button event uses a fresh XHR (sendButtonEvent) so that the two down/up pairs
// of a double-click are never aborted by reusing the same XHR object.
function msdown(e)
{
e=e||window.event;
if(isLeftButton(e.button))
{
msPosition(e);
ptX_drag=ptX;
ptY_drag=ptY;
wasDrag=false;
lastDownAltk=(e.ctrlKey?1:0)|(e.shiftKey?2:0)|(e.altKey?4:0);
var now=Date.now ? Date.now() : new Date().getTime();
if(lastMousedownTime>0 && (now-lastMousedownTime)<500)
{
// Only treat as a double-click when the second press is at
// approximately the same position as the first (within the
// standard Windows double-click area of ~4 pixels).  If the
// two presses are far apart the user is clicking on different
// items (e.g. different files in Explorer) and each should
// be treated as an independent single click.
var ddx=ptX-(ptX_last_down||ptX), ddy=ptY-(ptY_last_down||ptY);
if((ddx*ddx+ddy*ddy)<=16)
{
isDblClickSecond=true;
// Cancel any pending single-click timer so it cannot fire before
// ondblclick (or the fallback in msclick) handles the double-click.
// Also clear pendingClickParam so the first click of the double-click
// is not replayed when the double-click event arrives.
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
}
else
{
isDblClickSecond=false;
}
}
else
{
isDblClickSecond=false;
}
ptX_last_down=ptX;
ptY_last_down=ptY;
lastMousedownTime=now;
// Send left button-down immediately so that holding the button is felt on
// the remote host (e.g. scrollbar buttons, game input, UI hold-to-repeat).
// Button-up is sent from msup; msclick/msdblclick no longer send click
// events for the left button.
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+lastDownAltk+"&button=1&act=3");
// Promote mousemove and mouseup to document-level capture so that drag and
// button-release are tracked even after the cursor leaves the canvas element.
isLeftDown=true;
document.addEventListener('mousemove', _dragMove, true);
document.addEventListener('mouseup', _dragUp, true);
if(e.preventDefault) e.preventDefault();
}
else
{
// Right or middle button: send a button-down event immediately so the server
// injects WM_RBUTTONDOWN before the WM_RBUTTONUP that follows on mouseup.
// This is necessary for context menus on the Windows 7 taskbar to appear.
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var serverBtn=normalizeButton(e.button);
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+serverBtn+"&act=3");
}
}
// Handle mouse up: forward button-up for all buttons.
// The matching button-down was already sent via sendButtonEvent in msdown for
// all buttons (left, right, middle), giving the remote OS time to process
// WM_LBUTTONDOWN/WM_RBUTTONDOWN before WM_LBUTTONUP/WM_RBUTTONUP arrives.
// Cursor movement while holding the button (drag) is conveyed by the msmove
// events already dispatched; no separate act=4 drag event is needed.
function msup(e)
{
e=e||window.event;
var b=e.button;
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
// Cancel any pending mouse-move debounce timer so the post-click cursor-move
// event does not consume an extra browser HTTP connection slot after button-up.
if(timerID_move!=0){window.clearTimeout(timerID_move);timerID_move=0;}
if(isLeftButton(b))
{
// Remove document-level drag listeners; _dragUp may have already removed them
// (no-op if already gone) to ensure clean-up regardless of where mouseup fired.
document.removeEventListener('mousemove', _dragMove, true);
document.removeEventListener('mouseup', _dragUp, true);
// Cancel any pending click timer left over from earlier code paths.
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
// Guard: _dragUp (document capture phase, fires first) may have already sent
// the button-up and cleared isLeftDown; only send if we are still the first handler.
if(!isLeftDown) return;
isLeftDown=false;
// Send button-up; button-down was already sent from msdown.
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=6");
return;
}
var serverBtn=normalizeButton(b);
sendButtonEvent("x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+serverBtn+"&act=6");
}

function mswheel(e)
{
e=e||window.event;
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var wheelDelta=(e.wheelDelta !== undefined) ? e.wheelDelta : -e.deltaY;
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=0&act=5&wheel="+wheelDelta;
sendEvent("/msevent",param);
if(e.preventDefault) e.preventDefault();
e.returnValue=false;
return false;
}

function Kevent()
{
var param=txtKeyEvent;
txtKeyEvent="";
if(param=="")
{
if(timerID_key!=0)
window.clearInterval(timerID_key);
timerID_key=0;
}
else
{
var kxhr=getKeyXHR();
if(kxhr)
{
kxhr.open("POST", "/keyevent", true);
kxhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
kxhr.onreadystatechange=function(){
if(kxhr.readyState===4 && kxhr.status!==200)
console.log("[viewCtrl] keyEvent XHR error: status="+kxhr.status);
};
kxhr.send("vkey="+param);
}
else
{
console.log("[viewCtrl] keyEvent: Could not create XHR object");
}
}
}


function keydown(e)
{
e=e||window.event;
var kc=e.keyCode||e.which;
// Do not call preventDefault() for standalone modifier-key presses
// (Shift=16, Ctrl=17, Alt=18, Meta/Search=91/92/93).  On Chrome on ChromeOS,
// doing so suppresses the browser's own modifier-state tracking so that
// e.shiftKey / e.ctrlKey are reported as false on subsequent mouse events,
// breaking Shift/Ctrl+Click multi-selection in Windows Explorer.
// Modifier keys are forwarded from keyup instead so that bare modifier
// presses (e.g. Alt to focus the menu bar) are still sent to the server.
if(kc===16||kc===17||kc===18||kc===91||kc===92||kc===93) return false;
// Send the key event on keydown rather than keyup.  The browser fires
// keydown repeatedly while a key is held (auto-repeat), so sending here
// means a held key will trigger repeated keystrokes on the remote host.
var altk=0;
if(e.ctrlKey) { altk=altk | 1; heldModifiers=heldModifiers | 1; }
if(e.shiftKey) { altk=altk | 2; heldModifiers=heldModifiers | 2; }
if(e.altKey) { altk=altk | 4; heldModifiers=heldModifiers | 4; }
var kevent=altk*256+kc;
txtKeyEvent=txtKeyEvent+kevent+",";
if(timerID_key==0)
timerID_key=window.setInterval(Kevent,50);
if(e.preventDefault) e.preventDefault();
return false;
}

function keyup(e)
{
e=e||window.event;
var kc=e.keyCode||e.which;
// Non-modifier keys are sent on keydown (above) to support auto-repeat.
// Only standalone modifier-key events are forwarded here so that
// applications that respond to bare modifier presses (e.g. Alt to focus
// the Windows menu bar) continue to work correctly.
if(kc===16||kc===17||kc===18||kc===91||kc===92||kc===93)
{
// Determine whether this modifier was actually held as part of key
// combinations during this press (tracked in heldModifiers).  If so,
// send the special 0x0800 flag so the server injects only a VK key-up
// (keeping the modifier persistently held until now) rather than a
// press+release (bare-tap) sequence that would reset the selection
// anchor in apps like WinDbg.
var modBit=(kc===17?1:kc===16?2:kc===18?4:0);
var kevent;
if(modBit && (heldModifiers & modBit))
{
// Held modifier release: signal server to inject VK-up only.
kevent=0x0800|kc;
heldModifiers=heldModifiers & ~modBit;
}
else
{
// Bare modifier tap (e.g. Alt alone to open menu bar).
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
kevent=altk*256+kc;
}
txtKeyEvent=txtKeyEvent+kevent+",";
if(timerID_key==0)
timerID_key=window.setInterval(Kevent,50);
}
if(e.preventDefault) e.preventDefault();
return false;
}
