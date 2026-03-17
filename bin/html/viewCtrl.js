var ptX,ptY;
var ptX_drag,ptY_drag;
var timerID_key=0;
var timerID_click=0;
var timerID_move=0;
var txtKeyEvent="";

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

function msPosition(e) 
{ 
	var o=window.document.getElementById("divScreen");
 	ptX=e.x+o.scrollLeft-o.parentElement.offsetLeft;
  	ptY=e.y+o.scrollTop-o.parentElement.offsetTop;
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
	xmlHttp.open("POST", strurl, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send(param);
}

function msmove()
{
 	msPosition(window.event);
 	var param="x="+ptX+"&y="+ptY+"&altk=0&button=0&act=0";
	if(timerID_move!=0)
	{
		window.clearTimeout(timerID_move);
		timerID_move=0;
	}
	timerID_move=window.setTimeout("sendEvent(\"/msevent\",\""+param+"\")",50);
}

function msclick()
{
	msPosition(window.event);
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=1";
	if(timerID_click!=0) window.clearTimeout(timerID_click);
	if(timerID_move!=0) window.clearTimeout(timerID_move);
	timerID_click=window.setTimeout("sendEvent(\"/msevent\",\""+param+"\")",500);
}
function msdblclick()
{
	if(timerID_click!=0)
	{
		window.clearTimeout(timerID_click);
		timerID_click=0;
	}
	msPosition(window.event);
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=2";
	sendEvent("/msevent",param);
}

function msdrop()
{
	msPosition(window.event);
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=1&act=4&dragx="+ptX_drag+"&dragy="+ptY_drag;
	sendEvent("/msevent",param);
}
// Get mouse down event, record drag start point. ondrag event has offset issues
function msdown()
{
	if(window.event.button==1)
	{
		msPosition(window.event);
		ptX_drag=ptX;
		ptY_drag=ptY;
	}
}
// Get non-left button click actions
function msup()
{
	var b=window.event.button;
	if(b==1) return; // Skip left button click
	msPosition(window.event);
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button="+b+"&act=1";
	sendEvent("/msevent",param);
}

function mswheel()
{
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var d=window.event.wheelDelta;
	var param="x="+ptX+"&y="+ptY+"&altk="+altk+"&button=0&act=5&wheel="+window.event.wheelDelta;
	sendEvent("/msevent",param);
	window.event.returnValue=false;
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
		sendEvent("/keyevent","vkey="+param);
}


function keyup()
{
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	var kevent=altk*256+window.event.keyCode;
	txtKeyEvent=txtKeyEvent+kevent+",";
	if(timerID_key==0)
		timerID_key=window.setInterval(Kevent,1000);
	window.event.keyCode=0;
}

