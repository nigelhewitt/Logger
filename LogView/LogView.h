#pragma once
//-------------------------------------------------------------------------------------------------
// LogView.h
//-------------------------------------------------------------------------------------------------

// Header for the LogView.cpp

class DXCC;				// protect the forward references
class ADIF;
class LOTW;
class EQSL;
class LISTVIEWCHILD;

extern DXCC* dxcc;
extern ADIF* logbook;
extern LOTW* lotw;
extern EQSL* eqsl;

extern HINSTANCE hInstance;					// current instance
extern HWND hFrame, hClient;				// frame and client windows
extern CRITICAL_SECTION CriticalSection;

inline bool FileExists(const char* fn){ return GetFileAttributes(fn)!=0xffffffff; }
#define SIZEOF(A)	(sizeof(A)/sizeof(A[0]))
void error(DWORD err=0);					// Windows error code to text handler
