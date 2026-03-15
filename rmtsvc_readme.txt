Rmtsvc 2.5.x
A web-based remote control tool integrating FTP service, proxy service, port mapping, and more.


[History of RmtSvc]

Around the year 2000, I used a remote control software called remoteAnywhere. That was the first time I used, and the first time I had seen, a web-based remote control tool. It was very impressive - I had no idea a browser could do all of that.

In 2001, while working at a network company mainly writing ASP applications, I created a web-based file management program called WebE (Web Explorer) using ASP, which mimicked the Windows Explorer interface for remote file operations through a browser.

In 2002, at a telecom company doing network application design and development on Linux, I accumulated and encapsulated a simple and reliable Linux/Windows cross-platform network library: net4cpp1.0 (since there was a log4cpp library on Linux for logging, I named mine net4cpp :)).

In 2002, based on net4cpp1.0, I wrote the first prototype version of rmtsvc, called remoteCtrol 1.0, which only supported remote desktop control through a browser.

In 2003, remote file, telnet, ftp, and other features were added, and the software was officially renamed rmtsvc2.3.x. Over the next two years it was continuously optimized and updated, adding MSN integration and the vIDC module, continuing up to version 2.4.7.

In 2006, rmtsvc was completely rewritten and version 2.5.x was released.
At the end of 2007, version 2.5.2 was released. After 2007, rmtsvc was no longer updated or improved - partly due to the increasing demands of work, and partly due to a loss of motivation and passion for writing code.

rmtsvc is arguably the first fully browser/server (B/S) architecture remote control software in China. Although it has many shortcomings, its advantages are clear: it can easily support mobile devices such as smartphones and iPads - any internet-connected device can be used for remote control. To support more browsers, only the front-end web pages need to be modified.

Rather than let it rot away unused, open-sourcing it allows more people to participate and improve it.


[Directory Description]
net4cpp21  - Core network library; the networking portion of rmtsvc is developed based on this library. It includes wrappers for basic networking as well as common protocols and services such as HTTP, FTP, DNS, SMTP, proxy, etc.
libs       - Library files required to compile rmtsvc, as well as source code for other libraries (e.g., MSN - fairly old, only supports up to MSNP13 / MSN 7.x).
             The bin subdirectory contains all compiled lib files needed for building (except for the OpenSSL lib files).
libsidc  - vidc library source code
bin        - rmtsvc executable directory
bin\html   - rmtsvc web front-end static pages; web pages access rmtsvc services via JS to parse and render XML data.

[Build Instructions]
This project has been compiled and tested with VC6 without any issues. Other versions have not been tested.
When compiling with VC6, add the OpenSSL header file and lib file paths under net4cpp21 to the build environment:
VC6 menu --> Tools --> Options --> Directories
Add include path:
<rmtsvc path>
et4cpp21\OPENSSL
Add library path:
<rmtsvc path>
et4cpp21\OPENSSL\LIB


net4cpp21 Network Class Hierarchy
***********************socket*****************************

                     socketBase
                         |
     |-----------------------------------------|
     |                   |                     |
 socketRaw           socketUdp            socketTcp-----|
     |                   |                     |        |
     |                   |                     |        |
 |--------|          dnsClient            socketSSL     |
 |        |                                    |________|
socketIcmp                                     |        
                          |------------------------------|
                          |                              |
                      socketSvr                     socketProxy
                          |                              |
                          |                              |
          |--------------------------|        |-----------------------------|   
                |         |          |        |          |          |
                |         |          |        |          |          |
            httpServer ftpServer smtpServer smtpClient ftpClient httpClient
