//-------------------------------------------------------------------------------------------------
// LogView.cpp : Defines the entry point and the Frame window for the application.
//-------------------------------------------------------------------------------------------------

#include "framework.h"

// Global Variables:
HINSTANCE hInstance{};				// current instance
HWND  hFrame{}, hClient{};
int nChildren{};					// number of MDI windows
LISTVIEWCHILD *list1{};
ADIF* logbook{};
DXCC* dxcc{};
LOTW* lotw{};
EQSL* eqsl{};
CRITICAL_SECTION CriticalSection{}; 

//=====================================================================================================
// handler to unpack Windows error codes into text  (Work in Wide so we can handle anything)
//=====================================================================================================

void error(DWORD err)
{
	WCHAR temp[200];
	int cb = SIZEOF(temp);
	if(err == 0)
		err = GetLastError();
	wsprintfW(temp, L"%X ", err);
	DWORD i = (DWORD)wcslen(temp);
	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), &temp[i], cb-i, nullptr);
	// now remove the \r\n we get on the end
	for(auto n = wcsnlen_s(temp, cb); n>3 && (temp[n-1] == '\r' || temp[n-1] == '\n'); temp[n-- - 1] = 0);		// yes it does compile
	MessageBoxW(nullptr, temp, L"Error", MB_OK);
}

//-------------------------------------------------------------------------------------------------
// Message handler for the 'about' dialog.
//-------------------------------------------------------------------------------------------------

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

//-------------------------------------------------------------------------------------------------
// Message handler for the 'access' dialog.
//-------------------------------------------------------------------------------------------------

INT_PTR CALLBACK Access(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	char temp[50];
	switch(uMessage){
	case WM_INITDIALOG:
		readConfig("setup", "LOTWuser", "", temp, sizeof(temp));
		SetDlgItemText(hDlg, IDC_USERNAME1, temp);
		readConfig("setup", "LOTWpassword", "", temp, sizeof(temp));
		SetDlgItemText(hDlg, IDC_PASSWORD1, temp);
		readConfig("setup", "EQSLuser", "", temp, sizeof(temp));
		SetDlgItemText(hDlg, IDC_USERNAME2, temp);
		readConfig("setup", "EQSLpassword", "", temp, sizeof(temp));
		SetDlgItemText(hDlg, IDC_PASSWORD2, temp);

		return (INT_PTR)TRUE;

	case WM_COMMAND:
		switch LOWORD(wParam) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_USERNAME1, temp, sizeof(temp));
			writeConfig("setup", "LOTWuser", temp);
			GetDlgItemText(hDlg, IDC_PASSWORD1, temp, sizeof(temp));
			writeConfig("setup", "LOTWpassword", temp);
			GetDlgItemText(hDlg, IDC_USERNAME2, temp, sizeof(temp));
			writeConfig("setup", "EQSLuser", temp);
			GetDlgItemText(hDlg, IDC_PASSWORD2, temp, sizeof(temp));
			writeConfig("setup", "EQSLpassword", temp);
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
//-------------------------------------------------------------------------------------------------
// Tell a child window to go away
//-------------------------------------------------------------------------------------------------

void RemoveChild(HWND hChild)
{
	SendMessage(hClient, WM_MDIDESTROY, (WPARAM)hChild, 0);
}
//-------------------------------------------------------------------------------------------------
// Forwards a windows message to the top level child
//-------------------------------------------------------------------------------------------------

LRESULT SendToActiveChild(UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	HWND hwnd = reinterpret_cast<HWND>(SendMessage(hClient, WM_MDIGETACTIVE, 0, 0));
	if(hwnd) return SendMessage(hwnd, uMessage, wParam, lParam);
	return 0;
}

//=====================================================================================================
//  FrameWndProc(HWND, UINT, WPARAM, LPARAM)		WndProc for the Frame
//=====================================================================================================

#define WINDOWMENU		1		// zero based index of "Windows" menu to append the children list to

LRESULT CALLBACK FrameWndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch(uMessage){
	case WM_CREATE:
	{
		if(readConfig())			// if first time in so we need to offer the access dialog
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ACCESS), hWnd, Access);

		// The frame does not contain the children - an invisible window called
		// the MDICLIENT does so we create it
		CLIENTCREATESTRUCT ccs{};
		ccs.hWindowMenu = GetSubMenu(GetMenu(hWnd), WINDOWMENU);
		ccs.idFirstChild = IDM_WINDOWCHILD;

		// Create the MDI client window. (notice the 'magic number' in the HMENU field)
		hClient = CreateWindowEx(WS_EX_CLIENTEDGE, "MDICLIENT", nullptr,
							WS_CHILD | WS_CLIPCHILDREN | WS_VSCROLL | WS_HSCROLL,
							0, 0, 0, 0, hWnd, (HMENU)0xCAC, hInstance, (LPSTR)&ccs);
		if(!hClient){
			error();
			exit(99);
		}

		// since this is the first time and we want to show something pop in a child
		// on the default tile or ask for a new file selection
		new LISTVIEWCHILD;
		ShowWindow(hClient, SW_SHOW);
		return 0; // break;
	}
	case WM_COMMAND:
		// Parse the menu selections:
		switch(LOWORD(wParam)){
//		case IDM_ADDCHILD:
//		{
//			WCHAR temp[20];
//			static int children=0;
//			wsprintf(temp, L"Title %d", ++children);
//			AddChild(temp);
//			return 0;
//		}
//		case IDM_FORCHILD:
//			return SendToActiveChild(uMessage, wParam, lParam);

		case IDM_ACCESS:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ACCESS), hWnd, Access);
			return 0;

		case IDM_ABOUT:
			DialogBox(hInstance, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			return 0;

		case IDM_ARRANGEICONS:
			SendMessage(hClient, WM_MDIICONARRANGE, MDITILE_SKIPDISABLED, 0);
			return 0;

		case IDM_CASCADE:
			SendMessage(hClient, WM_MDICASCADE, MDITILE_SKIPDISABLED, 0);
			return 0;

		case IDM_TILEVERTICAL:
			SendMessage(hClient,WM_MDITILE,MDITILE_VERTICAL | MDITILE_SKIPDISABLED, 0);
			return 0;

		case IDM_TILEHORIZONTAL:
			SendMessage(hClient, WM_MDITILE, MDITILE_HORIZONTAL | MDITILE_SKIPDISABLED, 0);
			return 0;

		case IDM_NEW:				// open a new file
			new LISTVIEWCHILD;
			return 0;

		case IDM_CHANGEDEFAULT:		// select a new default file
		case IDM_DXCC:				// reload the country data
		case IDM_LOTW:				// reload the worked/QSLed lists
		case IDM_EQSL:				// reload the worked/QSLed lists
		case IDM_RELOAD:			// reload the existing file (may be updating live)
		case IDM_LARGE_ICONS:
		case IDM_SMALL_ICONS:
		case IDM_LIST:
		case IDM_REPORT:
			return SendToActiveChild(uMessage, wParam, lParam);

		case IDM_EXIT:
			DestroyWindow(hWnd);
			return 0;
		}
		break;

	case WM_SIZE:
		MoveWindow(hClient, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefFrameProc(hWnd, hClient, uMessage, wParam, lParam);
}
//-------------------------------------------------------------------------------------------------
// WinMain()		this is where we make most of our planets
//-------------------------------------------------------------------------------------------------

int WINAPI WinMain(	_In_ HINSTANCE		hInstance,
					_In_opt_ HINSTANCE	hPrevInstance,
					_In_ LPSTR			lpCmdLine,
					_In_ int			nCmdShow)
{
	::hInstance = hInstance;			// Store instance handle in our global variable
	InitializeCriticalSection(&CriticalSection);

	// Register the Frame Window Class
	WNDCLASSEX wcex{};
	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= FrameWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDC_LogView));
	wcex.hCursor		= LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_LogView);
	wcex.lpszClassName	= "LogViewFrame";
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_LogView));
	RegisterClassEx(&wcex);

	// Perform application initialization:
	INITCOMMONCONTROLSEX icex;			// Structure for control initialization.
	icex.dwICC = ICC_LISTVIEW_CLASSES;
	InitCommonControlsEx(&icex);

	hFrame = CreateWindow("LogViewFrame", "LogView", WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, LoadMenu(hInstance, MAKEINTRESOURCE(IDC_LogView)), hInstance, nullptr);

	if(!hFrame){
		error();
		exit(99);
	}

	ShowWindow(hFrame, nCmdShow);
	UpdateWindow(hFrame);

	HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_LogView));
	MSG msg;

	// Main message loop:
	while(GetMessage(&msg, nullptr, 0, 0))
		if(!TranslateMDISysAccel(hClient, &msg) && !TranslateAccelerator(hFrame, hAccelTable, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

	DeleteCriticalSection(&CriticalSection);
	return (int)msg.wParam;
}
