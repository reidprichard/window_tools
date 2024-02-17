#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <minwindef.h>
#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
// #include <WinUser.h>

#define MAX_SAVED_WINDOWS 32

HWND* savedWindowHandles[MAX_SAVED_WINDOWS];
TCHAR* savedWindowTitles[MAX_SAVED_WINDOWS];

int activateSavedWindow(int index) {

  return 0;
}

int saveWindow(int index) {

  return 0;
}

int writeSavedWindows(const TCHAR* filePath) {

  return 0;
}

int readSavedWindows(const TCHAR* filePath) {

  return 0;
}

int main(int argc, TCHAR *argv[]) {
  printf("Starting...\n");
  return 0;
}
