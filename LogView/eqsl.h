#pragma once
//-------------------------------------------------------------------------------------------------
// eqsl.h
//-------------------------------------------------------------------------------------------------

class EQSL {
public:
	bool update();										// read the data
	bool load(bool force=false);						// read the data if there is no local file
	bool read(const char* fname);						// expand the data

	std::unordered_map<std::string, ENTRY> eqslTable;	// basically a hash table
	char eqslFile[MAX_PATH]{};
};
