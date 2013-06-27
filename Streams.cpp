#include "Streams.h"

///////////////////////////////////////////////////////////////////////////////
// CMemInUStream
///////////////////////////////////////////////////////////////////////////////
CMemInUStream::CMemInUStream()
{
    Constructor();
}

CMemInUStream::CMemInUStream(LPWSTR sFileName)
{
    Constructor();
    LoadFile(sFileName);
}

CMemInUStream::CMemInUStream(CString sFileName)
{
    Constructor();
    LoadFile(sFileName.C());
}

CMemInUStream::~CMemInUStream()
{
    if (m_pMemory != NULL)
        HeapFree(GetProcessHeap(), 0, m_pMemory);
}

bool CMemInUStream::LoadFile(LPWSTR sFileName)
{
    HANDLE hFile = CreateFileW(sFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, 0, NULL);

    if (hFile == INVALID_HANDLE_VALUE) 
        return false;

    DWORD dwFileSize = GetFileSize(hFile, NULL);

    if (dwFileSize == 0)
    {
        CloseHandle(hFile);
        return true;
    }

    // Добавим к размеру файла ещё два символа - новую строку \r\n
    // и плюс один - null-терминатор
    Resize(dwFileSize / sizeof(WCHAR) + 2 + 1);

    DWORD dwRed;
    // Неудачное чтение - выйдем из ф-ии
    if (!ReadFile(hFile, m_pMemory, dwFileSize, &dwRed, NULL))
    {
        CloseHandle(hFile);
        return false;
    }

    // Проверим, что файл в "правильном" формате (посмотри на BOM)
    if ((DWORD) m_pMemory[0] != 0xFEFF)
        return false;

    m_dwPos = 1;

    // Длина данных в памяти - размер файла / 2 плюс два символа на новую строку \r\n
    m_dwLength = dwFileSize / sizeof(WCHAR) + 2;

    m_pMemory[m_dwLength - 2] = L'\r';
    m_pMemory[m_dwLength - 1] = L'\n';
    m_pMemory[m_dwLength] = 0;

    return true;
}

bool CMemInUStream::LoadFile(CString sFileName)
{
    return LoadFile(sFileName.C());
}
                             
bool CMemInUStream::Eof()
{
    return (m_dwPos >= m_dwLength - 1);
}

CString CMemInUStream::ReadLine()
{
    DWORD dwStart = m_dwPos;
    while (m_dwPos < m_dwLength - 1)
    {
        if (m_pMemory[m_dwPos] == L'\r' && m_pMemory[m_dwPos + 1] == L'\n')
        {
            m_dwPos++;
            break;
        }

        m_dwPos++;
    }

    DWORD dwLineLength = m_dwPos - dwStart;
    if (dwLineLength == 0)
        return CString();

    WCHAR* sLine = new WCHAR[dwLineLength];
    lstrcpynW(sLine, m_pMemory + dwStart, dwLineLength);

    CString res(sLine);
    delete sLine;
    m_dwPos++;

    return res;
}

void CMemInUStream::Constructor()
{
    m_pMemory = NULL;
    m_dwMemSize = 0;
	m_dwLength = 0;
	m_dwPos = 0;
}

void CMemInUStream::Resize(DWORD dwNewLength)
{
    // Выделим память только если требуется больше чем уже выделено
	if (dwNewLength * 2 > m_dwMemSize)
	{
        // Подсчитаем длину памяти в символах, что бы она было кратна MEMSTREAMBLOCK
        m_dwMemSize = MEMSTREAMBLOCK * ((dwNewLength * 2) / MEMSTREAMBLOCK + 1);

        // Выделим память
        if (m_pMemory == NULL)
            m_pMemory = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, m_dwMemSize);
        else
            m_pMemory = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, m_pMemory, m_dwMemSize);
	}
}

///////////////////////////////////////////////////////////////////////////////
// CMemOutUStream
///////////////////////////////////////////////////////////////////////////////
CMemOutUStream::CMemOutUStream()
{
    Constructor();
}

CMemOutUStream::CMemOutUStream(LPWSTR sFileName)
{
    Constructor(sFileName);
}

CMemOutUStream::CMemOutUStream(CString sFileName)
{
    Constructor(sFileName.C());
}

CMemOutUStream::~CMemOutUStream()
{
    if (m_pMemory != NULL)
        HeapFree(GetProcessHeap(), 0, m_pMemory);
}

void CMemOutUStream::WriteLine(LPWSTR sLine, ...)
{
	va_list val;
	va_start(val, sLine);

    AddLine(sLine, val);

    va_end(val);
}

void CMemOutUStream::WriteLine(CString sLine, ...)
{
	va_list val;
	va_start(val, sLine);

    AddLine(sLine.C(), val);

    va_end(val);
}

bool CMemOutUStream::SaveFile(LPWSTR sFileName)
{
    // Если нет имени файла куда записывать
    if (sFileName == NULL && m_sFileName.Empty())
        return false;

    if (sFileName != NULL)
        m_sFileName = sFileName;

    HANDLE hFile = CreateFile(m_sFileName.C(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    DWORD dwWrote;
    if (!WriteFile(hFile, m_pMemory, m_dwLength * 2, &dwWrote, NULL))
    {
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);
    return true;
}

bool CMemOutUStream::SaveFile(CString sFileName)
{
    if (sFileName.Empty())
        return SaveFile(NULL);    
    else
        return SaveFile(sFileName.C());
}

void CMemOutUStream::Constructor(LPWSTR sFileName)
{
    m_pMemory = NULL;
    m_dwMemSize = 0;
    m_dwLength = 0;
    
    if (sFileName == NULL)
        m_sFileName = L"";
    else
        m_sFileName = sFileName;

    // Запишем в начало файла BOM и null-терминатор
    Resize(2);
    m_pMemory[0] = 0xFEFF;
    m_pMemory[1] = 0;

    m_dwLength = 1;
}

void CMemOutUStream::Resize(DWORD dwNewLength)
{
    // Выделим память только если требуется больше, чем уже выделено
	if (dwNewLength * 2 > m_dwMemSize)
	{
        // Подсчитаем длину памяти в символах, что бы она было кратна MEMSTREAMBLOCK
        m_dwMemSize = MEMSTREAMBLOCK * ((dwNewLength * 2) / MEMSTREAMBLOCK + 1);

        // Выделим память
        if (m_pMemory == NULL)
            m_pMemory = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, m_dwMemSize);
        else
            m_pMemory = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, m_pMemory, m_dwMemSize);
	}
}

void CMemOutUStream::AddLine(LPWSTR sLine, va_list val)
{
    // Добавим в конец строки перевод каретки "\r\n"
    LPWSTR sNewLine = new WCHAR[lstrlen(sLine) + 3];
    lstrcpy(sNewLine, sLine);
    lstrcpy(sNewLine + lstrlen(sLine), L"\r\n");

    // Выделим память под буфер где будет храниться отформатированная строка
    LPWSTR sFormatted = new WCHAR[1024];
    int iFormattedLen = wvsprintf(sFormatted, sNewLine, val);

    // Увеличим размер на вновь добавленную строку и на null-терминатор
    Resize(m_dwLength + iFormattedLen + 1);

    lstrcpyW(m_pMemory + m_dwLength, sFormatted);
    m_dwLength += iFormattedLen;

    delete sFormatted;
    delete sNewLine;
}

///////////////////////////////////////////////////////////////////////////////
// CMemIniOutW
///////////////////////////////////////////////////////////////////////////////
void CMemIniOutW::AddSection(LPWSTR sSection)
{
    WriteLine(L"[ %s ]", sSection);
}

void CMemIniOutW::AddString(LPWSTR sKey, LPWSTR sValue)
{
    if (sValue == NULL)
    {
        WriteLine(L"%s", sKey);
        return;
    }

    CString s(sValue);
    if (s.Spaces())
        WriteLine(L"%s = \"%s\"", sKey, sValue);
    else
        WriteLine(L"%s = %s", sKey, sValue);
}

void CMemIniOutW::AddNumeric(LPWSTR sKey, int iValue)
{
    CString s;
    s.FromInt(iValue);

    AddString(sKey, s.C());
}

void CMemIniOutW::AddBoolean(LPWSTR sKey, bool bValue)
{
    bValue ? 
        AddString(sKey, L"true")
        : AddString(sKey, L"false");
}

///////////////////////////////////////////////////////////////////////////////
// CMemLogUStream
///////////////////////////////////////////////////////////////////////////////
CMemLogUStream::CMemLogUStream()
{
    Constructor();
}

CMemLogUStream::CMemLogUStream(LPWSTR sLogFile, DWORD dwMaxSize, BYTE bAddDateTime)
{
    Constructor(sLogFile, dwMaxSize, bAddDateTime);
}

CMemLogUStream::~CMemLogUStream()
{
}

void CMemLogUStream::WriteLogLine(LPWSTR sLine, ...)
{
    // Для начала добавим в начало строки дату и время, как указано
    // в переменной m_bAddDateTime
    SYSTEMTIME st;
	GetLocalTime(&st);

    LPWSTR sDate = new WCHAR[MAX_PATH];
    LPWSTR sTime = new WCHAR[MAX_PATH];
    LPWSTR sDateTime = new WCHAR[MAX_PATH];

    GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, sDate, MAX_PATH);
    GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, sTime, MAX_PATH);

    if (m_bAddDateTime == LOG_ADDTIME)
    {
        wsprintf(sDateTime, L"[%s] ", sTime);
    }
    else if (m_bAddDateTime == LOG_ADDDATE)
    {
        wsprintf(sDateTime, L"[%s] ", sDate);
    }
    else
    {
        wsprintf(sDateTime, L"[%s %s] ", sDate, sTime);
    }

    LPWSTR sNewLine = new WCHAR[lstrlen(sDateTime) + lstrlen(sLine) + 1];
    lstrcpy(sNewLine, sDateTime);
    lstrcat(sNewLine, sLine);

    // Затем добавим полученную строку в память, вызвав метод
    // базового класса AddLine, который так же отформатирует строку
    va_list val;
    va_start(val, sLine);
    AddLine(sNewLine, val);
    va_end(val);

    // Обновим файл на диске, если надо
    UpdateFile();

    // Уставноим размер строки в памяти равный одному - BOM
    m_dwLength = 1;
}

void CMemLogUStream::Constructor(LPWSTR sFileName, DWORD dwMaxSize, BYTE bAddDateTime)
{
    CMemOutUStream::Constructor(sFileName);
    m_dwMaxSize = dwMaxSize;
    m_bAddDateTime = bAddDateTime;
}

void CMemLogUStream::UpdateFile(bool bImmediate)
{
    // Откроем файл-лог на чтение и запись
    // Если файла нет на диске, он будет создан
    HANDLE hFile = CreateFile(m_sFileName.C(), GENERIC_READ | GENERIC_WRITE,
        0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return;

    DWORD dwFileSize = GetFileSize(hFile, NULL);
    DWORD dwWritten;

    if (dwFileSize == 0)
    {
        // Если размер файла 0 - запишем в начало файла BOM
        WriteFile(hFile, m_pMemory, m_dwLength * sizeof(WCHAR), &dwWritten, NULL);
    }
    else
    {
        // Иначе допишем в конец файла строку без BOM
        SetFilePointer(hFile, 0, NULL, FILE_END);

        WriteFile(hFile, m_pMemory + 1, (m_dwLength - 1) * sizeof(WCHAR),
            &dwWritten, NULL);
    }

    dwFileSize = GetFileSize(hFile, NULL);

    // Если размер файла больше максимально допустимого - обрезаем
    if (dwFileSize > m_dwMaxSize)
    {
        DWORD dwFileLen = dwFileSize / sizeof(WCHAR);

        LPWSTR sFileData = new WCHAR[m_dwMaxSize + 1];
        sFileData[m_dwMaxSize] = 0;

        SetFilePointer(hFile, -m_dwMaxSize, NULL, FILE_END);

        DWORD dwRed;
        ReadFile(hFile, sFileData, m_dwMaxSize, &dwRed, NULL);

        DWORD iStart = 0;
        while (iStart < m_dwMaxSize - 1 && sFileData[iStart] != L'\r'
            && sFileData[iStart + 1] != L'\n')
        {
            iStart++;
        }
        iStart += 2;

        SetFilePointer(hFile, sizeof(WCHAR), NULL, FILE_BEGIN);

        WriteFile(hFile, sFileData + iStart, m_dwMaxSize - (iStart * sizeof(WCHAR)),
            &dwWritten, NULL);
        SetEndOfFile(hFile);
        
    }

    CloseHandle(hFile);
}

///////////////////////////////////////////////////////////////////////////////
// CMemTextFile
///////////////////////////////////////////////////////////////////////////////

// Private members
void CMemTextFile::Resize(DWORD dwNewSize)
{
	if (dwNewSize > m_dwMemory)
	{
		m_dwMemory = BLOCK_SIZE * (dwNewSize / BLOCK_SIZE + 1);
		m_pMemory = (LPTSTR) HeapReAlloc(GetProcessHeap(), 0, m_pMemory, m_dwMemory);
	}
}

bool CMemTextFile::LoadFile(LPCTSTR cFileName)
{
	HANDLE hFile = CreateFile(cFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return false;

	m_dwSize = GetFileSize(hFile, NULL);
	Resize(m_dwSize);

	DWORD dwReaded;
	ReadFile(hFile, m_pMemory, m_dwSize, &dwReaded, NULL);

	CloseHandle(hFile);

	m_dwPos = 1;	// Unicode файлы имеют ByteOrderMark в начале файла
					// поэтому мы установим позицию "1" что бы пропустить BOM

	return (m_dwSize == dwReaded);
}

// Constructor \ destructor
CMemTextFile::CMemTextFile(LPCTSTR cFileName)
{
	m_dwMemory = BLOCK_SIZE;
	m_pMemory = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, m_dwMemory);
	
	m_dwSize = 0;
	m_dwPos = 0;	

	if (lstrlen(cFileName) > 0)
	{
		LoadFile(cFileName);
	}
	// вероятно файл открыт на запись - добавим BOM
	else
	{
		m_pMemory[0] = 65279 /* FEFF */;
		m_dwPos = 1;
	}
}

CMemTextFile::~CMemTextFile()
{
	HeapFree(GetProcessHeap(), 0, m_pMemory);
}

// Public members
bool CMemTextFile::SaveFile(LPCTSTR cFileName)
{
	HANDLE hFile = CreateFile(cFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE) return false;

	DWORD dwWritten;
	WriteFile(hFile, m_pMemory, m_dwSize, &dwWritten, NULL);
	CloseHandle(hFile);

	return (dwWritten == m_dwSize);
}

bool CMemTextFile::Eof()
{
	return (m_dwPos * sizeof(TCHAR) >= m_dwSize);
}

CString CMemTextFile::GetLine(bool bTrim)
{
	CString res;
	DWORD dwStart = m_dwPos;

	// m_dwSize в байтах
	// m_dwPos в символах
	// dwStart в символах
	while (m_dwPos * sizeof(TCHAR) < m_dwSize - sizeof(TCHAR))
	{
		if (m_pMemory[m_dwPos] == TEXT('\r') && m_pMemory[m_dwPos + 1] == TEXT('\n'))
		{
			res.Assign(m_pMemory + dwStart, m_dwPos - dwStart);
			m_dwPos += 2;

			break;
		}

		m_dwPos++;
	}

	if ((m_dwPos + 1) * sizeof(TCHAR) == m_dwSize && m_dwSize - dwStart * sizeof(TCHAR) > 0)
	{
		res.Assign(m_pMemory + dwStart, m_dwPos - dwStart - 1);
		m_dwPos++;
	}

	if (bTrim) res.Trim();

	return res;
}
void CMemTextFile::SetLine(LPCTSTR cLine)
{
	WORD len = lstrlen(cLine);
	m_dwSize += (len + 2) * sizeof(TCHAR);
	Resize(m_dwSize);

	CopyMemory(m_pMemory + m_dwPos, cLine, len * sizeof(TCHAR));
	m_dwPos += len + 2;

	m_pMemory[m_dwPos - 2] = TEXT('\r');
	m_pMemory[m_dwPos - 1] = TEXT('\n');
}

///////////////////////////////////////////////////////////////////////////////
// CMemLogFile
///////////////////////////////////////////////////////////////////////////////
CMemLogFile::CMemLogFile(LPCTSTR cFileName)
{
	m_sFileName = cFileName;
	m_bWroteFile = false;

	m_bAddDate = false;
	m_bAddTime = true;
}

CMemLogFile::~CMemLogFile()
{
	SetLine(TEXT(""));
	UpdateFile(true);
}

void CMemLogFile::UpdateFile(bool bImmediate)
{
	if (m_dwSize >= LOG_BUF_SIZE || (bImmediate && !m_bWroteFile))
	{
		HANDLE hFile = CreateFile(m_sFileName.C(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 
			FILE_ATTRIBUTE_NORMAL, NULL);

		if (hFile != INVALID_HANDLE_VALUE)
		{
			// Если размер файла лога переваливает за максимально допустимый
			DWORD dwFileSize = GetFileSize(hFile, NULL);
			if (dwFileSize + m_dwSize > m_dwMaxSize)
			{
				LPTSTR cFile = new TCHAR[(dwFileSize + m_dwSize) / sizeof(TCHAR) + 1 * sizeof(TCHAR)];

				// Читаем файл целиком в память
				DWORD dwReaded;
				ReadFile(hFile, cFile, dwFileSize, &dwReaded, NULL);

				// Дописываем к памяти свежие сообщения из буфера, которые нужно сбросить на диск
				CopyMemory(cFile + dwFileSize / sizeof(TCHAR), m_pMemory, m_dwSize);
				cFile[(dwFileSize + m_dwSize) / sizeof(TCHAR)] = 0;

				SetFilePointer(hFile, 0, 0, FILE_BEGIN);

				// Удалим несколько строк в начале файла что бы уложится в максимальный размер лог-файла
				WORD wStart = 0;
				while ((wStart + 1) * sizeof(TCHAR) < dwFileSize + m_dwSize)
				{
					// находим след. строку (\r\n)
					if (cFile[wStart] == TEXT('\r') && cFile[wStart + 1] == TEXT('\n'))
					{
						// если нашли те строки которые нужно удалить что бы уложиться в максимальный размер
						if (dwFileSize + m_dwSize - wStart * sizeof(TCHAR) <= m_dwMaxSize)
						{
							// выходим из цикла
							wStart += 2;
							break;
						}
					}

					wStart++;
				}

				// Не забудем про BOM
				if (wStart > 0)
				{
					wStart--;
					cFile[wStart] = 65279 /* FEFF */;
				}

				DWORD dwWritten;
				DWORD dwNewSize = dwFileSize + m_dwSize - wStart * sizeof(TCHAR);
				if (dwNewSize > 0)
					WriteFile(hFile, cFile + wStart, dwNewSize, &dwWritten, NULL);
				SetEndOfFile(hFile);

				delete cFile;
			}
			else
			{
				SetFilePointer(hFile, 0, 0, FILE_END);

				DWORD dwWritten;
				WriteFile(hFile, m_pMemory, m_dwSize, &dwWritten, NULL);
			}

			CloseHandle(hFile);

			m_bWroteFile = true;

			m_pMemory[0] = 0;
			m_dwSize = 0;
			m_dwPos = 0;
		}
	}
}

void CMemLogFile::AddLog(LPCTSTR cText, LPCTSTR cType, ...)
{
	CString res;
	TCHAR buf[MAX_PATH];

	if (m_bAddDate || m_bAddTime)
		res = TEXT("[");

	if (m_bAddDate)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		GetDateFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buf, MAX_PATH);

		res += buf;
	}

	if (m_bAddTime)
	{
		SYSTEMTIME st;
		GetLocalTime(&st);
		GetTimeFormat(LOCALE_USER_DEFAULT, 0, &st, NULL, buf, MAX_PATH);

		// если перед временем идёт дата - добавим пробел между ними
		if (m_bAddDate)
			res += TEXT(" ");

		res += buf;
	}

	if (m_bAddDate || m_bAddTime)
		res += TEXT("] ");

	// напишем тип сообщения
	res += CString(FormatC(TEXT("[%s] "), cType));
	// потом само сообщение
	res += (LPTSTR) cText;

	va_list val;
	va_start(val, cType);

	wvsprintf(buf, res.C(), val);

	va_end(val);

	m_bWroteFile = false;

	SetLine(buf);
	UpdateFile();
}

///////////////////////////////////////////////////////////////////////////////
// CMemValueTextFile
///////////////////////////////////////////////////////////////////////////////

CMemValueTextFile::CMemValueTextFile(LPCTSTR cFileName) : CMemTextFile(cFileName)
{
	
}

bool CMemValueTextFile::ScanLine(SLineInfo& LineInfo)
{
	LineInfo.bType = EMPTY;

	CString sLine = GetLine(true);
	if (sLine.Empty()) return false;

	// Skip comments
	if (sLine[0] == TEXT(';'))
		return false;
	// If section line
	else if (sLine[0] == TEXT('['))
	{
		if (sLine[sLine.Length() - 1] != TEXT(']')) return false;

		LineInfo.sSection.Assign(sLine.GetMemory() + 1, sLine.Length() - 2);
		LineInfo.sSection.Trim();

		LineInfo.bType = SECTION;
	}
	// if key-value line
	else
	{
		int pos = sLine.Find(TEXT('='));
		if (pos == -1)
		{
			LineInfo.sKey.Assign(sLine.GetMemory(), sLine.Length());
			LineInfo.sKey.Trim();

			LineInfo.bType = KEY_ONLY;
			return true;
		}

		LineInfo.sKey.Assign(sLine.GetMemory(), pos);
		LineInfo.sKey.Trim();
		//ToLower(LineInfo.sKey);

		LineInfo.sValue.Assign(sLine.GetMemory() + pos + 1, sLine.Length() - pos - 1);
		LineInfo.sValue.Trim();

		// Определяем кавычки
		if (LineInfo.sValue[0] == TEXT('\"') && LineInfo.sValue[LineInfo.sValue.Length() - 1] == TEXT('\"'))
		{
			LineInfo.sValue.Delete(0, 1);
			LineInfo.sValue.Delete(LineInfo.sValue.Length() - 1, 1);
		}

		LineInfo.bType = KEY_VALUE;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CMemSetTextFile
///////////////////////////////////////////////////////////////////////////////

CMemSetTextFile::CMemSetTextFile(LPCTSTR cFileName) : CMemValueTextFile(cFileName)
{
	if (lstrlen(cFileName) > 0)
		ParseFile();
}

bool CMemSetTextFile::ParseFile()
{
	BYTE bCurSect = (BYTE) -1;
	SLineInfo li;

	SSectionInfo* pSectInfo = new SSectionInfo;
	m_aSections.Add(pSectInfo);

	bCurSect = 0;

	while (!Eof())
	{
		if (ScanLine(li))
		{
			if (li.bType == SECTION)
			{
				pSectInfo = new SSectionInfo;
				pSectInfo->sName = li.sSection;

				m_aSections.Add(pSectInfo);
				bCurSect = m_aSections.Size() - 1;
			}
			else if ((li.bType == KEY_VALUE || li.bType == KEY_ONLY) && bCurSect != (BYTE) -1)
			{
				TStrPair* pStrPair = new TStrPair;
				pStrPair->First = li.sKey;
				pStrPair->Second = li.sValue;

				m_aSections[bCurSect]->aKeysValues.Add(pStrPair);
			}
		}
	}

	return true;
}

WORD CMemSetTextFile::GetSectionValuesCount(LPCTSTR cSection)
{
	CString sSect(cSection);
	sSect.ToLower();

	for (BYTE b = 0; b < m_aSections.Size(); b++)
	{
		SSectionInfo* pSectInfo = &*m_aSections[b];

		if (pSectInfo->sName.ToLowerNew() == sSect)
		{
			return pSectInfo->aKeysValues.Size();
		}
	}

	return 0;
}

CString CMemSetTextFile::GetString(LPCTSTR cSection, LPCTSTR cKey, LPCTSTR cDefault)
{
	CString sSect = CString(cSection).ToLowerNew();
	CString sKey = CString(cKey).ToLowerNew();

	for (BYTE b = 0; b < m_aSections.Size(); b++)
	{
		SSectionInfo* pSectInfo = &*m_aSections[b];

		if (pSectInfo->sName.ToLowerNew() == sSect)
		{
			for (WORD i = 0; i < pSectInfo->aKeysValues.Size(); i++)
			{
				TStrPair* pStrPair = pSectInfo->aKeysValues[i];

				if (pStrPair->First.ToLowerNew() == sKey)
				{
					return pStrPair->Second;
					break;
				}
			}

			return cDefault;
		}
	}

	return cDefault;
}

__int64 CMemSetTextFile::GetInt(LPCTSTR cSection, LPCTSTR cKey, int iDefault)
{
	CString sValue = GetString(cSection, cKey, TEXT(""));
	return sValue.ToInt(iDefault);
}

bool CMemSetTextFile::GetBool(LPCTSTR cSection, LPCTSTR cKey, bool bDefault)
{
	CString sValue = GetString(cSection, cKey, bDefault ? TEXT("true") : TEXT("false"));
	return sValue.ToBool(bDefault);
}

CString CMemSetTextFile::GetSection(WORD wIndex)
{
	if (wIndex >= 0 && wIndex < m_aSections.Size())
	{
		return m_aSections[wIndex]->sName;
	}

	return TEXT("");
}

CString CMemSetTextFile::GetKey(LPCTSTR cSection, WORD wIndex, LPCTSTR cDefault)
{
	CString sSect = CString(cSection).ToLowerNew();

	for (BYTE b = 0; b < m_aSections.Size(); b++)
	{
		SSectionInfo* pSectInfo = &*m_aSections[b];

		if (pSectInfo->sName.ToLowerNew() == sSect)
		{
			if (wIndex >= 0 && wIndex < pSectInfo->aKeysValues.Size())
			{
				TStrPair* pStrPair = pSectInfo->aKeysValues[wIndex];
				return pStrPair->First;
			}
			else 
				return cDefault;
		}
	}

	return cDefault;	
}