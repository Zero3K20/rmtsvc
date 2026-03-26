var curSname="";
var svrQX=true;
var ACCESS_SERVICE_ALL=0x0030;
var slistData=[];
var xmlSlist=null;
var slistSortCol="";
var slistSortAsc=true;
var selectedSrow=null;

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

function window_onload()
{
	if(!oPopup) createpopup();
	var qx=parent.frmLeft.userQX;
	if((qx & ACCESS_SERVICE_ALL)==ACCESS_SERVICE_ALL)
		svrQX=false;
	loadSlist("");
}

function loadSlist(query)
{
	showPopup(250, 200, 150, 20);
	if(!xmlSlist) {
		if(window.ActiveXObject) xmlSlist=new ActiveXObject("Microsoft.XMLHTTP");
		else xmlSlist=new XMLHttpRequest();
	}
	xmlSlist.abort();
	var url="/slist"+(query?"?"+query:"");
	xmlSlist.open("GET", url, true);
	xmlSlist.onreadystatechange=function() {
		if(xmlSlist.readyState==4) {
			if(xmlSlist.status==200) {
				var xmlobj=xmlSlist.responseXML;
				slistData=[];
				if(xmlobj) {
				var svcs=xmlobj.getElementsByTagName("service");
				for(var i=0;i<svcs.length;i++) {
					var s=svcs[i];
					slistData.push({
						id:getNodeText(s,"id"),
						status:getNodeText(s,"status"),
						rtype:getNodeText(s,"rtype"),
						stype:getNodeText(s,"stype"),
						sname:getNodeText(s,"sname"),
						sdisp:getNodeText(s,"sdisp"),
						sdesc:getNodeText(s,"sdesc"),
						spath:getNodeText(s,"spath")
					});
				}
				}
				renderSlist();
			}
			hidePopup();
		}
	};
	xmlSlist.send(null);
}

function makeCell(content, align) {
	var td=document.createElement("td");
	if(align) td.align=align;
	td.innerHTML=content;
	return td;
}

function renderSlist()
{
	var tbody=document.getElementById("slistTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	selectedSrow=null;
	for(var i=0;i<slistData.length;i++) {
		(function(idx) {
			var d=slistData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.style.cursor="pointer";
			tr.onmousemove=function(){if(this.className!="selected") this.style.background="#e5e5e5";};
			tr.onmouseout=function(){if(this.className!="selected") this.style.background="#ffffff";};
			tr.onclick=function(){serviceClick(this);};
			tr.appendChild(makeCell(escHtml(d.id),"center"));
			tr.appendChild(makeCell(escHtml(d.status),"center"));
			tr.appendChild(makeCell(escHtml(d.rtype),"center"));
			tr.appendChild(makeCell(escHtml(d.stype),"center"));
			tr.appendChild(makeCell(escHtml(d.sname),""));
			tr.appendChild(makeCell(escHtml(d.sdisp),""));
			tr.appendChild(makeCell(escHtml(d.sdesc),""));
			tr.appendChild(makeCell(escHtml(d.spath),""));
			tbody.appendChild(tr);
		})(i);
	}
}

function serviceClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=slistData.length) return;
	if(selectedSrow) { selectedSrow.className=""; selectedSrow.style.background="#ffffff"; }
	selectedSrow=tblElement;
	selectedSrow.className="selected";
	selectedSrow.style.background="#cce8ff";
	var d=slistData[idx];
	var stat=d.status;
	if(stat=="Running")
	{
		document.getElementById("btnStart").disabled=true;
		document.getElementById("btnStop").disabled=svrQX;
	}else{
		document.getElementById("btnStart").disabled=svrQX;
		document.getElementById("btnStop").disabled=true;
	}
	var runtype=d.rtype.toLowerCase();
	if(runtype=="auto")
	{
		document.getElementById("btnAuto").disabled=true;
		document.getElementById("btnManual").disabled=svrQX;
		document.getElementById("btnForbid").disabled=svrQX;
	}
	else if(runtype=="manual")
	{
		document.getElementById("btnManual").disabled=true;
		document.getElementById("btnAuto").disabled=svrQX;
		document.getElementById("btnForbid").disabled=svrQX;
	}
	else // Disabled
	{
		document.getElementById("btnManual").disabled=svrQX;
		document.getElementById("btnAuto").disabled=svrQX;
		document.getElementById("btnForbid").disabled=true;
		document.getElementById("btnStart").disabled=true;
	}
	curSname=d.sname;
	document.getElementById("lblService").innerText="Service : "+curSname;
}

function doServiceCmd(cmd)
{
	if(curSname=="") return;
	loadSlist("cmd="+cmd+"&sname="+encodeURIComponent(curSname));
}

function sortSlist(colName)
{
	if(slistSortCol==colName) slistSortAsc=!slistSortAsc;
	else { slistSortCol=colName; slistSortAsc=true; }
	slistData.sort(function(a,b) {
		var av=a[colName], bv=b[colName];
		if(av<bv) return slistSortAsc?-1:1;
		if(av>bv) return slistSortAsc?1:-1;
		return 0;
	});
	renderSlist();
}
