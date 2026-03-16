/*******************************************************************
   *	httpdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the HTTP protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-02-08
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_HTTPDEF_H__
#define __YY_HTTPDEF_H__


//HTTP constant definitions
#define HTTP_SERVER_PORT	80 //default HTTP service port
#define HTTPS_SERVER_PORT	443 //default HTTPS/SSL service port
#define HTTP_MAX_RESPTIMEOUT 10 //maximum response delay time for HTTP data receive

//HTTP return result constant definitions
#define SOCKSERR_HTTP_RESP -301 //command response error
#define SOCKSERR_HTTP_SENDREQ SOCKSERR_HTTP_RESP-1 //encode send HTTP request failure


//defineHTTPrequesttype
typedef enum
{
	HTTP_REQ_UNKNOWN   = 0,
	HTTP_REQ_POST      = 1,
	HTTP_REQ_GET       = 2,
	HTTP_REQ_HEAD      = 3,
	HTTP_REQ_PUT       = 4,
	HTTP_REQ_LINK      = 5,
	HTTP_REQ_DELETE    = 6,
	HTTP_REQ_UNLINK    = 7,	
	HTTP_REQ_CONNECT   = 8,
}HTTPREQ_TYPE;

//defineHTTPrequest的content的type
typedef enum
{
	HTTP_CONTENT_APPLICATION=0, //form提交data
	HTTP_CONTENT_TEXTXML=1, //xml内容
	HTTP_CONTENT_MULTIPART=2, //
	HTTP_CONTENT_UNKNOWED=3
}HTTPREQ_CONTENT_TYPE;

//httpauthenticationtype
typedef enum HttpAuthorization
{
	HTTP_AUTHORIZATION_ANONYMOUS = 0, //匿名访问
	HTTP_AUTHORIZATION_PLAINTEXT = 1,
}HTTPAUTH_TYPE;  

//HTTP response MIME type
typedef enum 
{
MIMETYPE_HTML=0,
MIMETYPE_XML,
MIMETYPE_TEXT,
MIMETYPE_CSS,
MIMETYPE_ZIP,
MIMETYPE_WORD,
MIMETYPE_OCTET,
MIMETYPE_ICON,
MIMETYPE_BMP,
MIMETYPE_GIF,
MIMETYPE_PNG,
MIMETYPE_JPG,
MIMETYPE_AVI,
MIMETYPE_ASF,
MIMETYPE_MPEG,
MIMETYPE_PDF,
MIMETYPE_MHT,
MIMETYPE_UNKNOWED,
MIMETYPE_NONE		//sendfile时no longer设定Content-Type,由user自己设定
}MIMETYPE_ENUM;

//HTTP service virtual directory permissions constant definitions
#define HTTP_ACCESS_READ 1 //read
#define HTTP_ACCESS_WRITE 2 //write
#define HTTP_ACCESS_EXEC 4 //executable
#define HTTP_ACCESS_LIST 8 //directory listing
#define HTTP_ACCESS_VHIDE 64 //show hidden files or directories
#define HTTP_ACCESS_SUBDIR_INHERIT 128 //subdirectory inheritance disabled

#define HTTP_ACCESS_ALL 0x7f
#define HTTP_ACCESS_NONE 0x0
#endif

/*
100 Continue：初始的request已经accept，客户应whencontinuesendrequest的其余partial。 
　　101tching Protocols：server将遵从客户的requestconvert到另外一种protocol 
　　200 OK：一切正常，对GETandPOSTrequest的应答文档跟at后面。ifnot用SetStatussetstatus代码，Servletdefault使用202status代码。 
　　201 Created server已经create了文档，Location头给出了它的URL。 
　　202 Accepted：已经accept request，但handle尚未complete。 
　　203 Non-Authoritative Information：文档已经正常地return，但一些应答头可能not正确，因为使用的yes文档的拷贝。 
　　204 No Content：没有新文档，浏览器shouldcontinue显示原来的文档。ifuser定期地refresh页面，而Servlet可以确定user文档足够新，这个status代码yes很有用的。 
　　205 Reset Content：没有新的内容，但浏览器shouldreset它所显示的内容。用来强制浏览器清除表单input内容。 
　　206 Partial Content：客户send了一个带有Range头的GETrequest，servercomplete了它。 
　　300 Multiple Choices：客户request的文档可以atmultiplepositionfound，这些position已经atreturn的文档内列出。ifserver要提出优先选择，则shouldatLocation应答头specifies。 
　　301 Moved Permanently：客户request的文档at其他地方，新的URLatLocation头中给出，浏览器shouldauto地访问新的URL。 
　　302 Found：class似于301，但新的URLshould被视为temporary性的替代，而is not永久性的。注意，atHTTP1.0中correspondingstatusinfoyes“Moved Temporatily”，而HttpServletResponse中correspondingconstantyesSC_MOVED_TEMPORARILY，而is notSC_FOUND。出现该status代码时，浏览器能够auto访问新的URL，therefore它is a很有用的status代码。为此，Servlet提供了一个专用的method，即sendRedirect。使用response.sendRedirect(url)比使用response.setStatus(response.SC_MOVED_TEMPORARILY)andresponse.setHeader("Location",url)更好。注意这个status代码有时候可以and301替换使用。　　 
　　303 See Other：class似于301/302，not同之处at于，if原来的requestyesPOST，Location头specified的重定向目标文档should通过GET提取。 
　　304 Not Modified：client有buffer的文档concurrent出了一个condition性的request（一般yes提供If-Modified-Since头表示客户只想比specifieddateupdate的文档）。server告诉客户，原来buffer的文档还可以continue使用。 
　　305 Use Proxy：客户request的文档should通过Location头所specifies的proxy service器提取。 
　　307 Temporary Redirect：and302（Found）相同。许多浏览器会error地response302应答进行重定向，即使原来的requestyesPOST，即使它实际上只能atPOSTrequest的应答yes303时才能重定向。由于这个原因，HTTP 1.1新增了307，以便更加清除地区分几个status代码：when出现303应答时，浏览器可以跟随重定向的GETandPOSTrequest；ifyes307应答，则浏览器只能跟随对GETrequest的重定向。注意：HttpServletResponse中没有为该status代码提供correspondingconstant。 
　　400 Bad Request：request出现语法error。 
　　401 Unauthorized：客户试图未经authorization访问受password保护的页面。应答中会contains一个WWW-Authenticate头，浏览器据此显示username字/password对话框，thenat填写合适的Authorization头后再次发出request。 
　　403 Forbidden：resourcenot可用。server理解客户的request，但rejecthandle它。通常由于server上fileordirectory的permissionsset导致。 
　　404 Not Found：无法foundspecifiedposition的resource。这也is a常用的应答，HttpServletResponse专门提供了correspondingmethod：sendError(message)。 
　　405 Method Not Allowed：request method（GET、POST、HEAD、DELETE、PUT、TRACE等）对specified的resourcenot适用。 
　　406 Not Acceptable：specified的resource已经found，但它的MIMEtypeand客户atAccpet头中所specified的incompatible。 
　　407 Proxy Authentication Required：class似于401，表示客户必须先经过proxy service器的authorization。 
　　408 Request Timeout：atserver许可的waitingtime内，客户一直没有发出任何request。客户可以at以后重复同一request。 
　　409 Conflict：通常andPUTrequest有关。由于requestandresource的currentstatus相冲突，thereforerequestcannotsuccess。 
　　410 Gone：所request的文档已经no longer可用，而且serverunknown道should重定向到哪一个address。它and404的not同at于，return407表示文档永久地离开了specified的position，而404表示由于unknown的原因文档not可用。 
　　411 Length Required：servercannothandlerequest，除非客户send一个Content-Length头。 
　　412 Precondition Failed：request头中specified的一些前提conditionfailure。 
　　413 Request Entity Too Large：目标文档的size超过servercurrent愿意handle的size。ifserver认为自己能够稍后再handle该request，则should提供一个Retry-After头。 
　　414 Request URI Too Long：URI太长。 
　　416 Requested Range Not Satisfiable：servercannot满足客户atrequest中specified的Range头。 
　　500 Internal Server Error：server遇到了意料not到的情况，cannotcomplete客户的request。 
　　501 Not Implemented：servernot supportedimplementationrequest所需要的功能。例如，客户发出了一个servernot supported的PUTrequest。 
　　502 Bad Gateway：server作为网关or者代理时，为了completerequest访问nextserver，但该serverreturn了非法的应答。 
　　503 Service Unavailable：server由于维护or者负载过重未能应答。例如，Servlet可能atdata库connect池已满的情况下return503。serverreturn503时可以提供一个Retry-After头。 
　　504 Gateway Timeout：由作为代理or网关的server使用，表示cannot及时地从remoteserverget应答。 
　　505 HTTP Version Not Supported：servernot supportedrequest中所specifies的HTTPversion。
*/

/*
httpprotocol通常packet括的method有很多，以下只列出athttp/1.1protocoldefine中看到的method：get、post、head、options、put、delete、trace、connect。

getmethod用于getURIresource，yes最为常用的一种method。

postmethod用于向specifiedURI提交内容，server端response其行为，该method也极为常用。

headmethod向URIsendrequest，仅仅只需要getresponse的protocol头。

putmethod用于向URIsendrequest，若URInotexists，则要求server端根据requestcreateresource。whenURIexists时，server端必须accept request内容，将其作为URIresource的modify后version。

deletemethod用于deleteURIidentifier的specifiedresource。

tracemethod用于激活server端对request的循环反馈，反馈作为HTTP response的正文内容被transfer回client。

connectmethod通常被用于使用代理connect。
*/
/* hhtp response头范例
HTTP/1.1 200 OK
Server: Microsoft-IIS/5.0
Date: Thu, 12 Jul 2007 07:10:25 GMT
X-Powered-By: ASP.NET
X-AspNet-Version: 1.1.4322
Cache-Control: no-cache
Pragma: no-cache
Expires: -1
Content-Type: text/html; charset=gb2312
Content-Length: 3381
*/