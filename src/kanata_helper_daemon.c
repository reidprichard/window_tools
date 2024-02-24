/*! file kanata_helper_daemon.c
 * \brief A simple Windows daemon that enables application-specific keybinds in Kanata.
 *
 *  This program monitors the active window and correspondingly requests layer
 *  changes from Kanata (over TCP). The daemon gets the name of the process
 *  that spawned the active window, removes the ".exe" from the end, and
 *  attempts to activate a layer with that name.
 *  For example, if you activated your Firefox window, it would send the
 *  following message to Kanata: {"ChangeLayer":{"new":"firefox"}. Process
 *  names are not always obvious, but when this program is running it will
 *  print the active window's process name to the console. Spaces in process
 *  names will be replaced with underscores.
 *  By default, it attempts to connect to Kanata at localhost:80, but the host
 *  and port can both be changed using command line arguments.
 *  If the users's .kbd config file is specified, all layer names will be
 *  extracted from the file. Activating a window without a corresponding layer
 *  will result in a "base" layer being activated. This layer is named
 *  "default" by default, but it can be changed with a command line argument.
 *
 * */

// TODO: Set up CMake or make or something.
// TODO: Add system tray icon so this can be run in the background

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <Psapi.h>
#include <minwindef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "kanata_helper_daemon.h"
#include "utils.h"

#ifdef CMAKE
#include "window_tools.h"
#else
#define window_tools_VERSION_MAJOR 0
#define window_tools_VERSION_MINOR 0
#define window_tools_VERSION_PATCH 0
#endif

// Needs to be linked to Ws2_32. With mingw64, it's as simple as adding -lWs2_32
// to the compile command. CMake should handle it automatically with the following.
#pragma comment(lib, "Ws2_32.lib")

#define BUFFER_LEN 256
const int maxWinTitleLen = sizeof(TCHAR) * BUFFER_LEN;
const int maxProcNameLen = sizeof(TCHAR) * BUFFER_LEN;

// The max number of layers in a configuration. 25 is Kanata's default max as of 2024-2-20.
#define MAX_LAYERS 25
// The max length of a layer name.
#define MAX_LAYER_NAME_LENGTH 64
// The max number of characters in a single line of the configuration file.
#define MAX_CONFIG_FILE_LINE_LENGTH 1024

// A list of layer names found in the Kanata .kbd config file. This would
// eventually be better as a hash-based type.
TCHAR layerNames[MAX_LAYERS][MAX_LAYER_NAME_LENGTH];
// The number of layers found in the Kanata .kbd config file.
int layerCount = 0;

/**
 * @brief Gets a list of names of layers in a Kanata (or potentially Kmonad) .kbd config file.
 *
 * @param[in] configPath The path to the config file.
 * @return 0 if successful, 1 if failed.
 */
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

  return (layerCount != 0);
}

/**
 * @brief Checks if a layer name exists in the previously-read Kanata config file.
 *
 * @param[in] layerName The layer name to be checked.
 * @return 1 if the layer name was in the config file, 0 if it was not.
 */
int checkLayer(const TCHAR *layerName) {
  for (int i = 0; i < layerCount; ++i) {
    if (strcmp(layerName, layerNames[i]) == 0) {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief Initializes a TCP connection.
 *
 * @param[in] host The hostname or address to connect to.
 * @param[in] port The port number to connect to.
 * @param[out] ConnectSocket The TCP socket.
 * @return 0 if successful, nonzero if failed.
 */
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
    // printf("'%s'\n", buf);
  } else if (iResult == 0) {
    printf("Connection closed\n");
    return 1;
  } else {
    printf("recv failed: %d\n", WSAGetLastError());
    return 1;
  }

  return 0;
}

/**
 * @brief Sends data over TCP to the given socket.
 *
 * @param[in] sock The socket to use.
 * @param[in] msg The message to be sent.
 * @return 0 if successful, nonzero if failed.
 */
int sendTCP(SOCKET sock, const TCHAR *msg) {
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

// The template for a TCP message to request a layer change.
#define LAYER_CHANGE_TEMPLATE "{\"ChangeLayer\":{\"new\":\"%s\"}}"

/**
 * @brief The function's main loop. When the focused window changes, sends a
 * TCP message to the socket requesting a layer change. If the connection dies,
 * attempts to reinitialize the socket.
 *
 * @param[in] hostname The hostname that Kanata is listening on. I cannot
 * imagine this needing to be anything but "localhost" or "127.0.0.1".
 * @param[in] port The port that Kanata is listening on.
 * @param[in] baseLayer The name of the Kanata layer that should be used if the
 * current application's process name does not match a layer name in the config
 * file.
 */
void loop(const TCHAR *hostname, const TCHAR *port, const TCHAR *baseLayer) {
  SOCKET kanataSocket;
  int iResult = initTcp(hostname, port, &kanataSocket);
  if (iResult == SOCKET_ERROR) {
    printf("TCP connection failed.\n");
    return;
  }

  HWND fg;
  TCHAR procName[sizeof(TCHAR) * BUFFER_LEN];
  TCHAR winTitle[sizeof(TCHAR) * BUFFER_LEN];
  TCHAR prevProcName[sizeof(TCHAR) * BUFFER_LEN];
  TCHAR prevWinTitle[sizeof(TCHAR) * BUFFER_LEN];
  TCHAR *buf = malloc(MAX_LAYER_NAME_LENGTH + strlen(LAYER_CHANGE_TEMPLATE));
  const TCHAR *activeLayerName;
  while (TRUE) {
    // If the title of the active window changes
    // TODO: If I'm not going to do anything with the window title, need this to just use the process name.
    if (getForegroundWindowInfo(&fg, &procName[0], &winTitle[0], BUFFER_LEN) && (strcmp(winTitle, prevWinTitle) != 0)) {
      for (int i = 0; i < strlen(procName); ++i) {
        if (procName[i] == ' ') {
          procName[i] = '_';
        }
      }
      printf("New window activated: hWnd='%p',\tTitle='%s',\tProcess='%s'\n", fg, winTitle, procName);

      // If the window process (minus .exe) is in the list of layer names,
      // or if layerCount is 0 (indicating --config-file wasn't specified),
      // activate that layer, otherwise activate baseLayer
      if (layerCount == 0 || checkLayer(procName)) {
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
  TCHAR portNumber[BUFFER_LEN] = "80";
  TCHAR configFileName[MAX_PATH];
  configFileName[0] = '\0';

  // The name of the default/base layer in your Kanata config. If the active
  // window's process isn't found in your list of layer names, this layer will
  // be activated instead.
  TCHAR defaultLayer[MAX_LAYER_NAME_LENGTH] = "default";

  const TCHAR *helpMessage = "Usage:\n"
                             "  ./kanata_client.exe [options]\tStart daemon\n"
                             "\n"
                             "Options:\n"
                             "  --config-file=<path>,          The path to your Kanata .kbd config file.\n"
                             "  -c <path>                      If your configuration is not a single file\n"
                             "                                 (i.e., you use 'include'), the included files\n"
                             "                                 will not be parsed.\n"
                             "\n"
                             "  --default-layer=<layer name>,  The name of your base layer in Kanata. If a\n"
                             "  -d <layer name>                program does not match one of your layer names,\n"
                             "                                 this layer will be used. You must also specify\n"
                             "                                 --config-file. (default: 'default')\n"
                             "\n"
                             "  --hostname=<hostname>,         The hostname Kanata's TCP server is listening on.\n"
                             "  -H <hostname>                  You should not need to change this.\n"
                             "                                 (default: 'localhost')\n"
                             "\n"
                             "  --port=<port number>,          The port Kanata is listening on. (default: '80')\n"
                             "  -p <port number>\n";

  // TODO: Move to an actual argument parser
  for (int i = min(1, argc); i < argc; ++i) {
    if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      printf("%s\n", helpMessage);
      return 0;
    }
    else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
      printf("kanata_helper_daemon version %d.%d.%d\n", window_tools_VERSION_MAJOR, window_tools_VERSION_MINOR, window_tools_VERSION_PATCH);
      return 0;
    } else if (strstr(argv[i], "--config-file=") == argv[i]) {
      sscanf_s(argv[i], "--config-file=%s", configFileName, (unsigned) sizeof(configFileName));
    } else if (strstr(argv[i], "--default-layer=") == argv[i]) {
      sscanf_s(argv[i], "--default-layer=%s", defaultLayer, (unsigned) sizeof(defaultLayer));
    } else if (strstr(argv[i], "--hostname=") == argv[i]) {
      sscanf_s(argv[i], "--hostname=%s", hostname, (unsigned) sizeof(hostname));
    } else if (strstr(argv[i], "--port=") == argv[i]) {
      sscanf_s(argv[i], "--port=%s", portNumber, (unsigned) sizeof(portNumber));
    } else if (strcmp(argv[i], "-c") == 0) {
      ++i;
      if (i < argc) {
        strcpy_s(configFileName, sizeof(configFileName), argv[i]);
      } else {
        printf("You must specify an argument value for '%s'\n", argv[i - 1]);
        return 1;
      }
    } else if (strcmp(argv[i], "-d") == 0) {
      ++i;
      if (i < argc) {
        strcpy_s(defaultLayer, sizeof(defaultLayer), argv[i]);
      } else {
        printf("You must specify an argument value for '%s'\n", argv[i - 1]);
        return 1;
      }
    } else if (strcmp(argv[i], "-H") == 0) {
      ++i;
      if (i < argc) {
        strcpy_s(hostname, sizeof(hostname), argv[i]);
      } else {
        printf("You must specify an argument value for '%s'\n", argv[i - 1]);
        return 1;
      }
    } else if (strcmp(argv[i], "-p") == 0) {
      ++i;
      if (i < argc) {
        strcpy_s(portNumber, sizeof(portNumber), argv[i]);
      } else {
        printf("You must specify an argument value for '%s'\n", argv[i - 1]);
        return 1;
      }
    } else {
      printf("ERROR: Invalid argument: '%s'. See --help for usage.\n", argv[i]);
      return 1;
    }
  }
  printf("Starting...\n");
  if (strlen(configFileName) > 0) {
    getLayerNames(configFileName);
  }
  loop(hostname, portNumber, defaultLayer);
  return 0;
}
