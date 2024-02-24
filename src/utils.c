#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Psapi.h>
#include <stdio.h>

#include "utils.h"

#define BUFFER_LEN 256
// TODO: Replace BUFFER_LEN with something more sensible
// TODO: Make getForegroundWindowInfo accept NULL for strings

int forceSetForegroundWindow(const HWND hWnd) {

  // First check if the desired window is already active. This is so that the
  // keypress of alt (below) doesn't cause the menubar to pop up.
  HWND currentActiveWindow;
  TCHAR buf1[BUFFER_LEN];
  TCHAR buf2[BUFFER_LEN];
  getForegroundWindowInfo(&currentActiveWindow, &buf1[0], &buf2[0], BUFFER_LEN);
  if (currentActiveWindow == hWnd) {
    return 0;
  }

  // Tricks here courtesy of https://gist.github.com/Aetopia/1581b40f00cc0cadc93a0e8ccb65dc8c
  // These were suggested to help, but I found them unnecessary:
  // AllocConsole();
  // FreeConsole();
  INPUT pInputs[] = {{.type = INPUT_KEYBOARD, .ki.wVk = VK_MENU, .ki.dwFlags = 0},
                     {.type = INPUT_KEYBOARD, .ki.wVk = VK_MENU, .ki.dwFlags = KEYEVENTF_KEYUP}};
  SendInput(2, pInputs, sizeof(INPUT));
  return SetForegroundWindow(hWnd);
}

int activateWindowByHandle(const HWND hWnd) { return forceSetForegroundWindow(hWnd); }

int activateWindowByTitle(const TCHAR *windowTitle) {
  HWND hWnd = FindWindow(NULL, windowTitle);
  if (hWnd) {
    return activateWindowByHandle(hWnd);
  } else {
    return 0;
  }
}

int getWindowProcessName(HWND hWnd, TCHAR* processName, int bufferLen) {
  DWORD dwProcId = 0;
  int returnCode = 0;
  // ** Get window process name **
  returnCode |= GetWindowThreadProcessId(hWnd, &dwProcId);
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
  TCHAR processPath[BUFFER_LEN];
  returnCode |= GetModuleFileNameExA((HMODULE)hProc, NULL, processPath, BUFFER_LEN);
  CloseHandle(hProc);
  // ** Get part of process name after last backslash
  TCHAR *procStart = processPath;
  TCHAR *backslashPos = strrchr(processPath, '\\');
  if (backslashPos) {
    procStart = backslashPos + 1; // Add one to omit the slash
  }
  // If the process path ends in .exe, replace the '.' with null byte to ignore the ".exe"
  if (strstr(processPath, ".exe")) {
    TCHAR* periodPos = strrchr(processPath, '.');
    *periodPos = '\0';
  }
  sprintf_s(processName, bufferLen, "%s", procStart);
  return returnCode;
}

int getForegroundWindowInfo(HWND *foregroundWindow, TCHAR *processName, TCHAR *windowTitle, int bufferLen) {
  int returnCode = 0;

  // ** Get fg window handle **
  *foregroundWindow = GetForegroundWindow();
  // ** Get window process name **
  if (processName != NULL) {
    returnCode |= getWindowProcessName(*foregroundWindow, processName, bufferLen);
  }
  // ** Get window title **
  returnCode |= GetWindowTextA(*foregroundWindow, windowTitle, bufferLen);
  return returnCode;
}
