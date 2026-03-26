/*******************************************************************
   *	webAction_vidc.cpp web request handling - vidc management
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/
#include "../../../rmtsvc.h"

static const char errMsg_failed[]="operation failed";
static const char errMsg_ok[]="operation succeeded";

//list all local map ports
//<?xml version="1.0" encoding="gb2312" ?>
//...
//</xmlroot>
//get local mapping service info
bool webServer::httprsp_mportl(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	const char *ptr_cmd=httpreq.Request("type");
	bool bTcpType=(ptr_cmd && strcmp(ptr_cmd,"udp")==0)?false:true;
	ptr_cmd=httpreq.Request("cmd");
	
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"list")==0){ //return local mapping list XML
		pvidc->xml_list_localip(buffer); //return local machine IP address
		(bTcpType)?pvidc->xml_list_mtcp(buffer):
				   pvidc->xml_list_mudp(buffer);
	}
	else if(strcasecmp(ptr_cmd,"info")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_info_mtcp(buffer,ptr_mapname):
				   pvidc->xml_info_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"start")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		if(bTcpType)
			pvidc->xml_start_mtcp(buffer,ptr_mapname);
		else 
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>this version temporarily does not support UDP mapping</retmsg>");
		//	pvidc->xml_start_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"stop")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_stop_mtcp(buffer,ptr_mapname):
				   pvidc->xml_stop_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"dele")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_dele_mtcp(buffer,ptr_mapname):
				   pvidc->xml_dele_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"save")==0)
	{
		const char *ptr_param=httpreq.Request("param");
		pvidc->parseIni((char *)ptr_param,0);
		(bTcpType)?pvidc->xml_list_mtcp(buffer):
				   pvidc->xml_list_mudp(buffer);
		if(buffer.Space()<48) buffer.Resize(buffer.size()+48);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
	}

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

//remotemapserviceinfo
bool webServer::httprsp_mportr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidccSets &vidccsets=pvidc->m_vidccSets;
	
	const char *ptr_cmd,*ptr_vname;
	VIDC_MAPTYPE maptype=VIDC_MAPTYPE_TCP;
	ptr_cmd=httpreq.Request("type");
	if(ptr_cmd && strcmp(ptr_cmd,"udp")==0) maptype=VIDC_MAPTYPE_UDP;
	else if(ptr_cmd && strcmp(ptr_cmd,"proxy")==0) maptype=VIDC_MAPTYPE_PROXY;
	ptr_cmd=httpreq.Request("cmd");
	ptr_vname=httpreq.Request("vname");
	vidcClient *pvidcc=NULL;

	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"vidcclist")==0){ //return the local configuration vidcc list
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccmodi")==0)
	{//add or modify a vIDCc client and its vIDCs association
		if(ptr_vname && ptr_vname[0]!=0){
			if( (pvidcc=vidccsets.GetVidcClient(ptr_vname,true)) )
			{
				VIDCSINFO &vinfo=pvidcc->vidcsinfo();
				const char *ptr_req=httpreq.Request("vhost");
				if(ptr_req) vinfo.m_vidcsHost.assign(ptr_req);
				ptr_req=httpreq.Request("vport");
				vinfo.m_vidcsPort=(ptr_req)?atoi(ptr_req):0;
				ptr_req=httpreq.Request("vpswd");
				if(ptr_req) vinfo.m_vidcsPswd.assign(ptr_req);
				ptr_req=httpreq.Request("vconn");
				vinfo.m_bAutoConn=(ptr_req && atoi(ptr_req)==1)?true:false;
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
			}//?if(pvidcc)
		}//?if(ptr_req && ptr_req[0]!=0)
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccdele")==0) 
	{//delete a vIDCc client and its vIDCs association
		if(ptr_vname && ptr_vname[0]!=0){
			if( vidccsets.DelVidcClient(ptr_vname) )
				 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
			else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
		}//?if(ptr_req && ptr_req[0]!=0)
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccinfo")==0)
	{
		if(ptr_vname==NULL || !vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype))
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"connect")==0)
	{//connect to the specified vIDCs service
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			SOCKSRESULT sr=pvidcc->ConnectSvr();
			if(sr<=0){
				if(sr==SOCKSERR_VIDC_VER)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,version mismatch</retmsg>");
				else if(sr==SOCKSERR_VIDC_PSWD)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,access password error</retmsg>");
				else if(sr==SOCKSERR_VIDC_RESP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,responsetimeout</retmsg>");
				else if(sr==SOCKSERR_THREAD)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>create threadfailure</retmsg>");
				else if(sr==SOCKSERR_CONN)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,vIDCs address is incorrect</retmsg>");
				else if(sr==SOCKSERR_TIMEOUT)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,connecttimeout</retmsg>");
				else
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>connectvIDCsfailure,err=%d</retmsg>",sr);
			}
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"disconn")==0)
	{//disconnect from the specified vIDCs service
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			pvidcc->DisConnSvr();
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"mapdel")==0)
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr_req=httpreq.Request("mapname");
			if(ptr_req) pvidcc->mapinfoDel(ptr_req);
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"mapmodi")==0)
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr,*ptr_req=httpreq.Request("mapname");
			if(ptr_req && ptr_req[0]!=0){
				mapInfo *pinfo=pvidcc->mapinfoGet(ptr_req,true);
				if(pinfo){
					pinfo->m_mapType=maptype;
					ptr_req=httpreq.Request("appsvr");
					if(ptr_req) pinfo->m_appsvr.assign(ptr_req);
					ptr_req=httpreq.Request("mport");
					if(ptr_req){
						int l=strlen(ptr_req);
						if(l>=4 && strcmp(ptr_req+l-4,"+ssl")==0)
							pinfo->m_ssltype=TCPSVR_SSLSVR;
						else if(l>=4 && strcmp(ptr_req+l-4,"-ssl")==0)
							pinfo->m_ssltype=SSLSVR_TCPSVR;
						else pinfo->m_ssltype=TCPSVR_TCPSVR;
						pinfo->m_mportBegin=atoi(ptr_req);
						if( (ptr=strchr(ptr_req,'-')) && *(ptr+1)!='s') //i.e., not -ssl
							pinfo->m_mportEnd=atoi(ptr+1);
						else pinfo->m_mportEnd=pinfo->m_mportBegin;
					}
					ptr_req=httpreq.Request("sslverify");
					if(pinfo->m_ssltype==TCPSVR_SSLSVR && ptr_req && strcmp(ptr_req,"1")==0)
						pinfo->m_sslverify=true;
					else pinfo->m_sslverify=false;
					
					ptr_req=httpreq.Request("maxconn");
					if(ptr_req && atol(ptr_req)>0) 
						pinfo->m_maxconn=atol(ptr_req);
					else pinfo->m_maxconn=0;
					ptr_req=httpreq.Request("maxratio");
					if(ptr_req && atol(ptr_req)>0) 
						pinfo->m_maxratio=atol(ptr_req);
					else pinfo->m_maxratio=0;

					//clientauthenticationcertificate
					pinfo->m_clicert="";
					pinfo->m_clikey="";
					pinfo->m_clikeypswd="";
					ptr_req=httpreq.Request("clicert");
					if(ptr_req){
						const char *ptr_cert=ptr_req;
						const char *ptr,*ptr_key=NULL,*ptr_pswd=NULL;
						if( (ptr=strchr(ptr_cert,',')) )
						{
							*(char *)ptr=0; ptr_key=ptr+1;
							if( (ptr=strchr(ptr_key,',')) )
							{
								*(char *)ptr=0; ptr_pswd=ptr+1;
							}
						}//?if( (ptr=strchr(ptr_cert,',')) )
						if(ptr_cert) pinfo->m_clicert.assign(ptr_cert);
						if(ptr_key) pinfo->m_clikey.assign(ptr_key);
						if(ptr_pswd) pinfo->m_clikeypswd.assign(ptr_pswd);
					}//?if(ptr_req)
					

					ptr_req=httpreq.Request("apptype");
					if(ptr_req && strcmp(ptr_req,"FTP")==0)
						pinfo->m_apptype=MPORTTYPE_FTP;
					else if(ptr_req && strcmp(ptr_req,"WWW")==0)
						pinfo->m_apptype=MPORTTYPE_WWW;
					else if(ptr_req && strcmp(ptr_req,"TCP")==0)
						pinfo->m_apptype=MPORTTYPE_TCP;
					else pinfo->m_apptype=MPORTTYPE_UNKNOW;
					ptr_req=httpreq.Request("appdesc");
					if(ptr_req) pinfo->m_appdesc.assign(ptr_req);
					ptr_req=httpreq.Request("autorun");
					if(ptr_req && atoi(ptr_req)==1)
						pinfo->m_bAutoMap=true;
					else pinfo->m_bAutoMap=false;
					
					ptr_req=httpreq.Request("svrtype");
					if(ptr_req) pinfo->m_proxyType=atoi(ptr_req);
					ptr_req=httpreq.Request("authuser");
					if(ptr_req) pinfo->m_proxyuser.assign(ptr_req);
					ptr_req=httpreq.Request("authpswd");
					if(ptr_req) pinfo->m_proxypswd.assign(ptr_req);
					ptr_req=httpreq.Request("bauth");
					if(ptr_req && atoi(ptr_req)==1)
						pinfo->m_proxyauth=true;
					else pinfo->m_proxyauth=false;
					if(pinfo->m_mapType==VIDC_MAPTYPE_PROXY){
						pinfo->m_appsvr="";
						pinfo->m_apptype=MPORTTYPE_TCP;
						pinfo->m_ssltype=TCPSVR_TCPSVR;
					}

					ptr_req=httpreq.Request("ipaddr");
					if(ptr_req) pinfo->m_ipRules.assign(ptr_req); else pinfo->m_ipRules="";
					ptr_req=httpreq.Request("access");
					if(ptr_req) pinfo->m_ipaccess=atoi(ptr_req);
				}//?pinfo
			}
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if( strcasecmp(ptr_cmd,"maped")==0 || //map the specified service
			  strcasecmp(ptr_cmd,"unmap")==0 || //cancel the specified map
			  strcasecmp(ptr_cmd,"mapinfo")==0 )
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr,*ptr_req=httpreq.Request("mapname");
			mapInfo *pinfo=NULL;
			if(ptr_req) pinfo=pvidcc->mapinfoGet(ptr_req,false);
			if(pinfo){
				int sr=0;
				if(strcasecmp(ptr_cmd,"unmap")==0)
					sr=pvidcc->Unmap(ptr_req,pinfo);
				else if(strcasecmp(ptr_cmd,"maped")==0)
					sr=pvidcc->Mapped(ptr_req,pinfo);
				if(sr<0){ //an error has occurred
					if(sr==SOCKSERR_VIDC_NAME)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>the specified map service is invalid</retmsg>");
					else if(sr==SOCKSERR_VIDC_MEMO)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>memory allocation failure</retmsg>");
					else if(sr==SOCKSERR_VIDC_SUPPORT)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>this version does not support this feature</retmsg>");
					else if(sr==SOCKSERR_VIDC_MAP)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>mapping failed, make sure the map port is not occupied</retmsg>");
					else
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>server-sideresponsetimeout</retmsg>");
				}
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapinfo>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapname>%s</mapname>",ptr_req);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport>",pinfo->m_mappedPort);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ifssl>%d</ifssl><ifsslv>%d</ifsslv>",
					((pinfo->m_ssltype==TCPSVR_SSLSVR)?1:0),((pinfo->m_mappedSSLv)?1:0) );
				
				if( (ptr=strchr(pinfo->m_appsvr.c_str(),',')) ){
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip><appport></appport>",pinfo->m_appsvr.c_str());
				}else if( (ptr=strchr(pinfo->m_appsvr.c_str(),':')) )
				{
					*(char *)ptr=0;
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip>",pinfo->m_appsvr.c_str());
					*(char *)ptr=':';
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appport>%s</appport>",ptr+1);
				}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip><appport></appport>",pinfo->m_appsvr.c_str());

				if(pinfo->m_apptype==MPORTTYPE_FTP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>FTP</apptype>");
				else if(pinfo->m_apptype==MPORTTYPE_WWW)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>WWW</apptype>");
				else if(pinfo->m_apptype==MPORTTYPE_TCP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>TCP</apptype>");
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>UNK</apptype>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appdesc>%s</appdesc>",pinfo->m_appdesc.c_str());
				
				//limit maximum connections
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxconn>%d</maxconn>",pinfo->m_maxconn);
				//limit maximum bandwidth kb/s
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxratio>%d</maxratio>",pinfo->m_maxratio);

				if(pinfo->m_mportBegin==pinfo->m_mportEnd)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d</mapport>",pinfo->m_mportBegin);
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d-%d</mapport>",pinfo->m_mportBegin,pinfo->m_mportEnd);
				
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip>",pinfo->m_bindLocalIP);
				if(pinfo->m_ssltype==TCPSVR_SSLSVR)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>+ssl</ssltype>");
				else if(pinfo->m_ssltype==SSLSVR_TCPSVR)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>-ssl</ssltype>");
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype></ssltype>");
				
				if(pinfo->m_sslverify)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<sslverify>1</sslverify>");
				
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrtype>%d</svrtype>",pinfo->m_proxyType);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bauth>%d</bauth>",((pinfo->m_proxyauth)?1:0) );
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<authuser>%s</authuser>",pinfo->m_proxyuser.c_str());
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<authpswd>%s</authpswd>",pinfo->m_proxypswd.c_str());

				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",((pinfo->m_bAutoMap)?1:0) );
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",pinfo->m_ipaccess);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",pinfo->m_ipRules.c_str());
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");

				if(pinfo->m_clicert!="")
				{
					int l=pinfo->m_clicert.length()+pinfo->m_clikey.length()+pinfo->m_clikeypswd.length()+32;
					if((int)buffer.Space()>l)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert>%s,%s,%s</clicert>",
						pinfo->m_clicert.c_str(),pinfo->m_clikey.c_str(),pinfo->m_clikeypswd.c_str());
				}
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"</mapinfo>");
			}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

//vIDCsservice management
bool webServer::httprsp_vidcsvr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidcServerEx &vidcsvr=pvidc->m_vidcsvr;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"run")==0) //startvIDCsservice
	{
		SOCKSRESULT sr=vidcsvr.Start();
		if(sr<=0)
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>startvIDCsservicefailure,err=%d</retmsg>",sr);
	}else if(strcasecmp(ptr_cmd,"stop")==0)
	{
		vidcsvr.Close();
	}else if(strcasecmp(ptr_cmd,"setting")==0)
	{//saveserviceparameter
		const char *ptr=httpreq.Request("svrport");
		if(ptr){ 
			if( (vidcsvr.m_svrport=atoi(ptr))<=0 ) vidcsvr.m_svrport=0;
		}
		ptr=httpreq.Request("bindip");
		if(ptr) vidcsvr.m_bindip.assign(ptr);
		ptr=httpreq.Request("autorun");
		if(ptr) vidcsvr.m_autorun=(atoi(ptr)==1)?true:false;
		ptr=httpreq.Request("bauth");
		if(ptr && atoi(ptr)==1) 
			vidcsvr.bAuthentication(true);
		else vidcsvr.bAuthentication(false);
		ptr=httpreq.Request("pswd");
		if(ptr) vidcsvr.accessPswd().assign(ptr);
		ptr=httpreq.Request("ipaccess");
		if(ptr) vidcsvr.m_ipaccess=atoi(ptr);
		ptr=httpreq.Request("ipaddr");
		if(ptr) vidcsvr.m_ipRules.assign(ptr);
	}
	
	//get vIDCs service parameter settings and status----start---------------------------------------------
	struct tm * ltime=NULL; time_t t;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcs_status>");
	if(vidcsvr.status()==SOCKS_LISTEN) //service is running
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>%d</status>",vidcsvr.getLocalPort());
		t=vidcsvr.getStartTime(); ltime=localtime(&t);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d-%02d-%02d %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	}
	else
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>0</status>");
	t=time(NULL); ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<curtime>%04d-%02d-%02d %02d:%02d:%02d</curtime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport>",vidcsvr.m_svrport);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",(vidcsvr.m_autorun)?1:0);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bauth>%d</bauth>",(vidcsvr.bAuthentication())?1:0);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<pswd>%s</pswd>",vidcsvr.accessPswd().c_str());
	//bind local machine IP
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip><localip>",vidcsvr.m_bindip.c_str());
	std::vector<std::string> vec;//get local machine IPs, returns the count of local machine IPs
	long n=socketBase::getLocalHostIP(vec);
	for(int i=0;i<n;i++) buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s ",vec[i].c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</localip>");
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",vidcsvr.m_ipaccess);
	if(buffer.Space()<(vidcsvr.m_ipRules.length()+64)) buffer.Resize(buffer.size()+(vidcsvr.m_ipRules.length()+64));
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",vidcsvr.m_ipRules.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcs_status>");
	//list currently connected vIDCc clients
	vidcsvr.xml_list_vidcc(buffer);
	//get proxy service parameter settings and status---- end ---------------------------------------------

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}
//management of vIDCc connections to vIDCs service
bool webServer::httprsp_vidccs(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidcServerEx &vidcsvr=pvidc->m_vidcsvr;
	const char *ptr_cmd=httpreq.Request("vid");
	long vidccID=(ptr_cmd)?atol(ptr_cmd):0;
	ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	if(strcasecmp(ptr_cmd,"info")==0) //get info for the specified connected vIDCc
	{
		vidcsvr.xml_info_vidcc(buffer,vidccID);
	}
	else if(strcasecmp(ptr_cmd,"dele")==0) //forcibly disconnect a vIDCc from this vIDCs service
	{
		vidcsvr.DisConnect(vidccID);
		vidcsvr.xml_list_vidcc(buffer);
	}
	else if(strcasecmp(ptr_cmd,"sets")==0) //enable logging for the service mapped by a vidcc
	{
		const char *ptr=httpreq.Request("blogd");
		bool b=(ptr && atoi(ptr)==1)?true:false;
		vidcsvr.setLogdatafile(vidccID,b);
		vidcsvr.xml_info_vidcc(buffer,vidccID);
	}

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

//import and export of vIDC configuration
bool webServer::httprsp_vidcini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	if(strcasecmp(ptr_cmd,"out")==0) //export vIDC configuration
	{
		std::string strini;
		pvidc->saveAsstring(strini);
		if(buffer.Space()<(strini.length()+64)) buffer.Resize(buffer.size()+(strini.length()+64));
		if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<settings><![CDATA[%s]]></settings>",strini.c_str());
	}
	else if(strcasecmp(ptr_cmd,"in")==0) //import vIDC configuration
	{
		const char *ptr=httpreq.Request("ini");
		if(ptr){
			pvidc->initSetting();
			pvidc->parseIni((char *)ptr,0);
			pvidc->saveIni(); //save vidc configurationparameter
		}//?if(ptr)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>Configuration imported successfully!</retmsg>");
	}//?else if(strcasecmp(ptr_cmd,"in")==0)

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//set MIME type, default is HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//set response content length
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

