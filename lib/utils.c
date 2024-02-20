#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Psapi.h>
#include <stdio.h>

#define BUFFER_LEN 256
// TODO: Replace BUFFER_LEN with something more sensible

int forceSetForegroundWindow(const HWND hWnd) {
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

int getForegroundWindowInfo(HWND *foregroundWindow, TCHAR *processName, TCHAR *windowTitle) {
  DWORD dwProcId = 0;
  int returnCode = 0;

  // ** Get fg window handle **
  *foregroundWindow = GetForegroundWindow();
  // ** Get window process name **
  returnCode |= GetWindowThreadProcessId(*foregroundWindow, &dwProcId);
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
  TCHAR processPath[BUFFER_LEN];
  returnCode |= GetModuleFileNameExA((HMODULE)hProc, NULL, processPath, BUFFER_LEN);
  CloseHandle(hProc);
  // ** Get part of process name after last backslash
  // This pointer math is IFFY, hope it's right lol
  TCHAR *procStart = processPath;
  TCHAR *backslashPos = strrchr(processPath, '\\');
  if (backslashPos) {
    procStart = backslashPos + 1; // Add one to omit the slash
  }
  TCHAR *procEnd = strstr(processPath, ".exe");
  int end = strlen(procStart);
  if (strstr(processPath, ".exe")) {
    end -= 4; // Subtract 4 for the ".exe"
  }
  end = max(0, min(end, BUFFER_LEN - 1)); // Subtract one so there's room for the null byte

  for (int charIndex = 0; charIndex < end; ++charIndex) {
    processName[charIndex] = *(procStart + charIndex); // sscanf_s would probably make this way easier?
  }
  processName[end] = '\0';
  // ** Get window title **
  returnCode |= GetWindowTextA(*foregroundWindow, windowTitle, BUFFER_LEN);
  return returnCode;
}
