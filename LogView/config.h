#pragma once
//-------------------------------------------------------------------------------------------------
// config.h
//-------------------------------------------------------------------------------------------------

// generally the usual collection of stuff that has no obvious other place to live

extern char cwd[];				// current working directory

// read/write the 'ini' file
void readConfig();
void readConfig(const char* section, const char* item, const char* def, char* buffer, int cb);
void writeConfig(const char* section, const char* item, const char* value);

// run the  common control 'select file name' dialog
bool GetFile(HWND hParent, const char* caption, char* file, int cb);

// read a file into a char[]
char* LoadFile(const char* fname);
