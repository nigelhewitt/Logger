#pragma once
//-------------------------------------------------------------------------------------------------
// framework.h : the file to include the includes
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
#include <urlmon.h>

// std:: library header files
#include <string>
#include <vector>
#include <unordered_map>
#include <time.h>
#include <direct.h>
#include <algorithm>

// Project specific header file
#include "resource.h"
#include "reader.h"
#include "lookup.h"
#include "config.h"
#include "lotw.h"
#include "eqsl.h"
#include "LogView.h"

// modules for the linker to include
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "URLmon.lib")
#pragma comment(lib, "Comdlg32.lib")
