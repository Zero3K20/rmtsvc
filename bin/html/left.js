var userQX=0;
var ssid="";
function ArrangeTaskList(){
	var TotalHeight=document.body.clientHeight;
	var TabHeight=0;
	var i;
	var tabtool = document.getElementsByTagName("TABLE");
	for(i=0;i<tabtool.length;i++)
	{
		if(tabtool[i].getAttribute("item")=="1")
		{
			TabHeight+=27;
		}
	}
	var show=true;
	for(i=0;i<tabtool.length;i++)
	{
		if(tabtool[i].getAttribute("item")=="1")
		{
			tabtool[i].style.height=(TotalHeight-TabHeight-4)+"px";
			if(show)
			{
				tabtool[i].style.visibility="visible";
				tabtool[i].style.display="";
				show=false;
			}
			else
			{
				tabtool[i].style.visibility="hidden";
				tabtool[i].style.display="none";
			}
		}
	}		
}

function Show_ChangeTool(name){
	var Arr=new Array("Control","Manage","VIDC"); 
	var i;
	for(i=0;i<Arr.length;i++)
	{
		if(document.getElementById("Tab"+Arr[i])!=null)
		{
			if(Arr[i]==name)
			{
				document.getElementById("Tab"+Arr[i]).style.visibility="visible";
				document.getElementById("Tab"+Arr[i]).style.display="block";
				document.getElementById("td"+Arr[i]).bgColor='#e5e5e5';
				document.getElementById("td"+Arr[i]).onmouseout="";
				document.getElementById("td"+Arr[i]).onmouseover="";
			}
			else
			{
				document.getElementById("Tab"+Arr[i]).style.visibility="hidden";
				document.getElementById("Tab"+Arr[i]).style.display="none";
				document.getElementById("td"+Arr[i]).bgColor='Silver';
				document.getElementById("td"+Arr[i]).onmouseout=new Function("this.bgColor='Silver'");
				document.getElementById("td"+Arr[i]).onmouseover=new Function("this.bgColor='#e5e5e5'");
			}
		}
	}
}

function setButtonStatus()
{
	if((userQX & ACCESS_SCREEN_ALL)!=0)
    	{
    		o=document.getElementById("fView")
    		o.disabled=false;
    		o.href="viewScreen.htm";
    		if((userQX & ACCESS_SCREEN_ALL)==ACCESS_SCREEN_ALL)
    		{
    		var o=document.getElementById("fKeys");
    		o.disabled=false;
    		o.href="javascript:sendKey();";
    		o=document.getElementById("fControl")
    		o.disabled=false;
    		o.href="viewCtrl.htm";
    		o=document.getElementById("fCtAlDe")
    		o.disabled=false;
    		o.href="/command?cmd=CtAlDe";
    		o=document.getElementById("fSetClip")
    		o.disabled=false;
    		o.href="javascript:SetClipBoard();";
    		o=document.getElementById("fGetClip")
    		o.disabled=false;
    		o.href="javascript:GetClipBoard();";
    		o=document.getElementById("fRun")
    		o.disabled=false;
    		o.href="javascript:RunProcess();";
    		o=document.getElementById("fShDw")
    		o.disabled=false;
    		o.href="/command?cmd=ShDw";
    		o=document.getElementById("fRest")
    		o.disabled=false;
    		o.href="/command?cmd=ReSt";
    		o=document.getElementById("fLgOf")
    		o.disabled=false;
    		o.href="/command?cmd=LgOf";
    		o=document.getElementById("fLock")
    		o.disabled=false;
    		o.href="/command?cmd=Lock";
    		o=document.getElementById("fAudio");
    		o.disabled=false;
    		o=document.getElementById("fProc")
    		o.disabled=false;
    		o.href="viewProcess.htm";
    		o=document.getElementById("fPort")
    		o.disabled=false;
    		o.href="viewPort.htm";
    		}
    	}
    	
    	if((userQX & ACCESS_REGIST_ALL)!=0)
    	{
    		var o=document.getElementById("fReg");
    		o.disabled=false;
    		o.href="viewReg.htm";
	}
    	if((userQX & ACCESS_SERVICE_ALL)!=0)
    	{
    		var o=document.getElementById("fServ");
    		o.disabled=false;
    		o.href="viewService.htm";
	}
    	if((userQX & ACCESS_TELNET_ALL)==ACCESS_TELNET_ALL)
    	{
    		var o=document.getElementById("fTele");
    		o.disabled=false;
    		o.href="javascript:start_telnet()";
	}
	if((userQX & ACCESS_FILE_ALL)!=0)
    	{
    		var o=document.getElementById("fFile");
    		o.disabled=false;
    		o.href="viewFile.htm";
    	}
    	if((userQX & ACCESS_FTP_ADMIN)!=0)
    	{
    		var o=document.getElementById("fFTP");
    		o.disabled=false;
    		o.href="ftpsets.htm";
    	}
    	if((userQX & ACCESS_VIDC_ADMIN)!=0)
    	{
    		var o=document.getElementById("fUPnP");
    		o.disabled=false;
    		o.href="upnp.htm";
    		o=document.getElementById("fMapL");
    		o.disabled=false;
    		o.href="vidcMapL.htm";
    		o=document.getElementById("fMapR");
    		o.href="vidcMapR.htm";
    		o.disabled=false;
    		o=document.getElementById("fProxy");
    		o.disabled=false;
    		o.href="proxysets.htm";
    		o=document.getElementById("fvIDCs");
    		o.disabled=false;
    		o.href="vidcsvr.htm";
    		o=document.getElementById("fvIDCi");
    		o.disabled=false;
    		o.href="vidcini.htm";
    	}
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // Check object state
		if (xmlHttp.status == 200) 
		{ 
		//	alert(xmlHttp.responseText);
			var xmlobj = xmlHttp.responseXML;
			var quality = xmlobj.getElementsByTagName("quality");
			document.getElementById("quality"+quality[0].firstChild.data).checked=true;
			var qxnode=xmlobj.getElementsByTagName("ssid");
			if(qxnode.length>0) ssid=qxnode[0].firstChild.data;
			qxnode=xmlobj.getElementsByTagName("qx");
			if(qxnode.length>0) userQX=qxnode[0].firstChild.data;
			var userAgent=xmlobj.getElementsByTagName("userAgent");
			strUserAgent=userAgent[0].firstChild.data;
			setButtonStatus();
			var savedAudio = localStorage.getItem("audioEnabled");
			if (savedAudio === "1")
			{
				var o = document.getElementById("fAudio");
				if (o && !o.disabled)
				{
					toggleAudio();
				}
			}
            	}
        }
}

function processRequestX() 
{
	if (xmlHttp.readyState == 4) { // Check object state
		if (xmlHttp.status == 200) { // Data returned successfully, start processing
			
			var xmlobj = xmlHttp.responseXML;
    			var retmsg=xmlobj.getElementsByTagName("retmsg");
    			var dwurl=xmlobj.getElementsByTagName("dwurl");
    			if(retmsg.length>0)
				alert((retmsg.item(0).textContent || retmsg.item(0).text));
			if(dwurl.length>0)
    				window.open((dwurl.item(0).textContent || dwurl.item(0).text));
			//refreshScreen();
            	} //else alert("Request error, status="+xmlHttp.status);
        }
}

function window_onload()
{
	ArrangeTaskList();
	Show_ChangeTool('Control');
	if(!xmlHttp) createXMLHttpRequest();
	var savedQuality = localStorage.getItem("imageQuality");
	if (savedQuality)
	{
		xmlHttp.open("GET", "/capSetting?quality=" + savedQuality + "&lockmskb=0", true);
	}
	else
	{
		xmlHttp.open("GET", "/capSetting", true);
	}
	xmlHttp.onreadystatechange = processRequest;
    	xmlHttp.send(null);
}

function capsetting()
{
	var lockmskb=0; var qualityVal=0;
//	if(document.all("chkMsKb").checked) lockmskb=1;
	if(document.getElementById("quality10").checked) qualityVal=10;
	else if(document.getElementById("quality30").checked) qualityVal=30;
	else if(document.getElementById("quality60").checked) qualityVal=60;
	else if(document.getElementById("quality90").checked) qualityVal=90;
	else qualityVal=60;
	localStorage.setItem("imageQuality", qualityVal);
	xmlHttp.open("GET", "/capSetting?quality="+qualityVal+"&lockmskb="+lockmskb, true);
	xmlHttp.onreadystatechange = processRequest;
    	xmlHttp.send(null);
}

function sendKey()
{
	var altk=document.getElementById("selCSA").value;
	var fx=document.getElementById("selFX").value;
	altk=altk*256+fx*1;
	xmlHttp.open("GET","/keyevent?vkey="+altk+",",false);
	xmlHttp.send(null);
}


function SetClipBoard()
{
	var v=prompt("\u8bf7\u8f93\u5165\u8981\u8bbe\u7f6e\u5230\u526a\u8d34\u677f\u7684\u5185\u5bb9:","");
	if(v!=null && v!="")
	{
		var v1=v.replace(/&/g,"%26"); 
		var strEncode="val="+v1;
		xmlHttp.open("POST", "/SetClipBoard", true);
		xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");

		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send(strEncode);
	}
}


function GetClipBoard()
{
	window.open("/GetClipBoard","_blank","height=300,width=300,directories=no,location=no,menubar=no,status=no,toolbar=no");
}

function RunProcess()
{
	var scmd=prompt("Enter the application path or name", "");
	if(scmd==null) return;
	var strEncode="path="+scmd.replace(/&/g,"%26");
	xmlHttp.open("POST","/file_run",true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequestX;
	xmlHttp.send(strEncode);
}

function start_telnet()
{
	xmlHttp.open("GET", "/telnet", false);
	xmlHttp.send(null);
	var rspUrl=xmlHttp.responseText;
	var s=rspUrl.substr(0,7);
	if(s=="telnet:")
		window.location.href=rspUrl;
	else alert(s);
	return;
}
function fileNanage()
{
	xmlHttp.open("GET", "/ftp", false);
	xmlHttp.send(null);
	var rspUrl=xmlHttp.responseText;
	var s=rspUrl.substr(0,4);
	if(s=="ftp:")
	{
		window.open(rspUrl);
	} else alert(s);
	return;
}

var audioEnabled = false;
var audioCtx = null;
var nextAudioTime = 0;

function toggleAudio()
{
	audioEnabled = !audioEnabled;
	localStorage.setItem("audioEnabled", audioEnabled ? "1" : "0");
	var o = document.getElementById("fAudio");
	if (audioEnabled)
	{
		if (!audioCtx)
		{
			audioCtx = new (window.AudioContext || window.webkitAudioContext)();
		}
		if (o) o.innerText = "Audio: ON";
		audioCtx.resume().then(function()
		{
			nextAudioTime = audioCtx.currentTime;
			fetchAudioChunk();
		});
	}
	else
	{
		if (audioCtx) audioCtx.suspend();
		if (o) o.innerText = "Audio: OFF";
	}
}

function fetchAudioChunk()
{
	if (!audioEnabled) return;
	var xhr = new XMLHttpRequest();
	xhr.open("GET", "/capAudio", true);
	xhr.responseType = "arraybuffer";
	var nextStarted = false;

	function startNext()
	{
		if (nextStarted) return;
		nextStarted = true;
		fetchAudioChunk();
	}

	// Send the next request ~2 s after this one so it arrives at the server
	// near the end of the current 2-second capture window.  This closes the
	// round-trip-latency gap that would otherwise appear between chunks.
	var prefetchTimer = setTimeout(startNext, 2000);

	xhr.onload = function()
	{
		clearTimeout(prefetchTimer);
		if (xhr.status === 200 && xhr.response && xhr.response.byteLength > 44)
		{
			startNext(); // start next fetch before decoding (no-op if timer fired)
			audioCtx.decodeAudioData(
				xhr.response,
				function(buffer)
				{
					var source = audioCtx.createBufferSource();
					source.buffer = buffer;
					source.connect(audioCtx.destination);
					var now = audioCtx.currentTime;
					if (nextAudioTime < now) nextAudioTime = now;
					source.start(nextAudioTime);
					nextAudioTime += buffer.duration;
				},
				function()
				{
					// decode failed; next fetch was already started above
				}
			);
		}
		else
		{
			setTimeout(fetchAudioChunk, 500);
		}
	};
	xhr.onerror = function() { clearTimeout(prefetchTimer); if (!nextStarted) setTimeout(fetchAudioChunk, 500); };
	xhr.send();
}


