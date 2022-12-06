#pragma once
//-------------------------------------------------------------------------------------------------
// lotw.h
//-------------------------------------------------------------------------------------------------

// obtain and use my Log of The World QSL records to include a tick
class LOTW {
public:
	char url[500]{};									// url of page with data
	void addArg(const char* name, const char* value);	// add an argument to the url
	char sep{'?'};										// argument separator

	bool update();										// read the data
	bool load(bool force=false);						// read the data if there is no local file
	bool read(const char* fname);						// expand the data

	std::unordered_map<std::string, ENTRY> lotwTable;	// basically a hash table
};
