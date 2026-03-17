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

// Get click action
function msup(e)
{
e=e||window.event;
var o=window.document.getElementById("divScreen");
var x1=(e.clientX !== undefined ? e.clientX : e.x)+o.scrollLeft-o.parentElement.offsetLeft;
var y1=(e.clientY !== undefined ? e.clientY : e.y)+o.scrollTop-o.parentElement.offsetTop;
var coords=scaleToScreen(x1,y1);
x1=coords.x; y1=coords.y;
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
 var o=window.document.getElementById("divScreen");
 var x1=(e.clientX !== undefined ? e.clientX : e.x)+o.scrollLeft-o.parentElement.offsetLeft;
  var y1=(e.clientY !== undefined ? e.clientY : e.y)+o.scrollTop-o.parentElement.offsetTop;
var coords=scaleToScreen(x1,y1);
  var w=window.parent.frmLeft;
  w.document.getElementById("lblXY").innerText="X:"+coords.x+" , Y:"+coords.y;
}

function window_onload()
{
if(!xmlHttp) createXMLHttpRequest();
return;
}
