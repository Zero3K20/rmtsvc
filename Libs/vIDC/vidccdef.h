/*******************************************************************
   *	vidcsdef.h
   *    DESCRIPTION:declaration and definition header file
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_VIDCCDEF_H__
#define __YY_VIDCCDEF_H__

#include "mportdef.h"

typedef struct _VIDCSINFO
{
	std::string m_vidcsHost;
	int m_vidcsPort;
	std::string m_vidcsPswd; //password to access vIDCs
	bool m_bAutoConn;

	long m_vidccID; //ID assigned by vIDCs after connecting to vIDCs
	std::string m_vidcsIPList; //IP address list of vIDCs hosts returned by vIDCs
	int m_vidcsVer; //vIDCs version
}VIDCSINFO;

class mapInfo
{
public:
	VIDC_MAPTYPE m_mapType;
	std::string m_appsvr;//application service address and port; multiple can be separated by commas.
	std::string m_appdesc;
	SSLTYPE m_ssltype;
	bool m_sslverify;  //whether client certificate authentication is required; only valid when ssltype==TCPSVR_SSLSVR
	int m_proxyType; //supported proxy type
	std::string m_proxyuser;
	std::string m_proxypswd;
	bool m_proxyauth; //whether proxy requires authentication
	MPORTTYPE m_apptype;
	int m_mportBegin;  //required map port range
	int m_mportEnd;
	char m_bindLocalIP[16]; //required local IP to bind
	bool m_bAutoMap; //whether to auto-map after connecting to vIDCs
	long m_ipaccess;
	std::string m_ipRules;//IP access rules 
	std::string m_clicert; //client authentication certificate info
	std::string m_clikey;
	std::string m_clikeypswd;
	int m_mappedPort; //actual mapped port
	bool m_mappedSSLv; //whether the service port after mapping requires client certificate authentication
	
	unsigned long m_maxconn; //limit maximum connections, limit maximum bandwidth kb/s
	unsigned long m_maxratio;
	std::vector<std::string> m_hrspRegCond;
	std::vector<std::string> m_hreqRegCond;
	mapInfo(){
		m_mapType=VIDC_MAPTYPE_TCP;
		m_appsvr="";
		m_appdesc="";
		m_ssltype=TCPSVR_TCPSVR;
		m_sslverify=false;
		m_proxyType=7;
		m_proxyuser="";
		m_proxypswd="";
		m_proxyauth=false;
		m_apptype=MPORTTYPE_UNKNOW;
		m_mportBegin=m_mportEnd=0;
		m_bindLocalIP[0]=0;
		m_bAutoMap=false;
		m_ipaccess=1;
		m_ipRules="";
		m_clicert="";
		m_clikey="";
		m_clikeypswd="";
		m_mappedPort=0;
		m_mappedSSLv=false;

		m_maxconn=0;
		m_maxratio=0;
	}
	~mapInfo(){}
};


#endif
