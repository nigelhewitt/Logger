//-------------------------------------------------------------------------------------------------
// ListViewChild.cpp : Defines the entry point for the application.
//-------------------------------------------------------------------------------------------------

#include "framework.h"


bool LISTVIEWCHILD::once{};
LISTVIEWCHILD* LISTVIEWCHILD::underCreation{};		// What is this? Well we need to direct the WndProc() calls into the class
													// member WndProc so it has access to the member data so as soon as we get
													// a windows handle we set it in the Window Data.
													// However the Client window sends a couple of messages as it start up and
													// before it returns. Hence we need a bodge round for those first few moments.
													// As it's a horrible race hazard I wrapped it in a CRITICAL_SECTION so it will
													// survive multi-threading.

void LISTVIEWCHILD::RegisterClass()
{
	if(!once){
		// Register the Window Class
		WNDCLASSEX			wcex{};
		wcex.cbSize			= sizeof(WNDCLASSEX);
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= LISTVIEWCHILD::WndProcStatic;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= sizeof(void*);			// put the 'this' pointer here
		wcex.hInstance		= hInstance;
		wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDC_LogView));
		wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName	= nullptr;
		wcex.lpszClassName	= "LogView";
		wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LogView));

		RegisterClassEx(&wcex);
		once = true;
	}
}

//=====================================================================================================
// add a new child Window
//=====================================================================================================

HWND LISTVIEWCHILD::AddChild(const char* title, void* data)
{
	if(IsIconic(hFrame)){
		ShowWindow(hFrame, SW_RESTORE);
		SetForegroundWindow(hFrame);
		UpdateWindow(hFrame);
	}

	MDICREATESTRUCT mcs{};					// do not use CreateWindow() for MDI children
	mcs.szClass	= "LogView";
	mcs.szTitle	= title;
	mcs.hOwner	= hInstance;
	mcs.x		= CW_USEDEFAULT;
	mcs.y		= CW_USEDEFAULT;
	mcs.cx		= CW_USEDEFAULT;
	mcs.cy		= CW_USEDEFAULT;
	mcs.style	= 0;
	mcs.lParam	= (LPARAM)data;

	EnterCriticalSection(&CriticalSection);
	underCreation = this;

	HWND hChild = reinterpret_cast<HWND>(SendMessage(hClient, WM_MDICREATE, 0, reinterpret_cast<LPARAM>(&mcs)));
	if(hChild == nullptr){
		// display some error message
		error();
		exit(99);
	}
	SetWindowLongPtr(hChild, 0, (LONG_PTR)this);

	LeaveCriticalSection(&CriticalSection);

	SetFocus(hChild);

	return hChild;
}

//-------------------------------------------------------------------------------------------------
// make the ListView track the size of the enclosing window
// used in create and WM_SIZE
//-------------------------------------------------------------------------------------------------

void LISTVIEWCHILD::ResizeListView(HWND hwndListView, HWND hwndParent)
{
	RECT rc;
	GetClientRect(hwndParent, &rc);
	MoveWindow(hwndListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}
//-------------------------------------------------------------------------------------------------
// create the ListView control
//-------------------------------------------------------------------------------------------------

HWND LISTVIEWCHILD::CreateListView(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD dwStyle =   WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT |  LVS_OWNERDATA;

	HWND hwndListView = CreateWindowEx(
				WS_EX_CLIENTEDGE,		// ex style
				WC_LISTVIEW,			// class name - defined in commctrl.h
				"",						// dummy text
				dwStyle,				// style
				0,						// x position
				0,						// y position
				0,						// width
				0,						// height
				hwndParent,				// parent
				(HMENU)ID_LISTVIEW,		// ID
				hInstance,				// instance
				nullptr);				// no extra data

	if(!hwndListView)
		return nullptr;

	ResizeListView(hwndListView, hwndParent);

	// set the image lists
	HIMAGELIST himlSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
	HIMAGELIST himlLarge = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK, 1, 0);

	if(himlSmall && himlLarge){
		HICON hIcon;

		// set up the small image list
		hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_Station), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		ImageList_AddIcon(himlSmall, hIcon);

		//set up the large image list
		hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_Station));
		ImageList_AddIcon(himlLarge, hIcon);

		ListView_SetImageList(hwndListView, himlSmall, LVSIL_SMALL);
		ListView_SetImageList(hwndListView, himlLarge, LVSIL_NORMAL);
	}

	return hwndListView;
}
void LISTVIEWCHILD::PositionHeader(HWND hwndListView)
{
	HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
	DWORD dwStyle	 = GetWindowLong(hwndListView, GWL_STYLE);

	// To ensure that the first item will be visible, create the control without 
	// the LVS_NOSCROLL style and then add it here
	dwStyle |= LVS_NOSCROLL;
	SetWindowLong(hwndListView, GWL_STYLE, dwStyle);

	// only do this if we are in report view and were able to get the header hWnd
	if(((dwStyle & LVS_TYPEMASK) == LVS_REPORT) && hwndHeader){
		RECT		rc;
		HD_LAYOUT	hdLayout;
		WINDOWPOS	wpos;

		GetClientRect(hwndListView, &rc);
		hdLayout.prc = &rc;
		hdLayout.pwpos = &wpos;

		Header_Layout(hwndHeader, &hdLayout);

		SetWindowPos(hwndHeader, 
						wpos.hwndInsertAfter, 
						wpos.x, 
						wpos.y,
						wpos.cx, 
						wpos.cy, 
						wpos.flags | SWP_SHOWWINDOW);

		ListView_EnsureVisible(hwndListView, 0, FALSE);
	}
}
// since we're using a callback to get the data we just set the number of items
void LISTVIEWCHILD::InsertListViewItems(HWND hwndListView)
{
	// empty the list
	ListView_DeleteAllItems(hwndListView);

	// set the number of items in the list
	ListView_SetItemCount(hwndListView, logbook->entries.size());
}
// set up the columns
void LISTVIEWCHILD::InitListView(HWND hwndListView)
{
	ListView_DeleteAllItems(hwndListView);

	// setting the width to a sensible value is a bit of a bodge.
	// the real trick would be to make a screen HDC, put in the font I'm using
	// and use GetExtents...   Mañana
#define CHAR_WIDTH	7				// pixel width of character

	// initialize the columns
	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	for(int i = 0; i < logbook->titles.size(); ++i){
		lvColumn.cx = (logbook->titles[i].maxW + 3 + (i==0?3:0))* CHAR_WIDTH;
		lvColumn.pszText = (LPSTR)logbook->titles[i].col;
		ListView_InsertColumn(hwndListView, i, &lvColumn);
	}
	InsertListViewItems(hwndListView);
}
// handle the 'Notify' messages
LRESULT LISTVIEWCHILD::ListViewNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR  lpnmh = (LPNMHDR) lParam;
	HWND     hwndListView = GetDlgItem(hWnd, ID_LISTVIEW);

	switch(lpnmh->code){
	case LVN_GETDISPINFO:			// get the text to display
		{
			LV_DISPINFO *lpdi = (LV_DISPINFO *)lParam;

			if(lpdi->item.mask & LVIF_TEXT){
				const char* col;
				if(lpdi->item.iSubItem)
					col  = logbook->titles[lpdi->item.iSubItem].col;		// sub-item column name
				else{
					col = logbook->titles[0].col;							// first column name
					if(lpdi->item.mask & LVIF_IMAGE)						// does it want the icon too?
						lpdi->item.iImage = 0;
				}
				ENTRY* e = &logbook->entries[lpdi->item.iItem];				// find entry for row
				ITEM*  i = e->find(col);									// find item for column
				if(i  && i->value)
					strcpy_s(lpdi->item.pszText, lpdi->item.cchTextMax, i->value);
			}
			return 0;
		}

	case LVN_COLUMNCLICK:		// I implemented a sort on header click
		{
			LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;	// click column header
			logbook->sort(logbook->titles[pnmv->iSubItem].col, false);
			InsertListViewItems(hView);
			return 0;
		}
	}
	return 0;
}
// change the listView style - it was in the example but is seriously useless for logs
void LISTVIEWCHILD::SwitchView(HWND hwndListView, DWORD dwView)
{
	DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

	SetWindowLong(hwndListView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
	ResizeListView(hwndListView, GetParent(hwndListView));
}
void LISTVIEWCHILD::UpdateMenu(HWND hwndListView, HMENU hMenu)
{
	UINT  uID = IDM_LIST;

	// uncheck all of these guys
	CheckMenuItem(hMenu, IDM_LARGE_ICONS,	MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_SMALL_ICONS,	MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_LIST,			MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_REPORT,		MF_BYCOMMAND | MF_UNCHECKED);

	// check the appropriate view menu item
	DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);
	switch(dwStyle & LVS_TYPEMASK){
	case LVS_ICON:
		uID = IDM_LARGE_ICONS;
		break;

	case LVS_SMALLICON:
		uID = IDM_SMALL_ICONS;
		break;

	case LVS_LIST:
		uID = IDM_LIST;
		break;

	case LVS_REPORT:
		uID = IDM_REPORT;
		break;
	}
	CheckMenuRadioItem(hMenu, IDM_LARGE_ICONS, IDM_REPORT, uID,  MF_BYCOMMAND | MF_CHECKED);
}
// from the example - kept as it might be useful later
BOOL LISTVIEWCHILD::DoContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	HWND  hwndListView = (HWND)wParam;
	HMENU hMenuLoad, hMenu;

	if(hwndListView != GetDlgItem(hWnd, ID_LISTVIEW))
		return FALSE;

	hMenuLoad = LoadMenu(hInstance, MAKEINTRESOURCE(IDM_CONTEXT_MENU));
	hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hwndListView, hMenu);

	TrackPopupMenu(hMenu,
					TPM_LEFTALIGN | TPM_RIGHTBUTTON,
					LOWORD(lParam),
					HIWORD(lParam),
					0,
					hWnd,
					nullptr);

	DestroyMenu(hMenuLoad);

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//  WndProc()		the main window procedure for the ListView child
//-------------------------------------------------------------------------------------------------

// wrap a static member round it so we match the Windows specification for a  WndProc
LRESULT CALLBACK LISTVIEWCHILD::WndProcStatic(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	LISTVIEWCHILD *me = (LISTVIEWCHILD*)GetWindowLongPtr(hWnd, 0);
	if(me==nullptr) me = underCreation;
	return me->WndProc(hWnd, uMessage, wParam, lParam);
}

LRESULT LISTVIEWCHILD::WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	char logFile[MAX_PATH];

	switch(uMessage){
	case WM_CREATE:
//		{														// get the data* passed in on AddChild()
//			MDICREATESTRUCT* cv = reinterpret_cast<MDICREATESTRUCT*>((reinterpret_cast<CREATESTRUCT*>(lParam))->lpCreateParams);
//			void* data = reinterpret_cast<void*>(cv->lParam);
//			SetWindowLongPtr(hWnd, 0, (LONG_PTR)data);			//  do something with passed in data
//		}

restart:
		// we jump to restart if we have uploaded new DXCC or LOTW files
		// and need to reload the display. We always reload the log file
		// on restart as that regenerates the data with new data
		if(dxcc==nullptr)
			dxcc = new DXCC;
		if(lotw==nullptr){
			lotw = new LOTW;
			lotw->load();
		}
		if(eqsl==nullptr){
			eqsl = new EQSL;
			eqsl->load();
		}
		if(logbook!=nullptr)
			delete logbook;
		logbook = new ADIF;
		do{
			readConfig("files", "log", "", logFile, sizeof(logFile));
			if(logFile[0]==0){
nFile:			if(!GetFile(hWnd, "Give name of log-file to open", logFile, sizeof(logFile))) exit(0);
				writeConfig("files", "log", logFile);
			}
		} while(logbook->read(logFile)==false);

		hView = CreateListView(hInstance, hWnd);
		InitListView(hView);
		PostMessage(hWnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);	// to arrive after the create is finished
		UpdateWindow(hWnd);
		break;

	case WM_NOTIFY:
		return ListViewNotify(hWnd, lParam);

	case WM_SIZE:
		ResizeListView(hView, hWnd);
		break;

	case WM_INITMENUPOPUP:
		UpdateMenu(hView, GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if(DoContextMenu(hWnd, wParam, lParam))
			return FALSE;
		break;
 
	case WM_COMMAND:
		// Parse the menu selections:
		switch (LOWORD(wParam)){
		case IDM_NEW:				// select a new file
			delete logbook;
			logbook = new ADIF;		// new but empty
			goto nFile;				// ask for a new file name to load

		case IDM_DXCC:				// reload the country data
			delete dxcc;
			dxcc = new DXCC(true);	// force a download
			goto restart;

		case IDM_LOTW:				// reload the worked/QSLed lists
			delete lotw;
			lotw = new LOTW;
			lotw->load(true);		// force a download
			goto restart;

		case IDM_EQSL:				// reload the worked/QSLed lists
			delete eqsl;
			eqsl = new EQSL;
			eqsl->load(true);		// force a download
			goto restart;

		case IDM_RELOAD:			// reload the existing file (may be updating live)
			goto restart;

		case IDM_LARGE_ICONS:
			SwitchView(hView, LVS_ICON);
			break;

		case IDM_SMALL_ICONS:
			SwitchView(hView, LVS_SMALLICON);
			break;

		case IDM_LIST:
			SwitchView(hView, LVS_LIST);
			break;

		case IDM_REPORT:
			SwitchView(hView, LVS_REPORT);
			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			return 0;
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

