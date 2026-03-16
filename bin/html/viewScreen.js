var autoRefresh=0; // Auto-refresh interval in ms
var imgLoaded=true; // Whether screen capture is done
function loadImg()
{
	if(imgLoaded)
	{
		document.screenimage.src="/capDesktop";
		imgLoaded=false;
	}
}

function Imgloaded()
{
	imgLoaded=true;
	if(autoRefresh>0) window.setTimeout("loadImg()",autoRefresh);
}

function msmove_IE()
{
 	var o=window.document.getElementById("divScreen");
 	var x1=window.event.x+o.scrollLeft-o.parentElement.offsetLeft; 
  	var y1=window.event.y+o.scrollTop-o.parentElement.offsetTop; 
  	var w=window.parent.frmLeft;
  	w.document.getElementById("lblXY").innerText="X:"+x1+" , Y:"+y1;
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
				loadImg();
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

// Get click action
function msup()
{
	var o=window.document.getElementById("divScreen");
 	var x1=window.event.x+o.scrollLeft-o.parentElement.offsetLeft; 
  	var y1=window.event.y+o.scrollTop-o.parentElement.offsetTop;
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	
  	if(altk==4) // Retrieve password from specified password box
  	{	
  		var qx=parent.frmLeft.userQX;
  		if((qx & ACCESS_SCREEN_ALL)==ACCESS_SCREEN_ALL)
  			xmlHttp.open("GET", "/getPassword?x="+x1+"&y="+y1, true);
  		else{ alert("You do not have permission to view password box contents"); return; }
  	}else{
  		var act=0;
  		if(altk==3 && window.event.button==1) act=1; // Capture specified window
  		else if(altk==3 && window.event.button==2) act=2; // Capture entire desktop
  		xmlHttp.open("GET", "/capWindow?act="+act+"&x="+x1+"&y="+y1, true);
  	}
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send(null);
}

function window_onload()
{
	var o=window.parent.frmLeft.document.getElementById("chkAuto");
	if( o.checked ) autoRefresh=o.value;
	if(!xmlHttp) createXMLHttpRequest();
	loadImg()
	return;	
}

