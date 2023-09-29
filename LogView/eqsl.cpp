//-------------------------------------------------------------------------------------------------
// eqsl.cpp : read the log for 'confirmed'
//-------------------------------------------------------------------------------------------------

#include "framework.h"

// https://www.eQSL.cc/qslcard/DownloadInBox.cfm?UserName=G8JFT&Password=Born76Free&RcvdSince=20050201

//-------------------------------------------------------------------------------------------------
// update()  download the data from the eqsl website
//-------------------------------------------------------------------------------------------------

bool EQSL::update()
{
	char url[400], pageFile[MAX_PATH], user[30], pass[30];

	readConfig("setup", "EQSLuser", "", user, sizeof(user));
	if(user[0]==0) return false;
	readConfig("setup", "EQSLpassword", "", pass, sizeof(pass));

	sprintf_s(url, sizeof(url), "https://www.eQSL.cc/qslcard/DownloadInBox.cfm?UserName=%s&Password=%s&RcvdSince=20050201", user, pass);

	strcpy_s(pageFile, sizeof(pageFile), dataFolder);
	strcat_s(pageFile, sizeof(pageFile), "\\eQSLpage.html");

	// slightly convoluted procedure: we supply the requested login and details to get a page
	// however that is a man-readable and we have to spot the link.
	_unlink(pageFile);
	if(URLDownloadToFile(nullptr, url, pageFile, 0, nullptr)!=S_OK)
		return false;

	char *page = LoadFile(pageFile);
	if(page==nullptr) return false;
#ifndef _DEBUG
	_unlink(pageFile);
#endif
	char *p = strstr(page, "Click one of the following to download");
	if(p==nullptr) return false;

	char *p2 = strstr(p, "../downloadedfiles/");
	if(p2==nullptr) return false;
	char url2[200] = "https://www.eqsl.cc/downloadedfiles/";
	p2 += 19;
	int i=(int)strlen(url2);
	while(*p2 && *p2!='"') url2[i++]=*p2++;

	return URLDownloadToFile(nullptr, url2, eqslFile, 0, nullptr)==S_OK;
}
//-------------------------------------------------------------------------------------------------
// EQSL::load()		get the data and if not present interface with the user
//-------------------------------------------------------------------------------------------------

bool EQSL::load(bool force)
{
	strcpy_s(eqslFile, sizeof(eqslFile), dataFolder);
	strcat_s(eqslFile, sizeof(eqslFile), "\\eQSL.adi");

	if(!FileExists(eqslFile) || force){
		if(force || MessageBox(nullptr, "No eQSL records\nDownload from web?", "", MB_YESNO)==IDYES){
			if(!update()){
				MessageBox(nullptr, "Nope, reading eQSL from the web didn't work", "", MB_YESNO);
				return false;
			}
		}
		else
			return false;
	}
	return read(eqslFile);
}
//-------------------------------------------------------------------------------------------------
//  EQSL::read() process the data from the local file
//-------------------------------------------------------------------------------------------------

bool EQSL::read(const char* fname)
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
			eqslTable[i->value] = *e;			// add to the std::unordered_map
		else
			delete e;							// although a log record with no callsign is an error
	}

	delete[] ix;
	return true;
}
