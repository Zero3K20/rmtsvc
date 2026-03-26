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

// Dedicated XHR for right/middle button-down events so the down request is not
// aborted by the subsequent button-up request which reuses the shared xmlHttp object.
var xmlHttpButtonDown = false;
function getButtonDownXHR()
{
if(!xmlHttpButtonDown)
{
if(window.XMLHttpRequest) xmlHttpButtonDown = new XMLHttpRequest();
else if(window.ActiveXObject) xmlHttpButtonDown = new ActiveXObject("Microsoft.XMLHTTP");
}
return xmlHttpButtonDown;
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
function fetchCursor()
{
var xhr;
if(window.XMLHttpRequest) xhr=new XMLHttpRequest();
else if(window.ActiveXObject) xhr=new ActiveXObject("Microsoft.XMLHTTP");
if(!xhr) return;
xhr.open("GET","/getCursor",true);
xhr.onreadystatechange=function(){
if(xhr.readyState===4 && xhr.status===200)
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
console.log("[viewCtrl] sendEvent: "+strurl+" "+param);
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
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=0&act=0";
if(timerID_move!=0)
{
window.clearTimeout(timerID_move);
timerID_move=0;
}
timerID_move=window.setTimeout(function(){ sendEvent("/msevent",param); },50);
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

// Get mouse down event, record drag start point. Prevents browser native drag/text-selection.
// For all buttons a button-down event (act=3) is forwarded immediately to the server.
// Left-button button-up is sent from msup; the remote OS detects double-clicks when it
// receives two button-down/up sequences within its double-click time window.
// Right/middle button-down uses a dedicated XHR so the down request is not
// aborted by the subsequent button-up request (required for context menus on the
// Windows 7 taskbar to appear).
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
var bdxhr=getButtonDownXHR();
if(bdxhr)
{
bdxhr.open("POST", "/msevent", true);
bdxhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
bdxhr.send("x="+ptX+"&y="+ptY+"&altk="+lastDownAltk+"&button=1&act=3");
}
if(e.preventDefault) e.preventDefault();
}
else
{
// Right or middle button: send a button-down event immediately so the server
// injects WM_RBUTTONDOWN before the WM_RBUTTONUP that follows on mouseup.
// This is necessary for context menus on the Windows 7 taskbar to appear.
// A dedicated XHR is used so this request is not aborted by the msup request.
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var serverBtn=normalizeButton(e.button);
var bdxhr=getButtonDownXHR();
if(bdxhr)
{
bdxhr.open("POST", "/msevent", true);
bdxhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
bdxhr.send("x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+serverBtn+"&act=3");
}
}
}
// Handle mouse up: forward button-up for all buttons.
// The matching button-down was already sent via a dedicated XHR in msdown for
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
if(isLeftButton(b))
{
// Cancel any pending click timer left over from earlier code paths.
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
pendingClickParam=null;
// Send button-up; button-down was already sent from msdown.
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=6";
sendEvent("/msevent",param);
return;
}
var serverBtn=normalizeButton(b);
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+serverBtn+"&act=6";
sendEvent("/msevent",param);
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
console.log("[viewCtrl] keyevent: vkey="+param);
kxhr.open("POST", "/keyevent", true);
kxhr.setRequestHeader("Content-Type","application/x-www-form-urlencoded");
kxhr.onreadystatechange=function(){
if(kxhr.readyState===4 && kxhr.status!==200)
console.log("[viewCtrl] keyevent XHR error: status="+kxhr.status);
};
kxhr.send("vkey="+param);
}
else
{
console.log("[viewCtrl] keyevent: could not create XHR object");
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
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var kevent=altk*256+kc;
console.log("[viewCtrl] keydown: keyCode="+kc+" altk="+altk+" kevent="+kevent);
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
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var kevent=altk*256+kc;
console.log("[viewCtrl] keyup (modifier): keyCode="+kc+" altk="+altk+" kevent="+kevent);
txtKeyEvent=txtKeyEvent+kevent+",";
if(timerID_key==0)
timerID_key=window.setInterval(Kevent,50);
}
if(e.preventDefault) e.preventDefault();
return false;
}
