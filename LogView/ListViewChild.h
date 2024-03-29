#pragma once
//-------------------------------------------------------------------------------------------------
// ListViewChild.h
//-------------------------------------------------------------------------------------------------

extern int nChildren;

#define WW_THIS		0		// index in the Window Data

class LISTVIEWCHILD {
public:
	LISTVIEWCHILD(const char *fname=0){
		RegisterClass();
		AddChild("LogView", fname);
	}
public:
	static bool once;
	static LISTVIEWCHILD* underCreation;
	HWND hView{};
	ADIF* logbook{};

	// windows interface
	// put the 'this' pointer into the window data area
	static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	LRESULT WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	// Windows stuff
	void RegisterClass();
	HWND AddChild(const char* title, const char* data);
	HWND CreateListView(HINSTANCE hInstance, HWND hwndParent);

	// ListView stuff
	void ResizeListView(HWND hwndListView, HWND hwndParent);
	void PositionHeader(HWND hwndListView);
	void InsertListViewItems(HWND hwndListView);
	void InitListView(HWND hwndListView);
	LRESULT ListViewNotify(HWND hWnd, LPARAM lParam);
	void SwitchView(HWND hwndListView, DWORD dwView);
	void UpdateMenu(HWND hwndListView, HMENU hMenu);
	BOOL DoContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam);
};
