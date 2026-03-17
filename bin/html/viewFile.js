var filepath="";
var bdsphide=false;
var folderData=[];
var fileData=[];
var fileSortCol="";
var fileSortAsc=true;

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
			var flds=(xmlobj.getElementsByTagName("folders")[0] || null);
			if(flds!=null)
			{
				document.getElementById("lblFolderNum").innerText=flds.childNodes.length;
				folderData=[];
				var items=flds.getElementsByTagName("fitem");
				for(var i=0;i<items.length;i++) {
					var it=items[i];
					folderData.push({
						fname:getNodeText(it,"fname"),
						alias:getNodeText(it,"alias"),
						bhide:getNodeText(it,"bhide"),
						hassub:getNodeText(it,"hassub")
					});
				}
				renderFolders();
				document.getElementById("lblFilePath").innerText=filepath;
			}
			var fies=(xmlobj.getElementsByTagName("files")[0] || null);
			if(fies!=null)
			{
				document.getElementById("lblFileNum").innerText=fies.childNodes.length;
				fileData=[];
				var fitems=fies.getElementsByTagName("fitem");
				for(var i=0;i<fitems.length;i++) {
					var it=fitems[i];
					fileData.push({
						fname:getNodeText(it,"fname"),
						bhide:getNodeText(it,"bhide"),
						fsize:getNodeText(it,"fsize"),
						lsize:getNodeText(it,"lsize"),
						ftype:getNodeText(it,"ftype"),
						ftime:getNodeText(it,"ftime")
					});
				}
				renderFiles();
			}
			var retmsg=xmlobj.getElementsByTagName("retmsg");
			if(retmsg.length>0)
				alert((retmsg.item(0).textContent || retmsg.item(0).text));
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

function renderFolders()
{
	var tbody=document.getElementById("folderTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	for(var i=0;i<folderData.length;i++) {
		(function(idx) {
			var d=folderData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.onmousemove=function(){this.style.background="#e5e5e5";};
			tr.onmouseout=function(){this.style.background="#ffffff";};
			tr.style.height="18px";
			var td1=makeCell("&nbsp;"+escHtml(d.hassub)+"&nbsp;","");
			td1.style.cursor="pointer";
			td1.onclick=function(){folderDblClick(tr);};
			tr.appendChild(td1);
			var td2=makeCell(escHtml(d.fname),"");
			td2.style.cursor="pointer";
			td2.setAttribute("nowrap","");
			td2.onclick=function(){folderClick(tr);};
			tr.appendChild(td2);
			var td3=makeCell(escHtml(d.bhide),"center");
			tr.appendChild(td3);
			tbody.appendChild(tr);
		})(i);
	}
}

function renderFiles()
{
	var tbody=document.getElementById("fileTbody");
	while(tbody.firstChild) tbody.removeChild(tbody.firstChild);
	for(var i=0;i<fileData.length;i++) {
		(function(idx) {
			var d=fileData[idx];
			var tr=document.createElement("tr");
			tr.setAttribute("data-idx",String(idx));
			tr.id="fFiles";
			tr.style.cursor="pointer";
			tr.onmousemove=function(){this.style.background="#e5e5e5";};
			tr.onmouseout=function(){this.style.background="#ffffff";};
			tr.onclick=function(){fileClick(tr);};
			// filename input cell
			var td1=document.createElement("td");
			var inp=document.createElement("input");
			inp.type="text";
			inp.className="txtInput_none";
			inp.readOnly=true;
			inp.value=d.fname;
			inp.setAttribute("data-fname",d.fname);
			inp.onclick=function(e){modifyItem(this,e||window.event);};
			inp.onblur=function(){cancelModifyItem(this);};
			inp.onkeypress=function(e){keypressHandler(this,e||window.event);};
			td1.appendChild(inp);
			tr.appendChild(td1);
			tr.appendChild(makeCell(escHtml(d.bhide),"center"));
			var td3=makeCell(escHtml(d.fsize),"right");
			td3.innerHTML="&nbsp;"+td3.innerHTML;
			tr.appendChild(td3);
			var td4=makeCell("&nbsp;"+escHtml(d.ftype),"");
			tr.appendChild(td4);
			var td5=makeCell("&nbsp;"+escHtml(d.ftime),"");
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
	xmlHttp.open("GET", "/filelist?listwhat=1&bdsph="+bdsphide, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );

	var qx=parent.frmLeft.userQX;
	if((qx & ACCESS_FILE_ALL)==ACCESS_FILE_ALL)
	{
		document.getElementById("fRenFolder").disabled=false;
		document.getElementById("fDelFolder").disabled=false;
		document.getElementById("fNewFolder").disabled=false;
		document.getElementById("fRunFile").disabled=false;
		document.getElementById("fDelFile").disabled=false;
		document.getElementById("fUpFile").disabled=false;
	}
}

function folderClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=folderData.length) return;
	var fld=folderData[idx].alias||folderData[idx].fname;
	var fpath=document.getElementById("lblFilePath").innerText;
	document.getElementById("lblFolder").innerText=fld;
	document.getElementById("lblFile").innerHTML="";
	var path=fpath+"\\"+fld;
	if(fpath=="") path=fpath+fld;
	strEncode="listwhat=2&bdsph="+bdsphide+"&path="+path.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}


function folderDblClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=folderData.length) return;
	var fld=folderData[idx].alias||folderData[idx].fname;
	var fpath=document.getElementById("lblFilePath").innerText;
	document.getElementById("lblFolder").innerText="";
	document.getElementById("lblFile").innerHTML="";
	if(fpath=="")
		filepath=fpath+fld;
	else
		filepath=fpath+"\\"+fld;
	strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}

function fileClick(tblElement)
{
	var idx=parseInt(tblElement.getAttribute("data-idx"));
	if(isNaN(idx)||idx<0||idx>=fileData.length) return;
	var fitem=fileData[idx].fname;
	if(fitem=="") document.getElementById("lblFile").innerHTML="";
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
			fpath=fpath+"\\"+fld;
		else fpath=fpath+fld;
		var ssid=parent.frmLeft.ssid;
		document.getElementById("lblFile").innerHTML="<a href=\"/dwfiles/"+ssid+"/"+fitem+"?path="+fpath+"\">"+escHtml(fitem)+"</a>";
	}
}

function goup()
{
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fpath=="")
		alert("Reached the end");
	else
	{
		var p=fpath.lastIndexOf("\\");
		if(p!=-1)
			filepath=fpath.substr(0,p);
		else filepath="";
		strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
		submitIt(strEncode,"/filelist");
	}
}

function goto()
{
	var fpath=document.getElementById("lblFilePath").innerText;
	var new_fpath=prompt("Enter a valid path (no trailing \\)",fpath);
	if(new_fpath!=null && new_fpath!="" )
	{
		filepath=new_fpath;
		strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
		submitIt(strEncode,"/filelist");
	}
}

function refreshFolder()
{
	filepath=document.getElementById("lblFilePath").innerText;
	strEncode="listwhat=1&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}

function refreshFile()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fld!=""){
		if(fpath=="")
			fpath=fpath+fld;
		else
			fpath=fpath+"\\"+fld;
	}
	strEncode="listwhat=2&bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}

function swichHide(e)
{
	if(bdsphide)
	{
		bdsphide=false;
		e.innerText="Show Hidden Files";
	}
	else
	{
		bdsphide=true;
		e.innerText="Hide Hidden Files";
	}
	refreshFile();
}

function delFolder()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fld=="")
		alert("Please select a subfolder to delete!");
	else if( confirm("Are you sure you want to delete subfolder "+fld+"?") )
	{
		document.getElementById("lblFolder").innerText="";
		strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26")+"&fname="+fld.replace(/&/g,"%26");
		submitIt(strEncode,"/folder_del");
	}
}

function renFolder()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fld=="")
		alert("Please select a subfolder to rename!");
	else
	{
		var fld1=prompt("Enter new subfolder name",fld);
		if(fld1!=null && fld1!="" )
		{
		document.getElementById("lblFolder").innerText="";
		strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26")+"&fname="+fld.replace(/&/g,"%26")+"&nname="+fld1.replace(/&/g,"%26");
		submitIt(strEncode,"/folder_ren");
		}
	}
}

function newFolder()
{
	var fpath=document.getElementById("lblFilePath").innerText;
	var fld=prompt("Enter subfolder name","New Folder");
	if(fld!=null && fld!="" )
	{
		strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26")+"&fname="+fld.replace(/&/g,"%26");
		submitIt(strEncode,"/folder_new");
	}
}

function runFile()
{
	var fitem=document.getElementById("lblFile").innerText;
	if(fitem=="")
		alert("Please select a file to run/open remotely!");
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
			fpath=fpath+"\\"+fld;
		else fpath=fpath+fld;
		if(fld!="")
			fpath=fpath+"\\"+fitem;
		else fpath=fpath+fitem;
		if( confirm("Are you sure you want to remotely run/open file "+fitem+"?") )
		{
			strEncode="path="+fpath.replace(/&/g,"%26");
			submitIt(strEncode,"/file_run");
		}
	}
}

function delFile()
{
	var fitem=document.getElementById("lblFile").innerText;
	if(fitem=="")
		alert("Please select a file to delete!");
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if( confirm("Are you sure you want to delete file "+fitem+" in "+fld+"?") )
		{
			if(fpath!="")
			{
				if(fld!="") fpath=fpath+"\\"+fld;
			}
			else fpath=fpath+fld;
			strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26")+"&fname="+fitem.replace(/&/g,"%26");
			document.getElementById("lblFile").innerText="";
			submitIt(strEncode,"/file_del");
		}
	}
}

function renFile()
{
	var fitem=document.getElementById("lblFile").innerText;
	if(fitem=="")
		alert("Please select a file to rename!");
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		var fitem1=prompt("Enter new filename",fitem);
		if(fitem1!=null && fitem1!="" )
		{
			document.getElementById("lblFile").innerText="";
			if(fpath!="")
			{
				if(fld!="") fpath=fpath+"\\"+fld;
			}
			else fpath=fpath+fld;
			strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26")+"&fname="+fitem.replace(/&/g,"%26")+"&nname="+fitem1.replace(/&/g,"%26");
			submitIt(strEncode,"/file_ren");
		}
	}
}

function dwFile()
{
	var fitem=document.getElementById("lblFile").innerText;
	if(fitem=="")
		alert("Please select a file to download!");
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
			fpath=fpath+"\\"+fld;
		else fpath=fpath+fld;
		window.open("/download/"+fitem+"?path="+fpath);
	}
}

function upFile()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fpath!="")
		fpath=fpath+"\\"+fld;
	else fpath=fpath+fld;
	if(fld!="") fpath=fpath+"\\";
	var w=window.open("upload.htm?path="+encodeURIComponent(fpath),"_blank","height=200,width=260,resizable=no,scrollbars=no,status=no");
	if(w) w._refreshCallback=function(){ refreshFile(); };
}

function proFile()
{
	var qx=parent.frmLeft.userQX;
	var fitem=document.getElementById("lblFile").innerText;
	if(fitem=="")
		alert("Please select a file to view properties!");
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
			fpath=fpath+"\\"+fld;
		else fpath=fpath+fld;
		if(fld!="")
			fpath=fpath+"\\"+fitem;
		else fpath=fpath+fitem;
		window.open("proFile.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"_blank","height=350,width=400,resizable=no,scrollbars=no,status=no");
	}
}

function proFolder()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	var qx=parent.frmLeft.userQX;
	if(fpath=="")
	{
		if(fld=="")
			alert("Please select a folder to view properties!");
		else
			window.open("proDrive.htm?qx="+qx+"&path="+encodeURIComponent(fld),"_blank","height=350,width=400,resizable=no,scrollbars=no,status=no");
	}
	else
	{
		fpath=fpath+"\\"+fld;
		window.open("proFolder.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"_blank","height=350,width=400,resizable=no,scrollbars=no,status=no");
	}
}


function keypressHandler(txtElement, e)
{
	if(document.getElementById("fUpFile").disabled) return;
	var kc=e?e.keyCode:0;
	var ctrl=e?e.ctrlKey:false;
	if((kc==10 || kc==13) && ctrl)
	{
		var tr=txtElement.parentNode.parentNode;
		var idx=parseInt(tr.getAttribute("data-idx"));
		if(isNaN(idx)||idx<0||idx>=fileData.length) return;
		var fname=fileData[idx].fname;
		var nname=txtElement.value;
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
		{
			if(fld!="") fpath=fpath+"\\"+fld;
		}
		else fpath=fpath+fld;
		document.getElementById("lblFile").innerHTML="";
		strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26");
		strEncode=strEncode+"&fname="+fname.replace(/&/g,"%26");
		strEncode=strEncode+"&nname="+nname.replace(/&/g,"%26");
		submitIt(strEncode,"/file_ren");
		if(e.preventDefault) e.preventDefault(); else e.returnValue=false;
	}
}

function cancelModifyItem(txtElement)
{
	var tr=txtElement.parentNode.parentNode;
	var idx=parseInt(tr.getAttribute("data-idx"));
	if(!isNaN(idx)&&idx>=0&&idx<fileData.length) txtElement.value=fileData[idx].fname;
	txtElement.className="txtInput_none";
	txtElement.readOnly=true;
	document.getElementById("lblHelp").innerHTML="";
}

function modifyItem(txtElement, e)
{
	if(document.getElementById("fUpFile").disabled) return;
	if(txtElement.readOnly==false) return;
	txtElement.className="txtInput_normal";
	txtElement.readOnly=false;
	document.getElementById("lblHelp").innerHTML="(<font color=red>Press Ctrl+Enter to save changes</font>)";
}

function sortFiles(colName)
{
	if(fileSortCol==colName) fileSortAsc=!fileSortAsc;
	else { fileSortCol=colName; fileSortAsc=true; }
	fileData.sort(function(a,b) {
		var av=a[colName], bv=b[colName];
		if(!isNaN(parseFloat(av)-parseFloat(bv))) { av=parseFloat(av); bv=parseFloat(bv); }
		if(av<bv) return fileSortAsc?-1:1;
		if(av>bv) return fileSortAsc?1:-1;
		return 0;
	});
	renderFiles();
}
