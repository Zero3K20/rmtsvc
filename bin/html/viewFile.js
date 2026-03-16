var filepath="";
var bdsphide=false;

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // Check object state
		if (xmlHttp.status == 200) { // Data returned successfully, start processing
			
			//alert(xmlHttp.responseText);
			var xmlobj = xmlHttp.responseXML;
			var flds=(xmlobj.getElementsByTagName("folders")[0] || null)
			if(flds!=null)
			{
				document.getElementById("lblFolderNum").innerText=flds.childNodes.length;
				if(folderXML.documentElement!=null)
					folderXML.removeChild(folderXML.documentElement);
				if(flds.childNodes.length>0) folderXML.appendChild(flds);
				document.getElementById("lblFilePath").innerText=filepath;
			}
			var fies=(xmlobj.getElementsByTagName("files")[0] || null)
			if(fies!=null)
    			{
    				document.getElementById("lblFileNum").innerText=fies.childNodes.length;
    				if(fileXML.documentElement!=null)
    					fileXML.removeChild(fileXML.documentElement);
				 if(fies.childNodes.length>0) fileXML.appendChild(fies);
    			}
    			var retmsg=xmlobj.getElementsByTagName("retmsg");
    			if(retmsg.length>0)
				alert((retmsg.item(0).textContent || retmsg.item(0).text));
            	} //else alert("Request error,status="+xmlHttp.status);
            	hidePopup();
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
	var row=tblElement.rowIndex;
	folderXML.recordset.absoluteposition=row;
	var fld=folderXML.recordset("alias");
	if(fld=="") fld=folderXML.recordset("fname");
	var fpath=document.getElementById("lblFilePath").innerText;
	
	document.getElementById("lblFolder").innerText=fld;
	document.getElementById("lblFile").innerHTML="";
	// MIME encode the special character & using .replace(/&/g,"%26")
	var path=fpath+"\\"+fld;
	if(fpath=="") path=fpath+fld;
	strEncode="listwhat=2&bdsph="+bdsphide+"&path="+path.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}


function folderDblClick(tblElement)
{
	var row=tblElement.rowIndex;
	folderXML.recordset.absoluteposition=row;
	var fld=folderXML.recordset("alias");
	if(fld=="") fld=folderXML.recordset("fname");
	var fpath=document.getElementById("lblFilePath").innerText;
	
	document.getElementById("lblFolder").innerText="";
	document.getElementById("lblFile").innerHTML="";
	if(fpath=="")
		filepath=fpath+fld;
	else 
		filepath=fpath+"\\"+fld;
	// MIME encode the special character & using .replace(/&/g,"%26")
	strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
	submitIt(strEncode,"/filelist");
}

function fileClick(tblElement)
{
	var row=tblElement.rowIndex;
	fileXML.recordset.absoluteposition=row;
	var fitem=fileXML.recordset("fname");
	if(fitem=="") document.getElementById("lblFile").innerHTML="";
	else
	{
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
			fpath=fpath+"\\"+fld;
		else	fpath=fpath+fld;
		var ssid=parent.frmLeft.ssid;
		document.getElementById("lblFile").innerHTML="<a href=\"/dwfiles/"+ssid+"/"+fitem+"?path="+fpath+"\">"+fitem+"</a>";
		
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
    		strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
		submitIt(strEncode,"/filelist");
	}
}

function refreshFolder()
{
	filepath=document.getElementById("lblFilePath").innerText;
	// MIME encode the special character & using .replace(/&/g,"%26")
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
	// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		else	fpath=fpath+fld;
		if(fld!="")
			fpath=fpath+"\\"+fitem;
		else	fpath=fpath+fitem;
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
			else	fpath=fpath+fld;
			// MIME encode the special character & using .replace(/&/g,"%26")
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
			else	fpath=fpath+fld;
			// MIME encode the special character & using .replace(/&/g,"%26")
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
		else	fpath=fpath+fld;
		window.open("/download/"+fitem+"?path="+fpath);
	}
}

function upFile()
{
	var fld=document.getElementById("lblFolder").innerText;
	var fpath=document.getElementById("lblFilePath").innerText;
	if(fpath!="")
		fpath=fpath+"\\"+fld;
	else	fpath=fpath+fld;
	if(fld!="") fpath=fpath+"\\";
	var ret=window.showModalDialog("upload.htm",fpath,"dialogHeight: 200px;dialogWidth: 260px;center: yes;resizable: no;scroll: no;status: no");
	if(ret==1) refreshFile();
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
		else	fpath=fpath+fld;
		if(fld!="")
			fpath=fpath+"\\"+fitem;
		else	fpath=fpath+fitem;
		window.showModalDialog("proFile.htm",qx+","+fpath,"dialogHeight: 350px;dialogWidth: 400px;center: yes;resizable: no;scroll: no;status: no");
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
			window.showModalDialog("proDrive.htm",qx+","+fld,"dialogHeight: 350px;dialogWidth: 400px;center: yes;resizable: no;scroll: no;status: no");
	}
	else
	{
		fpath=fpath+"\\"+fld;
		window.showModalDialog("proFolder.htm",qx+","+fpath,"dialogHeight: 350px;dialogWidth: 400px;center: yes;resizable: no;scroll: no;status: no");
	}
}


function keypress(txtElement)
{
	if(document.getElementById("fUpFile").disabled) return;
	if(window.event.keyCode==10 && window.event.ctrlKey)
	{
		var tblElement=txtElement.parentElement.parentElement;
		var row=tblElement.rowIndex;
		fileXML.recordset.absoluteposition=row;
		var fname=""+fileXML.recordset("fname");
		var nname=txtElement.value;
		var fld=document.getElementById("lblFolder").innerText;
		var fpath=document.getElementById("lblFilePath").innerText;
		if(fpath!="")
		{
			if(fld!="") fpath=fpath+"\\"+fld;
		}
		else	fpath=fpath+fld;
		document.getElementById("lblFile").innerHTML="";
		// MIME encode the special character & using .replace(/&/g,"%26")
		strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26");
		strEncode=strEncode+"&fname="+fname.replace(/&/g,"%26");
		strEncode=strEncode+"&nname="+nname.replace(/&/g,"%26");
		submitIt(strEncode,"/file_ren");
		
		window.event.keyCode=0;
	}
}
function cancelModifyItem(txtElement)
{
	txtElement.dataFld="fname";
	txtElement.className="txtInput_none";
	txtElement.readOnly=true;
	document.getElementById("lblHelp").innerHTML="";
}
function modifyItem(txtElement)
{
	if(document.getElementById("fUpFile").disabled) return;
	if(txtElement.readOnly==false) return;
	var tblElement=txtElement.parentElement.parentElement;
	var row=tblElement.rowIndex;
	fileXML.recordset.absoluteposition=row;
	txtElement.dataFld="";
	txtElement.value=fileXML.recordset("fname");
	txtElement.className="txtInput_normal";
	txtElement.readOnly=false;
	document.getElementById("lblHelp").innerHTML="(<font color=red>Press Ctrl+Enter to save changes</font>)"
}
//----------------sort func--------------------------
function sort(xmlObj, xslObj, sortByColName) 
{ 
try {
var xmlData=document.getElementById(xmlObj) && document.getElementById(xmlObj).XMLDocument;
var xslData=document.getElementById(xslObj) && document.getElementById(xslObj).XMLDocument;
if(!xmlData || !xslData) return;
var nodes=xslData.documentElement.selectSingleNode("xsl:for-each"); 
var s=nodes.selectSingleNode("@order-by").value;
if(s.substr(1)==sortByColName)
{
	if(s.charAt(0)=="+")
		s="-"+sortByColName;
	else s="+"+sortByColName;
}
else
	s="+"+sortByColName;
nodes.selectSingleNode("@order-by").value=s;

xmlData.documentElement.transformNodeToObject(xslData.documentElement,xmlData); 
} catch(e) {} 
} 
