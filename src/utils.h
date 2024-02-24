#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/**
 * @brief Sets the focused window.
 *
 * @param[in] window The handle of the window to be focused.
 * @return 0 if failed, nonzero if successful.
 */
int forceSetForegroundWindow(const HWND hWnd);
/**
 * @brief Convenience wrapper around forceSetForegroundWindow.
 *
 * @param[in] hWnd The handle of the window to be focused.
 * @return 0 if failed, nonzero if successful.
 */
int activateWindowByHandle(const HWND hWnd);
/**
 * @brief Finds the window with the specified title and focuses it.
 *
 * @param[in] windowTitle The title of the window to be focused.
 * @return 0 if failed, nonzero if successful.
 */
int activateWindowByTitle(const TCHAR *windowTitle);
/**
 * @brief Finds the handle, process name, and window title of the active window.
 *
 * @param[out] foregroundWindow The handle to the foreground window.
 * @param[out] processName The name of the process that spawned the foreground window.
 * @param[out] windowTitle The title of the foreground window.
 * @return 0 if failed, nonzero if successful.
 */
int getForegroundWindowInfo(HWND *foregroundWindow, TCHAR *processName, TCHAR *windowTitle, int bufferLen);
