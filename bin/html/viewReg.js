var regpath="";
var regkeyData=[];
var regitemData=[];
var xmlReg=null;

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
			var rkeys=(xmlobj.getElementsByTagName("regkeys")[0] || null);
			if(rkeys!=null && rkeys.childNodes.length>0)
			{
				document.getElementById("lblKeyNum").innerText=rkeys.childNodes.length;
				regkeyData=[];
				var keys=rkeys.getElementsByTagName("kitem");
				for(var i=0;i<keys.length;i++) {
					var k=keys[i];
					regkeyData.push({
						regkey:getNodeText(k,"regkey"),
						subkeys:getNodeText(k,"subkeys")
					});
				}
				renderRegkeys();
				document.getElementById("lblRegPath").innerText=regpath;
			}
			var ritems=(xmlobj.getElementsByTagName("regitems")[0] || null);
			if(ritems!=null && ritems.childNodes.length>0)
			{
				regitemData=[];
				var items=ritems.getElementsByTagName("vitem");
				for(var i=0;i<items.length;i++) {
					var it=items[i];
					regitemData.push({
						id:getNodeText(it,"id"),
						rtype:getNodeText(it,"rtype"),
						rdlen:getNodeText(it,"rdlen"),
						rname:getNodeText(it,"rname"),
						rdata:getNodeText(it,"rdata")
					});
				}
				renderRegitems();
			}
		} //else alert("Request error,status="+xmlHttp.status);
		hidePopup();
	}
}

function makeCell(content, align) {
	var td=document.createElement("td");
	if(align) td.align=align;
	td.innerHTML=content;
	return td;
}

function renderRegkeys()
{
	var tbody=document.getElementById("regkeyTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	for(var i=0;i<regkeyData.length;i++) {
		(function(idx) {
			var d=regkeyData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.onmousemove=function(){this.style.background="#e5e5e5";};
			tr.onmouseout=function(){this.style.background="#ffffff";};
			tr.style.height="18px";
			var td1=makeCell("&nbsp;"+escHtml(d.subkeys)+"&nbsp;","");
			td1.style.cursor="pointer";
			td1.onclick=function(){regkeyDblClick(tr);};
			tr.appendChild(td1);
			var td2=makeCell(escHtml(d.regkey),"");
			td2.style.cursor="pointer";
			td2.setAttribute("nowrap","");
			td2.onclick=function(){regkeyClick(tr);};
			tr.appendChild(td2);
			tbody.appendChild(tr);
		})(i);
	}
}

function renderRegitems()
{
	var tbody=document.getElementById("regitemTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	for(var i=0;i<regitemData.length;i++) {
		(function(idx) {
			var d=regitemData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.id="fItems";
			tr.style.cursor="pointer";
			tr.style.height="18px";
			tr.onmousemove=function(){this.style.background="#e5e5e5";};
			tr.onmouseout=function(){this.style.background="#ffffff";};
			tr.onclick=function(){regItemClick(tr);};
			tr.appendChild(makeCell(escHtml(d.id),"right"));
			tr.appendChild(makeCell(escHtml(d.rtype),"center"));
			tr.appendChild(makeCell(escHtml(d.rdlen),"center"));
			tr.appendChild(makeCell(escHtml(d.rname),""));
			// data textarea cell
			var td5=document.createElement("td");
			td5.style.verticalAlign="middle";
			var ta=document.createElement("textarea");
			ta.className="txtInput_none";
			ta.readOnly=true;
			ta.value=d.rdata;
			ta.setAttribute("data-rdata",d.rdata);
			ta.onclick=function(e){modifyItem(this,e||window.event);};
			ta.onblur=function(){cancelModifyItem(this);};
			ta.onkeypress=function(e){keypressHandler(this,e||window.event);};
			td5.appendChild(ta);
			tr.appendChild(td5);
			tbody.appendChild(tr);
		})(i);
	}
}

function submitIt(strEncode,strurl)
{
	showPopup(250, 200, 150, 20);
	xmlHttp.abort();
	xmlHttp.open("POST", strurl, true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( strEncode );
}

function window_onload()
{
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/reglist?listwhat=1", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );

	var qx=parent.frmLeft.userQX;
	if((qx & ACCESS_REGIST_ALL)==ACCESS_REGIST_ALL)
	{
		document.getElementById("fDelkey").disabled=false;
		document.getElementById("fAddkey").disabled=false;
		document.getElementById("fAddItem").disabled=false;
		document.getElementById("fDelItem").disabled=false;
	}
}

function regkeyClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=regkeyData.length) return;
	var regkey=regkeyData[idx].regkey;
	var rpath=document.getElementById("lblRegPath").innerText;
	document.getElementById("lblRegKey").innerText=regkey;
	document.getElementById("lblRegItem").innerText="";
	var rkey=rpath+"\\"+regkey;
	strEncode="listwhat=2&rkey="+rkey.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}


function regkeyDblClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=regkeyData.length) return;
	var regkey=regkeyData[idx].regkey;
	var rpath=document.getElementById("lblRegPath").innerText;
	document.getElementById("lblRegKey").innerText="";
	document.getElementById("lblRegItem").innerText="";
	regpath=rpath+"\\"+regkey;
	strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function regItemClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=regitemData.length) return;
	var d=regitemData[idx];
	if(d.id!="")
		document.getElementById("lblRegItem").innerText=d.rname;
}

function goup()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var p=rpath.lastIndexOf("\\");
	if(p!=-1)
	{
		regpath=rpath.substr(0,p);
		strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
		submitIt(strEncode,"/reglist");
	}
	else alert("Reached the end");
}

function goto()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var new_rpath=prompt("Enter a valid path",rpath);
	if(new_rpath!=null && new_rpath!="" )
	{
		regpath=new_rpath;
		strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
		submitIt(strEncode,"/reglist");
	}
}

function refreshKey()
{
	regpath=document.getElementById("lblRegPath").innerText;
	strEncode="listwhat=1&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function refreshItem()
{
	var regkey=document.getElementById("lblRegKey").innerText;
	var rpath=document.getElementById("lblRegPath").innerText;
	if(regkey!="") rpath=rpath+"\\"+regkey;
	strEncode="listwhat=2&rkey="+rpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function delKey()
{
	var regkey=document.getElementById("lblRegKey").innerText;
	var rpath=document.getElementById("lblRegPath").innerText;
	if(regkey=="")
		alert("Please select a subkey to delete!");
	else if( confirm("Are you sure you want to delete subkey "+regkey+"?") )
	{
		strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rkey="+regkey.replace(/&/g,"%26");
		submitIt(strEncode,"/regkey_del");
	}
}

function addKey()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var regkey=prompt("Enter subkey name","");
	if(regkey!=null && regkey!="" )
	{
		strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rkey="+regkey.replace(/&/g,"%26");
		submitIt(strEncode,"/regkey_add");
	}
}



function delItem()
{
	var regitem=document.getElementById("lblRegItem").innerText;
	if(regitem=="")
		alert("Please select an item to delete!");
	else
	{
		var regkey=document.getElementById("lblRegKey").innerText;
		var rpath=document.getElementById("lblRegPath").innerText;
		if( confirm("Are you sure you want to delete item "+regitem+" under subkey "+regkey+"?") )
		{
			if(regkey!="")
			{
				var rtmp=rpath+"\\"+regkey;
				strEncode="rpath="+rtmp.replace(/&/g,"%26")+"&rname="+regitem.replace(/&/g,"%26");
			}
			else
				strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rname="+regitem.replace(/&/g,"%26");
			document.getElementById("lblRegItem").innerText="";
			submitIt(strEncode,"/regitem_del");
		}
	}
}

function addItemCallback(v)
{
	if(v!=null && v!="")
	{
		var regkey=document.getElementById("lblRegKey").innerText;
		var rpath=document.getElementById("lblRegPath").innerText;
		if(regkey!="")
		{
			var rtmp=rpath+"\\"+regkey;
			strEncode="rpath="+rtmp.replace(/&/g,"%26")+"&"+v;
		}
		else
			strEncode="rpath="+rpath.replace(/&/g,"%26")+"&"+v;
		submitIt(strEncode,"/regitem_add");
	}
}

function addItem()
{
	var w=window.open("addRegitem.htm","_blank","height=300,width=300,resizable=no,scrollbars=no,status=no");
	if(w) w._addItemCallback=addItemCallback;
}

function keypressHandler(txtElement, e)
{
	if(document.getElementById("fAddItem").disabled) return;
	var kc=e?e.keyCode:0;
	var ctrl=e?e.ctrlKey:false;
	if((kc==10 || kc==13) && ctrl)
	{
		var tr=txtElement.parentNode.parentNode;
		var idx=parseInt(tr.getAttribute("data-idx"));
		if(isNaN(idx)||idx<0||idx>=regitemData.length) return;
		var d=regitemData[idx];
		if(d.id!="")
		{
			var rname=d.rname;
			var rtype=d.rtype;
			var rvalue=txtElement.value;
			var regkey=document.getElementById("lblRegKey").innerText;
			var rpath=document.getElementById("lblRegPath").innerText;
			var strEncode="";
			if(regkey!="")
			{
				var rtmp=rpath+"\\"+regkey;
				strEncode="rpath="+rtmp.replace(/&/g,"%26");
				strEncode=strEncode+"&rtype="+rtype;
				strEncode=strEncode+"&rname="+rname.replace(/&/g,"%26");
				strEncode=strEncode+"&rdata="+rvalue.replace(/&/g,"%26");
			}
			else
			{
				strEncode="rpath="+rpath.replace(/&/g,"%26");
				strEncode=strEncode+"&rtype="+rtype;
				strEncode=strEncode+"&rname="+rname.replace(/&/g,"%26");
				strEncode=strEncode+"&rdata="+rvalue.replace(/&/g,"%26");
			}
			submitIt(strEncode,"/regitem_md");
		}
		if(e.preventDefault) e.preventDefault(); else e.returnValue=false;
	}
}

function cancelModifyItem(txtElement)
{
	var tr=txtElement.parentNode.parentNode;
	var idx=parseInt(tr.getAttribute("data-idx"));
	if(!isNaN(idx)&&idx>=0&&idx<regitemData.length) txtElement.value=regitemData[idx].rdata;
	txtElement.className="txtInput_none";
	txtElement.readOnly=true;
	document.getElementById("lblHelp").innerHTML="";
}

function modifyItem(txtElement, e)
{
	if(document.getElementById("fAddItem").disabled) return;
	if(txtElement.readOnly==false) return;
	var tr=txtElement.parentNode.parentNode;
	var idx=parseInt(tr.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=regitemData.length) return;
	if(regitemData[idx].id!="")
	{
		txtElement.className="txtInput_normal";
		txtElement.readOnly=false;
		document.getElementById("lblHelp").innerHTML="(<font color=red>Press Ctrl+Enter to save changes</font>)";
	}
}
