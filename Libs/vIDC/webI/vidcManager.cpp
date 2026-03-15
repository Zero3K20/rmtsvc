/*******************************************************************
   *	vidcManager.cpp
   *    DESCRIPTION:vIDC collection management class
   *
   *    AUTHOR:yyc
   *
   *******************************************************************/

#include "vidcManager.h"

void getAbsolutfilepath(std::string &spath);

vidcManager :: vidcManager()
{
}

vidcManager :: ~vidcManager(){}

void vidcManager :: initSetting()
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.begin();
	for(;it!=m_tcpsets.end();it++){
		mportTCP *ptr_mtcp=(*it).second;
		TMapParam *p=(ptr_mtcp)?((TMapParam *)(ptr_mtcp->Tag())):NULL;
		ptr_mtcp->Stop();
		delete p; delete ptr_mtcp;
	}
	m_tcpsets.clear();
	m_upnp.Clear();
	m_vidccSets.Destroy(); //destroy all vidcc instances
}

void vidcManager :: Destroy()
{
	initSetting();
	m_vidcsvr.Stop();
}
//start local port mapping services configured for automatic start
//在程序start运行时调用
void vidcManager :: mtcpl_Start()
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.begin();
	for(;it!=m_tcpsets.end();it++){
		mportTCP * ptr_mtcp=(*it).second;
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(ptr_mtcp && p && p->bAutorun)
		{
			ptr_mtcp->rules().addRules_new(RULETYPE_TCP,p->ipaccess,p->ipRules.c_str());
			if(ptr_mtcp->getSSLType()==SSLSVR_TCPSVR){ //SSL解密服务,设置client证书
				std::string clicert=p->clicert,clikey=p->clikey;
				getAbsolutfilepath(clicert); getAbsolutfilepath(clikey);
#ifdef _SURPPORT_OPENSSL_
				ptr_mtcp->setCacert(clicert.c_str(),clikey.c_str(),p->clikeypswd.c_str(),false,NULL,NULL);
#endif
			}
			SOCKSRESULT sr=ptr_mtcp->Start(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),
				g_strCaCert.c_str(),g_strCaCRL.c_str());
			if(sr<=0) RW_LOG_PRINT(LOGLEVEL_WARN,"[mtcpl] 启动本定映射服务 %s failure, err=%d\r\n",(*it).first.c_str(),sr);
		}
	}//?for
	return;
}

void vidcManager :: xml_list_localip(cBuffer &buffer)
{
	std::vector<std::string> vec;//得到local machineIP，返回得到local machineIP的个数
	long n=socketBase::getLocalHostIP(vec);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<localip>");
	for(int i=0;i<n;i++) buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s ",vec[i].c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</localip>");
}

//<maplist>
//<mapped name="映射name">应用description</mapped>
//</maplist>
void vidcManager :: xml_list_mtcp(cBuffer &buffer)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maplist>");
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.begin();
	for(;it!=m_tcpsets.end();it++)
	{
		mportTCP *p=(*it).second;
		if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapped name=\"%s\"><![CDATA[%s]]></mapped>",
				(*it).first.c_str(),p->svrname());
		else break;
	}
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</maplist>");
	return;
}
//<mapinfo>
//<mapname></mapname>
//...
//</mapinfo>
void vidcManager :: xml_info_mtcp(cBuffer &buffer,const char *mapname)
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.find(mapname);
	if(it!=m_tcpsets.end()){
		mportTCP * ptr_mtcp=(*it).second;
		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapinfo>");
		if(buffer.Space()<64) buffer.Resize(buffer.size()+64);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapname>%s</mapname>",mapname);
		
		ptr_mtcp->xml_info_mtcp(buffer);
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p){
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",(p->bAutorun)?1:0);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",p->ipaccess);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",p->ipRules.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
			if(p->clicert!="")
			{
				int l=p->clicert.length()+p->clikey.length()+p->clikeypswd.length()+32;
				if((int)buffer.Space()<l) buffer.Resize(buffer.size()+l);
				if(buffer.str())
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert>%s,%s,%s</clicert>",
						p->clicert.c_str(),p->clikey.c_str(),p->clikeypswd.c_str());
			}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert></clicert>");
		}//?if(p)

		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</mapinfo>");
	}else{//error
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>specified的映射 %s 不存在</retmsg>",mapname);
	}
	return;
}


//启动specified的映射服务
void vidcManager :: xml_start_mtcp(cBuffer &buffer,const char *mapname)
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.find(mapname);
	if(it!=m_tcpsets.end()){
		mportTCP * ptr_mtcp=(*it).second;
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p){//设置IP filter rules
			ptr_mtcp->rules().addRules_new(RULETYPE_TCP,p->ipaccess,p->ipRules.c_str());
			if(ptr_mtcp->getSSLType()==SSLSVR_TCPSVR){ //SSL解密服务,设置client证书
				std::string clicert=p->clicert,clikey=p->clikey;
				getAbsolutfilepath(clicert); getAbsolutfilepath(clikey);
#ifdef _SURPPORT_OPENSSL_
				ptr_mtcp->setCacert(clicert.c_str(),clikey.c_str(),p->clikeypswd.c_str(),false,NULL,NULL);
#endif
			}
		}
		SOCKSRESULT sr=ptr_mtcp->Start(g_strMyCert.c_str(),g_strMyKey.c_str(),g_strKeyPswd.c_str(),
				g_strCaCert.c_str(),g_strCaCRL.c_str());
		if(sr<=0){ //start servicefailure
			if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
			if(buffer.str())
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>启动映射 %s failure, err=%d</retmsg>",mapname,sr);
		}
		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapinfo>");
		if(buffer.Space()<64) buffer.Resize(buffer.size()+64);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapname>%s</mapname>",mapname);

		ptr_mtcp->xml_info_mtcp(buffer);	
		if(p){
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",(p->bAutorun)?1:0);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",p->ipaccess);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",p->ipRules.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
			if(p->clicert!="")
			{
				int l=p->clicert.length()+p->clikey.length()+p->clikeypswd.length()+32;
				if((int)buffer.Space()<l) buffer.Resize(buffer.size()+l);
				if(buffer.str())
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert>%s,%s,%s</clicert>",
						p->clicert.c_str(),p->clikey.c_str(),p->clikeypswd.c_str());
			}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert></clicert>");
		}

		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</mapinfo>");
	}else{//error
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>specified的映射 %s 不存在</retmsg>",mapname);
	}
	return;
}

//停止specified的映射服务
void vidcManager :: xml_stop_mtcp(cBuffer &buffer,const char *mapname)
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.find(mapname);
	if(it!=m_tcpsets.end()){
		mportTCP * ptr_mtcp=(*it).second;
		ptr_mtcp->Stop();
		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapinfo>");
		if(buffer.Space()<64) buffer.Resize(buffer.size()+64);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapname>%s</mapname>",mapname);

		ptr_mtcp->xml_info_mtcp(buffer);
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p){
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",(p->bAutorun)?1:0);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",p->ipaccess);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",p->ipRules.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
			if(p->clicert!="")
			{
				int l=p->clicert.length()+p->clikey.length()+p->clikeypswd.length()+32;
				if((int)buffer.Space()<l) buffer.Resize(buffer.size()+l);
				if(buffer.str())
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert>%s,%s,%s</clicert>",
						p->clicert.c_str(),p->clikey.c_str(),p->clikeypswd.c_str());
			}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert></clicert>");
		}

		if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
		if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</mapinfo>");
	}else{//error
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>specified的映射 %s 不存在</retmsg>",mapname);
	}
	return;
}

//deletespecified的映射服务
void vidcManager :: xml_dele_mtcp(cBuffer &buffer,const char *mapname)
{
	std::map<std::string,mportTCP *>::iterator it=m_tcpsets.find(mapname);
	if(it!=m_tcpsets.end()){
		mportTCP * ptr_mtcp=(*it).second;
		ptr_mtcp->Stop();
		TMapParam *p=(ptr_mtcp)?((TMapParam *)(ptr_mtcp->Tag())):NULL;
		delete p; delete ptr_mtcp;
		m_tcpsets.erase(it);
		xml_list_mtcp(buffer);
	}else{//error
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>specified的映射 %s 不存在</retmsg>",mapname);
	}
	return;
}

//------------------------UDP映射集合----------------------

void vidcManager :: xml_list_mudp(cBuffer &buffer)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maplist>");
	
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</maplist>");
	return;
}
void vidcManager :: xml_info_mudp(cBuffer &buffer,const char *mapname){}
void vidcManager :: xml_start_mudp(cBuffer &buffer,const char *mapname){}
void vidcManager :: xml_stop_mudp(cBuffer &buffer,const char *mapname){}
void vidcManager :: xml_dele_mudp(cBuffer &buffer,const char *mapname){}