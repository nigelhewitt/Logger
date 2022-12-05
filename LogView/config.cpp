// reader.cpp : read/write operations on a log file.
//

#include "framework.h"
#include "LogView.h"

//-------------------------------------------------------------------------------------------------
// ini file
//-------------------------------------------------------------------------------------------------

// I am making the assumption that the read is called before write on all sequences
// but I wrote the code to ask first

static char iniFile[MAX_PATH]{};

void readConfig(const char* section, const char* item, const char* def, char* buffer, int cb)
{
	if(iniFile[0]==0){
		(void)_getcwd(iniFile, sizeof(iniFile));
		strcat_s(iniFile, sizeof(iniFile), "\\config.edc");
	}
	GetPrivateProfileString(section, item, def, buffer, cb, iniFile);
}
void  writeConfig(const char* section, const char* item, const char* value)
{
	WritePrivateProfileString(section, item, value, iniFile);
}

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
