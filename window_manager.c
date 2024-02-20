// gcc window_manager.c -o window_manager.exe -g

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <minwindef.h>
#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.c"

#define MAX_SAVED_WINDOWS 999
#define MAX_SAVED_WINDOWS_DIGITS 3
#define STRING_LEN 256
#define HWND_LEN 17 // Add one to account for null byte

int activateSavedWindow(int index, TCHAR* filePath) {
  if (index < 0 || index >= MAX_SAVED_WINDOWS) {
    return 1;
  }
  TCHAR windowTitle[STRING_LEN];
  TCHAR indexStr[MAX_SAVED_WINDOWS_DIGITS];
  TCHAR hWndStr[HWND_LEN];
  printf("Activating window #%d\n", index);
  sprintf(indexStr, "%d", index);

  int returnCode = 1;
  DWORD iniReadResult = GetPrivateProfileString(indexStr, "hWnd", NULL, hWndStr, HWND_LEN, filePath);
  if (iniReadResult > 0) {
    HWND hWnd;
    sscanf_s(hWndStr, "%p", &hWnd);
    printf("Attempting to restore by handle '%s'.\n", hWndStr);
    returnCode = activateWindowByHandle(hWnd);
  }
  if (returnCode != 0) {
    iniReadResult = GetPrivateProfileString(indexStr, "title", NULL, windowTitle, STRING_LEN, filePath);
    if (iniReadResult > 0) {
      printf("Attempting to restore by title.\n");
      returnCode = activateWindowByTitle(windowTitle);
    }
  } 
  return returnCode;
}

int saveWindow(int index, TCHAR* filePath) {
  if (index < 0 || index > MAX_SAVED_WINDOWS-1) {
    return 1;
  }
  printf("Saving current window to index #%d\n", index);
  TCHAR processTitle[STRING_LEN];
  TCHAR windowTitle[STRING_LEN];
  HWND hWnd;
  getForegroundWindowInfo(&hWnd, &processTitle[0], &windowTitle[0]);
  printf("Saving hWnd %p, process %s to %s\n", hWnd, processTitle, filePath);
  TCHAR indexStr[MAX_SAVED_WINDOWS_DIGITS];
  TCHAR hWndStr[HWND_LEN];
  sprintf(indexStr, "%d", index);
  sprintf(hWndStr, "%p", hWnd);
  int iResult = WritePrivateProfileString(indexStr, "hWnd", hWndStr, filePath);
  iResult |= WritePrivateProfileString(indexStr, "title", windowTitle, filePath);
  return iResult;
}

int main(int argc, TCHAR *argv[]) {
  TCHAR iniFilePath[MAX_PATH] = "./saved_windows.ini";
  TCHAR iniFileAbsolutePath[MAX_PATH];

  int saveIndex = -1;
  int loadIndex = -1;
  
  for (int i=min(1,argc); i<argc; ++i) {
    // printf("%d\t%s\n",i,argv[i]);
    if (strchr(argv[i], '-') != argv[i]) {
      printf("ERROR: Invalid syntax.\n");
      return 1;
    }
    else if (strstr(argv[i], "--path=")==argv[i]) {
      sscanf_s(argv[i], "--path=%s", iniFilePath);
    }
    else if (strstr(argv[i], "--get-current-window")==argv[i]) {
      printf("Current window: %p\n", GetForegroundWindow());
    }
    else if (strstr(argv[i], "--activate-window=")==argv[i]) {
      HWND windowHandle;
      sscanf_s(argv[i], "--activate-window=%p", &windowHandle);
      activateWindowByHandle(windowHandle);
    }
    else if (strstr(argv[i], "--load-window=")==argv[i]) {
      sscanf_s(argv[i], "--load-window=%d", &loadIndex);
    }
    else if (strstr(argv[i], "--save-window=")==argv[i]) {
      sscanf_s(argv[i], "--save-window=%d", &saveIndex);
    }
  }
  int result = GetFullPathName(iniFilePath, MAX_PATH, iniFileAbsolutePath, NULL);
  if (result != strlen(iniFileAbsolutePath)) {
    printf("Invalid config file path: %s", iniFilePath);
    return 1;
  }

  printf("Path: %s\n", iniFileAbsolutePath);
  if (saveIndex >= 0) {
    if (!saveWindow(saveIndex, iniFileAbsolutePath)) {
      printf("Could not write to '%s'\n", iniFilePath);
    }
  }
  if (loadIndex >= 0) {
    activateSavedWindow(loadIndex, iniFileAbsolutePath);
  }
  return 0;
}
