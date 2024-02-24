// gcc window_manager.c utils.c -o window_manager.exe

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Psapi.h>
#include <minwindef.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"
#include "window_manager.h"

#ifdef CMAKE
#include "window_tools.h"
#else
#define window_tools_VERSION_MAJOR 0
#define window_tools_VERSION_MINOR 0
#define window_tools_VERSION_PATCH 0
#endif

#define MAX_SAVED_WINDOWS 999 // Only exists to enforce LENGTH(MAX_SAVED_WINDOWS)
#define STRING_LEN 256        // Probably should differentiate different strings ¯\_(ツ)_/¯
#define HWND_LEN 17           // Add one to account for null byte

// Credit: https://stackoverflow.com/a/29952494
#define STRINGIFY(x) #x
#define LENGTH(x) (sizeof(STRINGIFY(x)) - 1)

#define MAX_SAVED_WINDOW_DIGITS LENGTH(MAX_SAVED_WINDOWS)

// TODO: Incorporate/spin off a daemon that periodically updates saved window
// titles and handles
// TODO: Add documentation
// TODO: Abstract saving/reading from ini to reduce repeated code

int activateSavedWindow(int index, const TCHAR *filePath) {
  if (index < 0 || index > MAX_SAVED_WINDOWS) {
    return 1;
  }
  TCHAR windowTitle[STRING_LEN];
  TCHAR indexStr[MAX_SAVED_WINDOW_DIGITS];
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
    if (returnCode != 0) {
      printf("Success.\n");
    }
  }
  else {
    printf("Couldn't read saved window %d from %s.\n", index, filePath);
  }
  // If unsuccessful, try activating by title
  if (returnCode == 0) {
    // If window title was read from ini
    iniReadResult = GetPrivateProfileString(indexStr, "title", NULL, windowTitle, STRING_LEN, filePath);
    if (iniReadResult > 0) {
      printf("Attempting to restore by title.\n");
      returnCode = activateWindowByTitle(windowTitle);
      if (returnCode != 0) {
        printf("Success.\n");
      }
      else {
        printf("Couldn't activate window. It may have already been active."); // TODO: Check if window being activated is already active
      }
    }
  }
  // If the window was successfully activated, re-save it in case the title or
  // handle has changed.
  if (returnCode != 0) {
    // Wait for window to activate first. TODO: Find optimal value.
    Sleep(10);
    printf("Updating saved window details.\n");
    saveWindow(index, filePath);
  }
  return returnCode;
}

int saveWindow(int index, const TCHAR *filePath) {
  if (index < 0 || index > MAX_SAVED_WINDOWS - 1) {
    return 1;
  }
  printf("Saving current window to index #%d\n", index);
  TCHAR processTitle[STRING_LEN];
  TCHAR windowTitle[STRING_LEN];
  HWND hWnd;
  getForegroundWindowInfo(&hWnd, &processTitle[0], &windowTitle[0], STRING_LEN);
  printf("Saving hWnd %p, process %s to %s\n", hWnd, processTitle, filePath);
  TCHAR indexStr[LENGTH(MAX_SAVED_WINDOWS)];
  TCHAR hWndStr[HWND_LEN];
  sprintf(indexStr, "%d", index);
  sprintf(hWndStr, "%p", hWnd);
  int iResult = WritePrivateProfileString(indexStr, "hWnd", hWndStr, filePath);
  iResult |= WritePrivateProfileString(indexStr, "title", windowTitle, filePath);
  return iResult;
}

#define MAX_SHOWN_TITLE_CHARACTERS 50
int showSavedWindows(TCHAR *filePath) {
  TCHAR savedWindowNumbers[MAX_SAVED_WINDOWS*(MAX_SAVED_WINDOW_DIGITS+1)]; // Add one for null byte separating entries
  int savedWindowNumbersSize = GetPrivateProfileString(NULL, "title", NULL, savedWindowNumbers, MAX_SAVED_WINDOWS, filePath);

  // Message that will be shown listing window numbers and titles
  // Enough space for: window index, delimiter, window title, newline
  // This is going to be very large; maybe I need to cut down on MAX_SAVED_WINDOWS?
  TCHAR message[MAX_SAVED_WINDOWS*(MAX_SAVED_WINDOW_DIGITS+1+MAX_SHOWN_TITLE_CHARACTERS+1)];
  // Position in `message`
  int messagePos = 0;

  // Window number extracted from `savedWindowNumbers`
  TCHAR windowNumber[MAX_SAVED_WINDOW_DIGITS];
  // Position in `windowNumber`
  int winNumberPos = 0;

  HWND hWnd;
  TCHAR hWndStr[HWND_LEN];
  while (winNumberPos < savedWindowNumbersSize) {
    // If not the first iteration, add a newline for the new entry
    if (winNumberPos > 0) {
      message[messagePos] = '\n';
      ++messagePos;
    }

    // Get next window number in `savedWindowNumbers`
    winNumberPos += sprintf(windowNumber, "%s", &savedWindowNumbers[winNumberPos]) + 1;

    // First, grab the hWnd and update the window title in case it has changed since it was saved
    GetPrivateProfileString(windowNumber, "hWnd", NULL, &hWndStr[0], HWND_LEN, filePath);
    sscanf_s(hWndStr, "%p", &hWnd);
    TCHAR windowTitle[STRING_LEN];
    GetWindowTextA(hWnd, windowTitle, STRING_LEN);
    WritePrivateProfileString(windowNumber, "title", windowTitle, filePath);

    // Write the window number
    messagePos += sprintf(&message[messagePos], "%s: ", windowNumber);
    // Get the title associated with `windowNumber`
    messagePos += GetPrivateProfileString(windowNumber, "title", NULL, &message[messagePos], min(sizeof(message)-messagePos, MAX_SHOWN_TITLE_CHARACTERS), filePath);
  }
  // MB_SYSTEMMODAL ensures the window comes to the front
  // In theory, MB_SETFOREGROUND ensures it is focused (so it can be closed w/ spacebar or enter); this doesn't appear to be the case though
  // TODO: Get this window to be focused on open
  MessageBox(NULL, message, "Window Manager: Saved Windows", MB_ICONINFORMATION | MB_SYSTEMMODAL | MB_SETFOREGROUND);
  return 0;
}

int main(int argc, TCHAR *argv[]) {
  TCHAR iniFilePath[MAX_PATH];
  iniFilePath[0] = 0;
  TCHAR iniFileAbsolutePath[MAX_PATH];

  BOOL show = FALSE;

  int saveIndex = -1;
  int loadIndex = -1;

  for (int i = min(1, argc); i < argc; ++i) {
    // printf("%d\t%s\n",i,argv[i]);
    if (strchr(argv[i], '-') != argv[i]) {
      printf("ERROR: Invalid syntax.\n");
      return 1;
    } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
      printf("kanata_helper_daemon version %d.%d.%d\n", window_tools_VERSION_MAJOR, window_tools_VERSION_MINOR,
             window_tools_VERSION_PATCH);
      return 0;
    } else if (strcmp(argv[i], "--show") == 0) {
      show = TRUE;
    } else if (strstr(argv[i], "--path=") == argv[i]) {
      sscanf_s(argv[i], "--path=%s", iniFilePath, (unsigned)sizeof(iniFilePath));
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
  if (show) {
    showSavedWindows(iniFileAbsolutePath);
  }
  return 0;
}
