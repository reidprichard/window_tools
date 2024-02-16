// What does this do??? Needed so winsock2 include statement doesn't error
#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <minwindef.h>
#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
// #include <WinUser.h>

#pragma comment(lib, "Ws2_32.lib")

int forceSetForegroundWindow(HWND window) {
  // Tricks here courtesy of
  // https://gist.github.com/Aetopia/1581b40f00cc0cadc93a0e8ccb65dc8c
  // AllocConsole();
  // FreeConsole();
  INPUT pInputs[] = {
      {.type = INPUT_KEYBOARD, .ki.wVk = VK_MENU, .ki.dwFlags = 0},
      {.type = INPUT_KEYBOARD,
       .ki.wVk = VK_MENU,
       .ki.dwFlags = KEYEVENTF_KEYUP}};
  SendInput(2, pInputs, sizeof(INPUT));
  return SetForegroundWindow(window);
}

#define BUFFER_LEN 256
int getForegroundWindowInfo(HWND* foregroundWindow, TCHAR** processName, TCHAR** windowTitle) {
  DWORD dwProcId = 0;
  int returnCode = 0;

  // ** Get fg window handle **
  *foregroundWindow = GetForegroundWindow();
  // ** Get window process name **
  returnCode |= GetWindowThreadProcessId(*foregroundWindow, &dwProcId);
  HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcId);
  TCHAR processPath[BUFFER_LEN];
  returnCode |= GetModuleFileNameExA((HMODULE) hProc, NULL,processPath, BUFFER_LEN);
  CloseHandle(hProc);
  // ** Get part of process name after last backslash
  // This pointer math is IFFY, hope it's right lol
  TCHAR* procStart = strrchr(processPath, '\\') + 1;
  TCHAR* procEnd = strstr(processPath, ".exe");
  int end = strlen(procStart);
  if (procEnd) {
    end = procEnd-procStart;
  }
  for (int charIndex = 0; charIndex < min(end, BUFFER_LEN-1); ++ charIndex) {
    (*processName)[charIndex] = *(procStart + charIndex);
  }
  (*processName)[min(strlen(procStart), BUFFER_LEN)] = '\0'; // Off by one here?
  // ** Get window title **
  returnCode |= GetWindowTextA(*foregroundWindow, *windowTitle, BUFFER_LEN);
  // printf("ID: %d,\tTitle: %s,\tProc: %s",(int) foregroundWindow, windowTitle, processTitleStart);
  return returnCode;
}

int initTcp(char* host, char* port, SOCKET* ConnectSocket) {
  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
  }

  struct addrinfo *result = NULL, *ptr = NULL, hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  iResult = getaddrinfo("localhost", "1337", &hints, &result);

  if (iResult != 0) {
    printf("getaddrinfo failed: %d\n", iResult);
    WSACleanup();
    return 1;
  }
  *ConnectSocket = INVALID_SOCKET;
  ptr = result;

  *ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

  if (*ConnectSocket == INVALID_SOCKET) {
    printf("Error at socket(): %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  iResult = connect(*ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    closesocket(*ConnectSocket);
    *ConnectSocket = INVALID_SOCKET;
  }

  freeaddrinfo(result);

  if (*ConnectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
  }

  return 0;
}

void loop() {
  HWND fg;
  TCHAR* procName = malloc(sizeof(TCHAR) * BUFFER_LEN);
  TCHAR* winTitle = malloc(sizeof(TCHAR) * BUFFER_LEN);
  while(TRUE) {
    if (getForegroundWindowInfo(&fg, &procName, &winTitle)) {
      printf("ID: '%d',\tTitle: '%s',\tProc: '%s'\n",(int) fg, winTitle, procName);
    }
    Sleep(1000);
  }
}

int main(int argc, char *argv[]) {
  printf("Starting...\n");

  SOCKET kanataSocket;
  initTcp("localhost", "1337", &kanataSocket);

  // loop();
  return 0;
}
