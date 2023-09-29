#pragma once
//-------------------------------------------------------------------------------------------------
// LogView.h
//
// 2023-09-29  Add style to select whole line and tidy up some trailing spaces
//
//-------------------------------------------------------------------------------------------------

// Header for the LogView.cpp

class DXCC;				// protect the forward references
class LOTW;
class EQSL;
class LISTVIEWCHILD;

extern DXCC* dxcc;		// there only ever need be one of each of these
extern LOTW* lotw;
extern EQSL* eqsl;

extern HINSTANCE hInstance;					// current instance
extern HWND hFrame, hClient;				// frame and client windows
extern CRITICAL_SECTION CriticalSection;

inline bool FileExists(const char* fn){ return GetFileAttributes(fn)!=0xffffffff; }

#define SIZEOF(A)	(sizeof(A)/sizeof(A[0]))	// number of elements in a compiled in array
void error(DWORD err=0);						// Windows error code to text handler
