#include "Instance.h"

HINSTANCE	Instance;
HWND		MainWindow;

LPCTSTR ThreadUpdateMessage = TEXT("WM_ThreadUpdateMessage");
LPCTSTR ThreadStringMessage = TEXT("WM_ThreadStringMessage");
LPCTSTR ThreadDoneMessage = TEXT("WM_ThreadDoneMessage");