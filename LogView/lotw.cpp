//-------------------------------------------------------------------------------------------------
// lotw.cpp : read the log for 'confirmed'
//-------------------------------------------------------------------------------------------------
//
// This is another ADIF file but just reading it into a vector will be slow when it
// come to matching things up so it gets it's own loader using parts of the ADIF
// system but loading into an unordered_map<string, ENTRY> for lookup speed

#include "framework.h"

// the definitive document:
//		https://lotw.arrl.org/lotw-help/developer-query-qsos-qsls/


void LOTW::addArg(const char* name, const char* value)
{
	char temp[100];
	sprintf_s(temp, sizeof(temp), "%c%s=%s", sep, name, value);
	strcat_s(url, sizeof(url), temp); 
	sep = '&';
}
//-------------------------------------------------------------------------------------------------
// update()  download the data from the lotw website
//-------------------------------------------------------------------------------------------------

bool LOTW::update()
{
	// make the URL
	strcpy_s(url, sizeof(url),  "https://lotw.arrl.org/lotwuser/lotwreport.adi");
	char login[20], password[20];
	readConfig("setup", "LOTWuser", "", login, sizeof(login));
	if(login[0]==0) return false;
	readConfig("setup", "LOTWpassword", "", password, sizeof(password));
	if(login[0]==0 || password[0]==0) return false;
	addArg("login", login);
	addArg("password", password);
	addArg("qso_query", "1");
	addArg("qso_startdate", "2022-01-01");
	addArg("qso_starttime", "");
	addArg("qso_qsl", "yes");				// only ask for the QSLed ones

	// make the target file name
	strcpy_s(lotwFile, sizeof(lotwFile), dataFolder);
	strcat_s(lotwFile, sizeof(lotwFile), "\\lotw.adi");

	// and do the 'read from internet' process
	_unlink(lotwFile);
	return URLDownloadToFile(nullptr, url, lotwFile, 0, nullptr)==S_OK;
}
//-------------------------------------------------------------------------------------------------
// LOTW::load()		get the data and if not present interface with the user
//-------------------------------------------------------------------------------------------------

bool LOTW::load(bool force)
{
	strcpy_s(lotwFile, sizeof(lotwFile), dataFolder);
	strcat_s(lotwFile, sizeof(lotwFile), "\\lotw.adi");

	if(!FileExists(lotwFile) || force){
		if(force || MessageBox(nullptr, "No LOTW records\nDownload from ARRL?", "", MB_YESNO)==IDYES){
			if(!update()){
				MessageBox(nullptr, "Nope, reading LoTW from the ARRL didn't work", "", MB_YESNO);
				return false;
			}
		}
		else
			return false;
	}
	return read(lotwFile);
}
//-------------------------------------------------------------------------------------------------
//  LOTW::read() process the data from the local file
//-------------------------------------------------------------------------------------------------

// there is a problem with hash tables and multiple entries - they just overwrite
// but I don't want to loose the fast of hash so...
void increment(char* c, int cb)
{
	char* p = strchr(c, '#');		// have we been here already?
	if(p==nullptr)
		strcat_s(c, cb, "#0");		// no
	else{
		*p = 0;						// yes so squash the #
		int n = atoi(p+1);			// read the number
		char number[10]="#";
		_itoa_s(n+1, number+1, sizeof(number)-1, 10);
		strcat_s(c, cb, number);
	}
}

bool LOTW::read(const char* fname)
{
	char* in =  LoadFile(fname);
	if(in==nullptr) return false;
	char *ix = in;								// save to do a clean delete
	while(*in && *in!='<') ++in;				// skip the headers 

	while(true){								// now skip the header entries
		ITEM *i = ITEM::read(in);
		if(i==nullptr) return false;
		if(i->isEOH()){
			delete i;
			break;
		}
		i = nullptr;
	}

	// now read the entries
	while(true){
		ENTRY *e = ENTRY::read(in);				// see reader.cpp
		if(e==nullptr) break;
		ITEM *i = e->find("CALL");
		if(i!=nullptr){
			char call[20];							// extract the callsign
			strcpy_s(call, sizeof(call), i->value);
			while(lotwTable.contains(call))		// is this a duplicate (different band, different time et al)
				increment(call, sizeof(call));		// append #number
			lotwTable[call] = *e;					// add to the std::unordered_map
		}
		else
			delete e;							// although a log record with no callsign is an error
	}

	delete[] ix;
	return true;
}
