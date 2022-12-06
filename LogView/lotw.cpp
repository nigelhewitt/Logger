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

char lotwFile[MAX_PATH]{};

void LOTW::addArg(const char* name, const char* value)
{
	char temp[100];
	sprintf_s(temp, sizeof(temp), "%c%s=%s", sep, name, value);
	strcat_s(url, sizeof(url), temp); 
	sep = '&';
}
bool LOTW::update()
{
	// make the URL
	strcpy_s(url, sizeof(url),  "https://lotw.arrl.org/lotwuser/lotwreport.adi");
	char login[20], password[20];
	readConfig("setup", "LOTWuser", "", login, sizeof(login));
	readConfig("setup", "LOTWpassword", "", password, sizeof(password));
	if(login[0]==0 || password[0]==0) return false;
	addArg("login", login);
	addArg("password", password);
	addArg("qso_query", "1");
	addArg("qso_startdate", "2022-01-01");
	addArg("qso_starttime", "");
	addArg("qso_qsl", "yes");				// only ask for the QSLed ones

	// make the target file name
	strcpy_s(lotwFile, sizeof(lotwFile), cwd);
	strcat_s(lotwFile, sizeof(lotwFile), "\\lotw.adi");

	// and do the 'read from internet' process
	_unlink(lotwFile);
	HRESULT ok = URLDownloadToFile(nullptr, url, lotwFile, 0, nullptr);
	return ok==S_OK;
}
//-------------------------------------------------------------------------------------------------
// LOTW::load()		get the data and if not present interface with the user
//-------------------------------------------------------------------------------------------------

bool LOTW::load(bool force)
{
	strcpy_s(lotwFile, sizeof(lotwFile), cwd);
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
		if(i!=nullptr)
			lotwTable[i->value] = *e;			// add to the std::unordered_map
		else
			delete e;							// although a log record with no callsign is an error
	}

	delete[] ix;
	return true;
}
