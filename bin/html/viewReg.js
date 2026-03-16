var regpath="";

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // Check object state
		if (xmlHttp.status == 200) { // Data returned successfully, start processing
			
			var xmlobj = xmlHttp.responseXML;
			var rkeys=(xmlobj.getElementsByTagName("regkeys")[0] || null)
			if(rkeys!=null && rkeys.childNodes.length>0)
			{
				document.getElementById("lblKeyNum").innerText=rkeys.childNodes.length;
				if(regkeyXML.documentElement!=null)
					regkeyXML.removeChild(regkeyXML.documentElement);
				regkeyXML.appendChild(rkeys);
				document.getElementById("lblRegPath").innerText=regpath;
			}
			var ritems=(xmlobj.getElementsByTagName("regitems")[0] || null)
			if(ritems!=null && ritems.childNodes.length>0)
    			{
    				
    				if(regitemXML.documentElement!=null)
    					regitemXML.removeChild(regitemXML.documentElement);
				regitemXML.appendChild(ritems);
    			}
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
	var row=tblElement.rowIndex;
	regkeyXML.recordset.absoluteposition=row;
	var regkey=regkeyXML.recordset("regkey");
	var rpath=document.getElementById("lblRegPath").innerText;
	
	document.getElementById("lblRegKey").innerText=regkey;
	document.getElementById("lblRegItem").innerText="";
	// MIME encode the special character & using .replace(/&/g,"%26")
	var rkey=rpath+"\\"+regkey;
	strEncode="listwhat=2&rkey="+rkey.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}


function regkeyDblClick(tblElement)
{
	var row=tblElement.rowIndex;
	regkeyXML.recordset.absoluteposition=row;
	var regkey=regkeyXML.recordset("regkey");
	var rpath=document.getElementById("lblRegPath").innerText;
	
	document.getElementById("lblRegKey").innerText="";
	document.getElementById("lblRegItem").innerText="";
	regpath=rpath+"\\"+regkey;
	// MIME encode the special character & using .replace(/&/g,"%26")
	strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function regItemClick(tblElement)
{
	var row=tblElement.rowIndex;
	regitemXML.recordset.absoluteposition=row;
	var id=regitemXML.recordset("id");
	if(id!="")
		document.getElementById("lblRegItem").innerText=regitemXML.recordset("rname");
}

function goup()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var p=rpath.lastIndexOf("\\");
	if(p!=-1)
	{
		regpath=rpath.substr(0,p);
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
    		strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
		submitIt(strEncode,"/reglist");
	}
}

function refreshKey()
{
	regpath=document.getElementById("lblRegPath").innerText;
	// MIME encode the special character & using .replace(/&/g,"%26")
	strEncode="listwhat=1&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function refreshItem()
{
	var regkey=document.getElementById("lblRegKey").innerText;
	var rpath=document.getElementById("lblRegPath").innerText;
	if(regkey!="") rpath=rpath+"\\"+regkey;
	// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
		// MIME encode the special character & using .replace(/&/g,"%26")
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
			// MIME encode the special character & using .replace(/&/g,"%26")
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

function addItem()
{
	var v=window.showModalDialog("addRegitem.htm","","dialogHeight:300px;dialogWidth=300px;center:yes;status:no;scroll:no;");
	if(v!=null && v!="")
	{
		var regkey=document.getElementById("lblRegKey").innerText;
		var rpath=document.getElementById("lblRegPath").innerText;
		// MIME encode the special character & using .replace(/&/g,"%26")
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

function keypress(txtElement)
{
	if(document.getElementById("fAddItem").disabled) return;
	if(window.event.keyCode==10 && window.event.ctrlKey)
	{
		var tblElement=txtElement.parentElement.parentElement;
		var row=tblElement.rowIndex;
		regitemXML.recordset.absoluteposition=row;
		if(regitemXML.recordset("id")!="")
		{
			var rname=""+regitemXML.recordset("rname");
			var rtype=regitemXML.recordset("rtype")
			var rvalue=""+txtElement.value;
			var regkey=document.getElementById("lblRegKey").innerText;
			var rpath=document.getElementById("lblRegPath").innerText;
			var strEncode="";
			// MIME encode the special character & using .replace(/&/g,"%26")
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
		window.event.keyCode=0;
	}
}
function cancelModifyItem(txtElement)
{
	txtElement.dataFld="rdata";
	txtElement.className="txtInput_none";
	txtElement.readOnly=true;
	document.getElementById("lblHelp").innerHTML="";
}
function modifyItem(txtElement)
{
	if(document.getElementById("fAddItem").disabled) return;
	if(txtElement.readOnly==false) return;
	var tblElement=txtElement.parentElement.parentElement;
	var row=tblElement.rowIndex;
	regitemXML.recordset.absoluteposition=row;
	if(regitemXML.recordset("id")!="")
	{
		txtElement.dataFld="";
		txtElement.value=regitemXML.recordset("rdata");
		txtElement.className="txtInput_normal";
		txtElement.readOnly=false;
		document.getElementById("lblHelp").innerHTML="(<font color=red>Press Ctrl+Enter to save changes</font>)"
	}
}

