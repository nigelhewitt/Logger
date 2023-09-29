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

// a string all digits
bool isdigits(const char *str)
{
	for(int i=0; str[i]; ++i)
		if(!isdigit(str[i]))
			return false;
	return true;
}
//-------------------------------------------------------------------------------------------------
// calculate the distance between squares
//-------------------------------------------------------------------------------------------------

// first convert a square to latitude and longitude

bool square2spherical(const char* square, double& latitude, double& longitude)
{
	// I am IO90WT or sometimes IO90
	int n = (int)strlen(square);
	if(n!=4 && n!=6) return false;

	// latitude:
	char c = square[1];			// 'O' in 'A' to 'R' A = south pole each slot is 10°
	if(!isalpha(c)) return false;
	c = toupper(c);
	if(c>'R') return false;		// so 0-17 inclusive
	c -= 'A';					// 'O'-'A' = 14 so 140° N of south pole so 50° N of the equator
	latitude = c*10 - 90;		// convert to degrees N (S is negative)

	c = square[3];				// '0'
	if(!isdigit(c)) return false;
	c -= '0';					// '0' - '0' = 0 0-9 are 10° so this is just degrees
	latitude += c;
	if(n==6){
		c = square[5];			// ' W'
		if(!isalpha(c)) return false;
		c = toupper(c);
		if(c>'X') return false;	// so 0-23 inclusive
		c -= 'A';				// 'W'-'A' = 19
		latitude += (c/24.0);	// 24ths of a degree
		latitude += (1/48.0);	// move to the centre of the square
	}
	else
		latitude += 0.5;		// move to the centre of the square

	// longitude:
	c = square[0];			// 'I' in 'A' to 'R' A = 180 longitude,
								//		out in the Pacific ocean, each slot is 20° (note difference)
	if(!isalpha(c)) return false;
	c = toupper(c);
	if(c>'R') return false;		// so 0-17 inclusive
	c -= 'A';					// 'I'-'A' = 8 so 160° E of 180° so 20° E of the meridian
	longitude = c*20 - 180;		// convert to degrees E (W is negative)

	c = square[2];				// '9'
	if(!isdigit(c)) return false;
	c -= '0';					// '0' - '9' = 0 0-9 are 20° so this is 2 degrees per slot
	longitude += 2*c;
	if(n==6){
		c = square[4];			// 'W'
		if(!isalpha(c)) return false;
		c = toupper(c);
		if(c>'X') return false;	// so 0-23 inclusive
		c -= 'A';				// 'W'-'A'
		longitude += (c/12.0);	// 12ths of a degree
		longitude += (1/24.0);	// move to the centre of the square
	}
	else
		longitude += 1.0;		// move to the centre of the square

	return true;
}

inline double sq(double x){ return x*x; }

double haversine(double lat1, double long1, double lat2, double long2, double radius)
{
	// the classic Haversine formula from our spherical trig class
	// convert to radians
	lat1  *= std::numbers::pi/180.0;
	long1 *= std::numbers::pi/180.0;
	lat2  *= std::numbers::pi/180.0;
	long2 *= std::numbers::pi/180.0;
	double deltaLat  = lat2-lat1;
	double deltaLong = long2-long1;

	double a = sq(std::sin(deltaLat/2)) + std::cos(lat1)*std::cos(lat2)*sq(std::sin(deltaLong/2));
	double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1-a));
	return radius * c;
}

// So I went to test this with the Brighton to Sidney path but somehow the internet, true to form,
// offered me numbers between 9142 and 10576 miles (and one at 572 as there is a Brighton in Queensland)

// I worked VK2PAA and he claimed QF55 which is south of Sidney and Google earth makes it 10602 from
// me to the centre of the square. I make it 10617 and considering that a 'square' is way bigger than
// that I'll accept it as 'good enough' for now.

// The equatorial radius of the earth is 6371km although the polar radius is 6357
// I'll go with 3963 miles. So half way round is 12450 (East of the south of South island NZ).

int mileage(const char* him, const char* me)
{
	double latHim, longHim, latMe, longMe;
	if(!square2spherical(him, latHim, longHim) || !square2spherical(me, latMe, longMe)) return -1;
	return (int)haversine(latHim, longHim, latMe, longMe, 3963);
}
