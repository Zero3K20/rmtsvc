var autoRefresh=1000; // Auto-refresh interval in ms
var imgLoaded=true;

var curPid=0;
var curPname="";
var curHmdl=0;
var loadcount=0;
var plistData=[];
var mlistData=[];
var xmlPlist=null;
var xmlMlist=null;
var plistSortCol="pid";
var plistSortAsc=true;
var mlistSortCol="";
var mlistSortAsc=true;
var selectedProw=null;
var selectedMrow=null;

function getNodeText(el, tag) {
	var n=el.getElementsByTagName(tag);
	if(n.length>0) return (n[0].textContent!==undefined?n[0].textContent:n[0].text)||"";
	return "";
}

function escHtml(s) {
	var d=document.createElement("div");
	d.appendChild(document.createTextNode(String(s)));
	return d.innerHTML;
}

function processRequest()
{
	if (xmlHttp.readyState == 4) { // Check object state
		if (xmlHttp.status == 200) { // Data returned successfully, start processing
			var xmlobj = xmlHttp.responseXML;
			var nodes=xmlobj.getElementsByTagName("pcname");
			if(nodes.length>0) document.getElementById("lblName").innerText=nodes[0].firstChild.data;
			nodes=xmlobj.getElementsByTagName("OS");
			if(nodes.length>0) document.getElementById("lblOS").innerText=nodes[0].firstChild.data;
			nodes=xmlobj.getElementsByTagName("CPU");
			if(nodes.length>0) document.getElementById("lblCPU").innerText=nodes[0].firstChild.data;
			nodes=xmlobj.getElementsByTagName("status");
			if(nodes.length>0) document.getElementById("lblSta").innerText=nodes[0].firstChild.data;
			nodes=xmlobj.getElementsByTagName("account");
			if(nodes.length>0) document.getElementById("lblAccount").innerText=nodes[0].firstChild.data;
			nodes=xmlobj.getElementsByTagName("pid");
			if(nodes.length>0) {
				curPid=nodes[0].firstChild.data;
				document.getElementById("lblProcess").innerText="pid:"+curPid;
				loadMlist(curPid);
			}
		} else alert("Request error");
	}
}

function loadPlist()
{
	showPopup(250, 200, 150, 20);
	if(!xmlPlist) {
		if(window.ActiveXObject) xmlPlist=new ActiveXObject("Microsoft.XMLHTTP");
		else xmlPlist=new XMLHttpRequest();
	}
	xmlPlist.abort();
	xmlPlist.open("GET", "/plist", true);
	xmlPlist.onreadystatechange=function() {
		if(xmlPlist.readyState==4) {
			if(xmlPlist.status==200) {
				var xmlobj=xmlPlist.responseXML;
				plistData=[];
				var procs=xmlobj.getElementsByTagName("process");
				for(var i=0;i<procs.length;i++) {
					var p=procs[i];
					plistData.push({
						id:getNodeText(p,"id"),
						pid:getNodeText(p,"pid"),
						ppid:getNodeText(p,"ppid"),
						priority:getNodeText(p,"priority"),
						threads:getNodeText(p,"threads"),
						pname:getNodeText(p,"pname")
					});
				}
				renderPlist();
			}
			hidePopup();
		}
	};
	xmlPlist.send(null);
}

function makeCell(content, align) {
	var td=document.createElement("td");
	if(align) td.align=align;
	td.innerHTML=content;
	return td;
}

function renderPlist()
{
	var tbody=document.getElementById("plistTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	selectedProw=null;
	for(var i=0;i<plistData.length;i++) {
		(function(idx) {
			var d=plistData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.style.cursor="pointer";
			tr.onmousemove=function(){if(this.className!="selected") this.style.background="#e5e5e5";};
			tr.onmouseout=function(){if(this.className!="selected") this.style.background="#ffffff";};
			tr.onclick=function(){processClick(this);};
			tr.appendChild(makeCell(escHtml(d.id),"center"));
			tr.appendChild(makeCell(escHtml(d.pid),"center"));
			tr.appendChild(makeCell(escHtml(d.ppid),"center"));
			tr.appendChild(makeCell(escHtml(d.priority),"center"));
			tr.appendChild(makeCell(escHtml(d.threads),"center"));
			tr.appendChild(makeCell(escHtml(d.pname),""));
			tbody.appendChild(tr);
		})(i);
	}
}

function loadMlist(pid)
{
	if(!pid) return;
	showPopup(250, 200, 150, 20);
	if(!xmlMlist) {
		if(window.ActiveXObject) xmlMlist=new ActiveXObject("Microsoft.XMLHTTP");
		else xmlMlist=new XMLHttpRequest();
	}
	xmlMlist.abort();
	xmlMlist.open("GET", "/mlist?pid="+pid, true);
	xmlMlist.onreadystatechange=function() {
		if(xmlMlist.readyState==4) {
			if(xmlMlist.status==200) {
				var xmlobj=xmlMlist.responseXML;
				mlistData=[];
				var mods=xmlobj.getElementsByTagName("module");
				for(var i=0;i<mods.length;i++) {
					var m=mods[i];
					mlistData.push({
						id:getNodeText(m,"id"),
						mbase:getNodeText(m,"mbase"),
						usage:getNodeText(m,"usage"),
						hmdl:getNodeText(m,"hmdl"),
						mname:getNodeText(m,"mname")
					});
				}
				renderMlist();
			}
			hidePopup();
		}
	};
	xmlMlist.send(null);
}

function renderMlist()
{
	var tbody=document.getElementById("mlistTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	selectedMrow=null;
	for(var i=0;i<mlistData.length;i++) {
		(function(idx) {
			var d=mlistData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.style.cursor="pointer";
			tr.onmousemove=function(){if(this.className!="selected") this.style.background="#e5e5e5";};
			tr.onmouseout=function(){if(this.className!="selected") this.style.background="#ffffff";};
			tr.onclick=function(){moduleClick(this);};
			tr.appendChild(makeCell(escHtml(d.id),"center"));
			tr.appendChild(makeCell(escHtml(d.mbase),"center"));
			tr.appendChild(makeCell(escHtml(d.usage),"center"));
			tr.appendChild(makeCell(escHtml(d.mname),""));
			tbody.appendChild(tr);
		})(i);
	}
}

function loadImg()
{
	if(imgLoaded)
	{
		document.usageimage.src="/usageimage";
		imgLoaded=false;
	}
}

function Imgloaded()
{
	imgLoaded=true;
	if(autoRefresh>0) window.setTimeout("loadImg()",autoRefresh);
}

function ifRefresh(t)
{
	if(autoRefresh>0)
		autoRefresh=0;
	else
	{
		autoRefresh=t;
		loadImg();
	}
}

function window_onload()
{
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/sysinfo", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send(null);
	loadImg();
	loadPlist();
}

function processClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=plistData.length) return;
	if(selectedProw) { selectedProw.className=""; selectedProw.style.background="#ffffff"; }
	selectedProw=tblElement;
	selectedProw.className="selected";
	selectedProw.style.background="#cce8ff";
	curPid=plistData[idx].pid;
	curPname=plistData[idx].pname;
	document.getElementById("lblProcess").innerText="pid:"+curPid+" - "+curPname;
	loadMlist(curPid);
}

function moduleClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=mlistData.length) return;
	if(selectedMrow) { selectedMrow.className=""; selectedMrow.style.background="#ffffff"; }
	selectedMrow=tblElement;
	selectedMrow.className="selected";
	selectedMrow.style.background="#cce8ff";
	curHmdl=mlistData[idx].hmdl;
	loadcount=mlistData[idx].usage;
	document.getElementById("lblModule").innerText=mlistData[idx].mname;
}

// Terminate the currently selected process
function terminateProcess()
{
	if(curPname!="" && confirm("Are you sure you want to terminate process "+curPname) )
	{
		xmlHttp.open("GET", "/pkill?pid="+curPid, false);
		xmlHttp.send(null);
		if(xmlHttp.responseText=="true")
			loadPlist();
		else alert(xmlHttp.responseText);
	}
}

function deattach()
{
	if(curPid==0 || curHmdl=="") return;
	if(loadcount<=0 || loadcount==65535)
		alert("Cannot unload this DLL");
	else if( confirm("Are you sure you want to unload this DLL?") )
	{
		xmlHttp.open("GET", "/mdattach?pid="+curPid+"&hmdl="+curHmdl+"&count="+loadcount, false);
		xmlHttp.send(null);
		loadMlist(curPid);
	}
}

function proFile()
{
	var qx=parent.frmLeft.userQX;
	var fpath=document.getElementById("lblModule").innerText;
	if(fpath=="")
		alert("Please select a file to view properties!");
	else
	{
		window.open("proFile.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"_blank","height=350,width=400,resizable=no,scrollbars=no,status=no");
	}
}

function sortPlist(colName)
{
	if(plistSortCol==colName) plistSortAsc=!plistSortAsc;
	else { plistSortCol=colName; plistSortAsc=true; }
	plistData.sort(function(a,b) {
		var av=a[colName], bv=b[colName];
		if(!isNaN(parseFloat(av)-parseFloat(bv))) { av=parseFloat(av); bv=parseFloat(bv); }
		if(av<bv) return plistSortAsc?-1:1;
		if(av>bv) return plistSortAsc?1:-1;
		return 0;
	});
	renderPlist();
}

function sortMlist(colName)
{
	if(mlistSortCol==colName) mlistSortAsc=!mlistSortAsc;
	else { mlistSortCol=colName; mlistSortAsc=true; }
	mlistData.sort(function(a,b) {
		var av=a[colName], bv=b[colName];
		if(!isNaN(parseFloat(av)-parseFloat(bv))) { av=parseFloat(av); bv=parseFloat(bv); }
		if(av<bv) return mlistSortAsc?-1:1;
		if(av>bv) return mlistSortAsc?1:-1;
		return 0;
	});
	renderMlist();
}
