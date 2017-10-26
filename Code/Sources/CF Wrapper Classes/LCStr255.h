// LCStr255

// LCStr255 with simple expansion to make an in-place c-string

#pragma once

class	LCStr255 {

public:
    inline  LCStr255( )                                     {memset(mString, 0, sizeof(mString));}
    inline  LCStr255( LCStr255&   str )                     {memset(mString, 0, sizeof(mString)); Append(str);}
    inline  LCStr255( const char* str )                     {memset(mString, 0, sizeof(mString)); Append(str);}
    inline  LCStr255( SInt32      num )                     {memset(mString, 0, sizeof(mString)); Append(num);}
    inline  LCStr255( char        chr )                     {memset(mString, 0, sizeof(mString)); Append(chr);}
    inline  LCStr255( CFStringRef str )                     {memset(mString, 0, sizeof(mString)); Append(str);}
	
	inline              operator const char*() const        {return (const char *) &mString[0];}
	inline              operator       char*() const        {return (      char *) &mString[0];}

    inline char&        operator [](int it)                 {return mString[it];}

	inline LCStr255&	operator  = ( LCStr255&   str )     {mString[0] = 0; return Append(str);}
	inline LCStr255&	operator  = ( const char* str )     {mString[0] = 0; return Append(str);}
	inline LCStr255&    operator  = ( SInt32      num )     {mString[0] = 0; return Append(num);}
	inline LCStr255&    operator  = ( char        chr )     {mString[0] = 0; return Append(chr);}

	inline LCStr255&	operator += ( LCStr255&   str )     {return Append(str);}
	inline LCStr255&	operator += ( const char* str )     {return Append(str);}
	inline LCStr255&    operator += ( SInt32      num )     {return Append(num);}
	inline LCStr255&    operator += ( char        chr )     {return Append(chr);}
	
    inline size_t       Length()                            {return strlen(mString);}
    inline char*        TextPtr()                           {return &mString[0];}
	inline void         UpdateCLength()                     {}

    LCStr255&   Append              (LCStr255&   str);
    LCStr255&   Append              (const char* str);
    LCStr255&   Append              (SInt32      num);
    LCStr255&   Append              (char        chr);
    LCStr255&   Append              (CFStringRef str);
    LCStr255&   AppendFormat        (const char * __restrict fmt, ...) __printflike(2, 3);
    
    bool		BeginsWith          (LCStr255&   str);
    bool		BeginsWith          (const char* str);
    bool		EndsWith            (LCStr255&   str);
    bool		EndsWith            (const char* str);
    int         Find                (char, int   at );  // -1 if not found
    
    LCStr255&   Remove              (int   inStartPos, int inCount);
    LCStr255&   Assign              (char* inChars,    int inCount);
    
	void AppendDecimalWithCommas    (long theNum);
	void SmartReplace               (const char* replaceWhat, const char* replaceWith);
	void RemoveFileExtension        ();
	void RemoveFileExtensionFromTree();
    
private:
	char    mString[256];

};

#if 0
LCStr255( const LStr255&	inOriginal) 	: LStr255(inOriginal) {}
LCStr255( const LString&	inOriginal) 	: LStr255(inOriginal) {}
LCStr255( ConstStringPtr	inStringPtr) 	: LStr255(inStringPtr) {}
LCStr255( UInt8				inChar) 		: LStr255(inChar) {}
LCStr255( char				inChar) 		: LStr255(inChar) {}
LCStr255( const char*		inCString) 		: LStr255(inCString) {}
LCStr255( const void*		inPtr,
         UInt8				inLength) 		: LStr255(inPtr, inLength) {}
LCStr255( Handle			inHandle) 		: LStr255(inHandle) {}
LCStr255( ResIDT			inResID,
         SInt16			inIndex) 		: LStr255(inResID, inIndex) {}
LCStr255( SInt32			inNumber) 		: LStr255(inNumber) {}
LCStr255( SInt16			inNumber) 		: LStr255(inNumber) {}
LCStr255( long double		inNumber,
         ConstStringPtr		inFormatString) : LStr255(inNumber, inFormatString) {}
LCStr255( long double				inNumber,
         const NumFormatString&	inNumFormat,
         const NumberParts&		inPartsTable) : LStr255(inNumber, inNumFormat, inPartsTable) {}
LCStr255( long double		inNumber,
         SInt8				inStyle,
         SInt16			inDigits) 		: LStr255(inNumber, inStyle, inDigits) {}
LCStr255( FourCharCode		inCode) 		: LStr255(inCode) {}
#endif

