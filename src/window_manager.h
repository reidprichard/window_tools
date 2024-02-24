#pragma once
#include <windows.h>
// TODO: Docs here
int activateSavedWindow(int index, const TCHAR *filePath);
int saveWindow(int index, const TCHAR *filePath);
int showSavedWindows(TCHAR *filePath);
