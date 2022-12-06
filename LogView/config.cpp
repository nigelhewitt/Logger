//-------------------------------------------------------------------------------------------------
// config.cpp : configuration and other utilities
//-------------------------------------------------------------------------------------------------

#include "framework.h"

//-------------------------------------------------------------------------------------------------
// ini file stuff
//-------------------------------------------------------------------------------------------------

char cwd[MAX_PATH]{};
static char iniFile[MAX_PATH]{};

void readConfig()
{
	if(iniFile[0]==0){
		(void)_getcwd(cwd, sizeof(cwd));
		strcpy_s(iniFile, sizeof(iniFile), cwd);
		strcat_s(iniFile, sizeof(iniFile), "\\config.edc");
	}
}
void readConfig(const char* section, const char* item, const char* def, char* buffer, int cb)
{
	readConfig();
	GetPrivateProfileString(section, item, def, buffer, cb, iniFile);
}
void  writeConfig(const char* section, const char* item, const char* value)
{
	readConfig();
	WritePrivateProfileString(section, item, value, iniFile);
}
//-------------------------------------------------------------------------------------------------
// GetFile()	I use the older style FileOpen dialog just because it isn't as smart as the new one
//-------------------------------------------------------------------------------------------------

bool GetFile(HWND hParent, const char* caption, char* file, int cb)
{
	OPENFILENAME ofn;
	char szDirName[MAX_PATH+10]{}, szFileTitle[MAX_PATH+10]{};
	file[0] = '\0';
	GetCurrentDirectory(sizeof(szDirName), szDirName);

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize		= sizeof(OPENFILENAME);
	ofn.hwndOwner		= hParent;
	ofn.lpstrFilter		= "ADIF files(*.adi)\0*.adi\0All Files\0*.*\0\0";
	ofn.nFilterIndex	= 1;
	ofn.lpstrFile		= file;
	ofn.nMaxFile		= cb;
	ofn.lpstrFileTitle	= szFileTitle;
	ofn.nMaxFileTitle	= sizeof(szFileTitle);
	ofn.lpstrInitialDir	= szDirName;
	ofn.Flags			= OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER;

	return GetOpenFileName(&ofn)!=0;
}

//-------------------------------------------------------------------------------------------------
// LoadFile()	read a file into an array of characters
//-------------------------------------------------------------------------------------------------

char* LoadFile(const char* fname)
{
	FILE* fh;
	char* buffer{};
	if(fopen_s(&fh, fname, "r")==0){
		if(fseek(fh, 0, SEEK_END)==0){
			size_t fsize = ftell(fh);
			fseek(fh, 0, SEEK_SET);
			buffer = new char[fsize+1];
			size_t n = fread_s(buffer, fsize+1, 1, fsize, fh);
			buffer[n] = 0;
			if(n==0){
				delete[] buffer;
				buffer = nullptr;
			}
		}
		fclose(fh);
	}
	return buffer;
}
