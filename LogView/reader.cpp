// reader.cpp : read/write operations on a log file.
//

#include "framework.h"

// read/write an ADIF file

// an example header is

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


//=================================================================================================
// read/write  a single item
//=================================================================================================

ITEM* ITEM::read(char* &in)
{
	while(*in && *in!='<') ++in;				// skip spaces and comments

	if(*in!='<') return nullptr;				// get the opening '<'
	++in;

	char c, name[50], type[50]{};				// read the name
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
	while((c=*in)!=0 && c!='>' && c!=':'){
		if(!isdigit(c)) return nullptr;
		n *= 10;
		n += c-'0';
		++in;
	}
	if(n>1000) return nullptr;					// zero is legal

	if(c==':'){									// if we have a type...
		++in;
		i = 0;
		while((c=*in)!=0 && c!='>' && i+1<(int)sizeof(type)){	// stupid cast to remove stupid warning
			type[i++] = c;
			++in;
		}
	}
	++in;			// step over the '>'

	ITEM* res = new ITEM;
	res->name = _strdup(name);
	if(type[0])
		res->type = _strdup(type);
	if(n){
		res->value = new char[n+1];
		i = 0;
		while((c=*in)!=0 && i<n){
			res->value[i++] = c;
			++in;
		}
		res->value[i] = 0;
	}
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
		if(_stricmp(i.name, name)==0)
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
	sprintf_s(temp, sizeof(temp), "ADIF Export from Nigel's Log\n"
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
	// slightly messy as the first record must have the DXCC and CODE values to ensure the headers go in the right place
	bool first = true;

	for(ENTRY& e : entries){					// for all entries  (BEWARE: use reference form or we get a copy and the update is lost)
		ITEM* call = e.find("CALL");			// find the callsign item
		if(call && call->value){
			LOOKUP* lu = dxcc->lookup(call->value);
			if(lu && lu->entity){
				ITEM *cx  = new ITEM("DXCC", lu->entity);
				e.items.insert(e.items.begin()+1, *cx);

				ITEM *cy  = new ITEM("CODE", lu->code);
				e.items.insert(e.items.begin()+2, *cy);
				first = false;								// no need for fake records
			}
		}
		if(first){
			first = false;
			e.items.insert(e.items.begin()+1, *(new ITEM("DXCC")));
			e.items.insert(e.items.begin()+2, *(new ITEM("CODE")));
		}
	}
	// patch in the LoTW QSL records
	first = true;
	for(ENTRY& e : entries){
		ITEM* call = e.find("CALL");			// find the callsign item
		const char* vx = "";
		if(call && call->value){
			char temp[30];								// provision for multiple calls logged
			strcpy_s(temp, sizeof(temp), call->value);
			while(lotw->lotwTable.contains(temp)){
				ENTRY* rep = &lotw->lotwTable[temp];
				if(matchReport(e, rep, ST_LOTW)){
					vx = "QSL";
					break;
				}
				increment(temp, sizeof(temp));			// try the next record for this callsign
			}
			ITEM* y = new ITEM("LOTW", vx);
			e.items.insert(e.items.begin()+3, *y);
			first = false;
		}
		if(first){
			first = false;
			e.items.insert(e.items.begin()+3, *(new ITEM("LOTW")));
		}
	}

	// patch in the EQSL QSL records
	first = true;
	for(ENTRY& e : entries){
		ITEM* call = e.find("CALL");			// find the callsign item
		const char* vx = "";
		if(call && call->value){
			char temp[30];								// provision for multiple calls logged
			strcpy_s(temp, sizeof(temp), call->value);
			while(eqsl->eqslTable.contains(temp)){
				ENTRY* rep = &eqsl->eqslTable[temp];
				if(matchReport(e, rep, ST_EQSL)){
					vx = "QSL";						// I really want a tick but that means using wide characters
					break;
				}
				increment(temp, sizeof(temp));
			}
			ITEM* y = new ITEM("EQSL", vx);
			e.items.insert(e.items.begin()+4, *y);
			first = false;
		}
		if(first){
			first = false;
			e.items.insert(e.items.begin()+4, *(new ITEM("EQSL")));
		}
	}

	// patch in the mileage
	first = true;
	for(ENTRY& e : entries){
		ITEM* square = e.find("GRIDSQUARE");			// find their square
		if(square && square->value){
			ITEM* me = e.find("MY_GRIDSQUARE");
			if(me && me->value){
				char temp[20];
				sprintf_s(temp, sizeof(temp), "%d", mileage(square->value, me->value));
				ITEM* y = new ITEM("Distance (miles)", temp);
				e.items.insert(e.items.begin()+5, *y);
				first = false;
			}
		}
		if(first){
			first = false;
			e.items.insert(e.items.begin()+4, *(new ITEM("Distance (miles)")));
		}
	}

	// now prepare the list of column headers
	titles.clear();
	for(ENTRY e : entries)						// for all entries
		for(ITEM &i : e.items){					// for all item in an entry
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
//-------------------------------------------------------------------------------------------------
// matchReport()
//-------------------------------------------------------------------------------------------------

// given two entries are they a match?
// this gets called for the same call signs but what if there and contacts on several bands?
// Date and time are a horror. One man's 20221231235959 is another man's 20230101000001
// fortunately the items we want have standard names

inline long iabs(long a){ if(a<0) return -a; return a; }

enum ST { ST_LOTW, ST_EQSL };

bool ADIF::matchReport(ENTRY& me, ENTRY* them, ST st)
{
	const char* qslReceived{};
	switch(st){
	case ST_LOTW:
		qslReceived = "QSL_RCVD";
		break;
	case ST_EQSL:
		qslReceived = "QSL_SENT";
		break;
	}
	ITEM* i1;
	ITEM* i2 = them->find(qslReceived);									// QSL received?
	if(i2==nullptr || _stricmp(i2->value, "Y")!=0) return false;		// it is only my record

	i1 = me.find("BAND");
	i2 = them->find("BAND");
	if(i1==nullptr || i2==nullptr || _stricmp(i1->value, i2->value)!=0) return false;

	i1 = me.find("MODE");
	i2 = them->find("MODE");
	if(i1==nullptr || i2==nullptr || _stricmp(i1->value, i2->value)!=0) return false;

	// now do the date and time
	char d1[15], d2[15];
	i1 = me.find("QSO_DATE");		if(i1==nullptr) return false;		strcpy_s(d1, sizeof(d1), i1->value);
	i2 = them->find("QSO_DATE");	if(i2==nullptr) return false;		strcpy_s(d2, sizeof(d2), i2->value);
	i1 = me.find("TIME_ON");		if(i1==nullptr) return false;		strcat_s(d1, sizeof(d1), i1->value);
	i2 = them->find("TIME_ON");		if(i2==nullptr) return false;		strcat_s(d2, sizeof(d2), i2->value);
	time_t t1 = unpackTime(d1);
	time_t t2 = unpackTime(d2);

	return  iabs((long)t1-(long)t2) < 30;			// let's give it 30 seconds
}

//=================================================================================================
// sorting the vector
//=================================================================================================

bool ADIF::compare(const ENTRY &e1, const ENTRY &e2)
{
	// returns true if the first argument is less than (before) the second
	ITEM* i1 = ((ENTRY&)e1).find(sortCol);
	ITEM* i2 = ((ENTRY&)e2).find(sortCol);
	if(i1==nullptr || i1->value==nullptr) return false;						// no value goes at the end of the file
	if(i2==nullptr || i2->value==nullptr) return true;
	int res = strcmp(i1->value, i2->value);
	if(reverse) return res>0;
	return res<0;
}
bool ADIF::sort(LISTVIEWCHILD* lv, const char* colName, bool rev)
{
	sortCol = _strdup(colName);
	reverse = rev;
	// I shall cheat and use a lambda to inject a third value into a function that strictly only takes two.
	std::sort(entries.begin(), entries.end(), [lv](const ENTRY& e1, const ENTRY& e2){ return lv->logbook->compare(e1, e2); });
	delete sortCol;
	sortCol = 0;
	return true;
}
