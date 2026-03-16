
//find string
int  FindingString(const char* lpszSour, const char* lpszFind, int nStart = 0);
//string matching with wildcards
bool MatchingString(const char* lpszSour, const char* lpszMatch, bool bMatchCase = false);//true);
//多重匹配
bool MultiMatching(const char* lpszSour, const char* lpszMatch, int nMatchLogic = 0, bool bRetReversed = 0, bool bMatchCase = false);//true);
