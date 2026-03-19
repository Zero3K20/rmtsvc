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


var _clipOverlayCopyHandler = null;

function _removeClipboardOverlay()
{
	var pdoc = parent.document;
	var existing = pdoc.getElementById('_clipboardOverlay');
	if (existing) existing.parentNode.removeChild(existing);
	if (_clipOverlayCopyHandler)
	{
		pdoc.removeEventListener('copy', _clipOverlayCopyHandler);
		_clipOverlayCopyHandler = null;
	}
}

function _createClipboardOverlay(title, message, onCancel)
{
	_removeClipboardOverlay();
	var pdoc = parent.document;
	var root = pdoc.documentElement;

	var overlay = pdoc.createElement('div');
	overlay.id = '_clipboardOverlay';
	overlay.style.cssText = 'position:fixed;top:0;left:0;width:100%;height:100%;' +
		'background:rgba(0,0,0,0.5);z-index:99999;' +
		'font-family:Arial,sans-serif;font-size:9pt;';

	var dialog = pdoc.createElement('div');
	dialog.style.cssText = 'position:fixed;top:50%;left:50%;' +
		'margin-top:-110px;margin-left:-140px;' +
		'background:#fff;border-radius:4px;width:280px;' +
		'padding:16px;box-shadow:0 4px 20px rgba(0,0,0,0.3);';

	var titleRow = pdoc.createElement('div');
	titleRow.style.cssText = 'overflow:hidden;margin-bottom:12px;' +
		'border-bottom:1px solid #eee;padding-bottom:8px;';

	var titleSpan = pdoc.createElement('b');
	titleSpan.style.fontSize = '11pt';
	titleSpan.appendChild(pdoc.createTextNode(title));

	var closeBtn = pdoc.createElement('span');
	closeBtn.style.cssText = 'float:right;cursor:pointer;font-size:16pt;line-height:1;color:#666;';
	closeBtn.innerHTML = '\u00D7';
	closeBtn.onclick = onCancel;

	titleRow.appendChild(closeBtn);
	titleRow.appendChild(titleSpan);

	var msgDiv = pdoc.createElement('div');
	msgDiv.style.cssText = 'border:2px dashed #bbb;padding:24px 16px;' +
		'text-align:center;margin:0 0 16px 0;line-height:1.8;';
	msgDiv.innerHTML = message;

	var btnRow = pdoc.createElement('div');
	btnRow.style.cssText = 'text-align:right;';

	var cancelBtn = pdoc.createElement('button');
	cancelBtn.style.cssText = 'background:silver;border:1px solid #888;' +
		'cursor:pointer;padding:3px 14px;' +
		'font-family:Arial,sans-serif;font-size:9pt;';
	cancelBtn.appendChild(pdoc.createTextNode('Cancel'));
	cancelBtn.onclick = onCancel;

	btnRow.appendChild(cancelBtn);
	dialog.appendChild(titleRow);
	dialog.appendChild(msgDiv);
	dialog.appendChild(btnRow);
	overlay.appendChild(dialog);
	root.appendChild(overlay);
}

function SetClipBoard()
{
	_createClipboardOverlay(
		'Write clipboard',
		'Press CTRL+C (CMD+C on Mac)<br><br>or<br><br>right-click and select Copy',
		_removeClipboardOverlay
	);

	_clipOverlayCopyHandler = function(e)
	{
		var text = '';
		if (parent.getSelection)
		{
			text = parent.getSelection().toString();
		}
		else if (parent.document.selection && parent.document.selection.createRange)
		{
			text = parent.document.selection.createRange().text;
		}
		if (text)
		{
			_removeClipboardOverlay();
			var strEncode = "val=" + encodeURIComponent(text);
			xmlHttp.open("POST", "/SetClipBoard", true);
			xmlHttp.setRequestHeader("Content-Type", "application/x-www-form-urlencoded; charset=utf-8");
			xmlHttp.onreadystatechange = processRequest;
			xmlHttp.send(strEncode);
		}
	};
	parent.document.addEventListener('copy', _clipOverlayCopyHandler);
}

function _fallbackWriteClipboard(text)
{
	var pdoc = parent.document;
	var ta = pdoc.createElement('textarea');
	ta.value = text;
	ta.style.cssText = 'position:fixed;top:0;left:-9999px;width:1px;height:1px;';
	pdoc.documentElement.appendChild(ta);
	ta.focus();
	ta.select();
	try { pdoc.execCommand('copy'); } catch(e) {}
	ta.parentNode.removeChild(ta);
}

function GetClipBoard()
{
	var req = new XMLHttpRequest();
	req.open("GET", "/GetClipBoard", true);
	req.onreadystatechange = function()
	{
		if (req.readyState == 4 && req.status == 200)
		{
			var text = req.responseText;
			if (navigator.clipboard && navigator.clipboard.writeText)
			{
				navigator.clipboard.writeText(text)['catch'](function()
				{
					_fallbackWriteClipboard(text);
				});
			}
			else
			{
				_fallbackWriteClipboard(text);
			}
			_createClipboardOverlay(
				'Read clipboard',
				'Press CTRL+V (CMD+V on Mac)<br><br>or<br><br>right-click and select Paste',
				_removeClipboardOverlay
			);
		}
	};
	req.send(null);
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

// Magic for the AudioFrameHeader envelope added by the server ('AUDF' LE).
var AUDIO_FRAME_MAGIC = 0x46445541;

// Decode a /capAudio response that may be wrapped in an AudioFrameHeader
// (13 bytes: 4 magic + 1 flags + 4 rawSize + 4 compSize) with optional
// RLE compression.  Returns a Uint8Array containing the plain WAV data.
// Falls back to treating the input as a plain WAV when no AUDF header is
// present (e.g. older server builds or direct file access).
function unwrapAudioFrame(bytes)
{
	if (bytes.length >= 13)
	{
		var magic = (bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24)) >>> 0;
		if (magic === AUDIO_FRAME_MAGIC)
		{
			var flags    = bytes[4];
			var compSize = (bytes[9] | (bytes[10]<<8) | (bytes[11]<<16) | (bytes[12]<<24)) >>> 0;
			var payload  = bytes.subarray(13, 13 + compSize);
			return (flags & 1) ? rleDecompress(payload) : new Uint8Array(payload);
		}
	}
	return bytes; // no AUDF wrapper: treat as plain WAV (backward compat)
}

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

	// Fire the next request exactly 2 s after this one starts.  The server-side
	// ring buffer fills one 2-second sequential chunk per request, so 2000 ms
	// matches the server's capture window precisely and eliminates cumulative
	// drift that would otherwise push each successive request further behind
	// the ring's write position until the 3-second wait timeout is exceeded.
	var prefetchTimer = setTimeout(startNext, 2000);

	xhr.onload = function()
	{
		clearTimeout(prefetchTimer);
		if (xhr.status === 200 && xhr.response && xhr.response.byteLength > 44)
		{
			startNext(); // start next fetch before decoding (no-op if timer fired)
			var wavData = unwrapAudioFrame(new Uint8Array(xhr.response));
			audioCtx.decodeAudioData(
				wavData.buffer,
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
			// Only retry if the prefetch chain has not already been started.
			// Without this guard, a slow/failed server response (which takes
			// longer than the 2000 ms prefetch timer) causes both the timer
			// and this else-branch to call fetchAudioChunk(), doubling the
			// number of concurrent request chains on every failure.  After a
			// few failures the chains multiply exponentially, exhaust the
			// browser's per-host connection limit (~6), and block all other
			// requests (screen updates, mouse events) — making the server
			// appear completely unresponsive.
			if (!nextStarted) setTimeout(fetchAudioChunk, 500);
		}
	};
	xhr.onerror = function() { clearTimeout(prefetchTimer); if (!nextStarted) setTimeout(fetchAudioChunk, 500); };
	xhr.send();
}


