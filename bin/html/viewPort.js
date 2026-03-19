var fportData=[];
var xmlFport=null;
var fportSortCol="ptype";
var fportSortAsc=true;
var selectedFportRow=null;

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
	if(!xmlHttp) createXMLHttpRequest();
	loadFport();
}

function loadFport()
{
	showPopup(250, 200, 150, 20);
	if(!xmlFport) {
		if(window.ActiveXObject) xmlFport=new ActiveXObject("Microsoft.XMLHTTP");
		else xmlFport=new XMLHttpRequest();
	}
	xmlFport.abort();
	xmlFport.open("GET", "/fport", true);
	xmlFport.onreadystatechange=function() {
		if(xmlFport.readyState==4) {
			if(xmlFport.status==200) {
				var xmlobj=xmlFport.responseXML;
				if(xmlobj) {
					fportData=[];
					var ports=xmlobj.getElementsByTagName("fport");
					for(var i=0;i<ports.length;i++) {
						var p=ports[i];
						fportData.push({
							id:getNodeText(p,"id"),
							ptype:getNodeText(p,"ptype"),
							pid:getNodeText(p,"pid"),
							pname:getNodeText(p,"pname"),
							laddr:getNodeText(p,"laddr"),
							raddr:getNodeText(p,"raddr"),
							status:getNodeText(p,"status")
						});
					}
					renderFport();
				}
			}
			hidePopup();
		}
	};
	xmlFport.send(null);
}

function makeCell(content, align) {
	var td=document.createElement("td");
	if(align) td.align=align;
	td.innerHTML=content;
	return td;
}

function renderFport()
{
	var tbody=document.getElementById("fportTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	selectedFportRow=null;
	for(var i=0;i<fportData.length;i++) {
		(function(idx) {
			var d=fportData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.style.cursor="pointer";
			tr.onmousemove=function(){if(this.className!="selected") this.style.background="#e5e5e5";};
			tr.onmouseout=function(){if(this.className!="selected") this.style.background="#ffffff";};
			tr.onclick=function(){processClick(this);};
			tr.appendChild(makeCell(escHtml(d.id),"center"));
			tr.appendChild(makeCell(escHtml(d.ptype),"center"));
			tr.appendChild(makeCell(escHtml(d.pid),"center"));
			var tdl=makeCell("&nbsp;&nbsp;&nbsp;&nbsp;"+escHtml(d.laddr),"");
			tr.appendChild(tdl);
			var tdr=makeCell("&nbsp;&nbsp;"+escHtml(d.raddr),"");
			tr.appendChild(tdr);
			tr.appendChild(makeCell("&nbsp;&nbsp;"+escHtml(d.status),""));
			tr.appendChild(makeCell("&nbsp;&nbsp;"+escHtml(d.pname),""));
			tbody.appendChild(tr);
		})(i);
	}
}

function processClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=fportData.length) return;
	if(selectedFportRow) { selectedFportRow.className=""; selectedFportRow.style.background="#ffffff"; }
	selectedFportRow=tblElement;
	selectedFportRow.className="selected";
	selectedFportRow.style.background="#cce8ff";
	var curPid=fportData[idx].pid;
	document.getElementById("lblProcess").innerText=" "+curPid;
}

// Terminate the currently selected process
function terminateProcess()
{
	var idx=parseInt(document.getElementById("lblProcess").innerText.trim());
	var curPid=isNaN(idx)?0:idx;
	if(curPid!=0 && confirm("Are you sure you want to terminate process "+curPid) )
	{
		xmlHttp.open("GET", "/pkill?pid="+curPid, false);
		xmlHttp.send(null);
		if(xmlHttp.responseText=="true")
			loadFport();
		else alert(xmlHttp.responseText);
	}
}


function proFile()
{
	var qx=parent.frmLeft.userQX;
	// find selected row
	var rows=document.getElementById("fportTbody").rows;
	var fpath="";
	for(var i=0;i<rows.length;i++) {
		var idx=parseInt(rows[i].getAttribute("data-idx"));
		if(!isNaN(idx)&&fportData[idx]&&fportData[idx].pid==document.getElementById("lblProcess").innerText.trim()) {
			fpath=fportData[idx].pname;
			break;
		}
	}
	if(fpath=="")
		alert("Please select a module to view properties!");
	else
	{
		window.open("proFile.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"_blank","height=350,width=400,resizable=no,scrollbars=no,status=no");
	}
}

function sortFport(colName)
{
	if(fportSortCol==colName) fportSortAsc=!fportSortAsc;
	else { fportSortCol=colName; fportSortAsc=true; }
	fportData.sort(function(a,b) {
		var av=a[colName], bv=b[colName];
		if(!isNaN(parseFloat(av)-parseFloat(bv))) { av=parseFloat(av); bv=parseFloat(bv); }
		if(av<bv) return fportSortAsc?-1:1;
		if(av>bv) return fportSortAsc?1:-1;
		return 0;
	});
	renderFport();
}
