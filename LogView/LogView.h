#pragma once

#include "resource.h"

inline bool FileExists(const char* fn){ return GetFileAttributes(fn)!=0xffffffff; }

//-------------------------------------------------------------------------------------------------
// LogView.cpp
//-------------------------------------------------------------------------------------------------

class DXCC;				// protect the forward references
class ADIF;
extern DXCC* dxcc;
extern ADIF* logbook;
void ErrorHandler();

//-------------------------------------------------------------------------------------------------
// reader.cpp
//-------------------------------------------------------------------------------------------------

class ITEM {			// an individual item
public:
	ITEM(){}			// use default initialisers
	ITEM(const char* n, const char* v = nullptr){
		name  = _strdup(n);
		if(v)
			value = _strdup(v);
	}
public:
	char *name{};
	char *value{};
public:
	bool isEOR(){ return value==nullptr && _stricmp(name, "EOR")==0; }
	bool isEOH(){ return value==nullptr && _stricmp(name, "EOH")==0; }
	static ITEM* read(char* &in);												// read the next item from a character array
	size_t length();															// return number of bytes needed to write this
	bool write(char* &out);														// write the item
	static size_t length(const char* name, const char* value=nullptr);			// number of bytes to write this
	static bool write(char* &out, const char* name, const char* value=nullptr, bool lf=false);	// write the item from parts
};

struct ENTRY {			// a group of items making up one log entry
public:
	ENTRY(){}
public:
	std::vector<ITEM> items{};
public:
	static ENTRY* read(char* &in);		// read the next entry
	bool write(char* &out);
	ITEM* find(const char* n);			// scan the vector
};

class ADIF {
public:
	ADIF(){}							// create empty
	~ADIF(){ delete preamble; }
public:
	char* preamble{};					// the lines of text before the header items
	std::vector<ITEM> headers{};		// lines before the <EOH>
	std::vector<ENTRY> entries{};		// log entries
public:
	void addHeader();								// add a reasonable header
	bool add(ENTRY *e){ entries.push_back(*e); }	// append an entry
	bool read(char* &in);							// read from a stream
	bool read(const char* filename);				// read a file
	bool write(char* &out);							// write to a stream
	bool write(const char* filename);				// write to a file

public:
	char* sortCol{};
	bool reverse{};
	static bool cmp(const ENTRY&, const ENTRY&);
	bool compare(const ENTRY&, const ENTRY&);
	bool sort(const char* column, bool rev);		// sort by field name

public:
	struct COL { const char* col; int maxW; };
	std::vector<COL> titles;						// column headers
};

//-------------------------------------------------------------------------------------------------
// lookup.cpp
//-------------------------------------------------------------------------------------------------

struct LOOKUP {
	const char* entity{};
	const char* continent{};
	const char* itu{};
	const char* cq{};
	const char* code{};
};
struct RAWLOOKUP {
	char prefix[30]{};
	char entity[35]{};
	char continent[6]{};
	char itu[6]{};
	char cq[6]{};
	char code[5]{};
	RAWLOOKUP(){}
	RAWLOOKUP(RAWLOOKUP*);
};

class DXCC {
public:
	DXCC();
	~DXCC(){}
	LOOKUP* lookup(const char* call);
private:
	std::unordered_map<std::string, LOOKUP> lookupTable;
	bool fetchARRL();					// get the ARRL file
	void postRaw(RAWLOOKUP* raw);		// post a lookup table entry
	bool processLookup(const char* p, int lineno);
	void processPrefix(RAWLOOKUP* raw);
};

//-------------------------------------------------------------------------------------------------
// config.cpp
//-------------------------------------------------------------------------------------------------

void readConfig(const char* section, const char* item, const char* def, char* buffer, int cb);
void writeConfig(const char* section, const char* item, const char* value);
bool GetFile(HWND hParent, const char* caption, char* file, int cb);
