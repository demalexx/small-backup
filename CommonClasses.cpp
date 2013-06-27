#include "CommonClasses.h"

///////////////////////////////////////////////////////////////////////////////
// CString
///////////////////////////////////////////////////////////////////////////////
CString::CString(WORD wInitSize)
{
	Init(wInitSize);
}

CString::CString(LPCTSTR ccText)
{
	Init();
	Assign((LPTSTR) ccText, lstrlen(ccText));
}

CString::CString(CString& csText)
{
	Init();
	Assign(csText.GetMemory(), csText.Length());
}

CString::CString(CString* csText)
{
	Init();
	Assign(csText->GetMemory(), csText->Length());
}

CString::CString(CString& csText, WORD wStart, WORD wLen)
{
	Init();
	Assign(csText.GetMemory() + wStart, wLen);
}

CString::~CString()
{
	HeapFree(m_hHeap, 0, m_pMemory);
}

WORD CString::Length()
{
	if (m_bCalcLength)
	{
		m_wLength = lstrlen(m_pMemory);
		m_bCalcLength = false;
	}

	return m_wLength;
}

void CString::Init(WORD wInitSize)
{
	m_pMemory = NULL;
	m_wMemory = wInitSize * sizeof(TCHAR);
	m_wLength = 0;
	m_bCalcLength = false;
	m_hHeap = GetProcessHeap();

	m_pMemory = (LPTSTR) HeapAlloc(m_hHeap, 0, m_wMemory);
}

void CString::Resize(WORD wNewSize, bool bExact)
{
	if (wNewSize > m_wMemory)
	{
		if (bExact)
			m_wMemory = wNewSize;
		else
			m_wMemory = (wNewSize / CSTRING_MEM_BLOCK + 1) * CSTRING_MEM_BLOCK;

		m_pMemory = (LPTSTR) HeapReAlloc(m_hHeap, 0, m_pMemory, m_wMemory);
	}
}

void CString::Assign(LPTSTR pMemory, WORD wSize, WORD wPos)
{
	// wSize и wPos - в символах, не в байтах

	Resize((wSize + wPos) * sizeof(TCHAR));
	CopyMemory(m_pMemory + wPos, pMemory, wSize * sizeof(TCHAR));
	m_wLength = wSize + wPos;
}

void CString::Trim()
{
	WORD wDelChars = 0; // кол-во символов для удаления
	for (int i = 0; i < Length(); i++)
		if ((*this)[i] == TEXT(' '))
			wDelChars++;
		else
			break;

	Delete(0, wDelChars);
	wDelChars = 0;

	for (int i = Length() - 1; i >= 0; i--)
		if ((*this)[i] == TEXT(' '))
			wDelChars++;
		else
			break;

	Delete(Length() - wDelChars, wDelChars);
}

CString CString::ToLower()
{
	CharLowerBuff((*this).C(), Length());
	return this;
}

CString CString::ToUpper()
{
	CharUpperBuff((*this).C(), Length());
	return this;
}

CString CString::ToLowerNew()
{
	CString res((*this));
	res.ToLower();
	return res;
}

CString CString::ToUpperNew()
{
	CString res((*this));
	res.ToUpper();
	return res;
}

CString CString::SubStr(WORD wStart, WORD wLen)
{
	CString res;
	res.Assign(m_pMemory + wStart, wLen);

	return res;
}

int CString::Find(TCHAR cFind, WORD wStart)
{
	for (WORD i = wStart; i < Length(); i++)
		if ((*this)[i] == cFind)
			return i;
	return -1;
}

void CString::Delete(WORD wStart, WORD wLen)
{
	if (wLen == 0) return;

	// Передвинем память только если не происходит удаления с конца строки
	if (wStart + wLen < Length())
	{
		CopyMemory(m_pMemory + wStart, m_pMemory + wStart + wLen,
			(Length() - wStart - wLen) * sizeof(TCHAR));
	}

	m_wLength -= wLen;
}

CString CString::FromInt(__int64 i64Value)
{
	CString sBuf;
	wsprintf(sBuf.Buf(20), TEXT("%d"), i64Value);
	(*this) = sBuf;

	return this;
}

__int64 CString::ToInt(__int64 i64Default)
{
	if (Empty()) return i64Default;

	// Удалим "пробелы"-разделители
	for (WORD i = 0; i < Length(); i++)
		if ((*this)[i] == 160 /* TEXT("\x00A0") Non-break space*/)
		{
			this->Delete(i, 1);
			break;
		}

	__int64 res = 0;
	WORD bPower = 1;
	for (int i = Length() - 1; i >=0 ; i--)
	{
		if ((*this)[i] < TEXT('0') || (*this)[i] > TEXT('9'))
			return i64Default;

		res = res + ((*this)[i] - TEXT('0')) * bPower;
		bPower *= 10;
	}

	return res;
}

bool CString::ToBool(bool bDefault)
{
	if ((*this) == TEXT("1") || (*this).ToLowerNew() == TEXT("true"))
		return true;
	else if ((*this) == TEXT("0") || (*this).ToLowerNew() == TEXT("false"))
		return false;
	else
		return bDefault;
}


bool CString::Spaces()
{
	if (Empty()) return false;

	return ((*this)[0] == TEXT(' ') || (*this)[Length() - 1] == TEXT(' '));
}

LPTSTR CString::C()
{
	// Увеличим размер строки на один символ
	Resize((Length() + 1) * sizeof(TCHAR));
	// и добавим null-terminator
	m_pMemory[Length()] = 0;

	return m_pMemory;
}

LPTSTR CString::Buf(WORD wSize)
{
	Resize(wSize * sizeof(TCHAR));
	
	if (Length() == 0)
		m_pMemory[Length()] = 0;
	else
		m_pMemory[Length() - 1] = 0;

	m_bCalcLength = true;

	return m_pMemory;
}

CString CString::operator = (CString* sOther)
{
	return (*this) = (*sOther);
}

CString CString::operator = (CString& sOther)
{
	Assign(sOther.GetMemory(), sOther.Length());
    return (*this);
}

CString CString::operator = (LPCTSTR cOther)
{
	Assign((LPTSTR) cOther, lstrlen(cOther));
    return (*this);
}

CString CString::operator + (CString& sOther)
{
	return CString(*this) += sOther;
}

CString CString::operator + (LPTSTR cOther)
{
	return CString(*this) += cOther;
}

CString CString::operator + (TCHAR cOther)
{
	return CString(*this) += cOther;
}

CString CString::operator += (CString& sOther)
{
	Assign(sOther.GetMemory(), sOther.Length(), Length());
	return (*this);
}

CString CString::operator += (LPTSTR cOther)
{
	Assign(cOther, lstrlen(cOther), Length());
	return (*this);
}

CString CString::operator += (TCHAR cOther)
{
	TCHAR c[2] = {0};
	c[0] = cOther;

	Assign(c, 1, Length());
	return (*this);
}

bool CString::operator == (CString& sOther)
{
	return lstrcmp((*this).C(), sOther.C()) == 0;
}

bool CString::operator == (LPTSTR cOther)
{
	return lstrcmp((*this).C(), cOther) == 0;
}

bool CString::operator != (CString& sOther)
{
	return !((*this) == sOther);
}

bool CString::operator != (LPTSTR cOther)
{
	return !((*this) == cOther);
}

///////////////////////////////////////////////////////////////////////////////
// CDateTime
///////////////////////////////////////////////////////////////////////////////
CDateTime::CDateTime()
{
	GetLocalTime(&m_SystemTime);
}

CDateTime::CDateTime(CDateTime& Other)
{
	m_SystemTime = Other.GetSystemTime();
}

CDateTime::CDateTime(SYSTEMTIME Other)
{
	m_SystemTime = Other;
}

CDateTime::CDateTime(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond, WORD wMilliseconds)
{
	m_SystemTime.wYear = wYear;
	m_SystemTime.wMonth = wMonth;
	m_SystemTime.wDay = wDay;
	m_SystemTime.wHour = wHour;
	m_SystemTime.wMinute = wMinute;
	m_SystemTime.wSecond = wSecond;
	m_SystemTime.wMilliseconds = wMilliseconds;
}

__int64 CDateTime::Delta(SYSTEMTIME st)
{
	CDateTime dt(st);

	// если просят узнать разницу где один из аргументов - нулевая дата - вернём ноль
	if (dt.IsNullTime())
		return 0;
	else
		dt -= (*this);

	return *(LONGLONG*) &dt.GetFileTime();
}

FILETIME CDateTime::GetFileTime()
{
	FILETIME ft;
	SystemTimeToFileTime(&m_SystemTime, &ft);
	return ft;
}

CString CDateTime::PackToString()
{
	CString res(15);
	// "Пакует" дату в строку вида 'HHMMSSddmmyyyy'
	wsprintf(res.Buf(15), TEXT("%.2d%.2d%.2d%.2d%.2d%.4d"), m_SystemTime.wHour, m_SystemTime.wMinute,
		m_SystemTime.wSecond, m_SystemTime.wDay, m_SystemTime.wMonth, m_SystemTime.wYear);

	return res;
}

bool CDateTime::UnpackFromString(CString sDateTime)
{
	// "Распаковывает" дату из строку вида 'HHMMSSddmmyyyy'
	m_SystemTime.wHour = sDateTime.SubStr(0, 2).ToInt();
	m_SystemTime.wMinute = sDateTime.SubStr(2, 2).ToInt();
	m_SystemTime.wSecond = sDateTime.SubStr(4, 2).ToInt();
	m_SystemTime.wDay = sDateTime.SubStr(6, 2).ToInt();
	m_SystemTime.wMonth = sDateTime.SubStr(8, 2).ToInt();
	m_SystemTime.wYear = sDateTime.SubStr(10, 4).ToInt();

	return true;
}

__int64 CDateTime::DeltaMSec(SYSTEMTIME st)
{
	return Delta(st) / DT_MSECOND;
}

__int64 CDateTime::DeltaSec(SYSTEMTIME st)
{
	return Delta(st) / (__int64) DT_SECOND;
}

__int64 CDateTime::DeltaMin(SYSTEMTIME st)
{
	return Delta(st) / (__int64) DT_MINUTE;
}

__int64 CDateTime::DeltaHour(SYSTEMTIME st)
{
	return Delta(st) / (__int64) DT_HOUR;
}

CDateTime CDateTime::operator + (SYSTEMTIME& st)
{
	FILETIME ftThis, ftOther;

	SystemTimeToFileTime(&m_SystemTime, &ftThis);
	SystemTimeToFileTime(&st, &ftOther);

    *(LONGLONG*) &ftThis += *(LONGLONG*) &ftOther;

	FileTimeToSystemTime(&ftThis, &m_SystemTime);
	return *this;
}

CDateTime CDateTime::operator + (CDateTime& st)
{
	operator + (st.GetSystemTime());
	return *this;
}

CDateTime CDateTime::operator + (__int64 i64)
{
	FILETIME ft;
	*(LONGLONG*) &ft = i64;
	SYSTEMTIME st;
	FileTimeToSystemTime(&ft, &st);

	operator + (st);
	return *this;
}

CDateTime CDateTime::operator - (SYSTEMTIME& st)
{
	FILETIME ftThis, ftOther;

	SystemTimeToFileTime(&m_SystemTime, &ftThis);
	SystemTimeToFileTime(&st, &ftOther);

    *(LONGLONG*) &ftThis -= *(LONGLONG*) &ftOther;

	FileTimeToSystemTime(&ftThis, &m_SystemTime);
	return *this;
}

CDateTime CDateTime::operator - (CDateTime& st)
{
	operator - (st.GetSystemTime());
	return *this;
}

CDateTime CDateTime::operator - (__int64 i64)
{
	operator + (-i64);
	return *this;
}

CDateTime CDateTime::operator += (SYSTEMTIME& st)
{
	operator + (st);
	return *this;
}

CDateTime CDateTime::operator += (CDateTime& st)
{
	operator + (st);
	return *this;
}

CDateTime CDateTime::operator += (__int64 i64)
{
	operator + (i64);
	return *this;
}

CDateTime CDateTime::operator -= (SYSTEMTIME& st)
{
	operator - (st);
	return *this;
}

CDateTime CDateTime::operator -= (CDateTime& st)
{
	operator - (st);
	return *this;
}

CDateTime CDateTime::operator -= (__int64 i64)
{
	operator + (-i64);
	return *this;
}

bool CDateTime::operator == (SYSTEMTIME& st)
{
	return (m_SystemTime.wYear == st.wYear &&
		m_SystemTime.wMonth == st.wMonth &&
		m_SystemTime.wDay == st.wDay &&
		m_SystemTime.wDayOfWeek == st.wDayOfWeek &&
		m_SystemTime.wHour == st.wHour &&
		m_SystemTime.wMinute == st.wMinute &&
		m_SystemTime.wSecond == st.wSecond &&
		m_SystemTime.wMilliseconds == st.wMilliseconds);			
}

bool CDateTime::operator == (CDateTime& dt)
{
	return operator == (dt.GetSystemTime());
}

bool CDateTime::operator > (SYSTEMTIME& st)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	return *(LONGLONG*) &GetFileTime() > *(LONGLONG*) &ft;
}

bool CDateTime::operator >= (SYSTEMTIME& st)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	return *(LONGLONG*) &GetFileTime() >= *(LONGLONG*) &ft;
}

bool CDateTime::operator < (SYSTEMTIME& st)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	return *(LONGLONG*) &GetFileTime() < *(LONGLONG*) &ft;
}

bool CDateTime::operator <= (SYSTEMTIME& st)
{
	FILETIME ft;
	SystemTimeToFileTime(&st, &ft);

	return *(LONGLONG*) &GetFileTime() <= *(LONGLONG*) &ft;
}

CDateTime DTNow()
{
	return CDateTime();
}

CDateTime DTNULL()
{
	CDateTime dt;
	SYSTEMTIME st = {0};
	*(dt.GetPSystemTime()) = st;

	return dt;
}

