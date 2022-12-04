// reader.cpp : read/write operations on a log file.
//

#include "framework.h"
#include "LogView.h"

// read/write an ADIF file

// the header is

//	ADIF Export from ADIFMaster v[3.4]
//	http://www.dxshell.com
//	Copyright (C) 2005 - 2022 UU0JC, DXShell.com
//	File generated on 28 Nov, 2022 at 18:38
//	<ADIF_VER:5>3.1.2
//	<PROGRAMID:10>ADIFMaster
//	<PROGRAMVERSION:3>3.4
//	<EOH>

// a line of data is
//	<CALL:6>SQ7HJG <GRIDSQUARE:4>JO91 <MODE:3>FT8 <RST_SENT:3>-10 <RST_RCVD:3>-04 <QSO_DATE:8>20221121 <TIME_ON:6>132730 <QSO_DATE_OFF:8>20221121
//	     <TIME_OFF:6>132954 <BAND:3>20m <FREQ:9>14.076004 <STATION_CALLSIGN:5>G8JFT <MY_GRIDSQUARE:6>IO90WT <EOR>

// read a file into an array of characters
static char* LoadFile(const char* fname)
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

//=================================================================================================
// read/write  a single item
//=================================================================================================

ITEM* ITEM::read(char* &in)
{
	char c;
	while((c=*in)!=0 && isspace(c)) ++in;		// skip leading spaces
	if(c!='<') return nullptr;					// get the opening '<'
	++in;

	char name[50];								// read the name
	int i{0};
	while((c=*in)!=0 && c!=':' && c!='>' && i+1<(int)sizeof(name)){	// stupid cast to remove stupid warning
		name[i++] = c;
		++in;
	}
	if(i==0) return nullptr;
	name[i] = 0;

	if(*in=='>'){								// if there is no value return just the name
		++in;
		ITEM* res = new ITEM;
		res->name = _strdup(name);
		return res;
	}

	if(*in!=':') return nullptr;
	++in;

	int n=0;									// read the character count
	while((c=*in)!=0 && c!='>'){
		if(!isdigit(c)) return nullptr;
		n *= 10;
		n += c-'0';
		++in;
	}
	if(n==0 || n>1000) return nullptr;
	++in;

	ITEM* res = new ITEM;
	res->name = _strdup(name);
	res->value = new char[n+1];
	i = 0;
	while((c=*in)!=0 && i<n){
		res->value[i++] = c;
		++in;
	}
	res->value[i] = 0;
	return res;
}
static bool icopy(char* &out, const char* x)
{
	for(int i=0; x[i]; *out++=x[i++]);
	return true;
}
size_t ITEM::length()
{
	size_t n = strlen(name)+3;
	if(value){
		char temp[10];
		size_t m = strlen(value);
		sprintf_s(temp, sizeof(temp), "%d", (int)m);
		n += strlen(temp)+m+1;
	}
	return n;
}
bool ITEM::write(char* &out)
{
	char temp[200];
	if(value==nullptr)
		sprintf_s(temp, sizeof(temp), "<%s> ", name);
	else {
		size_t n = strlen(value);
		sprintf_s(temp, sizeof(temp), "<%s:%d>%s ", name, (int)n, value);
	}
	return icopy(out, temp);
}
size_t ITEM::length(const char* name, const char* value)
{
	if(value==nullptr) return strlen(name)+3;
	size_t n = strlen(value);
	char temp[10];
	sprintf_s(temp, sizeof(temp), "%d", (int)n);
	return strlen(name)+strlen(temp)+n+4;
}
bool ITEM::write(char* &out, const char* name, const char* value, bool lf)
{
	char temp[200];
	if(value==nullptr)
		sprintf_s(temp, sizeof(temp), "<%s>\n", name);
	else {
		int n = (int)strlen(value);
		char eor = lf ? '\n' : ' ';
		sprintf_s(temp, sizeof(temp), "<%s:%d>%s%c", name, n, value, eor);
	}
	return icopy(out, temp);
}

//=================================================================================================
// read/write multiple items to make a log record
//=================================================================================================

ENTRY* ENTRY::read(char* &in)
{
	ENTRY *e = new ENTRY;
	while(true){
		ITEM *i = ITEM::read(in);
		if(i==nullptr){
			delete e;
			return nullptr;
		}
		if(i->isEOR()){
			delete i;
			return e;
		}
		e->items.push_back(*i);
	}
}
bool ENTRY::write(char* &out)
{
	for(ITEM i : items){
		if(!i.write(out))
			return false;
	}
	return ITEM::write(out, "EOR");
}
ITEM* ENTRY::find(const char* name)
{
	for(ITEM& i : items)
		if(strcmp(i.name, name)==0)
			return &i;
	return nullptr;
}

//=================================================================================================
// now read/write the full file with headers and some entries
//=================================================================================================

void ADIF::addHeader()
{
	char temp[100], date[20];
	time_t t  = time(0);
	tm now{};
	localtime_s(&now, &t);
	strftime(date, sizeof(date), "%d %b, 20%C at %H:%M", &now);
	sprintf_s(temp, sizeof(temp), "ADIF Export from ADIFMaster v[3.4]\n"
								  "http://www.dxshell.com\n"
								  "Copyright (C) 2005 - 2022 UU0JC, DXShell.com\n"
								  "File generated on %s\n", date);
	preamble = _strdup(temp);
	headers.push_back(ITEM("ADIF_VER", "3.1.2"));
	headers.push_back(ITEM("PROGRAMID", "NigSoft Logger"));
	headers.push_back(ITEM("PROGRAMVERSION", "3.4"));
}
bool ADIF::read(char* &in)
{
	char bx[1000];
	int i;
	for(i=0; i<sizeof(bx) && *in!='<'; bx[i++] = *in++);
	if(i==sizeof(bx)){ return false; }
	bx[i] = 0;
	delete preamble;
	preamble = _strdup(bx);

	while(true){
		ITEM *i = ITEM::read(in);
		if(i==nullptr) return false;
		if(i->isEOH()){
			delete i;
			break;
		}
		headers.push_back(*i);
		i = nullptr;
	}

	while(true){
		ENTRY *e = ENTRY::read(in);
		if(e==nullptr) break;
		entries.push_back(*e);
	}

	// now patch in a country value id we have a match in out table
	for(ENTRY& e : entries){					// for all entries  (BEWARE: use reference form or we get a copy and the update is lost)
		ITEM* call = e.find("CALL");			// find the callsign item
		if(call && call->value){
			LOOKUP* lu = dxcc->lookup(call->value);
			if(lu && lu->entity){
				ITEM *cx  = new ITEM;
				cx->name  = _strdup("DXCC");
				cx->value = _strdup(lu->entity);
				e.items.insert(e.items.begin()+1, *cx);

				ITEM *cy  = new ITEM;
				cy->name  = _strdup("CODE");
				cy->value = _strdup(lu->code);
				e.items.insert(e.items.begin()+2, *cy);
			}
		}
	}

	// now prepare the list of column headers
	titles.clear();
	for(ENTRY e : entries)						// for all entries
		for(ITEM i : e.items){					// for all item in an entry
			int n = i.value!=nullptr ? (int)strlen(i.value) : 0;	// length of the value
			// first check for new column names
			for(COL& c : titles)				// check the list of titles (use & so we get the real thing not a copy)
				if(_stricmp(i.name, c.col)==0){	// we have seen this one before?
					if(c.maxW<n) c.maxW=n;		// yes, so check its width
					goto found;					// already exists
				}
			// not found so generate a new COL
			COL* cc; cc= new COL;
			cc->col  = _strdup(i.name);
			int cw; cw = (int)strlen(i.name);
			if(cw>n) n=cw;
			cc->maxW = n;
			titles.push_back(*cc);
	found:	;
		}


	return true;
}
bool ADIF::read(const char* filename)
{
	char* data = LoadFile(filename);
	if(data==nullptr) return false;
	char* p=data;
	bool ok = read(p);
	delete[] data;
	return ok;
}
bool ADIF::write(char* &out)
{
	return false;
}
bool ADIF::write(const char* filename)
{
	return false;
}
//=================================================================================================
// sorting the vector
//=================================================================================================

bool ADIF::cmp(const ENTRY &e1, const ENTRY &e2)
{
	return logbook->compare(e1, e2);
}
bool ADIF::compare(const ENTRY &e1, const ENTRY &e2)
{
	// returns true if the first argument is less than (before) the second
	ITEM* i1 = ((ENTRY&)e1).find(sortCol);
	ITEM* i2 = ((ENTRY&)e2).find(sortCol);
	if(i1==nullptr) return false;						// no value goes at the end of the file
	if(i2==nullptr) return true;
	int res = strcmp(i1->value, i2->value);
	if(reverse) return res>0;
	return res<0;
}
bool ADIF::sort(const char* colName, bool rev)
{
	sortCol = _strdup(colName);
	reverse = rev;
	std::sort(entries.begin(), entries.end(), ADIF::cmp);
	delete sortCol;
	sortCol = 0;
	return true;
}
