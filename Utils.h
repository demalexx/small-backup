#ifndef _UTILS_H
#define _UTILS_H

#include <windows.h>
#include <Shlwapi.h>

#include "CommonClasses.h"

bool StrToBool(LPCTSTR cValue, bool bDefault = false);

SYSTEMTIME StrToSysTime(CString sTime, bool bTime = true, bool bDate = false);
CString SysTimeToStr(const SYSTEMTIME& SysTime, bool bTime = true);

class FormatC
{
private:
	TCHAR buf[1024];

public:
	explicit FormatC(LPTSTR cStr, ...);
	operator LPCTSTR () const { return buf; }
};

CString FormatDateTime(LPCTSTR cFormat);

//string AddSlash(const string& sPath);
void AddSlash(CString& sPath);
void DelSlash(CString& sPath);

bool ForceDirectories(LPCTSTR cPath);

// Проверяет является ли папка 2 подпапкой папки 1
bool IsSubFolder(CString sFolder1, CString sFolder2);

WORD DialogToScreen(HWND hDialog, WORD wValue, bool bHoriz = true);

CString GetDlgItemStr(HWND hDialog, int nIDDlgItem);

WORD GetPathPart(CString sPath, WORD wStart);
bool IsPathPartValid(CString sPart);
bool IsPathValid(CString sPath);

// Преобразует день недели в строку согласно установкам пользователя
CString DayOfWeekToStr(BYTE bDay);

// Форматирует размер, добавляя ед. измерения (x.x байт, x.x КБ, x.x Мб, ...)
CString FormatSize(__int64 unsigned i64Size, LPTSTR Bytes = TEXT("Байт"), LPTSTR Kb = TEXT("Кб"), 
				   LPTSTR Mb = TEXT("Мб"), LPTSTR Gb = TEXT("Гб"), LPTSTR Tb = TEXT("Тб"));

#endif