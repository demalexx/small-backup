#ifndef _STREAMS_H
#define _STREAMS_H

#include <windows.h>

#include "CommonClasses.h"
#include "Utils.h"

#define MS_BLOCK_SIZE	1024

#define MEMSTREAMBLOCK  10240   // Размер блока в байтах для потоков в памяти
                                // Размер выделяемой памяти кратен этому значению

///////////////////////////////////////////////////////////////////////////////
// CMemInUStream
///////////////////////////////////////////////////////////////////////////////
class CMemInUStream{
public:
	CMemInUStream();
	CMemInUStream(LPWSTR sFileName);
	CMemInUStream(CString sFileName);

	~CMemInUStream();

    bool LoadFile(LPWSTR sFileName);
    bool LoadFile(CString sFileName);

    bool Eof();
	CString ReadLine();

protected:
    void Constructor();

	void Resize(DWORD dwNewLength);

	LPWSTR	m_pMemory;      // Указатель на данные в памяти
    DWORD   m_dwMemSize;    // Размер выделенной памяти в байтах
	DWORD	m_dwLength;     // Длина файла в символах
	DWORD	m_dwPos;        // Текущая позиция в файле, в символах
};

///////////////////////////////////////////////////////////////////////////////
// CMemOutUStream
///////////////////////////////////////////////////////////////////////////////
class CMemOutUStream{
public:
    CMemOutUStream();
    CMemOutUStream(LPWSTR sFileName);
    CMemOutUStream(CString sFileName);

    ~CMemOutUStream();

    void WriteLine(LPWSTR sLine = L"", ...);
    void WriteLine(CString sLine, ...);

    bool SaveFile(LPWSTR sFileName = NULL);
    bool SaveFile(CString sFileName);

protected:
    void Constructor(LPWSTR sFileName = NULL);

    void Resize(DWORD dwNewLength);
    void AddLine(LPWSTR sLine, va_list val);

    LPWSTR  m_pMemory;      // Указатель на данные в памяти
    DWORD   m_dwMemSize;    // Размер в байтах выделенной памяти
    DWORD   m_dwLength;     // Длина строки в символах

    CString m_sFileName;
};

///////////////////////////////////////////////////////////////////////////////
// CMemIniOutW
///////////////////////////////////////////////////////////////////////////////
class CMemIniOutW : public CMemOutUStream{
public:
    CMemIniOutW::CMemIniOutW(LPWSTR sFileName = NULL) : CMemOutUStream(sFileName) {};

    void AddSection(LPWSTR sSection);

    void AddString(LPWSTR sKey, LPWSTR sValue = NULL);
    void AddNumeric(LPWSTR sKey, int iValue);
    void AddBoolean(LPWSTR sKey, bool bValue);
};

///////////////////////////////////////////////////////////////////////////////
// CMemLogUStream
///////////////////////////////////////////////////////////////////////////////
#define LOG_ADDTIME      1
#define LOG_ADDDATE      2
#define LOG_ADDDATETIME  3

#define LOG_BUF_SIZE     512

class CMemLogUStream : CMemOutUStream{
public:
    CMemLogUStream();
    CMemLogUStream(LPWSTR sLogFile, DWORD dwMaxSize = 0, BYTE bAddDateTime = LOG_ADDDATETIME);

    void SetMaxSize(DWORD dwMaxSize) { m_dwMaxSize = dwMaxSize; };

    void WriteLogLine(LPWSTR sLine = L"", ...);
    void WriteLogLine(CString sLine, ...);

protected:
    void Constructor(LPWSTR sFileName = NULL, DWORD dwMaxSize = MAXDWORD,
        BYTE bAddDateTime = LOG_ADDDATETIME);

    void UpdateFile(bool bImmediate = false);  // Следит за сбрасыванием файла-журнала на диск

    DWORD   m_dwMaxSize;
    BYTE    m_bAddDateTime;
};


///////////////////////////////////////////////////////////////////////////////
// CMemTextFile
///////////////////////////////////////////////////////////////////////////////
#define BLOCK_SIZE	1024

class CMemTextFile{
protected:
	LPTSTR	m_pMemory;	// указатель на строковые данные в памяти
	DWORD	m_dwMemory;	// выделенная память в байтах
	DWORD	m_dwSize;	// размер файла в байтах
	DWORD	m_dwPos;	// текущая позиция файлового указателя (в символах)

	void Resize(DWORD dwNewSize);
	bool LoadFile(LPCTSTR cFileName);

public:
	CMemTextFile(LPCTSTR cFileName = TEXT(""));
	~CMemTextFile();

	bool SaveFile(LPCTSTR cFileName);

	bool Eof();
	CString GetLine(bool bTrim = false);

	void SetLine(LPCTSTR cLine);
};    

///////////////////////////////////////////////////////////////////////////////
// CMemLogFile
///////////////////////////////////////////////////////////////////////////////
//#define LOG_BUF_SIZE	1024	// размер буфера, при его заполнении файл
                                // на диске будет обновлён

class CMemLogFile : public CMemTextFile{
private:
	CString		m_sFileName;
	DWORD		m_dwMaxSize;
	bool		m_bWroteFile;

	bool		m_bAddDate;
	bool		m_bAddTime;

	void UpdateFile(bool bImmediate = false);

public:
    CMemLogFile(LPCTSTR cFileName = TEXT(""));
	~CMemLogFile();

	void SetMaxSize(DWORD dwMaxSize) { m_dwMaxSize = dwMaxSize * 1024; } ;

	void SetAddDate(bool bValue) { m_bAddDate = bValue; } ;
	void SetAddTime(bool bValue) { m_bAddTime = bValue; } ;

	void AddLog(LPCTSTR cText, LPCTSTR cType, ...);
};

///////////////////////////////////////////////////////////////////////////////
// CMemValueTextFile
///////////////////////////////////////////////////////////////////////////////

#define EMPTY		0
#define SECTION		1
#define KEY_VALUE	2
#define KEY_ONLY	3

struct SLineInfo{
	BYTE	bType;
	CString	sSection;
	CString	sKey;
	CString	sValue;
};

class CMemValueTextFile : public CMemTextFile{
public:
	CMemValueTextFile(LPCTSTR cFileName = TEXT(""));

	bool ScanLine(SLineInfo& LineInfo);
};

///////////////////////////////////////////////////////////////////////////////
// CMemSetTextFile
///////////////////////////////////////////////////////////////////////////////

struct SSectionInfo{
	CString				sName;
	CList<TStrPair*>	aKeysValues;
};

class CMemSetTextFile : public CMemValueTextFile{
private:
	CList<SSectionInfo*>	m_aSections;

public:
	CMemSetTextFile(LPCTSTR cFileName = TEXT(""));

	bool ParseFile();

	WORD GetSectionsCount() {return m_aSections.Size();} ;
	WORD GetSectionValuesCount(LPCTSTR cSection);

	CString	GetString(LPCTSTR cSection, LPCTSTR cKey, LPCTSTR cDefault = TEXT(""));
	__int64	GetInt(LPCTSTR cSection, LPCTSTR cKey, int iDefault = 0);
	bool	GetBool(LPCTSTR cSection, LPCTSTR cKey, bool bDefault = false);

	CString GetSection(WORD wIndex);
	CString GetKey(LPCTSTR cSection, WORD wIndex, LPCTSTR cDefault = TEXT(""));
};

#endif