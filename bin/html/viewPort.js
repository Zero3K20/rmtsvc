
function window_onload()
{
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
}

function processClick(tblElement)
{
	var row=tblElement.rowIndex;
	fportXML.recordset.absoluteposition=row;
	var curPid=fportXML.recordset("pid");
	document.getElementById("lblProcess").innerText=" "+curPid;
}

// Kill the currently selected process
function pkill()
{
	var curPid=fportXML.recordset("pid");
	if(curPid!=0 && confirm("Are you sure you want to kill process "+curPid) )
	{
		xmlHttp.open("GET", "/pkill?pid="+curPid, false);
    		xmlHttp.send(null);
    		if(xmlHttp.responseText=="true")
    			fportXML.src='/fport';
    		else alert(xmlHttp.responseText);
    	}
}


function proFile()
{
	var qx=parent.frmLeft.userQX;
	var fpath=fportXML.recordset("pname");
	if(fpath=="")
		alert("Please select a module to view properties!");
	else
	{
		window.showModalDialog("proFile.htm",qx+","+fpath,"dialogHeight: 350px;dialogWidth: 400px;center: yes;resizable: no;scroll: no;status: no");
	}
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
