/*******************************************************************
   *	utils.cpp
   *    DESCRIPTION:utility functions collection
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-30
   *	net4cpp 2.1
   *******************************************************************/

#include "../include/sysconfig.h"  
#include "utils.h"


#include <cstdio>
#include <string>

using namespace std;
using namespace net4cpp21;

#ifdef WIN32

//return the size in KBytes of the deleted directory
unsigned long FILEIO::fileio_deleteDir(const char *spath)
{
	//the directory to delete must be an empty directory, so its contents must be deleted first
	string Path(spath);
	if(Path[Path.length()-1]!='\\') Path.append("\\");
	unsigned long lsize=0;
	WIN32_FIND_DATA finddata;
	string strTemp=Path;strTemp.append("*");
	HANDLE hd=::FindFirstFile(strTemp.c_str(), &finddata);
	if(hd!=INVALID_HANDLE_VALUE)
	{
		do{
			if(strcmp(finddata.cFileName,".")!=0&&strcmp(finddata.cFileName,"..")!=0)
			{
				if(finddata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				{
					strTemp=Path;strTemp.append(finddata.cFileName);
					lsize+=fileio_deleteDir(strTemp.c_str());
				}
				else
				{
					strTemp=Path;strTemp.append(finddata.cFileName);
					if(::DeleteFile(strTemp.c_str())!=0)
						lsize+=(finddata.nFileSizeLow>>10);
				}
			}
		}while(::FindNextFile(hd,&finddata));
		::FindClose(hd);
	}
	//**********************************************
	::RemoveDirectory(spath);
	return lsize;
}

//if file does not exist return -1
//if the specified path is a directory return -2
//otherwisereturnfile sizeBytes
long FILEIO::fileio_exist(const char *spath)
{
	if(spath==NULL || spath[0]==0) return -1;
	if(spath[2]==0 && spath[1]==':')
	{
		int i=0;
		if(spath[0]>='a' && spath[0]<='z')
			i=spath[0]-'a';
		else if(spath[0]>='A' && spath[0]<='Z')
			i=spath[0]-'A';
		else return -1;
		DWORD d=::GetLogicalDrives();
		if( d & (1<<i) ) return -2;
		return -1;
	}
	
	WIN32_FIND_DATA finddata; long lsize=0;
	HANDLE hd=::FindFirstFile(spath, &finddata);
	if(hd==INVALID_HANDLE_VALUE) return -1;
	if(finddata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
		lsize=-2;
	else if( (lsize=finddata.nFileSizeLow) <0 ) lsize=0;
	::FindClose(hd);
	return lsize;
}

bool FILEIO::fileio_exec(char *filename,bool ifHide)
{
	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	if(::CreateProcess(NULL,filename,NULL,NULL,FALSE,0,NULL, NULL,&si,&pi))
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return true;
	}
	return false;
}

#endif

/*
long FILEIO::fileio_filesize(const char *filename)
{
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return -1;
	::fseek(fp,0,SEEK_END);
	long filelen=::ftell(fp);
	::fclose(fp); return filelen;
}

//determine whether a directory or file exists
#include<io.h>
bool FILEIO::fileio_exist(const char *spath)
{
	return (_access(spath,0)==0)?true:false;
}
*/
