var ptX,ptY;
var ptX_drag,ptY_drag;
var timerID_key=0;
var timerID_click=0;
var timerID_move=0;
var txtKeyEvent="";

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
var w=window.parent.frmLeft;
w.document.getElementById("lblXY").innerText="X:"+ptX+" , Y:"+ptY;
document.getElementById("txtHide").focus();
}

function window_onload()
{
if(!xmlHttp) createXMLHttpRequest();
document.getElementById("txtHide").focus();
// Start the binary diff stream (falls back to JPEG polling on older browsers)
startScreenStream();
// Poll server cursor shape every 200 ms so the client cursor stays in sync
fetchCursor();
window.setInterval(fetchCursor, 200);
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
if(timerID_click!=0)
{
window.clearTimeout(timerID_click);
timerID_click=0;
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
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=1";
if(timerID_click!=0) window.clearTimeout(timerID_click);
if(timerID_move!=0) window.clearTimeout(timerID_move);
timerID_click=window.setTimeout(function(){ sendEvent("/msevent",param); },200);
}
function msdblclick(e)
{
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
function msdown(e)
{
e=e||window.event;
if(isLeftButton(e.button))
{
msPosition(e);
ptX_drag=ptX;
ptY_drag=ptY;
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
// Mouse moved enough to be a drag — send drag event instead of click
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
