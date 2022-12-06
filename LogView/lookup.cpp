//-------------------------------------------------------------------------------------------------
// lookup.cpp : find the country for a callsign.
//-------------------------------------------------------------------------------------------------

#include "framework.h"

// Well the definitive article is in man readable form at
//		http://www.arrl.org/files/file/DXCC/2022_Current_Deleted.txt
// so in theory I copy it to
//		dxcc.txt
// and just read it
// If only it were that simple

const char* szURL		 = "http://www.arrl.org/files/file/DXCC/2022_Current_Deleted.txt";
const char* szLookupFile = "dxcc.txt";

// A routine to ensure we have the DXCC definitions file. Since it almost never changes
// I rely on the locally cached version until told to do otherwise

bool DXCC::fetchARRL(bool force)
{
	bool exists = FileExists(szLookupFile);
	if(!force && exists) return true;			// we have it already
	
	if(!force){									// computers should not ask silly questions if given orders
		char temp[500];
		sprintf_s(temp, sizeof(temp), "Unable to locate file %s in folder %s\nDownload from internet?", szLookupFile, cwd);

		if(MessageBoxA(nullptr, temp, "Callsign to DXCC country name table", MB_YESNO)!=IDYES)
			return false;
	}
	else if(exists)
		_unlink(szLookupFile);

	if(URLDownloadToFile(nullptr, szURL, szLookupFile, 0, nullptr)==S_OK)
		return true;

	MessageBox(nullptr, "Failed to read DXCC definitions...", "Callsign to DXCC table", MB_OK);
	return false;
}
//-------------------------------------------------------------------------------------------------
// Some tools to make the job simpler
//-------------------------------------------------------------------------------------------------

// is the char in a list
static bool isin(char c, const char* list){ return strchr(list, c)!=nullptr; }

// remove trailing characters if they are in the list
static void strip_trailing(char* p, const char* list)
{
	while(*p) ++p;							// move to the end of line
	--p;									// back into text
	while(isin(*p, list)) *p--=0;
}
// remove "(45)" style footnote numbers
static void strip_brackets(char* p)
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
// is a whole string alphanumeric?
static bool isalmum(const char* str)
{
	while(*str)
		if(!isalnum(*str++))
			return false;
	return true;
}
// like _strdup() but only for n characters
char* _strndup(const char* p, int n)
{
	char temp[100];
	strncpy_s(temp, sizeof(temp), p, n);
	return _strdup(temp);
}
//-------------------------------------------------------------------------------------------------
// RAWLOKUP is a LOOKUP without all the _strdup()s to compact it so we can copy it about easily
//-------------------------------------------------------------------------------------------------

RAWLOOKUP::RAWLOOKUP(RAWLOOKUP* r)		// copy constructor
{
	strcpy_s(prefix,	sizeof(RAWLOOKUP::prefix),		r->prefix);
	strcpy_s(entity,	sizeof(RAWLOOKUP::entity),		r->entity);
	strcpy_s(continent,	sizeof(RAWLOOKUP::continent),	r->continent);
	strcpy_s(itu,		sizeof(RAWLOOKUP::itu),			r->itu);
	strcpy_s(cq,		sizeof(RAWLOOKUP::cq),			r->cq);
	strcpy_s(code,		sizeof(RAWLOOKUP::code),		r->code);
}
// take a RAWLOOKUP and convert it to a LOOKUP and put it in the lookupTable
void DXCC::postRaw(RAWLOOKUP* raw)
{
	LOOKUP* lu = new LOOKUP;			// make a 'proper' LOOKUP
	lu->entity		= _strdup(raw->entity);
	lu->continent	= _strdup(raw->continent);
	lu->itu			= _strdup(raw->itu);
	lu->cq			= _strdup(raw->cq);
	lu->code		= _strdup(raw->code);
	lookupTable[raw->prefix] = *lu;
}
// Routine to make some sense of the prefixes given
// you get simple single entities
//		A5
// you get alternatives:
//		G,GX,M
// you get footnotes
//		(17)
// you get qualifying symbols
//		#*
// and you get ranges
//		9Q-9T			meaning, I assume, 9Q,9R,9S,9T
// and you get them all wonderfully hodge-podged in together
//		YN,H6-7,HT#*
// and then there's Russia
//		see later, I changed the Russian strings for something that actually works

// BEWARE: this function is self-recursive!!!
void DXCC::processPrefix(RAWLOOKUP* raw)
{
	// Now deal with the more obvious bugs in the table
	if(raw->prefix[0]==0){											// Spratly Island has no callsign prefix
		delete raw;
		return;
	}
	if(strcmp(raw->prefix, "7")==0)									// Agalega & St. Brandon Is.
		strcpy_s(raw->prefix, sizeof(RAWLOOKUP::prefix), "3B7");	// I think it's a type of , for -

	// don't get me started on the Russian entries...
	if(strcmp(raw->prefix, "UA-UI1-7,RA-RZ")==0)					// European Russia
		strcpy_s(raw->prefix, sizeof(RAWLOOKUP::prefix), "UA-UI1-7,RA-RZ1-7,R1-7");
	if(strcmp(raw->prefix, "UA-UI8-0,RA-RZ")==0)					// Asiatic Russia
		strcpy_s(raw->prefix, sizeof(RAWLOOKUP::prefix), "UA-UI8-0,RA-RZ8-0,R8-0");

	// deal with groups
	if(strchr(raw->prefix, ',')!=nullptr){		// do we have commas?
		char* q{};
		// be careful strtok_s will butcher the value we're working on so we copy it out
		char* p = _strdup(raw->prefix);
		char* slice = strtok_s(p, ",", &q);		// first slice
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
	// eg: XA4-XI4  H6-7  XA-XI  PP0-PY0F
	char* p = strchr(raw->prefix, '-');					// this may be only the first dash...
	if(p!=nullptr){										// we have a dash meaning a sequence
		int n = (int)(p-raw->prefix);					// number of characters before the dash
		int m = (int)strlen(raw->prefix+n+1);			// number of characters after the dash
		// now separate it into before, incrementing, target, after (after may contain another dash)
		char before[30]{}, from[30]{}, to[30]{}, after[30]{};
		if(n==m){										// simplest case A4-A8 meaning A4 A5 A6 A7 A8 or A4T-A7T meaning A4T A5T A6T A7T
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
error:		MessageBox(nullptr, raw->prefix, "ARGH!!!", MB_OK);		// simplistic debug
		}
		int safety{};					// don't loop forever little program, ask for help
		// I have a problem with Russian Asiatic call signs using the range 8-0 which I assume means 8,9,0
		// I will just have to frig it out with a special case for n-0
		bool frig{};
		if(to[diff]=='0')
			frig = true;
		if(frig) to[diff] = ':';		// aka '9'+1

		while(strcmp(from, to)<=0){										// normal inclusive operation
			RAWLOOKUP *r1 = new RAWLOOKUP(raw);
			strcpy_s(r1->prefix, sizeof(RAWLOOKUP::prefix), before);
			char frigC = from[diff];									// frig starts
			if(frig && from[diff]==':') from[diff] = '0';				//
			strcat_s(r1->prefix, sizeof(RAWLOOKUP::prefix), from);		//
			from[diff] = frigC;											// frig ends
			strcat_s(r1->prefix, sizeof(RAWLOOKUP::prefix), after);
			processPrefix(r1);											// processes and deletes r1
			from[diff]++;
			if(safety++>30) goto error;
		}
		return;
	}
	// if we get this far it is safe to just post it
	postRaw(raw);
	delete raw;
}
// the lines look like this
//012345678901234567890123456789012345678901234567890123456789012345678901234567890
//    3B9                 Rodrigues I.                       AF    53    39    207\n
bool DXCC::processLookup(const char* p, int lineno)
{
	// break up the line into blocks
	// I AM ASSUMING STRICT COLUMNS AND NO TABS (good at the moment)
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
DXCC::DXCC(bool force)
{
	// if it fails we just get no look ups
	if(!fetchARRL(force)) return;
	
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
				&& strlen(buffer)>5							// yes it's just arbitrary...
				&& processLookup(buffer, ++line));
	
	fclose(fh);

#ifdef _DEBUG
	// pump out a test version
	FILE* fd{};
	_unlink("LookupDump.edc");
	if(fopen_s(&fd, "LookupDump.edc", "w")==0){	// overwrite if needed
		char bx[500];
		for(auto &x : lookupTable){
			std::string call = x.first;
			LOOKUP &y = x.second;
			sprintf_s(bx, sizeof(bx), "%-22s %-40s %-8s %-8s %-8s %-8s\n", call.c_str(), y.entity, y.continent, y.itu, y.cq, y.code);
			fputs(bx, fh);
		}
		fclose(fh);
	}
#endif
}
//-------------------------------------------------------------------------------------------------
// DFXCC::lookup		find the LOOKUP for a callsign
//-------------------------------------------------------------------------------------------------

LOOKUP* DXCC::lookup(const char* call)
{
	char temp[20];
	strcpy_s(temp, sizeof(temp), call);			// copy it so we can chop it up

	// we want the longest match as the table is a bit dodgy in its treatment of overlaps
	for(int i=(int)strlen(call); i>0; --i){	// index of terminating null
		if(lookupTable.contains(temp))
			return &lookupTable[temp];
		temp[i-1] = 0;
	}
	return nullptr;
}
