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
	xmlHttp.open("GET", "/capSetting", true);
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
var audioChainRunning = false;

// Magic for the AudioFrameHeader envelope added by the server ('AUDF' LE).
var AUDIO_FRAME_MAGIC = 0x46445541;

// ---------------------------------------------------------------------------
// QOA (Quite OK Audio) decoder – JavaScript port of phoboslab/qoa (MIT)
// Decodes a QOA byte stream into interleaved 16-bit PCM samples.
// Returns { pcm: Int16Array, channels: N, samplerate: N, samples: N }
// or null on error.
// ---------------------------------------------------------------------------
var _qoa_dequant_tab = [
	[   1,    -1,    3,    -3,    5,    -5,     7,     -7],
	[   5,    -5,   18,   -18,   32,   -32,    49,    -49],
	[  16,   -16,   53,   -53,   95,   -95,   147,   -147],
	[  34,   -34,  113,  -113,  203,  -203,   315,   -315],
	[  63,   -63,  210,  -210,  378,  -378,   588,   -588],
	[ 104,  -104,  345,  -345,  621,  -621,   966,   -966],
	[ 158,  -158,  528,  -528,  950,  -950,  1477,  -1477],
	[ 228,  -228,  760,  -760, 1368, -1368,  2128,  -2128],
	[ 316,  -316, 1053, -1053, 1895, -1895,  2947,  -2947],
	[ 422,  -422, 1405, -1405, 2529, -2529,  3934,  -3934],
	[ 548,  -548, 1828, -1828, 3290, -3290,  5117,  -5117],
	[ 696,  -696, 2320, -2320, 4176, -4176,  6496,  -6496],
	[ 868,  -868, 2893, -2893, 5207, -5207,  8099,  -8099],
	[1064, -1064, 3548, -3548, 6386, -6386,  9933,  -9933],
	[1286, -1286, 4288, -4288, 7718, -7718, 12005, -12005],
	[1536, -1536, 5120, -5120, 9216, -9216, 14336, -14336]
];

function qoaDecode(bytes)
{
	// Read 32-bit big-endian unsigned integer from bytes[pos]
	function u32(pos)
	{
		return ((bytes[pos] * 16777216) + (bytes[pos+1] * 65536) +
		        (bytes[pos+2] * 256) + bytes[pos+3]) >>> 0;
	}

	if (bytes.length < 16) return null;

	// File header: 4-byte magic + 4-byte total samples per channel
	if (u32(0) !== 0x716f6166) return null; // 'qoaf'
	var totalSamples = u32(4);
	if (totalSamples === 0) return null;

	// Peek at first frame header to get channels and samplerate
	var fhHi = u32(8);  // bits 63:32 of frame header
	var channels   = (fhHi >>> 24) & 0xff;
	var samplerate = fhHi & 0xffffff;
	if (channels === 0 || samplerate === 0) return null;

	// Allocate output: interleaved 16-bit PCM
	var pcm = new Int16Array(totalSamples * channels);

	// Per-channel LMS filter state
	var lms_h = [], lms_w = []; // history, weights
	for (var c = 0; c < channels; c++)
	{
		lms_h.push(new Int32Array(4));
		lms_w.push(new Int32Array(4));
	}

	var p = 8; // start decoding after file header
	var sampleOffset = 0;

	while (p < bytes.length && sampleOffset < totalSamples)
	{
		if (p + 8 > bytes.length) break;

		// Frame header
		var fhi = u32(p);
		var flo = u32(p + 4);
		p += 8;
		var fch      = (fhi >>> 24) & 0xff;
		var fsamples = (flo >>> 16) & 0xffff;
		if (fch !== channels || fsamples === 0) break;

		// LMS state per channel
		for (var c = 0; c < channels; c++)
		{
			var hhi = u32(p), hlo = u32(p + 4); p += 8;
			var whi = u32(p), wlo = u32(p + 4); p += 8;
			lms_h[c][0] = (hhi >> 16) << 16 >> 16; // sign-extend int16
			lms_h[c][1] = (hhi & 0xffff) << 16 >> 16;
			lms_h[c][2] = (hlo >> 16) << 16 >> 16;
			lms_h[c][3] = (hlo & 0xffff) << 16 >> 16;
			lms_w[c][0] = (whi >> 16) << 16 >> 16;
			lms_w[c][1] = (whi & 0xffff) << 16 >> 16;
			lms_w[c][2] = (wlo >> 16) << 16 >> 16;
			lms_w[c][3] = (wlo & 0xffff) << 16 >> 16;
		}

		// Decode slices: channels are interleaved per slice
		var slices = Math.floor((fsamples + 19) / 20);
		for (var si = 0; si < slices; si++)
		{
			var sliceBase = sampleOffset + si * 20;
			var sliceLen  = Math.min(20, fsamples - si * 20);

			for (var c = 0; c < channels; c++)
			{
				if (p + 8 > bytes.length) break;

				// Read 8-byte slice as two 32-bit words (big-endian)
				var hi = u32(p), lo = u32(p + 4);
				p += 8;

				// Top 4 bits = scalefactor index, shift them out
				var sf = (hi >>> 28) & 0xf;
				hi = ((hi << 4) | (lo >>> 28)) >>> 0;
				lo = (lo << 4) >>> 0;

				for (var ki = 0; ki < sliceLen; ki++)
				{
					// Extract top 3 bits as quantized residual, then shift out
					var qr  = (hi >>> 29) & 0x7;
					hi = ((hi << 3) | (lo >>> 29)) >>> 0;
					lo = (lo << 3) >>> 0;

					var dequantized = _qoa_dequant_tab[sf][qr];

					// LMS predict
					var predicted = lms_h[c][0] * lms_w[c][0] +
					                lms_h[c][1] * lms_w[c][1] +
					                lms_h[c][2] * lms_w[c][2] +
					                lms_h[c][3] * lms_w[c][3];
					predicted = Math.floor(predicted / 8192) | 0;

					var reconstructed = predicted + dequantized;
					if (reconstructed < -32768) reconstructed = -32768;
					if (reconstructed >  32767) reconstructed =  32767;

					// LMS update
					var delta = dequantized >> 4;
					for (var j = 0; j < 4; j++)
						lms_w[c][j] += lms_h[c][j] < 0 ? -delta : delta;
					lms_h[c][0] = lms_h[c][1];
					lms_h[c][1] = lms_h[c][2];
					lms_h[c][2] = lms_h[c][3];
					lms_h[c][3] = reconstructed;

					pcm[(sliceBase + ki) * channels + c] = reconstructed;
				}
			}
		}

		sampleOffset += fsamples;
	}

	return { pcm: pcm, channels: channels, samplerate: samplerate,
	         samples: sampleOffset };
}

// Convert QOA-decoded PCM to a plain WAV (RIFF/PCM) Uint8Array so that
// the Web Audio API's decodeAudioData() can play it unchanged.
function qoaToWav(qoaBytes)
{
	var decoded = qoaDecode(qoaBytes);
	if (!decoded) return null;

	var pcmBytes = decoded.pcm.length * 2; // 16-bit samples
	var buf  = new ArrayBuffer(44 + pcmBytes);
	var view = new DataView(buf);
	var p = 0;

	// RIFF header
	view.setUint32(p, 0x52494646, false); p += 4; // 'RIFF'
	view.setUint32(p, 36 + pcmBytes,  true);  p += 4;
	view.setUint32(p, 0x57415645, false); p += 4; // 'WAVE'
	// fmt  chunk
	view.setUint32(p, 0x666d7420, false); p += 4; // 'fmt '
	view.setUint32(p, 16, true); p += 4;
	view.setUint16(p, 1,  true); p += 2; // PCM
	view.setUint16(p, decoded.channels, true); p += 2;
	view.setUint32(p, decoded.samplerate, true); p += 4;
	view.setUint32(p, decoded.samplerate * decoded.channels * 2, true); p += 4;
	view.setUint16(p, decoded.channels * 2, true); p += 2; // block align
	view.setUint16(p, 16, true); p += 2; // bits per sample
	// data chunk
	view.setUint32(p, 0x64617461, false); p += 4; // 'data'
	view.setUint32(p, pcmBytes, true); p += 4;
	for (var i = 0; i < decoded.pcm.length; i++)
	{
		view.setInt16(p, decoded.pcm[i], true); p += 2;
	}
	return new Uint8Array(buf);
}

// Decode a /capAudio response that may be wrapped in an AudioFrameHeader
// (13 bytes: 4 magic + 1 flags + 4 rawSize + 4 compSize) with optional
// QOA encoding.  Returns a Uint8Array containing the plain WAV data.
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
			if (flags & 1)
			{
				var wav = qoaToWav(payload);
				return wav ? wav : new Uint8Array(payload);
			}
			return new Uint8Array(payload);
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
			// Some browsers (e.g. Chrome) start AudioContext in the "suspended"
			// state when it is created without a prior user gesture.  The
			// resume() promise may resolve immediately but leave the context
			// suspended, or it may only resolve after the browser decides to
			// allow audio (typically on the first user interaction with ANY
			// frame on the page).  onstatechange fires reliably when the
			// context actually transitions to "running", giving us a second
			// chance to start the chain even if the resume() promise resolved
			// early while the context was still paused.
			audioCtx.onstatechange = function()
			{
				if (audioCtx.state === 'running' && audioEnabled)
				{
					// Reset scheduled-ahead time so new chunks play from "now"
					// rather than from wherever the scheduler had advanced to
					// while the context was suspended.
					nextAudioTime = audioCtx.currentTime;
					if (!audioChainRunning) fetchAudioChunk();
				}
			};
		}
		if (o) o.innerText = "Audio: ON";
		audioCtx.resume().then(function()
		{
			if (audioEnabled)
			{
				nextAudioTime = audioCtx.currentTime;
				if (!audioChainRunning) fetchAudioChunk();
			}
		});
	}
	else
	{
		audioChainRunning = false;
		if (audioCtx) audioCtx.suspend();
		if (o) o.innerText = "Audio: OFF";
	}
}

function fetchAudioChunk()
{
	if (!audioEnabled) { audioChainRunning = false; return; }
	audioChainRunning = true;
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
			if (!nextStarted)
			{
				// Snap nextAudioTime back to "now" so the next successfully-
				// fetched chunk is scheduled for immediate playback.  If a
				// previous error left nextAudioTime far in the future (because
				// the scheduler was ahead when the failure occurred), the new
				// chunk would otherwise be silently queued seconds ahead,
				// creating a perceived gap even though audio data is available.
				if (audioCtx && nextAudioTime > audioCtx.currentTime + 2.5)
					nextAudioTime = audioCtx.currentTime;
				setTimeout(fetchAudioChunk, 500);
			}
		}
	};
	xhr.onerror = function()
	{
		clearTimeout(prefetchTimer);
		if (!nextStarted)
		{
			if (audioCtx && nextAudioTime > audioCtx.currentTime + 2.5)
				nextAudioTime = audioCtx.currentTime;
			setTimeout(fetchAudioChunk, 500);
		}
	};
	xhr.send();
}


