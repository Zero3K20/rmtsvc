/*******************************************************************
   *	utils.h
   *    DESCRIPTION:utility functions collection
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-30
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CUTILS_H__
#define __YY_CUTILS_H__

namespace net4cpp21
{
	class cUtils
	{
	public:
#ifdef WIN32
		static void usleep(unsigned int us)
		{
			Sleep(us/1000);
		}
		//create a process //return 0 on success
		static int execp(const char *pathfile,const char *args);
#else
		static void usleep(unsigned int us)
		{
			struct timeval to;
			//to.tv_sec = 0;
			//should add protection, us must be >=0 and less than 1000000
			//for efficiency, this protection is provided by the caller
			//to.tv_usec = us;
			to.tv_sec =us/1000000L;
			to.tv_usec =us%1000000L;
			select(0, NULL, NULL, NULL, &to);
		}
#endif
		//remove leading and trailing spaces from string
		static const char *strTrimLeft(const char *str)
		{
			if(str) for(;*str==' ';str++) NULL;
			return str;
		}
		static char *strTrimRight(char *str)
		{
			if(str)
			{
				char *ptr=str+strlen(str)-1;
				while(ptr>=str && *ptr==' ') *ptr--='\0';
			}
			return str;
		}
		static char *strTrim(char *str)
		{
			if(str){
				for(;*str==' ';str++) NULL;
				char *ptr=str+strlen(str)-1;
				while(ptr>=str && *ptr==' ') *ptr--='\0';
			}
			return str;
		}

		//return the number of characters replaced
		static int Replace(char *str,char findC,char replaceC)
		{
			int count=0;
			for(;*str!='\0' && *str==findC;*str++=replaceC,count++) NULL;
			return count;
		}
	};

	class FILEIO
	{
	public:
#ifdef WIN32
		static bool fileio_rename(const char *filename,const char *rename)
		{
		//	char *p=(char *)rename; while(*p){ if(*p=='/') *p='\\'; p++;}
			return (::MoveFile(filename,rename))?true:false;
		}

		static bool fileio_deleteFile(const char *filename)
		{
			return (::DeleteFile(filename))?true:false;
		}
		static bool fileio_createDir(const char *spath)
		{
			return (::CreateDirectory(spath,NULL))?true:false;
		}
		static unsigned long fileio_deleteDir(const char *spath);
		
		//if file does not exist return -1
		//if the specified path is a directory return -2
		//otherwise return file size in bytes
		static long fileio_exist(const char *spath);
		static bool fileio_exec(char *filename,bool ifHide);
#endif
	};
}//?namespace net4cpp21

#endif


/*
//binary search to find a specified value in a sorted array
	//return array index, return -1 if not found
	//uiSize -- array size, ifUp -- specifies whether sorted in ascending order
	template<class X> 
	int findit(const X iArray[],size_t uiSize,const X& xfind,bool ifUp)
	{
		if(uiSize==0) return -1;
		int idx,lbound=0,ubound=uiSize-1;
		if(ifUp)
		{
			while((idx=(ubound-lbound))>1)
			{
				idx=lbound+idx/2;
				if(iArray[idx]>xfind){ ubound=idx; continue; }
				if(iArray[idx]<xfind){ lbound=idx; continue; }
				return idx;
			}
		}
		else
		{
			while((idx=(ubound-lbound))>1)
			{
				idx=lbound+idx/2;
				if(iArray[idx]>xfind){ lbound=idx; continue; }
				if(iArray[idx]<xfind){ ubound=idx; continue; }
				return idx;
			}
		}
		if(iArray[lbound]==xfind) return lbound;
		if(iArray[ubound]==xfind) return ubound;
		return -1;
	}

	//how to determine if a number is a power of 2
	bool if2(long num)
	{
		return ((num-1) & num)==0);
	}

  //bubble sort
//The simplest sorting method is bubble sort. The basic idea is to treat elements as vertically arranged "bubbles",
//smaller elements are lighter and float upward. In bubble sort, this "bubble" sequence is processed multiple times.
//One pass means checking the sequence from bottom to top, paying attention to whether two adjacent elements are in the correct order.
//If two adjacent elements are in wrong order (lighter below), swap them. After
//one pass the lightest element reaches the top; after two passes the second lightest reaches the second position.
//On the second pass, the top position already holds the lightest element, so no need to check it. In general, on the i-th pass,
//no need to check elements above the i-th highest position, because after i-1 passes they are already correctly sorted	
template <class X>
void Sort(X iArray[],size_t lbound,size_t ubound)
{
	X tmp; 
	bool bSwap;
	for(size_t j=ubound;j>lbound;j--)
	{
		bSwap=false;
		for(size_t i=lbound;i<j;i++)
		{
			if(iArray[i]>iArray[i+1])
			{
				tmp=iArray[i];
				iArray[i]=iArray[i+1];
				iArray[i+1]=tmp;
				bSwap=true;
			}
		}
		if(!bSwap) break;
	}
	return;
}

//The basic idea of the quick sort algorithm:
//Quick sort is based on divide-and-conquer strategy. For an input subsequence ap..ar, if small enough sort directly, otherwise handle in three steps:
//
//Divide: Partition input sequence ap..ar into two non-empty subsequences ap..aq and aq+1..ar such that any element in ap..aq is not greater than any element in aq+1..ar.
//Conquer: Recursively call quick sort on ap..aq and aq+1..ar separately.
//Merge: Since sorting of the two subsequences is done in-place, once ap..aq and aq+1..ar are sorted, no further computation is needed and ap..ar is already sorted.
//This solution flow conforms to the basic steps of divide-and-conquer. Therefore, quick sort is one of the classic applications of divide-and-conquer.
template <class X>
void quickSort(X v[],int n)
{
	if(n<=1)  return;
	int i,last = 0;
	X temp;
    for(i=1; i<n; i++)
    {
         if(v[i] < v[0] && (++last!=i) )
         {
				temp = v[last];
				v[last] = v[i];
				v[i] = temp;
         }
    }
	temp = v[last];
	v[last] = v[0];
	v[0] = temp;

    quickSort(v, last);    //recursively srot
    quickSort(v+last+1, n-last-1);    //each part
}
*/


