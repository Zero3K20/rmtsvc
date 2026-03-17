var strUserAgent="";
var xmlHttp=false;
var oPopup=false;
var timerID_popup=0;
var ACCESS_SCREEN_ALL=0x0003;
var ACCESS_REGIST_ALL=0x000c
var ACCESS_SERVICE_ALL=0x0030
var ACCESS_TELNET_ALL=0x00c0
var ACCESS_FILE_ALL=0x0300
var ACCESS_FTP_ADMIN=0x0c00
var ACCESS_VIDC_ADMIN=0x3000

function Str2Bytes(str,charset)
{
	var ms=false;
	try
	{
		ms = new ActiveXObject("ADODB.Stream"); // Create stream object
		if(!ms) throw "error";
	}
	catch(e)
	{
		return str;
	}
	
	ms.Type = 2;			//Text
        ms.Charset = charset;		// Set stream charset
        ms.Open();
        ms.WriteText(str);		// Write str to stream
        
        ms.Position = 0;		// Set stream position to 0 for Charset property
        ms.Type = 1;			//Binary
        vout = ms.Read(ms.Size);		// Read character stream
        ms.close();			// Close stream object
        return vout;
}

function createXMLHttpRequest() {
    if (window.ActiveXObject) {
        xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
    } 
    else if (window.XMLHttpRequest) {
        xmlHttp = new XMLHttpRequest();
    }
    if(!xmlHttp) alert("Failed to create XMLHTTP object");
}

function createpopup()
{
	var div = document.createElement("div");
	div.id = "_progressPopup";
	div.style.cssText = "display:none;position:fixed;left:250px;top:200px;background:lightyellow;border:1px solid black;padding:2px 8px;z-index:9999;";
	div.innerHTML = "&nbsp;&nbsp;Processing... ";
	document.body.appendChild(div);
	oPopup = div;
}
var ista=0;
function moviePopup()
{
	if(!oPopup) return;
	if(ista==0)
	{
		oPopup.innerHTML = "&nbsp;&nbsp;Processing../ ";
		ista=1;
	}
	else if(ista==1)
	{
		oPopup.innerHTML = "&nbsp;&nbsp;Processing..- ";
		ista=2;
	}
	else if(ista==2)
	{
		oPopup.innerHTML = "&nbsp;&nbsp;Processing..\\ ";
		ista=0;
	}
}
function showPopup(lx,ty,w,h)
{
	if(!oPopup) return;
	oPopup.style.left = lx + "px";
	oPopup.style.top = ty + "px";
	oPopup.style.display = "block";
	timerID_popup=window.setInterval(moviePopup,500);
}

function hidePopup()
{
	if(!oPopup) return;
	if(timerID_popup!=0) window.clearInterval(timerID_popup);
	oPopup.style.display = "none";
	oPopup.innerHTML = "&nbsp;&nbsp;Processing... ";
	ista=0;
}
