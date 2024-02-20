#pragma once
#include <windows.h>
int getLayerNames(const TCHAR *configPath);
int checkLayer(const TCHAR *layerName);
int initTcp(const TCHAR *host, const TCHAR *port, SOCKET *ConnectSocket);
int sendTCP(SOCKET sock, const TCHAR *msg);
void loop(const TCHAR *hostname, const TCHAR *port, const TCHAR *baseLayer);
