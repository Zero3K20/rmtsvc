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

// Scale display coordinates back to actual screen coordinates when image is resized
function scaleToScreen(x, y)
{
var img=document.getElementById("screenimage");
if(img.naturalWidth && img.clientWidth && img.naturalWidth !== img.clientWidth)
{
x=Math.round(x * img.naturalWidth / img.clientWidth);
y=Math.round(y * img.naturalHeight / img.clientHeight);
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

function msPosition(e) 
{ 
var o=window.document.getElementById("divScreen");
ptX=(e.clientX !== undefined ? e.clientX : e.x)+o.scrollLeft-o.parentElement.offsetLeft;
ptY=(e.clientY !== undefined ? e.clientY : e.y)+o.scrollTop-o.parentElement.offsetTop;
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
// Poll server cursor shape every 200 ms so the client cursor stays in sync
fetchCursor();
window.setInterval(fetchCursor, 200);
}

function processRequest() 
{
// Event sent; the MJPEG stream updates the display automatically
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

function msdrop(e)
{
e=e||window.event;
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=4&dragx="+ptX_drag+"&dragy="+ptY_drag;
sendEvent("/msevent",param);
}
// Get mouse down event, record drag start point. ondrag event has offset issues
function msdown(e)
{
e=e||window.event;
if(isLeftButton(e.button))
{
msPosition(e);
ptX_drag=ptX;
ptY_drag=ptY;
}
}
// Get non-left button click actions
function msup(e)
{
e=e||window.event;
var b=e.button;
if(isLeftButton(b)) return; // Skip left button click
msPosition(e);
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
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
