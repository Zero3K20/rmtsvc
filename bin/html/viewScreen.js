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

function processRequest() 
{
if (xmlHttp.readyState == 4) { // Check object state
if (xmlHttp.status == 200) 
{ // Data returned successfully, start processing
var xmlobj = xmlHttp.responseXML;
var wtext=xmlobj.getElementsByTagName("wtext");
var hwnd=xmlobj.getElementsByTagName("hwnd");
if(hwnd!=null && hwnd.length>0)
{
var sHelp="\r\nPress Alt then left-click on a password box to retrieve the password";
if((hwnd.item(0).textContent || hwnd.item(0).text)==0)
alert("Currently capturing the entire desktop. To capture a specific window,\r\npress Ctrl+Shift then left-click on the target window"+sHelp);
else  alert("Currently capturing: "+"\""+(wtext.item(0).textContent || wtext.item(0).text)+"\"\r\nPress Ctrl+Shift then right-click to stop capturing this window"+sHelp);
}
var iret=xmlobj.getElementsByTagName("result");
if(iret!=null && iret.length>0)
{
if((iret.item(0).textContent || iret.item(0).text)==0)
alert("Password in password box: "+(wtext.item(0).textContent || wtext.item(0).text));
else if((iret.item(0).textContent || iret.item(0).text)==1)
alert("Not a password box: '"+(wtext.item(0).textContent || wtext.item(0).text)+"'")
else alert("Failed to retrieve, err="+(iret.item(0).textContent || iret.item(0).text));
}
            }
        }
}

// Detect Internet Explorer (pre-Edge)
var isIE = !!(window.ActiveXObject || (navigator.userAgent.indexOf("Trident") !== -1));

// Compute the mouse position relative to the screen image using getBoundingClientRect()
// so the result is accurate regardless of scroll, CSS transforms, or frame nesting.
// Returns already-scaled server screen coordinates.
function imgPosition(e)
{
var img=document.getElementById("screenimage");
var rect=img.getBoundingClientRect();
var cx=(e.clientX !== undefined ? e.clientX : e.x);
var cy=(e.clientY !== undefined ? e.clientY : e.y);
return scaleToScreen(cx-Math.round(rect.left), cy-Math.round(rect.top));
}

// Get click action
function msup(e)
{
e=e||window.event;
var coords=imgPosition(e);
var x1=coords.x; var y1=coords.y;
var altk=0;
if(e.ctrlKey) altk=altk | 1;
if(e.shiftKey) altk=altk | 2;
if(e.altKey) altk=altk | 4;
// Normalize button: IE uses 1=left,2=right; modern browsers use 0=left,2=right
var btn = isIE ? e.button : (e.button===0 ? 1 : e.button);

  if(altk==4) // Retrieve password from specified password box
  {
  var qx=parent.frmLeft.userQX;
  if((qx & ACCESS_SCREEN_ALL)==ACCESS_SCREEN_ALL)
  xmlHttp.open("GET", "/getPassword?x="+x1+"&y="+y1, true);
  else{ alert("You do not have permission to view password box contents"); return; }
  }else{
  var act=0;
  if(altk==3 && btn==1) act=1; // Capture specified window
  else if(altk==3 && btn==2) act=2; // Capture entire desktop
  xmlHttp.open("GET", "/capWindow?act="+act+"&x="+x1+"&y="+y1, true);
  }
xmlHttp.onreadystatechange = processRequest;
xmlHttp.send(null);
}

function msmove(e)
{
e=e||window.event;
var coords=imgPosition(e);
var w=window.parent.frmLeft;
w.document.getElementById("lblXY").innerText="X:"+coords.x+" , Y:"+coords.y;
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

function window_onload()
{
if(!xmlHttp) createXMLHttpRequest();
// Start the binary diff stream (falls back to BMP polling on older browsers)
startScreenStream();
// Poll server cursor shape every 200 ms so the displayed cursor matches the server
fetchCursor();
window.setInterval(fetchCursor, 200);
}
