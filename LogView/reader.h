#pragma once
//-------------------------------------------------------------------------------------------------
// reader.h
//-------------------------------------------------------------------------------------------------

// read an ADIF log file retaining it's free-form structure
// 'read' is based on handing a reference to a pointer into on enormous char[] around
// 'write' first gets the length of the string needed calling recursive length() then
//			puts the parts together

// !!!!!!!!!!!!!!!!!!!!! write is unfinished and untested !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

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
	bool isEOR(){ return value==nullptr && _stricmp(name, "EOR")==0; }			// test for delimiters
	bool isEOH(){ return value==nullptr && _stricmp(name, "EOH")==0; }
	
	static ITEM* read(char* &in);												// read the next item from a character array

	size_t length();																			// return number of bytes needed to write this
	bool write(char* &out);																		// write the item
	static size_t length(const char* name, const char* value=nullptr);							// number of bytes to write this
	static bool write(char* &out, const char* name, const char* value=nullptr, bool lf=false);	// write the item from parts
};

struct ENTRY {			// a group of items making up one log entry
public:
	ENTRY(){}
public:
	static ENTRY* read(char* &in);		// read the next entry
	ITEM* find(const char* n);			// scan the vector for a named item
	size_t length();
	bool write(char* &out);

	std::vector<ITEM> items{};
};

class ADIF {							// a representation of the file read
public:
	ADIF(){}							// create empty
	~ADIF(){ delete preamble; }
public:
	char* preamble{};					// the lines of text before the header items
	std::vector<ITEM> headers{};		// lines before the <EOH> end-of-heading
	std::vector<ENTRY> entries{};		// the actual log entries

public:
	bool add(ENTRY *e){ entries.push_back(*e); }	// append an entry
	bool read(char* &in);							// read from a stream
	bool read(const char* filename);				// read a file

	void addHeader();								// add a reasonable header for writing
	bool write(char* &out);							// write to a stream
	bool write(const char* filename);				// write to a file

public:
	char* sortCol{};								// name of column currently being sorted
	bool  reverse{};								// use reverse sort
	static bool cmp(const ENTRY&, const ENTRY&);	// I'm sure I can work round having a static with a lambda......
	bool  compare(const ENTRY&, const ENTRY&);
	bool  sort(const char* column, bool rev);		// sort by field name

public:
	struct COL { const char* col; int maxW; };
	std::vector<COL> titles;						// column headers
};
