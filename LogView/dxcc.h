#pragma once
//-------------------------------------------------------------------------------------------------
// lookup.h
//-------------------------------------------------------------------------------------------------

// Expand and use the ARRL's DXCC entity document to map callsign prefixes to DXCC entities

struct LOOKUP {					// this is the item saved in the unordered_map (hash table)
	const char* entity{};		// the prefix is the key and the data is _strdup()ed for size
	const char* continent{};
	const char* itu{};
	const char* cq{};
	const char* code{};
};
struct RAWLOOKUP {				// this is the stuff to make a LOOKUP from but easily copyable
	char prefix[30]{};			// high speed copy helps loading speed
	char entity[35]{};
	char continent[6]{};
	char itu[6]{};
	char cq[6]{};
	char code[5]{};
	RAWLOOKUP(){}				// empty constructor
	RAWLOOKUP(RAWLOOKUP*);		// copy constructor
};

class DXCC {
public:
	DXCC(bool force=false);						// force makes it reload the master file
	~DXCC(){}
	LOOKUP* lookup(const char* call);			// get the data for a call

private:
	std::unordered_map<std::string, LOOKUP> lookupTable;

	bool fetchARRL(bool force);					// get the ARRL file
	void postRaw(RAWLOOKUP* raw);				// post a lookup table entry
	bool processLookup(const char* p, int lineno);
	void processPrefix(RAWLOOKUP* raw);
};
