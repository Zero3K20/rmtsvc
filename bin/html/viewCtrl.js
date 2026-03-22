var ptX,ptY;
var ptX_drag,ptY_drag;
var timerID_key=0;
var timerID_click=0;
var timerID_move=0;
var txtKeyEvent="";
// Double-click detection state.  msdown records the time of each left-button
// press; when a second press arrives within the double-click window we set
// isDblClickSecond so that msclick can send act=2 even if the browser does not
// fire ondblclick (which some browsers suppress when mousedown is preventDefault-ed).
var lastMousedownTime=0;
var isDblClickSecond=false;
// Set to true by msup when a drag event is sent so that msclick can suppress
// the spurious click event that browsers fire after a mouse drag, which would
// otherwise deselect the text that was just selected on the remote PC.
var wasDrag=false;
// Position of the most recent left-button mousedown, used to decide whether a
// subsequent mousedown within the double-click time window is at the same spot
// (a true double-click) or at a different location (two separate single clicks,
// e.g. selecting different files in Explorer).
var ptX_last_down, ptY_last_down;

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
var param="x="+ptX+"&y="+ptY+"&altk=0&button=0&act=0";
if(timerID_move!=0)
{
window.clearTimeout(timerID_move);
timerID_move=0;
}
timerID_move=window.setTimeout(function(){ sendEvent("/msevent",param); },50);
}

function msclick(e)
{
e=e||window.event;
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
// Suppress the spurious click that browsers fire after a mouse drag.  msup
// already sent the drag event and text is selected on the remote PC; sending
// another click here would immediately deselect that text.
if(wasDrag)
{
wasDrag=false;
return;
}
if(isDblClickSecond)
{
// This click event is the second of a double-click sequence detected in msdown.
// Set a short fallback timer to send act=2 in case the browser does not fire
// ondblclick (e.g. because mousedown's preventDefault suppressed it).
// msdblclick will cancel this timer and send act=2 directly if it does fire.
isDblClickSecond=false;
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
if(timerID_move!=0){window.clearTimeout(timerID_move);timerID_move=0;}
var dblParam="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=2";
timerID_click=window.setTimeout(function(){timerID_click=0;sendEvent("/msevent",dblParam);},50);
return;
}
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=1";
if(timerID_click!=0) window.clearTimeout(timerID_click);
if(timerID_move!=0) window.clearTimeout(timerID_move);
timerID_click=window.setTimeout(function(){ sendEvent("/msevent",param); },200);
}
function msdblclick(e)
{
// The browser fires ondblclick whenever two clicks on the same element occur
// within the system double-click time, regardless of position.  When the user
// clicks on two different items (e.g. two different files in Explorer) within
// that window isDblClickSecond will be false because msdown detected the two
// presses were at different positions.  In that case do not send act=2; let
// the pending single-click timer from msclick fire instead.
if(!isDblClickSecond)
return;
isDblClickSecond=false;
if(timerID_click!=0)
{
window.clearTimeout(timerID_click);
timerID_click=0;
}
e=e||window.event;
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=2";
sendEvent("/msevent",param);
}

// Get mouse down event, record drag start point. Prevents browser native drag/text-selection.
// Also performs double-click detection: if two left-button presses arrive within 500 ms we
// set isDblClickSecond=true and cancel any pending single-click timer immediately, so that
// msclick can send act=2 as a fallback even when the browser suppresses ondblclick.
function msdown(e)
{
e=e||window.event;
if(isLeftButton(e.button))
{
msPosition(e);
ptX_drag=ptX;
ptY_drag=ptY;
wasDrag=false;
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
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
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
}
if(e.preventDefault) e.preventDefault();
}
// Handle mouse up: detect left-button drag vs click; forward right/middle button clicks
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
var dx=ptX-ptX_drag, dy=ptY-ptY_drag;
if(ptX_drag!==undefined && (dx*dx+dy*dy)>4)
{
// Mouse moved enough to be a drag — send drag event instead of click.
// Set wasDrag so that the subsequent browser click event (which some
// browsers fire even after a mouse drag) is ignored in msclick and does
// not send a spurious single-click that would deselect the remote text.
// Also cancel any pending click timer: a drag replaces a pending click.
wasDrag=true;
if(timerID_click!=0){window.clearTimeout(timerID_click);timerID_click=0;}
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=4&dragx="+ptX_drag+"&dragy="+ptY_drag;
sendEvent("/msevent",param);
}
return;
}
var serverBtn=normalizeButton(b);
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+serverBtn+"&act=1";
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
if(e.preventDefault) e.preventDefault();
return false;
}

function keyup(e)
{
e=e||window.event;
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var kevent=altk*256+(e.keyCode||e.which);
console.log("[viewCtrl] keyup: keyCode="+(e.keyCode||e.which)+" altk="+altk+" kevent="+kevent);
txtKeyEvent=txtKeyEvent+kevent+",";
if(timerID_key==0)
timerID_key=window.setInterval(Kevent,50);
if(e.preventDefault) e.preventDefault();
return false;
}
