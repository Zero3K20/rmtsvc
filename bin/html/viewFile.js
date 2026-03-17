var filepath="";
var bdsphide=false;
var folderData=[];
var fileData=[];
var selectedFileRow=-1;
var sortCol="";
var sortAsc=true;

function getNodeText(node, tagname) {
var child=node.getElementsByTagName(tagname)[0];
if(child) return (child.textContent || child.text || "");
return "";
}

function processRequest() 
{
if (xmlHttp.readyState == 4) { // Check object state
if (xmlHttp.status == 200) { // Data returned successfully, start processing

var xmlobj = xmlHttp.responseXML;
var flds=(xmlobj.getElementsByTagName("folders")[0] || null);
if(flds!=null)
{
folderData=[];
for(var i=0;i<flds.childNodes.length;i++)
{
var n=flds.childNodes[i];
if(n.nodeType!=1) continue;
folderData.push({
hassub: getNodeText(n,"hassub"),
fname:  getNodeText(n,"fname"),
alias:  getNodeText(n,"alias"),
bhide:  getNodeText(n,"bhide")
});
}
renderFolderTable();
document.getElementById("lblFolderNum").innerText=folderData.length;
document.getElementById("lblFilePath").innerText=filepath;
}
var fies=(xmlobj.getElementsByTagName("files")[0] || null);
if(fies!=null)
{
fileData=[];
for(var i=0;i<fies.childNodes.length;i++)
{
var n=fies.childNodes[i];
if(n.nodeType!=1) continue;
fileData.push({
fname: getNodeText(n,"fname"),
bhide: getNodeText(n,"bhide"),
fsize: getNodeText(n,"fsize"),
lsize: getNodeText(n,"lsize"),
ftype: getNodeText(n,"ftype"),
ftime: getNodeText(n,"ftime")
});
}
if(sortCol!="") applySortToFileData();
renderFileTable();
document.getElementById("lblFileNum").innerText=fileData.length;
}
var retmsg=xmlobj.getElementsByTagName("retmsg");
if(retmsg.length>0)
alert((retmsg.item(0).textContent || retmsg.item(0).text));
} //else alert("Request error,status="+xmlHttp.status);
hidePopup();
}
}

function renderFolderTable()
{
var tbody=document.getElementById("tblFolderBody");
tbody.innerHTML="";
for(var i=0;i<folderData.length;i++)
{
var tr=document.createElement("tr");
tr.style.background="#ffffff";
tr.style.height="18px";
tr.onmousemove=function(){ this.style.background="#e5e5e5"; };
tr.onmouseout=function(){ this.style.background="#ffffff"; };

var td1=document.createElement("td");
td1.style.cursor="pointer";
td1.innerHTML="&nbsp;"+(folderData[i].hassub||"")+"&nbsp;";
td1.onclick=(function(idx){ return function(){ folderDblClick(idx); }; })(i);

var td2=document.createElement("td");
td2.style.cursor="pointer";
td2.style.whiteSpace="nowrap";
td2.innerText=folderData[i].fname;
td2.onclick=(function(idx){ return function(){ folderClick(idx); }; })(i);

var td3=document.createElement("td");
td3.align="center";
td3.innerText=folderData[i].bhide||"";

tr.appendChild(td1);
tr.appendChild(td2);
tr.appendChild(td3);
tbody.appendChild(tr);
}
}

function renderFileTable()
{
selectedFileRow=-1;
var tbody=document.getElementById("tblFileBody");
tbody.innerHTML="";
for(var i=0;i<fileData.length;i++)
{
var tr=document.createElement("tr");
tr.style.background="#ffffff";
tr.style.cursor="pointer";
tr.onmousemove=function(){ this.style.background="#e5e5e5"; };
tr.onmouseout=function(){ this.style.background="#ffffff"; };
tr.onclick=(function(idx){ return function(){ fileClick(idx); }; })(i);

var td1=document.createElement("td");
var inp=document.createElement("input");
inp.type="text";
inp.className="txtInput_none";
inp.value=fileData[i].fname;
inp.readOnly=true;
inp.onclick=(function(idx){ return function(e){ modifyItem(this,idx); e.stopPropagation(); }; })(i);
inp.onblur=function(){ cancelModifyItem(this); };
inp.onkeypress=(function(idx){ return function(e){ keypressHandler(this,idx,e); }; })(i);
td1.appendChild(inp);

var td2=document.createElement("td");
td2.align="center";
td2.innerText=fileData[i].bhide||"";

var td3=document.createElement("td");
td3.align="right";
td3.innerText=fileData[i].fsize||"\u00a0";

var td4=document.createElement("td");
td4.innerHTML="&nbsp;"+(fileData[i].ftype||"");

var td5=document.createElement("td");
td5.innerHTML="&nbsp;"+(fileData[i].ftime||"");

tr.appendChild(td1);
tr.appendChild(td2);
tr.appendChild(td3);
tr.appendChild(td4);
tr.appendChild(td5);
tbody.appendChild(tr);
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

function folderClick(idx)
{
var fld=folderData[idx].alias||folderData[idx].fname;
var fpath=document.getElementById("lblFilePath").innerText;

document.getElementById("lblFolder").innerText=fld;
document.getElementById("lblFile").innerHTML="";
// MIME encode the special character & using .replace(/&/g,"%26")
var path=fpath+"\\"+fld;
if(fpath=="") path=fld;
strEncode="listwhat=2&bdsph="+bdsphide+"&path="+path.replace(/&/g,"%26");
submitIt(strEncode,"/filelist");
}


function folderDblClick(idx)
{
var fld=folderData[idx].alias||folderData[idx].fname;
var fpath=document.getElementById("lblFilePath").innerText;

document.getElementById("lblFolder").innerText="";
document.getElementById("lblFile").innerHTML="";
if(fpath=="")
filepath=fld;
else 
filepath=fpath+"\\"+fld;
// MIME encode the special character & using .replace(/&/g,"%26")
strEncode="listwhat=3&bdsph="+bdsphide+"&path="+filepath.replace(/&/g,"%26");
submitIt(strEncode,"/filelist");
}

function fileClick(idx)
{
selectedFileRow=idx;
var fitem=fileData[idx].fname;
if(fitem=="") document.getElementById("lblFile").innerHTML="";
else
{
var fld=document.getElementById("lblFolder").innerText;
var fpath=document.getElementById("lblFilePath").innerText;
if(fpath!="")
fpath=fpath+"\\"+fld;
elsefpath=fpath+fld;
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
elsefpath=fpath+fld;
if(fld!="")
fpath=fpath+"\\"+fitem;
elsefpath=fpath+fitem;
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
elsefpath=fpath+fld;
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
elsefpath=fpath+fld;
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
elsefpath=fpath+fld;
window.open("/download/"+fitem+"?path="+fpath);
}
}

function upFile()
{
var fld=document.getElementById("lblFolder").innerText;
var fpath=document.getElementById("lblFilePath").innerText;
if(fpath!="")
fpath=fpath+"\\"+fld;
elsefpath=fpath+fld;
if(fld!="") fpath=fpath+"\\";
window.open("upload.htm?path="+encodeURIComponent(fpath),"","height=200,width=260,directories=no,location=no,menubar=no,status=no,toolbar=no,resizable=no,scrollbars=no");
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
elsefpath=fpath+fld;
if(fld!="")
fpath=fpath+"\\"+fitem;
elsefpath=fpath+fitem;
window.open("proFile.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"","height=350,width=400,directories=no,location=no,menubar=no,status=no,toolbar=no,resizable=no,scrollbars=no");
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
window.open("proDrive.htm?qx="+qx+"&path="+encodeURIComponent(fld),"","height=350,width=400,directories=no,location=no,menubar=no,status=no,toolbar=no,resizable=no,scrollbars=no");
}
else
{
fpath=fpath+"\\"+fld;
window.open("proFolder.htm?qx="+qx+"&path="+encodeURIComponent(fpath),"","height=350,width=400,directories=no,location=no,menubar=no,status=no,toolbar=no,resizable=no,scrollbars=no");
}
}


function keypressHandler(txtElement,idx,e)
{
if(document.getElementById("fUpFile").disabled) return;
var keyCode=e.keyCode || e.which;
if(keyCode==10 && e.ctrlKey)
{
var fname=fileData[idx].fname;
var nname=txtElement.value;
var fld=document.getElementById("lblFolder").innerText;
var fpath=document.getElementById("lblFilePath").innerText;
if(fpath!="")
{
if(fld!="") fpath=fpath+"\\"+fld;
}
elsefpath=fpath+fld;
document.getElementById("lblFile").innerHTML="";
// MIME encode the special character & using .replace(/&/g,"%26")
strEncode="bdsph="+bdsphide+"&path="+fpath.replace(/&/g,"%26");
strEncode=strEncode+"&fname="+fname.replace(/&/g,"%26");
strEncode=strEncode+"&nname="+nname.replace(/&/g,"%26");
submitIt(strEncode,"/file_ren");

e.preventDefault();
}
}
function cancelModifyItem(txtElement)
{
if(selectedFileRow>=0 && selectedFileRow<fileData.length)
txtElement.value=fileData[selectedFileRow].fname;
txtElement.className="txtInput_none";
txtElement.readOnly=true;
document.getElementById("lblHelp").innerHTML="";
}
function modifyItem(txtElement,idx)
{
if(document.getElementById("fUpFile").disabled) return;
if(txtElement.readOnly==false) return;
selectedFileRow=idx;
txtElement.value=fileData[idx].fname;
txtElement.className="txtInput_normal";
txtElement.readOnly=false;
document.getElementById("lblHelp").innerHTML="(<font color=red>Press Ctrl+Enter to save changes</font>)"
}
//----------------sort func--------------------------
function applySortToFileData()
{
var col=sortCol;
var asc=sortAsc;
fileData.sort(function(a,b){
var va=a[col]||"";
var vb=b[col]||"";
if(va<vb) return asc?-1:1;
if(va>vb) return asc?1:-1;
return 0;
});
}
function sortFiles(colName)
{
if(sortCol==colName)
sortAsc=!sortAsc;
else
{
sortCol=colName;
sortAsc=true;
}
applySortToFileData();
renderFileTable();
document.getElementById("lblFile").innerHTML="";
}
