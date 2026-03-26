/*******************************************************************
   *	ftpdef.h
   *    DESCRIPTION:constants, structures and enum definitions for the FTP protocol
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-16
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_FTPDEF_H__
#define __YY_FTPDEF_H__

#include <map>
#include <string>
#include "IPRules.h"

//FTP constant definitions
#define FTP_SERVER_PORT	21 //default FTP service port
#define FTP_MAX_COMMAND_SIZE 256 //maximum byte length of FTP commands
#define FTP_MAX_LOGINTIMEOUT 10 //s maximum FTP client login delay; close connection if exceeded

//FTP return result constant definitions
#define SOCKSERR_FTP_RESP -301 //command response error
#define SOCKSERR_FTP_AUTH SOCKSERR_FTP_RESP-1 //FTP authentication failure
#define SOCKSERR_FTP_SUPPORT SOCKSERR_FTP_RESP-2 //FTP password encryption transfer method not supported
#define SOCKSERR_FTP_REST SOCKSERR_FTP_RESP-3 //this site does not support resume transfer
#define SOCKSERR_FTP_DATACONN SOCKSERR_FTP_RESP-4 //cannot connect to data port
#define SOCKSERR_FTP_LIST SOCKSERR_FTP_RESP-5 //LIST response error
#define SOCKSERR_FTP_RETR SOCKSERR_FTP_RESP-6 //RETR response error
#define SOCKSERR_FTP_STOR SOCKSERR_FTP_RESP-7 //STOR response error
#define SOCKSERR_FTP_FILE SOCKSERR_FTP_RESP-8 //file operation error
#define SOCKSERR_FTP_FAILED SOCKSERR_FTP_RESP-9 //general error
#define SOCKSERR_FTP_DENY SOCKSERR_FTP_RESP-10 //access denied
#define SOCKSERR_FTP_NOEXIST SOCKSERR_FTP_RESP-11 //directory or file does not exist
#define SOCKSERR_FTP_EXIST SOCKSERR_FTP_RESP-12 //directory or file exists
#define SOCKSERR_FTP_USER SOCKSERR_FTP_RESP-13 //non-existent account
#define SOCKSERR_FTP_UNKNOWED SOCKSERR_FTP_RESP-14 //unknown error

#define FTP_DATACONN_PORT 0 //FTP data transfer mode
#define FTP_DATACONN_PASV 1

//ftplogeventdefine
#define FTP_LOGEVENT_LOGIN 1
#define FTP_LOGEVENT_LOGOUT 2
#define FTP_LOGEVENT_UPLOAD 4
#define FTP_LOGEVENT_DWLOAD 8
#define FTP_LOGEVENT_DELETE 16
#define FTP_LOGEVENT_RMD 32
#define FTP_LOGEVENT_SITE 64

//bits 0~1: password verification mode
#define OTP_NONE 0
#define OTP_MD4 1
#define OTP_MD5 2

//defineaccounttype
#define ACCOUNT_NORMAL 0
#define ACCOUNT_ADMIN 1
#define ACCOUNT_ROOT 2

typedef struct _ftpaccount //ftpaccountinfo
{
	std::string m_username;//account name, case-insensitive (converted to lowercase)
	std::string m_userpwd;//password; if password=="", no password verification is required
	std::string m_username_root;//the ROOT permission user this account belongs to; if not null, this account was dynamically created by a ROOT user
	unsigned long m_maxupratio;//max upload rate K/s, 0 means unlimited
	unsigned long m_maxdwratio;//max download rate K/s, 0 means unlimited
	unsigned long m_maxupfilesize;//max upload file size in KBytes, 0 means unlimited
	unsigned long m_maxdisksize;//maximum disk space limit in KBytes, 0 means unlimited
	unsigned long m_curdisksize;//current used disk space in KBytes.
	std::map<std::string,std::pair<std::string,long> > m_dirAccess;//directory access permissions; directory names are case-sensitive
			//first --- string : FTP virtual directory path, ending with /. For example: / or /aa/,
			//second --- pair : the actual directory and access permissions corresponding to this FTP virtual directory; the actual directory must end with \ (Windows platform)
	net4cpp21::iprules m_ipRules;//IP access rules
	long m_loginusers;//current number of users logged in to this FTP service with this account; the account can only be deleted when no users are connected
	long m_maxLoginusers;//limit the maximum simultaneous logged-in users for this account; <=0 means unlimited 
	time_t m_limitedTime;//limit this account to be valid only before a certain date; ==0 means unlimited
	long m_bitQX; //bits 0~1: password verification mode
				  //bits 2~3: define account type
				  //bit 4: whether to show hidden files

	bool bDsphidefiles() { return ((m_bitQX & 0x10)!=0); }
	void bDsphidefiles(bool b)
	{
		if(b)
			m_bitQX |=0x10;
		else
			m_bitQX &=0xffffffef;
		return;
	}
	long lRemoteAdmin() { return (m_bitQX & 0x0c)>>2; }
	void lRemoteAdmin(long l)
	{
		l =(l & 0x3)<<2; 
		m_bitQX &=0xfffffff3;
		m_bitQX |=l; return;
	}
	long lPswdMode() { return (m_bitQX & 0x03); }
	void lPswdMode(long l)
	{
		l &=0x3; 
		m_bitQX &=0xfffffffc;
		m_bitQX |=l; return;
	}
}FTPACCOUNT;

//FTP read/write permission constant definitions
#define FTP_ACCESS_FILE_READ 1 //file read allowed
#define FTP_ACCESS_FILE_WRITE 2 //file write allowed
#define FTP_ACCESS_FILE_DELETE 4 //file delete allowed
#define FTP_ACCESS_FILE_EXEC 8 //file execute allowed
#define FTP_ACCESS_DIR_LIST 16 //allow directory listing
#define FTP_ACCESS_DIR_CREATE 32 //allow create directory
#define FTP_ACCESS_DIR_DELETE 64 //allow delete directory
#define FTP_ACCESS_SUBDIR_INHERIT 128 //subdirectory inheritance disabled

#define FTP_ACCESS_ALL 0x7f
#define FTP_ACCESS_NONE 0x0
#endif

/*
The following are the FTP commands:

            USER <SP> <username> <CRLF>
            PASS <SP> <password> <CRLF>
            ACCT <SP> <account-information> <CRLF>
            CWD  <SP> <pathname> <CRLF>
            CDUP <CRLF>
            SMNT <SP> <pathname> <CRLF>
            QUIT <CRLF>
            REIN <CRLF>
            PORT <SP> <host-port> <CRLF>
            PASV <CRLF>
            TYPE <SP> <type-code> <CRLF>
            STRU <SP> <structure-code> <CRLF>
            MODE <SP> <mode-code> <CRLF>
            RETR <SP> <pathname> <CRLF>
            STOR <SP> <pathname> <CRLF>
            STOU <CRLF>
            APPE <SP> <pathname> <CRLF>
            ALLO <SP> <decimal-integer>
                [<SP> R <SP> <decimal-integer>] <CRLF>
            REST <SP> <marker> <CRLF>
            RNFR <SP> <pathname> <CRLF>
            RNTO <SP> <pathname> <CRLF>
            ABOR <CRLF>
            DELE <SP> <pathname> <CRLF>
            RMD  <SP> <pathname> <CRLF>
            MKD  <SP> <pathname> <CRLF>
            PWD  <CRLF>
            LIST [<SP> <pathname>] <CRLF>
            NLST [<SP> <pathname>] <CRLF>
            SITE <SP> <string> <CRLF>
            SYST <CRLF>
            STAT [<SP> <pathname>] <CRLF>
            HELP [<SP> <string>] <CRLF>
            NOOP <CRLF>

  The syntax of the above argument fields (using BNF notation
         where applicable) is:

            <username> ::= <string>
            <password> ::= <string>
            <account-information> ::= <string>
            <string> ::= <char> | <char><string>
            <char> ::= any of the 128 ASCII characters except <CR> and
            <LF>
            <marker> ::= <pr-string>
            <pr-string> ::= <pr-char> | <pr-char><pr-string>
            <pr-char> ::= printable characters, any
                          ASCII code 33 through 126
            <byte-size> ::= <number>
            <host-port> ::= <host-number>,<port-number>
            <host-number> ::= <number>,<number>,<number>,<number>
            <port-number> ::= <number>,<number>
            <number> ::= any decimal integer 1 through 255
            <form-code> ::= N | T | C
            <type-code> ::= A [<sp> <form-code>]
                          | E [<sp> <form-code>]
                          | I
                          | L <sp> <byte-size>
            <structure-code> ::= F | R | P
            <mode-code> ::= S | B | C
            <pathname> ::= <string>
            <decimal-integer> ::= any decimal integer

*/

/*
>>3.3 SSL FTP extension
In RFC 2228, the FTP protocol extended the following commands:
AUTH (Authentication/Security Mechanism),
ADAT (Authentication/Security Data),
PROT (Data Channel Protection Level),
PBSZ (Protection Buffer Size),
CCC (Clear Command Channel),
MIC (Integrity Protected Command),
CONF (Confidentiality Protected Command), and
ENC (Privacy Protected Command).

The main commands related to SSL extension are:
AUTH (negotiate extended authentication): specifies the extended authentication method, SSL or TLS;
PBSZ (negotiate protection buffer): specifies the protection buffer; must be 0 in SSL/TLS mode;
PROT (switch protection level): switches protection level; can be "C" (no protection) or "P" (protected);
*/

/*
*****Reply Codes by Function Groups******
200 Command okay.
         500 Syntax error, command unrecognized.
             This may include errors such as command line too long.
         501 Syntax error in parameters or arguments.
         202 Command not implemented, superfluous at this site.
         502 Command not implemented.
         503 Bad sequence of commands.
         504 Command not implemented for that parameter.

110 Restart marker reply.
             In this case, the text is exact and not left to the
             particular implementation; it must read:
                  MARK yyyy = mmmm
             Where yyyy is User-process data stream marker, and mmmm
             server's equivalent marker (note the spaces between markers
             and "=").
         211 System status, or system help reply.
         212 Directory status.
         213 File status.
         214 Help message.
             On how to use the server or the meaning of a particular
             non-standard command.  This reply is useful only to the
             human user.
         215 NAME system type.
             Where NAME is an official system name from the list in the
             Assigned Numbers document.
          
         120 Service ready in nnn minutes.
         220 Service ready for new user.
         221 Service closing control connection.
             Logged out if appropriate.
         421 Service not available, closing control connection.
             This may be a reply to any command if the service knows it
             must shut down.
         125 Data connection already open; transfer starting.
         225 Data connection open; no transfer in progress.
         425 Can't open data connection.
         226 Closing data connection.
             Requested file action successful (for example, file
             transfer or file abort).
         426 Connection closed; transfer aborted.
         227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
          
         230 User logged in, proceed.
         530 Not logged in.
         331 User name okay, need password.
         332 Need account for login.
         532 Need account for storing files.

150 File status okay; about to open data connection.
         250 Requested file action okay, completed.
         257 "PATHNAME" created.
         350 Requested file action pending further information.
         450 Requested file action not taken.
             File unavailable (e.g., file busy).
         550 Requested action not taken.
             File unavailable (e.g., file not found, no access).
         451 Requested action aborted. Local error in processing.
         551 Requested action aborted. Page type unknown.
         452 Requested action not taken.
             Insufficient storage space in system.
         552 Requested file action aborted.
             Exceeded storage allocation (for current directory or
             dataset).
         553 Requested action not taken.
             File name not allowed.

*****Numeric  Order List of Reply Codes***
110 Restart marker reply.
 In this case, the text is exact and not left to the
 particular implementation; it must read:
      MARK yyyy = mmmm
 Where yyyy is User-process data stream marker, and mmmm
 server's equivalent marker (note the spaces between markers
 and "=").
120 Service ready in nnn minutes.
125 Data connection already open; transfer starting.
150 File status okay; about to open data connection.

200 Command okay.
202 Command not implemented, superfluous at this site.
211 System status, or system help reply.
212 Directory status.
213 File status.
214 Help message.
 On how to use the server or the meaning of a particular
 non-standard command.  This reply is useful only to the
 human user.
215 NAME system type.
 Where NAME is an official system name from the list in the
 Assigned Numbers document.
220 Service ready for new user.
221 Service closing control connection.
 Logged out if appropriate.
225 Data connection open; no transfer in progress.
226 Closing data connection.
 Requested file action successful (for example, file
 transfer or file abort).
227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
230 User logged in, proceed.
250 Requested file action okay, completed.
257 "PATHNAME" created.

331 User name okay, need password.
332 Need account for login.
350 Requested file action pending further information.

421 Service not available, closing control connection.
 This may be a reply to any command if the service knows it
 must shut down.
425 Can't open data connection.
426 Connection closed; transfer aborted.
450 Requested file action not taken.
 File unavailable (e.g., file busy).
451 Requested action aborted: local error in processing.
452 Requested action not taken.
 Insufficient storage space in system.

500 Syntax error, command unrecognized.
 This may include errors such as command line too long.
501 Syntax error in parameters or arguments.
502 Command not implemented.
503 Bad sequence of commands.
504 Command not implemented for that parameter.
530 Not logged in.
532 Need account for storing files.
550 Requested action not taken.
 File unavailable (e.g., file not found, no access).
551 Requested action aborted: page type unknown.
552 Requested file action aborted.
 Exceeded storage allocation (for current directory or
 dataset).
553 Requested action not taken.
 File name not allowed.

*/
