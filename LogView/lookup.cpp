// lookup.cpp : find a country for a callsign.
//

#include "framework.h"
#include "LogView.h"


// well the definitive article is in man readable at
//	http://www.arrl.org/files/file/DXCC/2022_Current_Deleted.txt
// I copy it to
//	dxcc.txt

const char* szURL		 = "http://www.arrl.org/files/file/DXCC/2022_Current_Deleted.txt";
const char* szLookupFile = "dxcc.txt";

int trapX{};
void trap() { ++trapX; }

bool DXCC::fetchARRL()
{
	if(FileExists(szLookupFile)) return true;
	
	char szCWD[260];
	(void)_getcwd(szCWD, sizeof(szCWD));

	char temp[500];
	sprintf_s(temp, sizeof(temp), "Unable to locate file %s in folder %s\nDownload from internet?", szLookupFile, szCWD);
	int res = MessageBoxA(nullptr, temp, "Callsign to DXCC table", MB_YESNO);
	if(res!=IDYES) return false;

	HRESULT ok = URLDownloadToFile(nullptr, szURL, szLookupFile, 0, nullptr);
	if(ok==S_OK) return true;
	MessageBox(nullptr, "No that failed too...", "Callsign to DXCC table", MB_OK);
	return false;
}

//012345678901234567890123456789012345678901234567890123456789012345678901234567890
//    3B9                 Rodrigues I.                       AF    53    39    207\n

static bool isin(char c, const char* list){ return strchr(list, c)!=nullptr; }

static void strip_trailing(char* p, const char* strip)
{
	while(*p) ++p;							// move to the end of line
	--p;									// back into text
	while(isin(*p, strip)) *p--=0;
}
static void strip_brackets(char* p)			// ie: footnotes
{
	int i, j;
	for(i=0, j=0; p[i]; ++i){
		if(p[i]=='('){
			while(p[i]!=')') ++i;
			continue;
		}
		if(i!=j) p[j] = p[i];
		++j;
	}
	p[j] = 0;
}
static bool isalmum(const char* str)
{
	while(*str)
		if(!isalnum(*str++))
			return false;
	return true;
}
char* _strndup(const char* p, int n)
{
	char temp[40];
	strncpy_s(temp, sizeof(temp), p, n);
	return _strdup(temp);
}

// this is a LOOKUP without all the _strdup()s to compact it so we can copy it

RAWLOOKUP::RAWLOOKUP(RAWLOOKUP* r)		// copy constructor
{
	strcpy_s(prefix,	sizeof(RAWLOOKUP::prefix),		r->prefix);
	strcpy_s(entity,	sizeof(RAWLOOKUP::entity),		r->entity);
	strcpy_s(continent,	sizeof(RAWLOOKUP::continent),	r->continent);
	strcpy_s(itu,		sizeof(RAWLOOKUP::itu),			r->itu);
	strcpy_s(cq,		sizeof(RAWLOOKUP::cq),			r->cq);
	strcpy_s(code,		sizeof(RAWLOOKUP::code),		r->code);
}
void DXCC::postRaw(RAWLOOKUP* raw)
{
	LOOKUP* lu = new LOOKUP;			// make a 'proper' LOOKUP
	lu->prefix		= _strdup(raw->prefix);
	lu->entity		= _strdup(raw->entity);
	lu->continent	= _strdup(raw->continent);
	lu->itu			= _strdup(raw->itu);
	lu->cq			= _strdup(raw->cq);
	lu->code		= _strdup(raw->code);
	lookupTable[raw->prefix] = *lu;
}
void DXCC::processPrefix(RAWLOOKUP* raw)
{
	// Now deal with the more obvious bugs in the table
	if(raw->prefix[0]==0){										// Spratly Is.
		delete raw;
		return;
	}
	if(strcmp(raw->prefix, "7")==0)								// Agalega & St. Brandon Is.
		strcpy_s(raw->prefix, sizeof(RAWLOOKUP::prefix),"3B7");	

	// deal with groups
	if(strchr(raw->prefix, ',')!=nullptr){		// do we have commas?
		char* q{};
		// be careful strtok_s will butcher the value we're working on but we copy it out
		char* p = _strdup(raw->prefix);
		char* slice = strtok_s(p, ",", &q);	// first slice
		while(slice!=nullptr){
			RAWLOOKUP *r1 = new RAWLOOKUP(raw);
			strcpy_s(r1->prefix, sizeof(RAWLOOKUP::prefix), slice);
			processPrefix(r1);					// recursive call in case it has '-' too (deletes r1)
			slice = strtok_s(nullptr, ",", &q);
		}
		delete p;
		delete raw;
		return;
	}
	// now deal with the ranges
	// the problem here is that there aren't any stated rules and you just have to magically infer stuff
	// eg: XA4-XI4 H6-7  XA-XI PP0-PY0F
	if(strncmp(raw->prefix, "UA-", 3)==0)
		trap();
	char* p = strchr(raw->prefix, '-');					// this may be only the first dash...
	if(p!=nullptr){										// we have a dash meaning a sequence
		int n = (int)(p-raw->prefix);					// number of characters before the dash
		int m = (int)strlen(raw->prefix+n+1);			// number of characters after the dash
		// now separate it into before, incrementing, target, after (after may contain another dash)
		char before[30]{}, from[30]{}, to[30]{}, after[30]{};
		if(n==m){										// simplest case A4-A8 meaning A4 A% A6 A7 A8 or A4T-A7T meaning A4T A5T A6T A7T
			strncpy_s(from, sizeof(from), raw->prefix, n);
			strncpy_s(to, sizeof(to), raw->prefix+n+1, n);
		}
		else if(n>m){									// probably AB6-8 for AB6 AB7 AB8
			strncpy_s(before, sizeof(before), raw->prefix, n-m);
			strncpy_s(from, sizeof(from), raw->prefix+n-m, m);
			strncpy_s(to, sizeof(to), raw->prefix+n+1, m);
		}
		else{	// n<m
			strncpy_s(from, sizeof(from), raw->prefix, n);
			strncpy_s(to, sizeof(to), raw->prefix+n+1, n);
			strncpy_s(after, sizeof(after), raw->prefix+2*n+1, m-n);
		}
		// annoyingly it isn't just the last character that increments so first spot the difference
		int diff;
		for(diff=0; from[diff] && from[diff]==to[diff]; ++diff);
		if(from[diff]==0){
error:		MessageBox(nullptr, raw->prefix, "ARGH!!!", MB_OK);
		}
		int safety{};
		while(strcmp(from, to)!=0){
			RAWLOOKUP *r1 = new RAWLOOKUP(raw);
			strcpy_s(r1->prefix, sizeof(RAWLOOKUP::prefix), before);
			strcat_s(r1->prefix, sizeof(RAWLOOKUP::prefix), from);
			strcat_s(r1->prefix, sizeof(RAWLOOKUP::prefix), after);
			processPrefix(r1);					// processes and deletes r1
			from[diff]++;
			if(from[diff]==':') from[diff]='0';	// well I thing that's what 8-0 means
			if(safety++>30) goto error;
		}
		return;
	}
	// if we get this far it is safe to just post it
	postRaw(raw);
	delete raw;
}
bool DXCC::processLookup(const char* p, int lineno)
{
	// break up the line into blocks
	RAWLOOKUP *raw = new RAWLOOKUP;
	strncpy_s(raw->prefix,		sizeof(RAWLOOKUP::prefix),		p+4,  19);
	strncpy_s(raw->entity,		sizeof(RAWLOOKUP::entity),		p+24, 34);
	strncpy_s(raw->continent,	sizeof(RAWLOOKUP::continent),	p+59, 5);
	strncpy_s(raw->itu,			sizeof(RAWLOOKUP::itu),			p+65, 5);
	strncpy_s(raw->cq,			sizeof(RAWLOOKUP::cq),			p+71, 5);
	strncpy_s(raw->code,		sizeof(RAWLOOKUP::code),		p+77, 4);

	// do the easy clean-ups first
	// remove #* from the end of a prefix (see the notes for explanations)
	strip_brackets(raw->prefix);			strip_trailing(raw->prefix, " *#,^");	
	strip_trailing(raw->entity, " ");
	strip_trailing(raw->continent, " ");
	strip_brackets(raw->itu);				strip_trailing(raw->itu, " ");			
	strip_brackets(raw->cq);				strip_trailing(raw->cq, " ");			
	strip_trailing(raw->code, " \n");

	processPrefix(raw);
	return true;		// must return true to continue
}
DXCC::DXCC()
{
	// if it fails we just get no look ups
	if(!fetchARRL()) return;
	
	// open the file
	FILE* fh{};
	int line=0;
	if(fopen_s(&fh, szLookupFile, "r")!=0) return;
	char buffer[200];

	// skip over the heading to get to the real data
	while(fgets(buffer, sizeof(buffer), fh)!=nullptr){
		++line;
		if(strstr(buffer, "________________")!=nullptr) goto skip;
	}
	fclose(fh);			// failed to find data
	return;
skip:
	// there should now be 276 lines and then a blank line
	while(fgets(buffer, sizeof(buffer), fh)!=nullptr
				&& strlen(buffer)>5
				&& processLookup(buffer, ++line));
	
	fclose(fh);


	// pump out a test version
	FILE* fd{};
	_unlink("LookupDump.edc");
	if(fopen_s(&fd, "LookupDump.edc", "w")==0){	// overwrite if needed
		char bx[500];
		for(auto &x : lookupTable){
			LOOKUP &y = x.second;
			sprintf_s(bx, sizeof(bx), "%-22s %-40s %-8s %-8s %-8s %-8s\n", y.prefix, y.entity, y.continent, y.itu, y.cq, y.code);
			fputs(bx, fh);
		}
		fclose(fh);
	}
	else
		ErrorHandler();
}

LOOKUP* DXCC::lookup(const char* call)
{
	char temp[20];
	strcpy_s(temp, sizeof(temp), call);			// copy it so we can chop it up

	// we want the longest match
	for(int i=(int)strlen(call)-1; i>0; --i){	// index of last char
		if(lookupTable.contains(temp))
			return &lookupTable[temp];
		temp[i] = 0;
	}
	return nullptr;
}
