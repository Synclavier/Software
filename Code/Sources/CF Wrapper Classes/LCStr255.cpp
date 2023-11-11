// LCStr255

// Simple extension to LStr255 to provide in-place C strings

#include "LCStr255.h"

LCStr255&   LCStr255::Append ( LCStr255& str )
{
    return Append(str.mString);
}

LCStr255& LCStr255::Append ( const char* str )
{
    strncat(mString, str, sizeof(mString) - strlen(str) - 1);
    
    return *this;
}

LCStr255& LCStr255::Append ( SInt32 num )
{
    char it[256] = {0};
    
    sprintf(it, "%d", (int) num);
    
    return Append(it);
}

LCStr255& LCStr255::Append ( char chr )
{
    if (Length()+1 >= sizeof(mString)) return *this;
    
    char* where = &mString[Length()];

    *where++ = chr;
    *where++ = 0;
    
    return *this;
}

LCStr255& LCStr255::Append ( CFStringRef str )
{
    if (Length()+1 >= sizeof(mString)) return *this;
    
    char* where = &mString[Length()];
    
    CFStringGetCString(str, where, sizeof(mString)-Length(), kCFStringEncodingUTF8);
    
    return *this;
}

LCStr255& LCStr255::AppendFormat ( const char *fmt, ... )
{
    char it[256] = {0};
    va_list	vaList;
    va_start(vaList, fmt);
    
    vsnprintf(it, sizeof(it), fmt, vaList);
    
    Append(it);

    return *this;
}

bool LCStr255::BeginsWith ( LCStr255& str )
{
    return BeginsWith((const char *) str);
}

bool LCStr255::BeginsWith ( const char* str )
{
    char* where = strcasestr(mString, str);
    
    return (where == &mString[0]);
}

bool LCStr255::EndsWith ( LCStr255& str )
{
    return EndsWith((const char *) str);
}

bool LCStr255::EndsWith ( const char* str )
{
    size_t strLen  = strlen(str);
    size_t mStrLen = strlen(mString);
    
    if (strLen > mStrLen)
        return false;
    
    char* whereToLook = &mString[mStrLen - strLen];
    char* where       = strcasestr(whereToLook, str);
    
    return (where == whereToLook);
}

int LCStr255::Find(char c, int at)  // -1 if not found
{
    char* whereAt = strchr(&mString[at], c);
    
    if (!whereAt) return (-1);
    
    return (int) (whereAt - mString);
}

LCStr255& LCStr255::Remove ( int inStartPos, int inCount )
{
    if (inStartPos >= Length()) return *this;
    
    if (inStartPos + inCount > Length())
        inCount = (int) (Length() - inStartPos);
    
    char* where = &mString[inStartPos];
    
    memmove(where, where+inCount, &mString[Length()+1] - (where + inCount));   // Copy down at least the trailing null
    
    return *this;
}

LCStr255& LCStr255::Assign ( char* inChars, int inCount )
{
    if (inCount > (sizeof(mString)-1))
        inCount = (int) (sizeof(mString)-1);
    
    memmove(mString, inChars, inCount);
    
    mString[inCount] = 0;
    
    return *this;
}

void LCStr255::AppendDecimalWithCommas(long theNum)
{
	if (theNum < 0)
	{
		*this += "-";
		theNum = -theNum;
	}
	
	if (theNum >= 1000000)
	{
		*this +=  (SInt32) (theNum / 1000000);
		theNum %= 1000000;
		*this += ",";
		
		if (theNum <100000)
			*this += "0";
			
		if (theNum <10000)
			*this += "0";
			
		if (theNum <1000)
			*this += "0,";

		if (theNum <100)
			*this += "0";
			
		if (theNum <10)
			*this += "0";
	}

	if (theNum >= 1000)
	{
		*this += (SInt32) (theNum / 1000);
		theNum %= 1000;
		*this += ",";
		
		if (theNum <100)
			*this += "0";
			
		if (theNum <10)
			*this += "0";
	}
	
	*this += (SInt32) theNum;
}

void LCStr255::SmartReplace(const char* replaceWhat, const char*  replaceWith)
{
    char* where = strcasestr(mString, replaceWhat);
    
    if (!where) return;
    
    if (Length() + strlen(replaceWith) - strlen(replaceWhat) >= sizeof(mString)) return;
    
    memmove(where+strlen(replaceWith), where+strlen(replaceWhat), &mString[Length()+1] - (where + strlen(replaceWhat)));
    memmove(where, replaceWith, strlen(replaceWith));
}

// Remove synclavier 4 character file name extension
void LCStr255::RemoveFileExtension()
{
	// Look for .xxxx
	if (Length() > 5 && TextPtr()[Length()-5] == '.')	// Look for .xxxx
	{
		if ((TextPtr()[Length()-6] != ':')				// But disallow w0:.xxxx
        &&  (TextPtr()[Length()-6] != '/'))             // And w0/.xxxx
			Remove((int) Length()-5, 5);
        
        return;
	}
	
	// Also look for .xxxx:
	if (Length() > 6 && TextPtr()[Length()-6] == '.' && TextPtr()[Length()-1] == ':')	// Look for .xxxx:
	{
		if (TextPtr()[Length()-7] != ':')				// But disallow w0:.xxxx
		{
			Remove((int) Length()-6, 6);
			Append(':');
		}
        
        return;
	}
    
	// Also look for .xxxx/
	if (Length() > 6 && TextPtr()[Length()-6] == '.' && TextPtr()[Length()-1] == '/')	// Look for .xxxx:
	{
		if (TextPtr()[Length()-7] != '/')				// But disallow w0/.xxxx
		{
			Remove((int) Length()-6, 6);
			Append('/');
		}
        
        return;
	}
}

// Remove synclavier 4 character file name extension from complete tree
void LCStr255::RemoveFileExtensionFromTree()
{
    if (Length() <= 1) return;
    
    char* where = strcasestr(&mString[1], ".simg");

    while (where) {
		Remove((int) (where-mString), 5);
        
        if (Length() <= 1) return;
        
        where = strcasestr(&mString[1], ".simg");
    }
}

