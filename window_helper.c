#define WIN32_LEAN_AND_MEAN
#include <ws2tcpip.h>
#include <winsock2.h>
#include <windows.h>
#include <minwindef.h>
#include <Psapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
// #include <WinUser.h>

#pragma comment(lib, "Ws2_32.lib")
const TCHAR* configFilePath = "../murphpad_kanata.kbd";

#define MAX_LAYERS 25
#define MAX_LAYER_NAME_LENGTH 64
#define MAX_CONFIG_FILE_LINE_LENGTH 256

const TCHAR* layerStartStr = "(deflayer ";
TCHAR layerNames[MAX_LAYERS][MAX_LAYER_NAME_LENGTH];
int layerCount = 0;
const TCHAR* hostname = "localhost";
const TCHAR* port = "1337";

static BOOL CALLBACK enumWindowCallback(HWND hWnd, LPARAM lparam) {
  int length = GetWindowTextLength(hWnd);
  TCHAR* buff = malloc(length);
  

  free(buff);
  return 0;
}

int getWindowHwnd(const TCHAR* windowTitle, HWND* hWnd) {
  EnumWindows(enumWindowCallback, NULL);
}

int getLayerNames(const TCHAR* configPath) {
  FILE *fptr;
  fopen_s(&fptr, configPath, "r");
  TCHAR buf[MAX_CONFIG_FILE_LINE_LENGTH];
  int layerNum = 0;
  while (fgets(buf, MAX_CONFIG_FILE_LINE_LENGTH, fptr)) {
    TCHAR* pos = strstr(buf, layerStartStr);
    // Check that:
    // 1: layerStartStr is found
    // 2. The line continues past layerStartStr (add 1 to account for newline)
    if (pos && strlen(buf) > strlen(layerStartStr) + 1) {
      int layerNameLength = strlen(buf) - strlen(layerStartStr);
      for (int charIndex = 0; charIndex < min(layerNameLength, MAX_LAYER_NAME_LENGTH); ++charIndex) {
        layerNames[layerNum][charIndex] = buf[strlen(layerStartStr)+charIndex];
      }
      layerNames[layerNum][min(layerNameLength, MAX_LAYER_NAME_LENGTH)] = '\0';
      ++layerNum;
    }
  }
  layerCount = layerNum;
  while (layerNum < MAX_LAYERS) {
    layerNames[layerNum][0] = '\0';
    ++layerNum;
  }
  printf("%d layers found\n", layerCount);
  for (int i = 0; i < layerCount; ++i) {
    printf("Layer %d: %s\n", i, layerNames[i]);
  }

  return 0;
}

int forceSetForegroundWindow(HWND window) {
  // Tricks here courtesy of https://gist.github.com/Aetopia/1581b40f00cc0cadc93a0e8ccb65dc8c
  // These were suggested to help, but I found them unnecessary:
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

  // It seems that 
  // for (int charIndex = 0; charIndex < BUFFER_LEN; ++charIndex) {
  //   (*processName)[charIndex] = '\0';
  // }

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
  TCHAR* procStart = processPath;
  TCHAR* backslashPos = strrchr(processPath, '\\');
  if (backslashPos) {
    procStart = backslashPos + 1; // Add one to omit the slash
  }
  TCHAR* procEnd = strstr(processPath, ".exe");
  int end = strlen(procStart);
  if (strstr(processPath, ".exe")) {
    end -= 4;
  }

  for (int charIndex = 0; charIndex < min(end, BUFFER_LEN-1); ++ charIndex) {
    (*processName)[charIndex] = *(procStart + charIndex);
  }
  (*processName)[max(0, min(strlen(procStart)-4, BUFFER_LEN))] = '\0'; // Subtract 4 for the ".exe"
  // ** Get window title **
  returnCode |= GetWindowTextA(*foregroundWindow, *windowTitle, BUFFER_LEN);
  return returnCode;
}

int initTcp(const TCHAR* host, const TCHAR* port, SOCKET* ConnectSocket) {
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
    printf("Error at socket(): %d\n", WSAGetLastError());
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


  printf("TCP connection successful.\n");
  return 0;
}

int sendTCP(SOCKET sock, TCHAR* msg) {
  printf("Sending: '%s'\n", msg);
  // TCHAR* msg = "hello world";
  int iResult = send(sock, msg, (int)strlen(msg), 0);
  // int iResult = WSAGetLastError();
  if (iResult == SOCKET_ERROR) {
    printf("send failed: %d\n", iResult);
    closesocket(sock);
    WSACleanup();
    return 1;
  }
  return 0;

  // do {
  //   iResult = recv(sock, recvbuf, sizeof(recvbuf), 0);
  //   if (iResult > 0) {
  //     printf("Bytes received: %d\n", iResult);
  //   } else if (iResult == 0) {
  //     printf("Connection closed\n");
  //   } else {
  //     printf("recv failed: %d\n", WSAGetLastError());
  //   }
  // } while (iResult > 0);
}

#define LAYER_CHANGE_TEMPLATE "{\"ChangeLayer\":{\"new\":\"%s\"}}"

const int maxProcNameLen = sizeof(TCHAR) * BUFFER_LEN;
const int maxWinTitleLen = sizeof(TCHAR) * BUFFER_LEN;

void loop() {
  SOCKET kanataSocket;
  int iResult = initTcp(hostname, port, &kanataSocket);
  if (iResult == SOCKET_ERROR) {
    printf("TCP connection failed.\n");
    return;
  }

  HWND fg;
  TCHAR* procName = malloc(maxProcNameLen);
  TCHAR* winTitle = malloc(maxWinTitleLen);
  TCHAR* prevProcName = malloc(maxProcNameLen);
  TCHAR* prevWinTitle = malloc(maxWinTitleLen);
  TCHAR* buf = malloc(MAX_LAYER_NAME_LENGTH + strlen(LAYER_CHANGE_TEMPLATE));
  while(TRUE) {
    if (getForegroundWindowInfo(&fg, &procName, &winTitle) && (strcmp(winTitle, prevWinTitle) != 0)) {
      printf("HWND: '%p',\tTitle: '%s',\tProc: '%s'\n", fg, winTitle, procName);
      sprintf_s(buf, MAX_LAYER_NAME_LENGTH + strlen(LAYER_CHANGE_TEMPLATE), LAYER_CHANGE_TEMPLATE, procName);
      iResult = sendTCP(kanataSocket, buf);
      if (iResult) {
        printf("Socket error. Attempting to reconnect.\n");
        initTcp("localhost", "1337", &kanataSocket);
      }
      strcpy_s(prevWinTitle, maxWinTitleLen, winTitle);
    }
    // Sleep(1);
  }
  free(procName);
  free(winTitle);
  free(prevProcName);
  free(prevWinTitle);
  free(buf);
}

int main(int argc, TCHAR *argv[]) {
  printf("Starting...\n");
  getLayerNames("../murphpad_kanata.kbd");
  loop();
  return 0;
}
