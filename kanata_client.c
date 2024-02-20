#define WIN32_LEAN_AND_MEAN
#include "utils.c"
#include <Psapi.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Needs to be linked to this. With mingw64, it's as simple as adding -lWs2_32
// to the compile command
#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 256
const int maxWinTitleLen = sizeof(TCHAR) * BUFFER_LEN;
const int maxProcNameLen = sizeof(TCHAR) * BUFFER_LEN;

#define MAX_LAYERS 25                   // Kanata default max
#define MAX_LAYER_NAME_LENGTH 64        // Seems reasonable?
#define MAX_CONFIG_FILE_LINE_LENGTH 256 // Seems reasonable?
TCHAR layerNames[MAX_LAYERS][MAX_LAYER_NAME_LENGTH];
int layerCount = 0;

int getLayerNames(const TCHAR *configPath) {
  // Layer definitions in the config file are assumed to start with this string.
  const TCHAR *layerStartStr = "(deflayer ";

  // Open the config file
  FILE *fptr;
  fopen_s(&fptr, configPath, "r");

  // Buffer to read lines into
  TCHAR buf[MAX_CONFIG_FILE_LINE_LENGTH];
  // Index of the current layer. Incremented each time a new layer is read.
  int layerNum = 0;
  // Read the file line line at a time
  while (fgets(buf, MAX_CONFIG_FILE_LINE_LENGTH, fptr)) {
    TCHAR *pos = strstr(buf, layerStartStr);
    // Check that:
    // 1: layerStartStr is found at the start of the line
    // 2. The line continues past layerStartStr (add 1 to account for newline)
    if (pos == buf && strlen(buf) > strlen(layerStartStr) + 1) {
      int layerNameLength = strlen(buf) - strlen(layerStartStr) - 1;
      strcpy_s(layerNames[layerNum], MAX_LAYER_NAME_LENGTH, buf + strlen(layerStartStr));
      // Set the proper character to the null byte to indicate the end of the string.
      layerNames[layerNum][min(layerNameLength, MAX_LAYER_NAME_LENGTH)] = '\0';
      ++layerNum;
    }
  }
  layerCount = layerNum;
  // Zero out unused entries in layerNames
  while (layerNum < MAX_LAYERS) {
    layerNames[layerNum][0] = '\0';
    ++layerNum;
  }
  printf("%d layers found in %s.\n", layerCount, configPath);
  for (int i = 0; i < layerCount; ++i) {
    printf("Layer %d: '%s'\n", i, layerNames[i]);
  }

  return 0;
}

int checkLayer(const TCHAR *layerName) {
  for (int i = 0; i < layerCount; ++i) {
    if (strcmp(layerName, layerNames[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

int initTcp(const TCHAR *host, const TCHAR *port, SOCKET *ConnectSocket) {
  printf("Connecting to %s:%s...\n", host, port);

  int iResult;
  WSADATA wsaData;
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
  }

  struct addrinfo *result = NULL, *ptr = NULL, hints;
  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  iResult = getaddrinfo(host, port, &hints, &result);

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

  TCHAR buf[BUFFER_LEN];
  buf[0] = '\0';
  iResult = recv(*ConnectSocket, buf, BUFFER_LEN, 0);
  if (iResult > 0) {
    printf("TCP connection successful.\n");
    printf("'%s'\n", buf);
  } else if (iResult == 0) {
    printf("Connection closed\n");
  } else {
    printf("recv failed: %d\n", WSAGetLastError());
  }

  return 0;
}

int sendTCP(SOCKET sock, TCHAR *msg) {
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
}

#define LAYER_CHANGE_TEMPLATE "{\"ChangeLayer\":{\"new\":\"%s\"}}"

void loop(const TCHAR *hostname, const TCHAR *port, const TCHAR *baseLayer) {
  SOCKET kanataSocket;
  int iResult = initTcp(hostname, port, &kanataSocket);
  if (iResult == SOCKET_ERROR) {
    printf("TCP connection failed.\n");
    return;
  }

  HWND fg;
  TCHAR procName[maxProcNameLen];
  TCHAR winTitle[maxWinTitleLen];
  TCHAR prevProcName[maxProcNameLen];
  TCHAR prevWinTitle[maxWinTitleLen];
  TCHAR *buf = malloc(MAX_LAYER_NAME_LENGTH + strlen(LAYER_CHANGE_TEMPLATE));
  const TCHAR *activeLayerName;
  while (TRUE) {
    // If the title of the active window changes
    if (getForegroundWindowInfo(&fg, &procName[0], &winTitle[0]) && (strcmp(winTitle, prevWinTitle) != 0)) {
      printf("New window activated: hWnd='%p',\tTitle='%s',\tProcess='%s'\n", fg, winTitle, procName);

      // If the window process (minus .exe) is in the list of layer names,
      // activate that layer, otherwise activate baseLayer
      if (checkLayer(procName)) {
        activeLayerName = procName;
      } else {
        activeLayerName = baseLayer;
      }
      sprintf_s(buf, MAX_LAYER_NAME_LENGTH + strlen(LAYER_CHANGE_TEMPLATE), LAYER_CHANGE_TEMPLATE, activeLayerName);
      iResult = sendTCP(kanataSocket, buf);
      if (iResult) {
        printf("Socket error. Attempting to reconnect.\n");
        initTcp(hostname, port, &kanataSocket);
      }
      strcpy_s(prevWinTitle, maxWinTitleLen, winTitle);
    }
    Sleep(10);
  }
  free(buf);
}

int main(int argc, TCHAR *argv[]) {
  // The hostname of the TCP server Kanata is running on. I can't imagine ever
  // needing this to be anything but localhost.
  TCHAR hostname[BUFFER_LEN] = "localhost";
  // The port of Kanata's TCP server. You may want to put Kanata on something
  // more obscure than 80.
  TCHAR port[BUFFER_LEN] = "80";
  // The name of the default/base layer in your Kanata config. If the active
  // window's process isn't found in your list of layer names, this layer will
  // be activated instead.
  TCHAR defaultLayer[MAX_LAYER_NAME_LENGTH] = "default";
  for (int i = min(1, argc); i < argc; ++i) {
    if (strchr(argv[i], '-') != argv[i]) {
      printf("ERROR: Invalid syntax.\n");
      return 1;
    } else if (strstr(argv[i], "--hostname=") == argv[i]) {
      sscanf_s(argv[i], "--hostname=%s", hostname);
    } else if (strstr(argv[i], "--port=") == argv[i]) {
      sscanf_s(argv[i], "--port=%s", port);
    } else if (strstr(argv[i], "--default-layer=") == argv[i]) {
      sscanf_s(argv[i], "--default-layer=%s", defaultLayer);
    } else {
      printf("Invalid argument.\n");
      return 1;
    }
  }
  printf("Starting...\n");
  getLayerNames("../murphpad_kanata.kbd");
  loop(hostname, port, defaultLayer);
  return 0;
}
