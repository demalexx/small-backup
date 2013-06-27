#include "Utils.h"

bool StrToBool(LPCTSTR cValue, bool bDefault)
{
	LPTSTR value = new TCHAR[lstrlen(cValue) + sizeof(TCHAR)];
	lstrcpy(value, cValue);
	CharLowerBuff(value, lstrlen(value));

	bool res;

	if (lstrcmp(cValue, TEXT("1")) == 0 || lstrcmp(value, TEXT("true")) == 0)
	{
		res = true;
	}
	else if (lstrcmp(cValue, TEXT("0")) == 0 || lstrcmp(value, TEXT("false")) == 0)
		res = false;
	else
		res = bDefault;

	delete value;
	return res;
}

SYSTEMTIME StrToSysTime(CString sTime, bool bTime, bool bDate)
{
	SYSTEMTIME res;
	GetLocalTime(&res);

	if ((bTime && bDate && sTime.Length() < 14) ||
		(bTime && !bDate && sTime.Length() < 6) ||
		(!bTime && bDate && sTime.Length() < 8))
	{
		return res;
	}

	if (bTime)
	{
		res.wHour = sTime.SubStr(0, 2).ToInt();
		res.wMinute = sTime.SubStr(2, 2).ToInt();
		res.wSecond = sTime.SubStr(4, 2).ToInt();

		if (bDate)
		{
			res.wDay = sTime.SubStr(6, 2).ToInt();
			res.wMonth = sTime.SubStr(8, 2).ToInt();
			res.wYear = sTime.SubStr(10, 4).ToInt();
		}
	}
	else if (!bTime && bDate)
	{
		res.wDay = sTime.SubStr(0, 2).ToInt();
		res.wMonth = sTime.SubStr(2, 2).ToInt();
		res.wYear = sTime.SubStr(4, 4).ToInt();
	}

	return res;
}

CString SysTimeToStr(const SYSTEMTIME& SysTime, bool bTime)
{
	if (bTime)
	{
		TCHAR cTime[MAX_PATH];
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &SysTime, NULL, cTime, MAX_PATH);

		return cTime;
	}
	else
	{
		TCHAR cDate[MAX_PATH];
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE, &SysTime, NULL, cDate, MAX_PATH);

		return cDate;
	}
}

FormatC::FormatC(LPTSTR cStr, ...)
{
	va_list val;
	va_start(val, cStr);

	wvsprintf(buf, cStr, val);

	va_end(val);
}

CString FormatDateTime(LPCTSTR cFormat)
{
	TCHAR res[MAX_PATH] = {0};

	WORD wPos = 0;
	WORD wLen = lstrlen(cFormat);

	WORD wResPos = 0;

	SYSTEMTIME stNow;
	GetLocalTime(&stNow);

	while (wPos < wLen)
	{
		if (cFormat[wPos] != TEXT('%'))
		{
			res[wResPos] = cFormat[wPos];
			wPos++;
			wResPos++;
		}
		else if (cFormat[wPos] == TEXT('%'))
		{
			TCHAR cMacro[MAX_PATH] = {0};

			wPos++;
			WORD wMacroStart = wPos;

			while (wPos < wLen)
			{
				if (cFormat[wPos] != TEXT('%'))
					wPos++;
                else if (cFormat[wPos] == TEXT('%'))
				{	   
					CopyMemory(cMacro, (void*) &cFormat[wMacroStart], (wPos - wMacroStart) * sizeof(TCHAR));

					TCHAR cTime[MAX_PATH];
					
					// Форматируем время
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stNow, cMacro, cTime, MAX_PATH);

					// Узнаем как пишутся AM и PM
					TCHAR AMPM[] = TEXT("AM");
					GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stNow, TEXT("tt"), AMPM, 2);

					// Находим "PM", "AM" и заменяем другими символами
					for (WORD i = 0; i < lstrlen(cTime) - 1; i++)
						if ((cTime[i] == AMPM[0]) && cTime[i + 1] == AMPM[1])
						{
							cTime[i + 1] = 1;
							break;
						}

					TCHAR cDateTime[MAX_PATH];

					// Форматируем дату
					GetDateFormat(LOCALE_USER_DEFAULT, 0, &stNow, cTime, cDateTime, MAX_PATH);

					// Возвращаем "AM"\"PM"
					for (WORD i = 0; i < lstrlen(cDateTime); i++)
						if (cDateTime[i] == 1)
						{
							cDateTime[i] = AMPM[1];
							break;
						}

					lstrcat(res, cDateTime);

					wResPos = lstrlen(res);
                    wPos++;

					break;
				}
			}
		}
	}

    return res;
}

void AddSlash(CString& sPath)
{
	if (sPath[sPath.Length() - 1] != TEXT('\\'))
		sPath += TEXT("\\");
}

void DelSlash(CString& sPath)
{
	if (sPath[sPath.Length() - 1] == TEXT('\\'))
		sPath.Delete(sPath.Length() - 1, 1);
}

bool ForceDirectories(LPCTSTR cPath)
{
	if (PathFileExists(cPath))
		return true;

	for (int i = lstrlen(cPath); i >= 0; i--)
		if (cPath[i] == TEXT('\\'))
		{
			TCHAR buf[MAX_PATH];
			lstrcpyn(buf, cPath, i + 1);

			if (ForceDirectories(buf))
				return CreateDirectory(cPath, NULL);
		}	

	return true;
}

bool IsSubFolder(CString sFolder1, CString sFolder2)
{
	// Проверят является ли папка 2 подпапкой папки 1 ('c:\' и 'c:\backup')
	DelSlash(sFolder1);

	if (sFolder2.Length() >= sFolder1.Length())
	{
		return (sFolder2.SubStr(0, sFolder1.Length()).ToLowerNew() == sFolder1.ToLowerNew());
	}

	return false;
}

WORD DialogToScreen(HWND hDialog, WORD wValue, bool bHoriz)
{
	RECT rc = {0};
	if (bHoriz) rc.right = wValue;
	else rc.bottom = wValue;

	MapDialogRect(hDialog, &rc);

	if (bHoriz) return rc.right;
	else return rc.bottom;
}

CString GetDlgItemStr(HWND hDialog, int nIDDlgItem)
{
	CString res;
	GetDlgItemText(hDialog, nIDDlgItem, res.Buf(), MAX_PATH);

	return res;
}

WORD GetPathPart(CString sPath, WORD wStart)
{
	WORD wLen = sPath.Length();
	WORD wPos = wStart;

	WORD res;
	while (wPos < wLen)
	{
		if (sPath[wPos] == TEXT('\\'))
		{
			return wPos;
			break;
		}
		wPos++;
	}

	return wLen;
}

bool IsPathPartValid(CString sPart)
{
	CString slPart = sPart;
	if (slPart.Empty()) return false;

	BYTE bLen = sPart.Length();

	// Начальные и конечные пробелы
	if (slPart[0] == TEXT(' ') || slPart[0] == TEXT('\t') || slPart[bLen - 1] == TEXT(' ') || slPart[bLen - 1] == TEXT('\t'))
		return false;

	for (BYTE i = 0; i < sPart.Length(); i++)
	{
		if ((PathGetCharType(sPart[i]) & GCT_INVALID) != 0 || (PathGetCharType(sPart[i]) & GCT_SEPARATOR) != 0 ||
			(PathGetCharType(sPart[i]) & GCT_WILD) != 0)
			return false;
	}
	return true;
}

bool IsPathValid(CString sPath)
{
	CString sLoPath = sPath.ToLowerNew();
	DelSlash(sLoPath);

	WORD wPos = 0;
	WORD wStart = 0;

	// Начало пути 'c:\...' или '\\server\c\...'
	if (sLoPath.Length() >= 2)
	{
		// '\\server...'
		if (sLoPath[0] == TEXT('\\') && sLoPath[1] == TEXT('\\'))
		{
			wPos = 2;
			wStart = 2;
		}
		// 'c:\...'
		else if (sLoPath[1] == TEXT(':') && (sLoPath[0] >= TEXT('a') && sLoPath[0] <= TEXT('z')))
		{
			if (sLoPath.Length() == 2)
				return true;
			else if (sLoPath[2] != TEXT('\\'))
				return false;

			wPos = 3;
			wStart = 3;
		}
	}

	// Относительный путь
	while (wPos < sLoPath.Length())
	{
		if (sLoPath[wPos] == TEXT('\\') || wPos == sLoPath.Length() - 1)
		{
			CString sPart;

			if (wPos == sLoPath.Length() - 1)
				sPart.Assign(sLoPath.GetMemory() + wStart, wPos - wStart + 1);
			else
				sPart.Assign(sLoPath.GetMemory() + wStart, wPos - wStart);

			if (!IsPathPartValid(sPart))
				return false;

			wPos++;
			wStart = wPos;				
		}
		else
			wPos++;
	}

	return true;
}

CString DayOfWeekToStr(BYTE bDay)
{
	CString res;
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDAYNAME1 + bDay - 1, res.Buf(64), 64);	
	return res;
}

CString FormatSize(__int64 unsigned i64Size, LPTSTR Bytes, LPTSTR Kb, LPTSTR Mb, LPTSTR Gb, LPTSTR Tb)
{
	CString res;
	float fSize;	// размер с целой и дробной частями (xx.xx)
	LONG iiSize;	// значение до точки
	LONG ifSize;	// значение после точки (дробная часть) в виде целого числа

	if (i64Size < 1024)
		res = FormatC(TEXT("%I64d %s"), i64Size, Bytes);
	else if (i64Size >= (__int64) 1024 && i64Size < (__int64) 1024 * 1024)
	{
		fSize = (float) i64Size / 1024;
		iiSize = (int) i64Size / 1024;
		VarI4FromR4((fSize - iiSize) * 100, &ifSize);

        res = FormatC(TEXT("%d.%.2d %s"), iiSize, ifSize, Kb);		
	}
	else if (i64Size >= (__int64) 1024 * 1024 && i64Size < (__int64) 1024 * 1024 * 1024)
	{
		fSize = (float) i64Size / (1024 * 1024);
		iiSize = (int) i64Size / (1024 * 1024);
		VarI4FromR4((fSize - iiSize) * 100, &ifSize);

        res = FormatC(TEXT("%d.%.2d %s"), iiSize, ifSize, Mb);
	}
	else if (i64Size >= (__int64) 1024 * 1024 * 1024 && i64Size < (__int64) 1024 * 1024 * 1024 * 1024)
	{
		fSize = (float) i64Size / ((__int64) 1024 * 1024 * 1024);
		iiSize = i64Size / ((__int64) 1024 * 1024 * 1024);
		VarI4FromR4((fSize - iiSize) * 100, &ifSize);

        res = FormatC(TEXT("%d.%.2d %s"), iiSize, ifSize, Gb);
	}
	else if (i64Size >= (__int64) 1024 * 1024 * 1024 * 1024 && i64Size < (__int64) 1024 * 1024 * 1024 * 1024 * 1024)
	{
		fSize = (float) i64Size / ((__int64) 1024 * 1024 * 1024 * 1024);
		iiSize = i64Size / ((__int64) 1024 * 1024 * 1024 * 1024);
		VarI4FromR4((fSize - iiSize) * 100, &ifSize);

        res = FormatC(TEXT("%d.%.2d %s"), iiSize, ifSize, Tb);
	}

	return res;
}