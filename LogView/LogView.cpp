// LogView.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "LogView.h"

// Global Variables:
HINSTANCE hInstance{};				// current instance
HWND hView{};
adif logbook;
DXCC* dxcc = nullptr;

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uMessage){
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch LOWORD(wParam) {
		case IDOK:
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// make the ListView track the size of the enclosing window
void ResizeListView(HWND hwndListView, HWND hwndParent)
{
	RECT rc;
	GetClientRect(hwndParent, &rc);
	MoveWindow(hwndListView, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

HWND CreateListView(HINSTANCE hInstance, HWND hwndParent)
{
	DWORD		dwStyle;
	HWND		hwndListView;
	HIMAGELIST	himlSmall;
	HIMAGELIST	himlLarge;
	BOOL        bSuccess = TRUE;

	dwStyle =   WS_TABSTOP | WS_CHILD | WS_BORDER | WS_VISIBLE | LVS_AUTOARRANGE | LVS_REPORT |  LVS_OWNERDATA;

	hwndListView = CreateWindowEx(WS_EX_CLIENTEDGE,			// ex style
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
	himlSmall = ImageList_Create(16, 16, ILC_COLORDDB | ILC_MASK, 1, 0);
	himlLarge = ImageList_Create(32, 32, ILC_COLORDDB | ILC_MASK, 1, 0);

	if(himlSmall && himlLarge){
		HICON hIcon;

		// set up the small image list
		hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_DISK), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
		ImageList_AddIcon(himlSmall, hIcon);

		//set up the large image list
		hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DISK));
		ImageList_AddIcon(himlLarge, hIcon);

		ListView_SetImageList(hwndListView, himlSmall, LVSIL_SMALL);
		ListView_SetImageList(hwndListView, himlLarge, LVSIL_NORMAL);
	}

	return hwndListView;
}
void PositionHeader(HWND hwndListView)
{
	HWND  hwndHeader = GetWindow(hwndListView, GW_CHILD);
	DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

	/* To ensure that the first item will be visible, create the control without 
	   the LVS_NOSCROLL style and then add it here*/
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
void InsertListViewItems(HWND hwndListView)
{
	// empty the list
	ListView_DeleteAllItems(hwndListView);

	// set the number of items in the list
	ListView_SetItemCount(hwndListView, logbook.entries.size());
}
void InitListView(HWND hwndListView)
{
	ListView_DeleteAllItems(hwndListView);

#define CHAR_WIDTH	7				// pixel width of character		??????????????????????????????????????????

	// initialize the columns
	LV_COLUMN lvColumn;
	lvColumn.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
	lvColumn.fmt = LVCFMT_LEFT;
	for(int i = 0; i < logbook.titles.size(); ++i){
		lvColumn.cx = (logbook.titles[i].maxW + 3 + (i==0?3:0))* CHAR_WIDTH;
		lvColumn.pszText = (LPSTR)logbook.titles[i].col;
		ListView_InsertColumn(hwndListView, i, &lvColumn);
	}
	InsertListViewItems(hwndListView);
}
LRESULT ListViewNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR  lpnmh = (LPNMHDR) lParam;
	HWND     hwndListView = GetDlgItem(hWnd, ID_LISTVIEW);

	switch(lpnmh->code){
	case LVN_GETDISPINFO:
	{
		LV_DISPINFO *lpdi = (LV_DISPINFO *)lParam;

		if(lpdi->item.iSubItem){
			if(lpdi->item.mask & LVIF_TEXT){
				const char* col = logbook.titles[lpdi->item.iSubItem].col;		// column name
				entry* e = &logbook.entries[lpdi->item.iItem];					// find entry for row
				item*  i = e->find(col);										// find item for column
				if(i)
					strcpy_s(lpdi->item.pszText, lpdi->item.cchTextMax, i->value);
			}
		}
		else{
			if(lpdi->item.mask & LVIF_TEXT){
				const char* col = logbook.titles[0].col;						// first column name
				entry* e = &logbook.entries[lpdi->item.iItem];					// find entry for row
				item*  i = e->find(col);										// find item for column
				strcpy_s(lpdi->item.pszText, lpdi->item.cchTextMax, i->value);
			}

			if(lpdi->item.mask & LVIF_IMAGE)
				lpdi->item.iImage = 0;
		}
	}
	return 0;

	case LVN_ODCACHEHINT:
	{
		LPNMLVCACHEHINT	lpCacheHint = (LPNMLVCACHEHINT)lParam;
	/*
		This sample doesn't use this notification, but this is sent when the 
		ListView is about to ask for a range of items. On this notification, 
		you should load the specified items into your local cache. It is still 
		possible to get an LVN_GETDISPINFO for an item that has not been cached, 
		therefore, your application must take into account the chance of this 
		occurring.
	*/
	}
	return 0;

	case LVN_ODFINDITEM:
	{
		LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
		/*
			This sample doesn't use this notification, but this is sent when the 
			ListView needs a particular item. Return -1 if the item is not found.
		*/
	}
	return 0;
	}

	return 0;
}
void ErrorHandler(DWORD err, char* temp, int cb)
{
	if(err==0)
		err = GetLastError();

	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), temp, cb, nullptr);
	// now remove the \r\n we get on the end
	for(int n=(int)strlen(temp); n && (temp[n-1]=='\r' || temp[n-1]=='\n'); temp[n---1]=0);		// yes it does compile
}
void ErrorHandler()
{
	char temp[200];
	ErrorHandler(0, temp, sizeof(temp));
	MessageBox(0, temp, "Error", MB_OK);
}

void SwitchView(HWND hwndListView, DWORD dwView)
{
	DWORD dwStyle = GetWindowLong(hwndListView, GWL_STYLE);

	SetWindowLong(hwndListView, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
	ResizeListView(hwndListView, GetParent(hwndListView));
}
void UpdateMenu(HWND hwndListView, HMENU hMenu)
{
	UINT  uID = IDM_LIST;
	DWORD dwStyle;

	// uncheck all of these guys
	CheckMenuItem(hMenu, IDM_LARGE_ICONS,	MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_SMALL_ICONS,	MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_LIST,			MF_BYCOMMAND | MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_REPORT,		MF_BYCOMMAND | MF_UNCHECKED);

	// check the appropriate view menu item
	dwStyle = GetWindowLong(hwndListView, GWL_STYLE);
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

BOOL DoContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
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

//
//  WndProc(HWND, UINT, WPARAM, LPARAM)
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uMessage){
	case WM_CREATE:
//		char temp[260];
//		_getcwd(temp, sizeof(temp));
		dxcc = new DXCC;
		logbook.read("..\\wsjtx_log.adi");

		hView = CreateListView(hInstance, hWnd);
		InitListView(hView);
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

		case IDM_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			return 0;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			return 0;
		}
		break;

#if false
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		EndPaint(hWnd, &ps);
		return 0;
	}
#endif

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

int APIENTRY wWinMain(	_In_ HINSTANCE		hInstance,
						_In_opt_ HINSTANCE	hPrevInstance,
						_In_ LPWSTR			lpCmdLine,
						_In_ int			nCmdShow)
{
	// Register the Window Class
	WNDCLASSEX wcex{};
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDC_Log));
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_Log);
	wcex.lpszClassName	= "LogView";
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	RegisterClassEx(&wcex);

	// Perform application initialization:
	::hInstance = hInstance;			// Store instance handle in our global variable

	INITCOMMONCONTROLSEX icex;			// Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	HWND hWnd = CreateWindow("LogView", "LogView", WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd) return 99;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_Log));
	MSG msg;

	// Main message loop:
	while(GetMessage(&msg, nullptr, 0, 0)){
		if(!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int)msg.wParam;
}
