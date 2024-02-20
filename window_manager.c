// gcc window_manager.c -o window_manager.exe -g

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Psapi.h>
#include <minwindef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.c"

#define MAX_SAVED_WINDOWS 999 // Only exists to enforce LENGTH(MAX_SAVED_WINDOWS)
#define STRING_LEN 256 // Probably should differentiate different strings ¯\_(ツ)_/¯
#define HWND_LEN 17 // Add one to account for null byte

// Credit: https://stackoverflow.com/a/29952494
#define STRINGIFY(x) #x
#define LENGTH(x) (sizeof(STRINGIFY(x)) - 1)

// TODO: Incorporate/spin off a daemon that periodically updates saved window
// titles and handles

int activateSavedWindow(int index, TCHAR *filePath);
int saveWindow(int index, TCHAR *filePath);

int activateSavedWindow(int index, TCHAR *filePath) {
  if (index < 0 || index >= MAX_SAVED_WINDOWS) {
    return 1;
  }
  TCHAR windowTitle[STRING_LEN];
  TCHAR indexStr[LENGTH(MAX_SAVED_WINDOWS)];
  TCHAR hWndStr[HWND_LEN];
  printf("Activating window #%d\n", index);
  sprintf(indexStr, "%d", index);

  int returnCode = 0;

  // First, try activating the window by its saved hWnd
  DWORD iniReadResult = GetPrivateProfileString(indexStr, "hWnd", NULL, hWndStr, HWND_LEN, filePath);
  // If hWnd was read from ini
  if (iniReadResult > 0) {
    HWND hWnd;
    // Read pointer address from string into HWND
    sscanf_s(hWndStr, "%p", &hWnd);
    printf("Attempting to restore by handle '%s'.\n", hWndStr);
    returnCode = activateWindowByHandle(hWnd);
  }
  // If unsuccessful, try activating by title
  if (returnCode == 0) {
    // If window title was read from ini
    iniReadResult = GetPrivateProfileString(indexStr, "title", NULL, windowTitle, STRING_LEN, filePath);
    if (iniReadResult > 0) {
      printf("Attempting to restore by title.\n");
      returnCode = activateWindowByTitle(windowTitle);
    }
  }
  // If the window was successfully activated, re-save it in case the title or
  // handle has changed.
  if (returnCode != 0) {
    saveWindow(index, filePath);
  }
  return returnCode;
}

int saveWindow(int index, TCHAR *filePath) {
  if (index < 0 || index > MAX_SAVED_WINDOWS - 1) {
    return 1;
  }
  printf("Saving current window to index #%d\n", index);
  TCHAR processTitle[STRING_LEN];
  TCHAR windowTitle[STRING_LEN];
  HWND hWnd;
  getForegroundWindowInfo(&hWnd, &processTitle[0], &windowTitle[0]);
  printf("Saving hWnd %p, process %s to %s\n", hWnd, processTitle, filePath);
  TCHAR indexStr[LENGTH(MAX_SAVED_WINDOWS)];
  TCHAR hWndStr[HWND_LEN];
  sprintf(indexStr, "%d", index);
  sprintf(hWndStr, "%p", hWnd);
  int iResult = WritePrivateProfileString(indexStr, "hWnd", hWndStr, filePath);
  iResult |= WritePrivateProfileString(indexStr, "title", windowTitle, filePath);
  return iResult;
}

int main(int argc, TCHAR *argv[]) {
  TCHAR iniFilePath[MAX_PATH];
  iniFilePath[0] = 0;
  TCHAR iniFileAbsolutePath[MAX_PATH];

  int saveIndex = -1;
  int loadIndex = -1;

  for (int i = min(1, argc); i < argc; ++i) {
    // printf("%d\t%s\n",i,argv[i]);
    if (strchr(argv[i], '-') != argv[i]) {
      printf("ERROR: Invalid syntax.\n");
      return 1;
    } else if (strstr(argv[i], "--path=") == argv[i]) {
      sscanf_s(argv[i], "--path=%s", iniFilePath);
    } else if (strstr(argv[i], "--get-current-window") == argv[i]) {
      printf("Current window: %p\n", GetForegroundWindow());
    } else if (strstr(argv[i], "--activate-window=") == argv[i]) {
      HWND windowHandle;
      sscanf_s(argv[i], "--activate-window=%p", &windowHandle);
      activateWindowByHandle(windowHandle);
    } else if (strstr(argv[i], "--load-window=") == argv[i]) {
      sscanf_s(argv[i], "--load-window=%d", &loadIndex);
    } else if (strstr(argv[i], "--save-window=") == argv[i]) {
      sscanf_s(argv[i], "--save-window=%d", &saveIndex);
    } else {
      printf("Invalid argument.\n");
      return 1;
    }
  }

  if (iniFilePath[0] == 0) {
    DWORD bufCharCount = MAX_PATH;
    GetComputerName(iniFilePath, &bufCharCount);
    printf("%s\n", iniFilePath);
    TCHAR *buf = malloc(sizeof(iniFilePath) + 4);
    sprintf(buf, "saved_windows-%s.ini", iniFilePath);
    strcpy_s(iniFilePath, MAX_PATH, buf);
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
