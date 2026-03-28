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

// ---------------------------------------------------------------------------
// Inter-frame diff stream (binary, RLE-compressed).
// Used by viewScreen.htm and viewCtrl.htm.
//
// Server sends frames as:
//   DiffFrameHeader (17 bytes packed):
//     magic    [4]  = 0x46464944 ('D','I','F','F' LE)
//     flags    [1]  bit0: 0=full RGB, 1=XOR diff; bit1: 0=raw, 1=RLE
//     width    [2]  uint16 LE
//     height   [2]  uint16 LE
//     rawSize  [4]  uint32 LE
//     compSize [4]  uint32 LE  (bytes of payload following the header)
//   payload  [compSize bytes]
// ---------------------------------------------------------------------------
var DIFF_MAGIC        = 0x46464944;
var DIFF_HEADER_SIZE  = 17; // 4+1+2+2+4+4
var _diffBuf          = null;  // Uint8Array accumulation buffer
var _diffPos          = 0;     // read position within _diffBuf
var _diffCtrl         = null;  // AbortController for current fetch
var _diffCanvasW      = 0;     // last rendered canvas content width
var _diffCanvasH      = 0;     // last rendered canvas content height
var _diffCanvas       = null;  // cached canvas element (avoids repeated getElementById)
var _diffCtx2d        = null;  // cached 2D context (avoids repeated getContext)
var _diffImageData    = null;  // cached ImageData — eliminates getImageData GPU→CPU readback on every XOR diff frame

// RLE (PackBits-style) decompressor.
// Control byte encoding:
//   0x00..0x7F  literal run:  next (ctrl+1) bytes are copied verbatim (1..128)
//   0x80..0xFF  repeat run:   next byte is repeated (ctrl-0x80+2) times   (2..129)
// dstSize must equal the uncompressed frame size (rawSize from the frame header)
// so that the output Uint8Array can be pre-allocated without a resize pass.
function rleDecompress(src, dstSize)
{
	var out    = new Uint8Array(dstSize);
	var pos    = 0;
	var outPos = 0;
	while (pos < src.length && outPos < dstSize)
	{
		var ctrl = src[pos++];
		if (ctrl & 0x80)
		{
			// Repeat run
			var count = (ctrl & 0x7F) + 2;
			if (pos >= src.length) break;
			var b   = src[pos++];
			var end = outPos + count;
			if (end > dstSize) end = dstSize;
			while (outPos < end) out[outPos++] = b;
		}
		else
		{
			// Literal run
			var count = ctrl + 1;
			var end   = outPos + count;
			if (end > dstSize) end = dstSize;
			while (outPos < end && pos < src.length)
				out[outPos++] = src[pos++];
		}
	}
	return out;
}

// Append newly received bytes to the accumulation buffer
function _diffAppend(bytes)
{
	if (_diffBuf === null || _diffPos >= _diffBuf.length)
	{
		_diffBuf = bytes;
		_diffPos = 0;
	}
	else
	{
		var rem  = _diffBuf.length - _diffPos;
		var comb = new Uint8Array(rem + bytes.length);
		comb.set(_diffBuf.subarray(_diffPos), 0);
		comb.set(bytes, rem);
		_diffBuf = comb;
		_diffPos = 0;
	}
}

// Parse and render any complete frames from the accumulation buffer
function _diffParseFrames()
{
	var b = _diffBuf;
	while (b !== null)
	{
		var avail = b.length - _diffPos;
		if (avail < DIFF_HEADER_SIZE) break;

		var p     = _diffPos;
		var magic = (b[p] | (b[p+1]<<8) | (b[p+2]<<16) | (b[p+3]<<24)) >>> 0;
		if (magic !== DIFF_MAGIC) { _diffPos++; continue; } // resync

		var flags    = b[p+4];
		var width    = b[p+5] | (b[p+6] << 8);
		var height   = b[p+7] | (b[p+8] << 8);
		var rawSize  = (b[p+9]  | (b[p+10]<<8) | (b[p+11]<<16) | (b[p+12]<<24)) >>> 0;
		var compSize = (b[p+13] | (b[p+14]<<8) | (b[p+15]<<16) | (b[p+16]<<24)) >>> 0;

		if (avail < DIFF_HEADER_SIZE + compSize) break; // incomplete frame

		var payload  = b.subarray(p + DIFF_HEADER_SIZE, p + DIFF_HEADER_SIZE + compSize);
		var isDiff   = (flags & 1) !== 0;
		var isRle    = (flags & 2) !== 0;
		var raw      = isRle ? rleDecompress(payload, rawSize) : payload;

		_diffRenderFrame(isDiff, width, height, raw);
		_diffPos = p + DIFF_HEADER_SIZE + compSize;
	}
}

// Fit the screen canvas to fill the available window space while preserving
// aspect ratio (scales both up and down to fill the frame, like "Fit to Screen").
// Uses window.innerWidth/Height so it works even when the CSS height chain
// (height:100% through table cells) doesn't resolve correctly in the browser.
function _fitCanvasToContainer()
{
	if (!_diffCanvas) _diffCanvas = document.getElementById("screenimage");
	var canvas = _diffCanvas;
	if (!canvas) return;
	var nw = parseInt(canvas.getAttribute("data-nw") || "0") || canvas.width;
	var nh = parseInt(canvas.getAttribute("data-nh") || "0") || canvas.height;
	if (!nw || !nh) return;
	var cw = window.innerWidth  || document.documentElement.clientWidth  || 0;
	var ch = window.innerHeight || document.documentElement.clientHeight || 0;
	if (!cw || !ch) return;
	var scale = Math.min(cw / nw, ch / nh);
	canvas.style.width  = Math.round(nw * scale) + "px";
	canvas.style.height = Math.round(nh * scale) + "px";
}

// Render one frame onto the canvas with id="screenimage"
function _diffRenderFrame(isDiff, width, height, rgb)
{
	if (!_diffCanvas) _diffCanvas = document.getElementById("screenimage");
	var canvas = _diffCanvas;
	if (!canvas || !canvas.getContext) return;
	if (!_diffCtx2d) _diffCtx2d = canvas.getContext("2d");
	var ctx = _diffCtx2d;

	if (canvas.width !== width || canvas.height !== height)
	{
		canvas.width  = width;
		canvas.height = height;
		canvas.setAttribute("data-nw", width);
		canvas.setAttribute("data-nh", height);
		_diffCanvasW = width;
		_diffCanvasH = height;
		_diffImageData = null; // invalidate cached ImageData on dimension change
		_fitCanvasToContainer();
	}

	// Lazily create the cached ImageData.  Re-using the same object across frames
	// avoids both the createImageData allocation cost and — for XOR diff frames —
	// the expensive getImageData GPU→CPU readback that was previously required to
	// read back the current canvas pixels before XOR-ing the diff on top.
	if (_diffImageData === null)
		_diffImageData = ctx.createImageData(width, height);

	var d = _diffImageData.data;

	if (!isDiff)
	{
		// Full frame: top-down RGB → cached RGBA ImageData
		var len = rgb.length - 2;
		for (var i = 0, j = 0; j < len; i += 4, j += 3)
		{
			d[i]   = rgb[j];
			d[i+1] = rgb[j+1];
			d[i+2] = rgb[j+2];
			d[i+3] = 255;
		}
		ctx.putImageData(_diffImageData, 0, 0);
	}
	else
	{
		// XOR diff: apply directly to cached ImageData — no getImageData needed
		var len = rgb.length - 2;
		for (var i = 0, j = 0; j < len; i += 4, j += 3)
		{
			d[i]   ^= rgb[j];
			d[i+1] ^= rgb[j+1];
			d[i+2] ^= rgb[j+2];
		}
		ctx.putImageData(_diffImageData, 0, 0);
	}
}

// Start the binary diff stream using fetch + ReadableStream
function _startDiffStream()
{
	if (_diffCtrl) { try { _diffCtrl.abort(); } catch(e){} }
	_diffCtrl = (typeof AbortController !== "undefined") ? new AbortController() : null;
	_diffBuf       = null;
	_diffPos       = 0;
	_diffImageData = null; // invalidate cached ImageData on reconnect

	var opts = _diffCtrl ? {signal: _diffCtrl.signal} : {};
	fetch("/capStream", opts)
		.then(function(resp)
		{
			if (!resp.ok || !resp.body) { _scheduleReconnect(); return; }
			var reader = resp.body.getReader();
			// pump() deliberately does NOT return the inner promise so that
			// each iteration creates an independent, short-lived promise chain
			// that the GC can collect immediately after it resolves.  Returning
			// the inner promise (the original approach) causes every iteration to
			// retain a reference to all prior promises, forming an ever-growing
			// chain that cannot be collected for the lifetime of the connection
			// and leads to steadily increasing memory consumption.
			function pump()
			{
				reader.read().then(function(result)
				{
					if (result.done) { _scheduleReconnect(); return; }
					_diffAppend(result.value);
					_diffParseFrames();
					pump();
				}).catch(function() { _scheduleReconnect(); });
			}
			pump();
		})
		.catch(function() { _scheduleReconnect(); });
}

// Fallback for browsers without ReadableStream: poll /capDesktop (BMP) repeatedly
function _startBmpPoll()
{
	var img = new Image();
	function poll()
	{
		img.onload = function()
		{
			var canvas = document.getElementById("screenimage");
			if (canvas && canvas.getContext)
			{
				if (canvas.width !== img.width || canvas.height !== img.height)
				{
					canvas.width  = img.width;
					canvas.height = img.height;
					canvas.setAttribute("data-nw", img.width);
					canvas.setAttribute("data-nh", img.height);
					_fitCanvasToContainer();
				}
				canvas.getContext("2d").drawImage(img, 0, 0);
			}
			setTimeout(poll, 33);
		};
		img.onerror = function() { setTimeout(poll, 1000); };
		img.src = "/capDesktop?" + new Date().getTime();
	}
	poll();
}

var _reconnectTimer = 0;
function _scheduleReconnect()
{
	if (_reconnectTimer) return;
	_reconnectTimer = setTimeout(function()
	{
		_reconnectTimer = 0;
		_startDiffStream();
	}, 100);
}

// Call from window_onload() in each screen-viewer page.
// Uses the binary diff stream on modern browsers; falls back to BMP polling.
function startScreenStream()
{
	if (window.addEventListener)
		window.addEventListener("resize", _fitCanvasToContainer, false);
	else if (window.attachEvent)
		window.attachEvent("onresize", _fitCanvasToContainer);

	if (typeof fetch !== "undefined" &&
	    typeof ReadableStream !== "undefined" &&
	    typeof Uint8Array !== "undefined")
	{
		_startDiffStream();
	}
	else
	{
		_startBmpPoll();
	}
}
