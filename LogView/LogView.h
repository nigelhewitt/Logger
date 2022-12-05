#pragma once
// The basic header for the LogView system
// Yes I could have put all the classes in separate files but then it all gets messy

#include "resource.h"

inline bool FileExists(const char* fn){ return GetFileAttributes(fn)!=0xffffffff; }

//-------------------------------------------------------------------------------------------------
// LogView.cpp
//-------------------------------------------------------------------------------------------------

class DXCC;				// protect the forward references
class ADIF;
class LOTW;
extern DXCC* dxcc;
extern ADIF* logbook;
extern LOTW* lotw;

//-------------------------------------------------------------------------------------------------
// reader.cpp
//-------------------------------------------------------------------------------------------------

class ITEM {			// an individual ADIF item   eg:  <CALL:5>G8JFT
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
	static ENTRY* read(char* &in);		// read the next entry
	bool write(char* &out);
	ITEM* find(const char* n);			// scan the vector

	std::vector<ITEM> items{};
};

class ADIF {
public:
	ADIF(){}							// create empty
	~ADIF(){ delete preamble; }
public:
	char* preamble{};					// the lines of text before the header items
	std::vector<ITEM> headers{};		// lines before the <EOH>
	std::vector<ENTRY> entries{};		// the actual log entries

public:
	void addHeader();								// add a reasonable header
	bool add(ENTRY *e){ entries.push_back(*e); }	// append an entry
	bool read(char* &in);							// read from a stream
	bool read(const char* filename);				// read a file
	bool write(char* &out);							// write to a stream
	bool write(const char* filename);				// write to a file

public:
	char* sortCol{};
	bool  reverse{};
	static bool cmp(const ENTRY&, const ENTRY&);
	bool  compare(const ENTRY&, const ENTRY&);
	bool  sort(const char* column, bool rev);		// sort by field name

public:
	struct COL { const char* col; int maxW; };
	std::vector<COL> titles;						// column headers
};

//-------------------------------------------------------------------------------------------------
// lookup.cpp
//-------------------------------------------------------------------------------------------------

struct LOOKUP {					// this is the item saved in the unordered_map (hash table)
	const char* entity{};
	const char* continent{};
	const char* itu{};
	const char* cq{};
	const char* code{};
};
struct RAWLOOKUP {				// this is the stuff to make a LOOKUP from but copyable
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
	DXCC(bool force=false);
	~DXCC(){}
	LOOKUP* lookup(const char* call);

private:
	std::unordered_map<std::string, LOOKUP> lookupTable;
	bool fetchARRL(bool force);					// get the ARRL file
	void postRaw(RAWLOOKUP* raw);				// post a lookup table entry
	bool processLookup(const char* p, int lineno);
	void processPrefix(RAWLOOKUP* raw);
};

//-------------------------------------------------------------------------------------------------
// config.cpp
//-------------------------------------------------------------------------------------------------

extern char cwd[];
void readConfig();
void readConfig(const char* section, const char* item, const char* def, char* buffer, int cb);
void writeConfig(const char* section, const char* item, const char* value);
bool GetFile(HWND hParent, const char* caption, char* file, int cb);
char* LoadFile(const char* fname);

//-------------------------------------------------------------------------------------------------
// lotw.cpp
//-------------------------------------------------------------------------------------------------

class LOTW {
public:
	char url[500]{};
	void addArg(const char* name, const char* value);
	char sep{'?'};

	bool update();
	bool load(bool force=false);
	bool read(const char* fname);

	std::unordered_map<std::string, ENTRY> lotwTable;
};
