#pragma once
//-------------------------------------------------------------------------------------------------
// LogView.h
//-------------------------------------------------------------------------------------------------

// The basic header for the LogView system

class DXCC;				// protect the forward references
class ADIF;
class LOTW;
class EQSL;
extern DXCC* dxcc;
extern ADIF* logbook;
extern LOTW* lotw;
extern EQSL* eqsl;

inline bool FileExists(const char* fn){ return GetFileAttributes(fn)!=0xffffffff; }
