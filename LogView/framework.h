#pragma once
//-------------------------------------------------------------------------------------------------
// framework.h :  the file to include the includes
//-------------------------------------------------------------------------------------------------

// set the SDK version we want (default windows)
#include "targetver.h"

// Windows Header Files
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>

// C RunTime Header Files
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <urlmon.h>
#include <time.h>
#include <direct.h>
#include <shlobj_core.h>

// std:: library header files
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numbers>

// Project specific header files
#include "resource.h"
#include "reader.h"
#include "dxcc.h"
#include "config.h"
#include "lotw.h"
#include "eqsl.h"
#include "ListViewChild.h"
#include "LogView.h"

// modules for the linker to include
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "URLmon.lib")
#pragma comment(lib, "Comdlg32.lib")
