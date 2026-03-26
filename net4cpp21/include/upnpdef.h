/*******************************************************************
   *	upnpdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the UPnP protocol
   *	UPnP - Universal Plug and Play, used to find NAT/router devices that support UPnP for port mapping on the gateway
   *	UPnP devices listen on multicast ports. When a search request is received, the device checks the search criteria to determine if they match.
   *	if matched, a unicast SSDP (via HTTPU) response is sent to the control point	
   *	similarly, when a device is added to the network, it sends a multicast SSDP advertisement to announce its supported services. 
   *	
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_UPNPDEF_H__
#define __YY_UPNPDEF_H__

#define UPnP_MULTI_ADDR	"239.255.255.250" //UPnP multicast address and port
#define UPnP_MULTI_PORT	1900 
#define UPnP_MAX_MESAGE_SIZE 4096

#define HTTP_STATUS_OK 200

typedef struct _TUPnPInfo
{
	bool budp; //whether it is UDP type
	int mapport;
	int appport;
	bool bsuccess; //whether the mapping was successful
	std::string appsvr;
	std::string appdesc;
	std::string retmsg;  //error message returned on failure

}UPnPInfo,*PUPnPInfo;

#endif
/* what is the range of multicast addresses?
Multicast addresses are the reserved Class D address range from 224.0.0.0 to 239.255.255.255, and some addresses have specific uses:
224.0.0.0-244.0.0.255 can only be used within a LAN; routers will not forward them. 224.0.0.1 is the address for all hosts,
224.0.0.2 is the address for all routers, 224.0.0.5 is the address for all OSPF routers, 224.0.13 is the address for PIMv2 routers;
239.0.0.0-239.255.255.255 are private addresses (like 192.168.x.x); 224.0.1.0-238.255.255.255 can be used on the Internet.
*/
/*
[UPnP] Sended Search Packet(len=132), return 132
M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 6
ST: urn:schemas-upnp-org:service:WANIPConnection:1

[UPnP] Received response ,len=322
HTTP/1.1 200 OK
CACHE-CONTROL: max-age=100
DATE: Wed, 25 Jul 2007 00:42:39 GMT
EXT:
LOCATION: http://192.168.0.3:1900/igd.xml
SERVER: TP-LINK Router, UPnP/1.0
ST: urn:schemas-upnp-org:service:WANIPConnection:1
USN: uuid:upnp-WANConnectionDevice-192168035678900001::urn:schemas-upnp-org:service:WANIPConnection:1

[UPnP] Found Loaction: http://192.168.0.3:1900/igd.xml
[httpreq] Sending HTTP Request Header,len=207
GET /igd.xml HTTP/1.1
Accept: * /*
Accept-Language: zh-cn
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
Cache-Control: no-cache
Connection: close
Host: 192.168.0.3
Pragma: no-cache

[httprsp] Received HTTP Response Header
HTTP/1.1 200 OK
CONTENT-LENGTH: 2884
CONTENT-TYPE: text/xml
DATE: Wed, 25 Jul 2007 00:42:39 GMT
LAST-MODIFIED: Tue, 28 Oct 2003 08:46:08 GMT
SERVER: TP-LINK Router, UPnP/1.0
CONNECTION: close
[UPnP] Receive XML: 2884 / 2884


*/
