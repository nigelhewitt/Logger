//-------------------------------------------------------------------------------------------------
// config.cpp : configuration and other utilities
//-------------------------------------------------------------------------------------------------

#include "framework.h"

//-------------------------------------------------------------------------------------------------
// ini file stuff
//-------------------------------------------------------------------------------------------------

char dataFolder[MAX_PATH]{};
static char iniFile[MAX_PATH]{};

bool readConfig()				// returns true if it set things up new
{
	if(iniFile[0]==0){
		bool newLoad = false;
		PWSTR appdata{};
		if(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &appdata) == S_OK){
		    wcstombs_s(nullptr, dataFolder, sizeof(dataFolder), appdata, MAX_PATH);

			if(!FileExists(dataFolder))
				exit(1);
			strcat_s(dataFolder, sizeof(dataFolder), "\\Nigsoft");
			if(!FileExists(dataFolder))
				(void)_mkdir(dataFolder);

			if(!FileExists(dataFolder))
				exit(1);
			strcat_s(dataFolder, sizeof(dataFolder), "\\LogView");
			if(!FileExists(dataFolder)){
				newLoad = true;
				(void)_mkdir(dataFolder);
			}
		}
		else
			exit(99);

		strcpy_s(iniFile, sizeof(iniFile), dataFolder);
		strcat_s(iniFile, sizeof(iniFile), "\\config.edc");
		return newLoad;
	}
	return false;
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
// GetFileName()	I use the older style FileOpen dialog just because it isn't as smart as the new one
//-------------------------------------------------------------------------------------------------

bool GetFileName(HWND hParent, const char* caption, char* file, int cb)
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
//-------------------------------------------------------------------------------------------------
// unpack time
//-------------------------------------------------------------------------------------------------

unsigned int atou(const char* p, int cb)
{
	unsigned int x=0;
	for(int i=0; i<cb; ++i){
		x *= 10;
		x += p[i]-'0';
	}
	return x;
}
time_t unpackTime(const char* text)
{
	// input is "20221206190425"
	//           yyyymmddhhmmss
	size_t n = strlen(text);
	if(n!=14 && n!=12) return 0;
	tm t{};
	t.tm_year = atou(text,    4) - 1900;	// years since 1900
	t.tm_mon  = atou(text+4,  2) - 1;		// months since January (0-11)
	t.tm_mday = atou(text+6,  2);			// day of the month (1-31)
	t.tm_hour = atou(text+8,  2);			// hours
	t.tm_min  = atou(text+10, 2);			// minutes
	if(text[12])							// does that come with or without seconds?
		t.tm_sec  = atou(text+12, 2);		// seconds

	return mktime(&t);
}
