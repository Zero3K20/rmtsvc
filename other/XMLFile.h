

// XMLFile.h v1.0
// Purpose: interface for the CXMLFile class.
// by hujinshan @2004-11-9 10:15:58

#ifndef _XMLFILE_H_
#define _XMLFILE_H_

#pragma warning(disable: 4786)

//#import "C:/WINDOWS/SYSTEM32/msxml4.dll" named_guids raw_interfaces_only
//#import <msxml4.dll>
#import <msxml3.dll>

#include <atlbase.h>
#include <string>
#include <map>
#include <vector>

//typedef const char* LPCTSTR;

//!  XMLFile class for reading/writing XML files 
/*!
  can be used to save application settings.
*/

class CXMLFile
{
/// Construction
public:
	CXMLFile();
	CXMLFile(const char* filename);
	
	virtual ~CXMLFile();

/// Implementation
public:

	///getnode值(长整型)
	///saveXMLfile
	/** 
		\param cstrBaseKeyName 基键名.
		\param cstrValueName 键名(取值名).
		\param lDefaultValue default长整值.
	*/
	long GetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lDefaultValue);
	///setnode值(长整型)
	/** 
		\param cstrBaseKeyName 基键名.
		\param cstrValueName 键名(取save值名).
		\param lDefaultValue default长整值.
	*/
	long SetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lValue);

	///getnode值(string)
	/** 
		\param cstrBaseKeyName 基键名.
		\param cstrValueName 键名(取值名).
		\param cstrDefaultValue defaultstring值.
	*/
	std::string GetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue);
	std::string GetStringC(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue);
	///setnode值(string)
	/** 
		\param cstrBaseKeyName 基键名.
		\param cstrValueName 键名(save值名).
		\param cstrDefaultValue defaultstring值.
	*/
	long SetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrValue);
	
	///getnodeattribute
	/** 
		\param cstrBaseKeyName 基键名.
		\param cstrValueName 键名(saveattribute键名).
		\param cstrAttributeName attribute名(saveattribute值名).
		\param cstrDefaultAttributeValue defaultattribute值.
	*/
	std::string GetAttribute(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrAttributeName, const char* cstrDefaultAttributeValue);
	///setnodeattribute
	long SetAttribute(const char* cstrBaseKeyName, const char* cstrValueName,
					const char* cstrAttributeName, const char* cstrAttributeValue);

	///getnode值
	long GetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrDefaultValue, std::string& strValue, const char* cstrAttributeName, 
		const char* cstrDefaultAttributeValue,std::string& strAttributeValue);
	
	///setnode值
	long SetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrValue=NULL, const char* cstrAttributeName=NULL,
		const char* cstrAttributeValue=NULL);

	///delete某nodeand其all子node
	/*!
      all child node key values are saved in the keys_val parameter.
    */
	long DeleteSetting(const char* cstrBaseKeyName, const char* cstrValueName);

	///get某node的子node的键名
	/*!
      all child node key names are saved in the keys_val parameter.
    */
	long GetKeysValue(const char* cstrBaseKeyName, 
		std::map<std::string, std::string>& keys_val);

	///get某node的子node的键名
	long GetKeys(const char* cstrBaseKeyName, 
		std::vector<std::string>& keys);

	///saveXMLfile
	/** 
		\param filename savefilename.
	*/
	bool save(const char* filename=NULL);
	
	///装载XMLfile
	/** 
		\param filename 装入filename.
	*/
	bool load(const char* filename, const char* root_name="xmlRoot");
	//装载xmlcharacter流 //yyc add
	bool loadXML(const char *xmlBuffer, const char* root_name="xmlRoot");
	//get装载xmlerror
	std::string loadError();
	///notsave改变
	void DiscardChanges();
	///clear内容
	void clear();

	//------------------------------------------------------------------------------------
	long GetRootElem(MSXML2::IXMLDOMElementPtr rootElem);
	long GetNode(const char* cstrKeyName, MSXML2::IXMLDOMNodePtr& foundNode);

protected:

	MSXML2::IXMLDOMDocument2Ptr XmlDocPtr;

	std::string xml_file_name, m_root_name;

	std::string* ParseKeys(const char* cstrFullKeyPath, int &iNumKeys);	

	MSXML2::IXMLDOMNodePtr FindNode(MSXML2::IXMLDOMNodePtr parentNode, std::string* pCStrKeys, int iNumKeys, bool bAddNodes = FALSE);

};

#endif // _XMLFILE_H_

// end of file 


