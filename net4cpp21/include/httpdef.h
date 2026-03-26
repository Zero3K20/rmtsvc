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

//define HTTP request content type
typedef enum
{
	HTTP_CONTENT_APPLICATION=0, //form-submitted data
	HTTP_CONTENT_TEXTXML=1, //XML content
	HTTP_CONTENT_MULTIPART=2, //
	HTTP_CONTENT_UNKNOWED=3
}HTTPREQ_CONTENT_TYPE;

//httpauthenticationtype
typedef enum HttpAuthorization
{
	HTTP_AUTHORIZATION_ANONYMOUS = 0, //anonymous access
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
MIMETYPE_NONE		//when sending a file, Content-Type is no longer set automatically; the user sets it manually
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
100 Continue: the initial request has been accepted; the client should continue sending the rest of the request.
   101 Switching Protocols: the server will comply with the client's request to switch to another protocol.
   200 OK: everything is fine; the response body for GET and POST requests follows. If SetStatus is not used, Servlet uses status code 202 by default.
   201 Created: the server has created the document; the Location header gives its URL.
   202 Accepted: the request has been accepted, but processing is not yet complete.
   203 Non-Authoritative Information: the document has been returned normally, but some response headers may be incorrect because a copy of the document was used.
   204 No Content: no new document; the browser should continue displaying the original document. Useful when the user periodically refreshes the page and the Servlet can confirm the document is still fresh.
   205 Reset Content: no new content, but the browser should reset what it displays. Used to force the browser to clear form input fields.
   206 Partial Content: the client sent a GET request with a Range header, and the server completed it.
   300 Multiple Choices: the requested document can be found at multiple locations, which are listed in the returned document. If the server wants to suggest a preference, it should specify it in the Location response header.
   301 Moved Permanently: the requested document is elsewhere; the new URL is given in the Location header, and the browser should automatically access the new URL.
   302 Found: similar to 301, but the new URL should be treated as a temporary replacement, not permanent. Note that in HTTP 1.0 the corresponding status description is "Moved Temporarily", and in HttpServletResponse the corresponding constant is SC_MOVED_TEMPORARILY, not SC_FOUND. When this status code appears, browsers can automatically access the new URL, so it is a very useful status code. Servlet provides a dedicated method for this purpose: sendRedirect. Using response.sendRedirect(url) is better than using response.setStatus(response.SC_MOVED_TEMPORARILY) and response.setHeader("Location",url). Note that this status code can sometimes be used interchangeably with 301.
   303 See Other: similar to 301/302, except that if the original request was POST, the redirect target document specified in the Location header should be fetched with GET.
   304 Not Modified: the client has a cached document and issued a conditional request (usually by providing the If-Modified-Since header indicating the client only wants documents updated after a specified date). The server tells the client that the originally cached document can still be used.
   305 Use Proxy: the requested document should be fetched through the proxy server specified in the Location header.
   307 Temporary Redirect: same as 302 (Found). Many browsers incorrectly handle 302 responses by redirecting even if the original request was POST, even though redirection should only happen when the response to a POST request is 303. For this reason, HTTP 1.1 added 307 to more clearly distinguish status codes: when a 303 response occurs, browsers can follow the redirect for both GET and POST requests; if the response is 307, browsers should only follow the redirect for GET requests. Note: HttpServletResponse does not provide a corresponding constant for this status code.
   400 Bad Request: a syntax error occurred in the request.
   401 Unauthorized: the client attempted to access a password-protected page without authorization. The response will contain a WWW-Authenticate header; the browser will display a username/password dialog, and then re-issue the request with an appropriate Authorization header.
   403 Forbidden: the resource is unavailable. The server understands the client's request but refuses to handle it. Usually caused by file or directory permission settings on the server.
   404 Not Found: unable to find the resource at the specified location. This is a common response; HttpServletResponse provides a dedicated method: sendError(message).
   405 Method Not Allowed: the request method (GET, POST, HEAD, DELETE, PUT, TRACE, etc.) is not applicable to the specified resource.
   406 Not Acceptable: the specified resource was found, but its MIME type is incompatible with what the client specified in the Accept header.
   407 Proxy Authentication Required: similar to 401, indicates that the client must first be authorized by the proxy server.
   408 Request Timeout: the client did not send any request within the time allowed by the server. The client can repeat the same request later.
   409 Conflict: usually related to PUT requests. The request cannot succeed because it conflicts with the current state of the resource.
   410 Gone: the requested document is no longer available, and the server does not know which address to redirect to. The difference from 404 is that returning 410 indicates the document has permanently left the specified location, while 404 means the document is unavailable for unknown reasons.
   411 Length Required: the server cannot handle the request unless the client sends a Content-Length header.
   412 Precondition Failed: some preconditions specified in the request headers failed.
   413 Request Entity Too Large: the size of the target document exceeds what the server is currently willing to handle. If the server thinks it can handle the request later, it should provide a Retry-After header.
   414 Request URI Too Long: the URI is too long.
   416 Requested Range Not Satisfiable: the server cannot satisfy the Range header specified in the request.
   500 Internal Server Error: the server encountered an unexpected condition and cannot complete the client's request.
   501 Not Implemented: the server does not support the functionality required to fulfill the request. For example, the client issued a PUT request that the server does not support.
   502 Bad Gateway: the server, acting as a gateway or proxy, received an invalid response while accessing the next server to complete the request.
   503 Service Unavailable: the server is unavailable due to maintenance or overload. For example, a Servlet may return 503 when the database connection pool is full. The server can provide a Retry-After header when returning 503.
   504 Gateway Timeout: used by a server acting as a proxy or gateway, indicates it could not get a response from the remote server in time.
   505 HTTP Version Not Supported: the server does not support the HTTP version specified in the request.
*/

/*
The HTTP protocol typically includes many methods; below are only those defined in the HTTP/1.1 protocol specification: GET, POST, HEAD, OPTIONS, PUT, DELETE, TRACE, CONNECT.

GET method is used to retrieve URI resources; it is the most commonly used method.

POST method is used to submit content to a specified URI; the server responds to the request accordingly. This method is also extremely common.

HEAD method sends a request to a URI, requiring only the protocol headers of the response.

PUT method sends a request to a URI; if the URI does not exist, the server is required to create the resource based on the request. When the URI exists, the server must accept the request content and treat it as a modified version of the URI resource.

DELETE method is used to delete the specified resource identified by the URI.

TRACE method is used to activate a loopback of the request on the server side; the loopback is transferred back to the client as the body of an HTTP response.

CONNECT method is typically used to establish a proxy connection.
*/
/* HTTP response header example
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