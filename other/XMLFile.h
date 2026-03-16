

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

	///get node value (long integer)
	///saveXMLfile
	/** 
		\param cstrBaseKeyName base key name.
		\param cstrValueName key name (value name).
		\param lDefaultValue default long integer value.
	*/
	long GetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lDefaultValue);
	///set node value (long integer)
	/** 
		\param cstrBaseKeyName base key name.
		\param cstrValueName key name (for saving value).
		\param lDefaultValue default long integer value.
	*/
	long SetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lValue);

	///get node value (string)
	/** 
		\param cstrBaseKeyName base key name.
		\param cstrValueName key name (value name).
		\param cstrDefaultValue default string value.
	*/
	std::string GetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue);
	std::string GetStringC(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue);
	///set node value (string)
	/** 
		\param cstrBaseKeyName base key name.
		\param cstrValueName key name (value name for saving).
		\param cstrDefaultValue default string value.
	*/
	long SetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrValue);
	
	///getnodeattribute
	/** 
		\param cstrBaseKeyName base key name.
		\param cstrValueName key name (for saving attribute key name).
		\param cstrAttributeName attribute name (for saving attribute value name).
		\param cstrDefaultAttributeValue default attribute value.
	*/
	std::string GetAttribute(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrAttributeName, const char* cstrDefaultAttributeValue);
	///setnodeattribute
	long SetAttribute(const char* cstrBaseKeyName, const char* cstrValueName,
					const char* cstrAttributeName, const char* cstrAttributeValue);

	///get node value
	long GetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrDefaultValue, std::string& strValue, const char* cstrAttributeName, 
		const char* cstrDefaultAttributeValue,std::string& strAttributeValue);
	
	///set node value
	long SetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrValue=NULL, const char* cstrAttributeName=NULL,
		const char* cstrAttributeValue=NULL);

	///delete a node and all its child nodes
	/*!
      all child node key values are saved in the keys_val parameter.
    */
	long DeleteSetting(const char* cstrBaseKeyName, const char* cstrValueName);

	///get key names of child nodes of a node
	/*!
      all child node key names are saved in the keys_val parameter.
    */
	long GetKeysValue(const char* cstrBaseKeyName, 
		std::map<std::string, std::string>& keys_val);

	///get key names of child nodes of a node
	long GetKeys(const char* cstrBaseKeyName, 
		std::vector<std::string>& keys);

	///saveXMLfile
	/** 
		\param filename savefilename.
	*/
	bool save(const char* filename=NULL);
	
	///load XML file
	/** 
		\param filename the filename to load.
	*/
	bool load(const char* filename, const char* root_name="xmlRoot");
	//load XML character stream //yyc add
	bool loadXML(const char *xmlBuffer, const char* root_name="xmlRoot");
	//get XML load error
	std::string loadError();
	///discard changes
	void DiscardChanges();
	///clear content
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


