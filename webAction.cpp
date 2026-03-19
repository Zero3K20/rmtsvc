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
#include <mutex>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")
// WASAPI loopback audio capture (Windows Vista and later)
#include <audioclient.h>
#include <mmdeviceapi.h>
#pragma comment(lib, "ole32.lib")
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

// ---------------------------------------------------------------------------
// RLE (PackBits-style) compressor / decompressor
// ---------------------------------------------------------------------------
// Control byte encoding:
//   0x00..0x7F  literal run:  next (ctrl+1) bytes are copied verbatim (1..128)
//   0x80..0xFF  repeat run:   next byte is repeated (ctrl-0x80+2) times   (2..129)
// Worst-case expansion: 1 extra control byte per 128 literal bytes (~0.8%).
// ---------------------------------------------------------------------------
// Compress 'src' (srcSize bytes) into 'dst' (dstSize bytes) using RLE.
// Returns compressed size, or 0 on failure (output overflow).
static DWORD rleCompress(LPBYTE src, DWORD srcSize, LPBYTE dst, DWORD dstSize)
{
	DWORD out = 0;
	DWORD i   = 0;
	while (i < srcSize)
	{
		BYTE  b      = src[i];
		DWORD runLen = 1;
		while (runLen < 129 && i + runLen < srcSize && src[i + runLen] == b)
			runLen++;

		if (runLen >= 2)
		{
			// Repeat run
			if (out + 2 > dstSize) return 0;
			dst[out++] = (BYTE)(0x80 | ((runLen - 2) & 0x7F));
			dst[out++] = b;
			i += runLen;
		}
		else
		{
			// Literal run: accumulate bytes until we hit a run or the 128-byte limit
			DWORD litStart = i;
			DWORD litLen   = 0;
			while (litLen < 128 && i < srcSize)
			{
				if (i + 1 < srcSize && src[i] == src[i + 1])
					break; // upcoming run – stop collecting literals
				litLen++;
				i++;
			}
			if (out + 1 + litLen > dstSize) return 0;
			dst[out++] = (BYTE)(litLen - 1);
			memcpy(dst + out, src + litStart, litLen);
			out += litLen;
		}
	}
	return out;
}

// ---------------------------------------------------------------------------
// Binary diff-stream frame header
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct DiffFrameHeader
{
	DWORD magic;    // DIFF_FRAME_MAGIC
	BYTE  flags;    // bit 0: 0=full frame, 1=XOR diff;  bit 1: 0=raw, 1=RLE-compressed
	WORD  width;
	WORD  height;
	DWORD rawSize;  // uncompressed payload size (bytes)
	DWORD compSize; // size of the payload that follows this header
};
#pragma pack(pop)

static const DWORD DIFF_FRAME_MAGIC = 0x46464944; // 'D','I','F','F' LE

// ---------------------------------------------------------------------------
// Audio frame header (WAV payload, optionally RLE-compressed)
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct AudioFrameHeader
{
	DWORD magic;    // AUDIO_FRAME_MAGIC
	BYTE  flags;    // bit 0: 0=raw WAV, 1=RLE-compressed WAV
	DWORD rawSize;  // original (uncompressed) WAV size in bytes
	DWORD compSize; // size of the payload that follows this header
};
#pragma pack(pop)

static const DWORD AUDIO_FRAME_MAGIC = 0x46445541; // 'AUDF' LE

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
					char tmp[16]; sprintf(tmp,"%ld",(*it).second.second);
					session["lAccess"]=string(tmp);

					const char *ptr_remember=httpreq.Request("remember");
					if(ptr_remember && strcmp(ptr_remember,"1")==0)
					{//generate a remember-me token and set a long-lived cookie
						static const char tokenchars[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
						char token[33]; token[32]=0;
						BYTE randBytes[32]={0};
						HCRYPTPROV hProv=0;
						if(CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT|CRYPT_SILENT)){
							CryptGenRandom(hProv,32,randBytes);
							CryptReleaseContext(hProv,0);
						}else{
							srand((unsigned)time(NULL)^(unsigned)(size_t)token);
							for(int i=0;i<32;i++) randBytes[i]=(BYTE)rand();
						}
						for(int i=0;i<32;i++) token[i]=tokenchars[randBytes[i]%62];

						time_t tExpires=time(NULL)+30*24*60*60; //30 days
						RememberEntry entry; entry.user=(*it).first;
						entry.lAccess=(*it).second.second; entry.expires=tExpires;
						{
							std::lock_guard<cMutex> lk(m_rememberMutex);
							m_rememberTokens[std::string(token)]=entry;
						}
						saveRememberTokens();

						struct tm *tmExpires=gmtime(&tExpires);
						char strExpires[64];
						strftime(strExpires,sizeof(strExpires),"%a, %d-%b-%Y %H:%M:%S GMT",tmExpires);

						httprsp.SetCookie("rmtsvc_remember",token,"/");
						TNew_Cookie *pCookie=httprsp.SetCookie("rmtsvc_remember");
						if(pCookie){ pCookie->cookie_expires=std::string(strExpires);
							pCookie->cookie_httponly=true; }
					}

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

//get/set capture settings; bSetting - whether setting is allowed
//getcurrentloginuserandpermissions
bool webServer:: httprsp_capSetting(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp
									,httpSession &session,bool bSetting)
{
	const char *ptr; char buf[1024];
//	if(bSetting && (ptr=httpreq.Request("lockmskb")) )
//		SetMouseKeybHook( (atol(ptr)==1) );

	if(bSetting && (ptr=httpreq.Request("imgsize")) )
		m_dwImgSize=(DWORD)atol(ptr);
	
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	int buflen=sprintf(buf,"<xmlroot>\r\n"
				"<userAgent><![CDATA[%s]]></userAgent>\r\n"
				"<imgsize>%d</imgsize>\r\n"
				"<qx>%s</qx>\r\n"
				"<ssid>%s</ssid>\r\n"
				"<lockmskb>%d</lockmskb>\r\n"
				"<hwnd>%ld</hwnd>\r\n"
				"</xmlroot>",httpreq.Header("User-Agent"),
				((m_dwImgSize==0)?0:1),session["lAccess"].c_str(),
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
	struct CursorMapping { LPTSTR id; const char *css; };
	static const CursorMapping cursorMap[] = {
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

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,LPBYTE &lpbits);
DWORD capDesktopRaw(HWND hWnd,WORD w,WORD h,WORD &actual_w,WORD &actual_h,LPBYTE &lpbits);
bool webServer:: httprsp_capDesktop(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	bool ifCapCursor=true;
	WORD w=LOWORD(m_dwImgSize);
	WORD h=HIWORD(m_dwImgSize);
	LPBYTE lpbits=NULL;
	Wutils::selectDesktop();
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	DWORD dwRet=capDesktop(hwnd,w,h,ifCapCursor,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_BMP);
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

// Binary inter-frame diff stream using RLE compression.
//
// Wire protocol – one frame per iteration:
//   DiffFrameHeader (17 bytes, packed)
//   followed by compSize bytes of payload
//
// flags bit 0: 0 = full top-down RGB frame, 1 = XOR diff frame
// flags bit 1: 0 = payload is raw,           1 = payload is RLE-compressed
bool webServer::httprsp_capStream(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	char hdr[256];
	int hlen = sprintf(hdr,
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: application/octet-stream\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Pragma: no-cache\r\n"
		"Connection: close\r\n"
		"\r\n");
	if (psock->Send(hlen, hdr, -1) < 0) return true;

	WORD w    = LOWORD(m_dwImgSize);
	WORD h    = HIWORD(m_dwImgSize);
	HWND hwnd = (HWND)atol(session["cap_hwnd"].c_str());

	LPBYTE lpPrevFrame = NULL; // previous top-down RGB frame
	WORD   prevW = 0, prevH = 0;

	while (psock->checkSocket(0, SOCKS_OP_WRITE) >= 0)
	{
		Wutils::selectDesktop();

		LPBYTE lpCurrFrame = NULL;
		WORD   currW = 0, currH = 0;
		DWORD  rawSize = capDesktopRaw(hwnd, w, h, currW, currH, lpCurrFrame);

		if (rawSize == 0 || !lpCurrFrame)
		{
			if (lpCurrFrame) ::free(lpCurrFrame);
			Sleep(100);
			continue;
		}

		bool bFull = (lpPrevFrame == NULL || prevW != currW || prevH != currH);

		// Build the data to compress: full frame or XOR diff
		LPBYTE lpToCompress = lpCurrFrame;
		LPBYTE lpDiff       = NULL;
		if (!bFull)
		{
			lpDiff = (LPBYTE)::malloc(rawSize);
			if (lpDiff)
			{
				for (DWORD i = 0; i < rawSize; i++)
					lpDiff[i] = lpCurrFrame[i] ^ lpPrevFrame[i];
				lpToCompress = lpDiff;
			}
			else
			{
				bFull = true; // fall back to full frame on alloc failure
			}
		}

		// Attempt RLE compression.
		// Worst-case expansion: 1 extra control byte per 128 literal bytes (~0.8%).
		DWORD  compBufSize = rawSize + (rawSize >> 7) + 64;
		LPBYTE lpCompBuf   = (LPBYTE)::malloc(compBufSize);
		DWORD  compSize    = 0;
		bool   bCompressed = false;
		if (lpCompBuf)
		{
			compSize = rleCompress(lpToCompress, rawSize, lpCompBuf, compBufSize);
			if (compSize > 0 && compSize < rawSize)
				bCompressed = true;
			else
				compSize = 0; // keep raw on failure or expansion
		}

		// Build frame header
		DiffFrameHeader fhdr;
		fhdr.magic    = DIFF_FRAME_MAGIC;
		fhdr.flags    = (bFull ? 0 : 1) | (bCompressed ? 2 : 0);
		fhdr.width    = currW;
		fhdr.height   = currH;
		fhdr.rawSize  = rawSize;
		fhdr.compSize = bCompressed ? compSize : rawSize;

		const char *lpSend = bCompressed
			? (const char *)lpCompBuf
			: (const char *)lpToCompress;

		SOCKSRESULT sr = psock->Send(sizeof(fhdr), (const char *)&fhdr, HTTP_MAX_RESPTIMEOUT);
		if (sr >= 0)
			sr = psock->Send(fhdr.compSize, lpSend, HTTP_MAX_RESPTIMEOUT);

		if (lpCompBuf) ::free(lpCompBuf);
		if (lpDiff)    ::free(lpDiff);

		// Current frame becomes previous frame
		if (lpPrevFrame) ::free(lpPrevFrame);
		lpPrevFrame = lpCurrFrame;
		prevW = currW; prevH = currH;

		if (sr < 0) break;
		Sleep(100); // ~10 fps
	}

	if (lpPrevFrame) ::free(lpPrevFrame);
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
	LPBYTE lpbits=new BYTE[sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+bih.biSizeImage];
	DWORD dwret=chkcodeImage(&bih,lpbits,tmpBuf);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_BMP);
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
	LPBYTE lpbits=new BYTE[sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+bih.biSizeImage];
	DWORD dwret=usageImage(&bih,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_BMP);
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
	
	//get image data and encode as BMP
	DWORD dwret=0;
	DWORD hdrSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits+hdrSize,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
	{
		BITMAPFILEHEADER bmfh;
		::memset(&bmfh, 0, sizeof(bmfh));
		bmfh.bfType = 0x4D42; // 'BM'
		bmfh.bfSize = hdrSize + lpbih->biSizeImage;
		bmfh.bfOffBits = hdrSize;
		::memcpy(lpbits, &bmfh, sizeof(BITMAPFILEHEADER));
		::memcpy(lpbits + sizeof(BITMAPFILEHEADER), lpbih, sizeof(BITMAPINFOHEADER));
		dwret = bmfh.bfSize;
	}

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
	
	//get image data and encode as BMP
	DWORD dwret=0;
	DWORD hdrSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits+hdrSize,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
	{
		BITMAPFILEHEADER bmfh;
		::memset(&bmfh, 0, sizeof(bmfh));
		bmfh.bfType = 0x4D42; // 'BM'
		bmfh.bfSize = hdrSize + lpbih->biSizeImage;
		bmfh.bfOffBits = hdrSize;
		::memcpy(lpbits, &bmfh, sizeof(BITMAPFILEHEADER));
		::memcpy(lpbits + sizeof(BITMAPFILEHEADER), lpbih, sizeof(BITMAPINFOHEADER));
		dwret = bmfh.bfSize;
	}

	::DeleteObject(hMemBmp);
	::SelectObject(hMemDC, hOldBmp);	
	::DeleteDC(hMemDC);
	::ReleaseDC(NULL, hWndDC);
	return dwret;
}

// ---------------------------------------------------------------------------
// capDesktopRaw – capture desktop as top-down 24-bit RGB.
// The returned buffer (lpbits) is allocated with malloc() and must be freed
// by the caller.  actual_w / actual_h receive the output dimensions.
// Returns total byte count of lpbits, or 0 on failure.
// ---------------------------------------------------------------------------
DWORD capDesktopRaw(HWND hWnd, WORD w, WORD h,
                    WORD &actual_w, WORD &actual_h, LPBYTE &lpbits)
{
	lpbits = NULL; actual_w = 0; actual_h = 0;

	if (hWnd == NULL) hWnd = ::GetDesktopWindow();
	RECT rect;
	::GetWindowRect(hWnd, &rect);

	BITMAPINFOHEADER bih;
	::memset(&bih, 0, sizeof(bih));
	bih.biSize        = sizeof(BITMAPINFOHEADER);
	bih.biPlanes      = 1;
	bih.biCompression = BI_RGB;
	bih.biBitCount    = 24;
	bih.biHeight      = rect.bottom - rect.top;
	bih.biWidth       = rect.right  - rect.left;
	bih.biSizeImage   = DIBSCANLINE_WIDTHBYTES(bih.biWidth * bih.biBitCount) * bih.biHeight;

	LPBYTE lpbuffer = (LPBYTE)::malloc(bih.biSizeImage);
	if (!lpbuffer) return 0;

	HDC     hWndDC = ::GetDCEx(hWnd, NULL, DCX_WINDOW);
	HDC     hMemDC = ::CreateCompatibleDC(hWndDC);
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hWndDC, bih.biWidth, bih.biHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	::BitBlt(hMemDC, 0, 0, bih.biWidth, bih.biHeight, hWndDC, 0, 0, SRCCOPY);

	DWORD dwret = 0;
	if (::GetDIBits(hWndDC, hMemBmp, 0, bih.biHeight, lpbuffer,
	                (LPBITMAPINFO)&bih, DIB_RGB_COLORS))
	{
		// Scale down if requested (same logic as capDesktop).
		// In-place rewrite is safe here: the output stride (lEffwidth_dst = w*3)
		// is strictly less than the source stride (lEffwidth_src), so each
		// destination pixel is always at a lower address than its source pixel.
		if (w != 0 && h != 0)
		{
			float f  = (float)bih.biWidth / bih.biHeight;
			float f1 = (float)w / h;
			if (f1 > f) w = (WORD)(h * f); else h = (WORD)(w / f);
			if (w < bih.biWidth && h < bih.biHeight)
			{
				long lEffwidth_src = bih.biSizeImage / bih.biHeight;
				long lEffwidth_dst = w * 3;
				float fX = (float)bih.biWidth / w;
				float fY = (float)bih.biHeight / h;
				LPBYTE ptrD = lpbuffer;
				for (int i = 0; i < (int)h; i++)
				{
					LPBYTE ptr = ptrD;
					for (int j = 0; j < (int)w; j++)
					{
						long x = (long)(fX * j), y = (long)(fY * i);
						LPBYTE ptrS = lpbuffer + y * lEffwidth_src + x * 3;
						*ptr++ = *ptrS; *ptr++ = *(ptrS+1); *ptr++ = *(ptrS+2);
					}
					ptrD += lEffwidth_dst;
				}
				bih.biWidth    = w;
				bih.biHeight   = h;
				bih.biSizeImage = (DWORD)lEffwidth_dst * h;
			}
		}

		// DIB is bottom-up BGR; convert to top-down RGB for the browser
		WORD  outW      = (WORD)bih.biWidth;
		WORD  outH      = (WORD)bih.biHeight;
		DWORD dibStride = DIBSCANLINE_WIDTHBYTES(outW * bih.biBitCount); // row stride with DWORD padding
		DWORD outStride = outW * 3;                          // no padding
		LPBYTE rgbBuf   = (LPBYTE)::malloc((DWORD)outW * outH * 3);
		if (rgbBuf)
		{
			for (WORD row = 0; row < outH; row++)
			{
				// Row 0 of DIB is the bottom row of the image
				LPBYTE src = lpbuffer + (DWORD)(outH - 1 - row) * dibStride;
				LPBYTE dst = rgbBuf  + (DWORD)row * outStride;
				for (WORD col = 0; col < outW; col++)
				{
					dst[col*3+0] = src[col*3+2]; // R <- B
					dst[col*3+1] = src[col*3+1]; // G
					dst[col*3+2] = src[col*3+0]; // B <- R
				}
			}
			::free(lpbuffer);
			lpbits   = rgbBuf;
			actual_w = outW;
			actual_h = outH;
			dwret    = (DWORD)outW * outH * 3;
		}
		else
		{
			::free(lpbuffer);
		}
	}
	else
	{
		::free(lpbuffer);
	}

	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hWndDC);
	return dwret;
}

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,LPBYTE &lpbits)
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
			//GetIconInfo allocates these bitmaps; the caller must delete them
			if(IconInfo.hbmMask)  ::DeleteObject(IconInfo.hbmMask);
			if(IconInfo.hbmColor) ::DeleteObject(IconInfo.hbmColor);
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
		//encode as BMP
		DWORD hdrSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		LPBYTE bmpBuf = (LPBYTE)::malloc(hdrSize + bih.biSizeImage);
		if(bmpBuf)
		{
			BITMAPFILEHEADER bmfh;
			::memset(&bmfh, 0, sizeof(bmfh));
			bmfh.bfType = 0x4D42; // 'BM'
			bmfh.bfSize = hdrSize + bih.biSizeImage;
			bmfh.bfOffBits = hdrSize;
			::memcpy(bmpBuf, &bmfh, sizeof(BITMAPFILEHEADER));
			::memcpy(bmpBuf + sizeof(BITMAPFILEHEADER), &bih, sizeof(BITMAPINFOHEADER));
			::memcpy(bmpBuf + hdrSize, lpbuffer, bih.biSizeImage);
			::free(lpbuffer);
			lpbits = bmpBuf;
			dwret = bmfh.bfSize;
		}
		else
		{
			::free(lpbuffer);
			lpbits = NULL;
		}
	}else{
		//GDI capture failed (e.g. desktop switch, screen lock): free the buffer
		//so the stream loop does not accumulate leaked allocations
		::free(lpbuffer);
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

// ─────────────────────────────────────────────────────────────────────────────
// Persistent WASAPI loopback ring buffer
//
// A dedicated background thread runs WASAPI loopback capture continuously and
// writes 16-bit stereo PCM into a circular buffer.  Each /capAudio HTTP
// request reads the next sequential 2-second window from the ring without
// re-initialising WASAPI between requests.  This eliminates the ~20-50 ms
// silence gap that per-request WASAPI initialisation would otherwise inject
// at every chunk boundary.
//
// Ring capacity: 8 seconds at up to 96 kHz / 16-bit / 2-ch (~3 MB).
// The thread auto-stops after 10 s of inactivity and restarts on demand.
// ─────────────────────────────────────────────────────────────────────────────

// 8 s at 96 kHz / stereo / 16-bit ≈ 3 MB
#define AUDIO_RING_CAP (96000 * 2 * 2 * 8)

// Maximum acceptable llNextChunk lead over llWrTotal, expressed in chunk
// multiples.  If the lead exceeds this threshold, llNextChunk is reset to
// the current ring write position so that the server self-heals instead of
// timing out on every subsequent request.
#define AUDIO_MAX_DRIFT_CHUNKS 2

struct AudioRing
{
	LPBYTE           pBuf;        // circular PCM buffer
	DWORD            dwCap;       // capacity in bytes (AUDIO_RING_CAP)
	LONGLONG         llWrTotal;   // monotonically increasing bytes-written counter
	LONGLONG         llNextChunk; // first byte of the next 2-s chunk to serve
	CRITICAL_SECTION cs;          // guards all fields above
	HANDLE           hEvent;      // auto-reset: signalled after each batch write
	HANDLE           hThread;     // capture thread handle (NULL when stopped)
	volatile LONG    lStop;       // interlocked: non-zero = thread should exit
	ULONGLONG        ullLastReq;  // GetTickCount64 of most recent /capAudio request
	WAVEFORMATEX     wfxOut;      // 16-bit stereo PCM at the device's native rate
};

static AudioRing g_ar;
static bool      g_arInited = false;

static void arEnsureInit()
{
	if (g_arInited) return;
	memset(&g_ar, 0, sizeof(g_ar));
	g_ar.pBuf   = (LPBYTE)malloc(AUDIO_RING_CAP);
	g_ar.dwCap  = g_ar.pBuf ? AUDIO_RING_CAP : 0;
	InitializeCriticalSection(&g_ar.cs);
	g_ar.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	g_arInited  = true;
}

// Background thread: runs WASAPI loopback and fills the ring until lStop is
// set or 10 seconds pass with no /capAudio request.
static DWORD WINAPI audioRingThread(LPVOID)
{
	HRESULT hr;
	IMMDeviceEnumerator *pEnum    = NULL;
	IMMDevice           *pDev     = NULL;
	IAudioClient        *pClient  = NULL;
	IAudioCaptureClient *pCapture = NULL;
	WAVEFORMATEX        *pDevFmt  = NULL;

	HRESULT hrCom  = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	bool    bNeedCo = (hrCom == S_OK);

	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
	                      __uuidof(IMMDeviceEnumerator), (void **)&pEnum);
	if (FAILED(hr)) goto done;

	hr = pEnum->GetDefaultAudioEndpoint(eRender, eConsole, &pDev);
	if (FAILED(hr)) goto done;

	hr = pDev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void **)&pClient);
	if (FAILED(hr)) goto done;

	hr = pClient->GetMixFormat(&pDevFmt);
	if (FAILED(hr)) goto done;

	hr = pClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
	                         AUDCLNT_STREAMFLAGS_LOOPBACK,
	                         5000000, 0, pDevFmt, NULL); // 500 ms engine buffer
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

		// Publish the output format so request handlers can size their chunks.
		EnterCriticalSection(&g_ar.cs);
		memset(&g_ar.wfxOut, 0, sizeof(WAVEFORMATEX));
		g_ar.wfxOut.wFormatTag      = WAVE_FORMAT_PCM;
		g_ar.wfxOut.nChannels       = nDstCh;
		g_ar.wfxOut.nSamplesPerSec  = pDevFmt->nSamplesPerSec;
		g_ar.wfxOut.wBitsPerSample  = 16;
		g_ar.wfxOut.nBlockAlign     = nDstCh * 2;
		g_ar.wfxOut.nAvgBytesPerSec = g_ar.wfxOut.nSamplesPerSec * g_ar.wfxOut.nBlockAlign;
		LeaveCriticalSection(&g_ar.cs);

		while (!InterlockedCompareExchange(&g_ar.lStop, 0, 0))
		{
			// Auto-stop after 10 s of no /capAudio requests.
			if (GetTickCount64() - g_ar.ullLastReq > 10000) break;

			Sleep(10);

			UINT32 nPktLen = 0;
			while (SUCCEEDED(pCapture->GetNextPacketSize(&nPktLen)) && nPktLen > 0)
			{
				BYTE  *pData   = NULL;
				UINT32 nFrames = 0;
				DWORD  dwFlags = 0;
				if (FAILED(pCapture->GetBuffer(&pData, &nFrames, &dwFlags, NULL, NULL)))
					break;

				bool bSilent = ((dwFlags & AUDCLNT_BUFFERFLAGS_SILENT) != 0) || (pData == NULL);

				EnterCriticalSection(&g_ar.cs);
				for (UINT32 f = 0; f < nFrames; f++)
				{
					for (UINT32 c = 0; c < nDstCh; c++)
					{
						short s = 0;
						if (!bSilent)
						{
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
						DWORD pos = (DWORD)(g_ar.llWrTotal % g_ar.dwCap);
						memcpy(g_ar.pBuf + pos, &s, sizeof(short));
						g_ar.llWrTotal += sizeof(short);
					}
				}
				LeaveCriticalSection(&g_ar.cs);
				SetEvent(g_ar.hEvent); // wake any waiting request handler
				pCapture->ReleaseBuffer(nFrames);
			}
		}

		pClient->Stop();
	}

done:
	if (pCapture) pCapture->Release();
	if (pClient)  pClient->Release();
	if (pDevFmt)  CoTaskMemFree(pDevFmt);
	if (pDev)     pDev->Release();
	if (pEnum)    pEnum->Release();
	if (bNeedCo)  CoUninitialize();

	// Clear the thread handle (under the lock) so the next request can restart.
	EnterCriticalSection(&g_ar.cs);
	HANDLE hSelf    = g_ar.hThread;
	g_ar.hThread    = NULL;
	g_ar.lStop      = 0;
	LeaveCriticalSection(&g_ar.cs);
	if (hSelf) CloseHandle(hSelf); // safe: decrements refcount, thread still runs to return
	return 0;
}

// Read the next sequential 2-second chunk from the persistent ring buffer.
// Ensures the background capture thread is running; blocks until 2 s of new
// audio is available.  On success writes PCM into lpPCMOut, sets *pwfxOut,
// and returns the byte count.  Returns 0 on failure (caller falls back to WinMM).
static DWORD capAudioWASAPI(LPBYTE lpPCMOut, DWORD dwMaxBytes, WAVEFORMATEX *pwfxOut)
{
	arEnsureInit();
	if (!g_ar.pBuf) return 0; // allocation failed at init

	g_ar.ullLastReq = GetTickCount64();

	// Start the background thread if it is not currently running.
	// Reset llNextChunk to the current write position so we skip any window
	// that was claimed but never fully filled (e.g., after an inactivity stop).
	EnterCriticalSection(&g_ar.cs);
	if (g_ar.hThread == NULL)
	{
		g_ar.llNextChunk = g_ar.llWrTotal; // start fresh
		g_ar.lStop       = 0;
		g_ar.hThread     = CreateThread(NULL, 0, audioRingThread, NULL, 0, NULL);
	}
	LeaveCriticalSection(&g_ar.cs);

	// Wait for the ring thread to publish its output format (~30-50 ms after start).
	WAVEFORMATEX wfx;
	{
		ULONGLONG tLimit = GetTickCount64() + 2000;
		for (;;)
		{
			EnterCriticalSection(&g_ar.cs);
			wfx = g_ar.wfxOut;
			LeaveCriticalSection(&g_ar.cs);
			if (wfx.nAvgBytesPerSec > 0) break;
			if (GetTickCount64() >= tLimit) return 0;
			WaitForSingleObject(g_ar.hEvent, 50);
		}
	}

	// How many PCM bytes make exactly 2 seconds at this device's rate?
	DWORD dwChunkBytes = wfx.nAvgBytesPerSec * 2;
	if (dwChunkBytes > dwMaxBytes) dwChunkBytes = dwMaxBytes;

	// Atomically claim the next sequential window so that concurrent requests
	// receive non-overlapping, perfectly back-to-back audio chunks.
	// If llNextChunk has drifted too far ahead of the ring's write position
	// (e.g., due to a previous timeout that left it advanced beyond where the
	// ring can fill in time), reset it to the current write position so that
	// the server self-heals rather than timing out on every subsequent request.
	LONGLONG llStart;
	EnterCriticalSection(&g_ar.cs);
	// Both llNextChunk and llWrTotal are read under the critical section so
	// this comparison is thread-safe.  llWrTotal is monotonically increasing;
	// resetting llNextChunk to it discards any audio that would have been
	// served in the skipped range, but that is preferable to every subsequent
	// request timing out and exhausting the browser's connection pool.
	if (g_ar.llNextChunk > g_ar.llWrTotal + (LONGLONG)dwChunkBytes * AUDIO_MAX_DRIFT_CHUNKS)
		g_ar.llNextChunk = g_ar.llWrTotal; // clamp excessive drift
	llStart           = g_ar.llNextChunk;
	g_ar.llNextChunk += (LONGLONG)dwChunkBytes;
	LeaveCriticalSection(&g_ar.cs);

	// Block until the ring has filled the claimed window (up to 3 s grace).
	LONGLONG  llEnd  = llStart + (LONGLONG)dwChunkBytes;
	ULONGLONG tLimit = GetTickCount64() + 3000;
	for (;;)
	{
		EnterCriticalSection(&g_ar.cs);
		LONGLONG llCur = g_ar.llWrTotal;
		LeaveCriticalSection(&g_ar.cs);
		if (llCur >= llEnd) break;
		if (GetTickCount64() >= tLimit)
		{
			// Timeout: undo the chunk claim so that llNextChunk is not left
			// permanently advanced past a position the ring cannot fill.
			// Only rewind if no concurrent request has already advanced
			// llNextChunk further (i.e., we are still the most-recent claim).
			EnterCriticalSection(&g_ar.cs);
			if (g_ar.llNextChunk == llEnd)
				g_ar.llNextChunk = llStart;
			LeaveCriticalSection(&g_ar.cs);
			return 0;
		}
		WaitForSingleObject(g_ar.hEvent, 20);
	}

	// Copy the window from the ring buffer.  No lock needed here: the bytes in
	// [llStart, llEnd) were fully committed before we exited the loop above, and
	// the ring is large enough (≥8 s) that the writer cannot wrap to this region
	// before a simple memcpy-equivalent loop completes.
	for (DWORD i = 0; i < dwChunkBytes; i++)
	{
		DWORD pos  = (DWORD)((llStart + (LONGLONG)i) % (LONGLONG)g_ar.dwCap);
		lpPCMOut[i] = g_ar.pBuf[pos];
	}

	*pwfxOut = wfx;
	return dwChunkBytes;
}

// Capture ~2 s of system audio and return it as a WAV (RIFF/PCM) response.
// Uses a persistent background capture thread (ring buffer) so WASAPI never
// re-initialises between requests, eliminating the inter-chunk silence gap.
// Falls back to WinMM loopback if WASAPI is unavailable.
bool webServer::httprsp_capAudio(socketTCP *psock, httpResponse &httprsp)
{
	// Maximum PCM buffer: 2 seconds at up to 96000 Hz / 16-bit / 2ch = 768 KB
	const DWORD dwMaxPCM  = 96000 * 2 * 2 * 2;
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

		// Wrap the WAV in an AudioFrameHeader with optional RLE compression
		// to reduce bandwidth.  RLE worst-case expansion: ~1/128 of input.
		const DWORD dwCompBound = dwSendSize + (dwSendSize >> 7) + 64;
		LPBYTE lpOut = (LPBYTE)malloc(sizeof(AudioFrameHeader) + dwCompBound);
		if (lpOut)
		{
			AudioFrameHeader afh;
			afh.magic   = AUDIO_FRAME_MAGIC;
			afh.rawSize = dwSendSize;

			DWORD compSize = rleCompress(lpBuffer, dwSendSize,
			                             lpOut + sizeof(AudioFrameHeader), dwCompBound);
			// Only use compression when it actually reduces the payload size.
			if (compSize > 0 && compSize < dwSendSize)
			{
				afh.flags    = 1; // RLE-compressed
				afh.compSize = compSize;
			}
			else
			{
				// Compression not beneficial; embed raw WAV after header
				afh.flags    = 0;
				afh.compSize = dwSendSize;
				memcpy(lpOut + sizeof(AudioFrameHeader), lpBuffer, dwSendSize);
			}

			memcpy(lpOut, &afh, sizeof(AudioFrameHeader));
			free(lpBuffer);
			lpBuffer   = lpOut;
			dwSendSize = (DWORD)sizeof(AudioFrameHeader) + afh.compSize;
		}
		// If malloc fails, lpBuffer still holds the plain WAV; the client
		// detects the missing AUDF magic and decodes it as plain WAV.
	}

	httprsp.NoCache();
	if (dwSendSize > 0)
	{
		httprsp.Header()["Content-Type"] = "application/octet-stream";
		httprsp.lContentLength(dwSendSize);
		httprsp.send_rspH(psock, 200, "OK");
		psock->Send(dwSendSize, (const char*)lpBuffer, HTTP_MAX_RESPTIMEOUT);
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