#ifndef _COMMONCLASSES_H
#define _COMMONCLASSES_H

#include <windows.h>

#define CSTRING_MEM_BLOCK		64
#define CSTRING_PREDEF_BLOCK	MAX_PATH

///////////////////////////////////////////////////////////////////////////////
// CString
///////////////////////////////////////////////////////////////////////////////
class CString{
private:
	LPTSTR	m_pMemory;		// Указатель, где хранятся символы строки
	WORD	m_wMemory;		// Кол-во байт зарезервированной памяти
	WORD	m_wLength;		// Длина строки (в символах, не в байтах)
	bool	m_bCalcLength;	// Флаг что длину строки надо пересчитать

	HANDLE	m_hHeap;		// Хендл кучи где хранятся данные строки

	void Init(WORD wInitSize = CSTRING_MEM_BLOCK);
	void Resize(WORD wNewSize, bool bExact = false);

public:
	CString(WORD wInitSize = CSTRING_MEM_BLOCK);
	CString(LPCTSTR ccText);
	CString(CString& csText);
	CString(CString* csText);
	CString(CString& csText, WORD wStart, WORD wLen);

	~CString();

	WORD Length() ;
	WORD BufLen() { return m_wMemory / sizeof(TCHAR); } ;
	bool Empty() { return Length() == 0; } ; 
	LPTSTR GetMemory() { return m_pMemory; } ; 

	void Assign(LPTSTR pMemory, WORD wSize, WORD wPos = 0);

	void Trim();
	CString ToLower();
	CString ToUpper();

	CString ToLowerNew(); 
	CString ToUpperNew(); 

	CString SubStr(WORD wStart, WORD wLen); 
	int Find(TCHAR cFind, WORD wStart = 0); 

	void Delete(WORD wStart, WORD wLen);

	CString FromInt(__int64 i64Value);

	__int64 ToInt(__int64 i64Default = 0);
	bool ToBool(bool bDefault = false);

	bool Spaces();
	
	LPTSTR C();		// Возвращает си-строку не для изменения
	LPTSTR Buf(WORD wSize = CSTRING_PREDEF_BLOCK);
		// Выделяет память CSTRING_PREDEF_BLOCK и возвращает
		// си-строку для записи (как буфер)

	TCHAR& operator [] (int wIndex) { return m_pMemory[wIndex]; } ; 

	CString operator = (CString* sOther);
	CString operator = (CString& sOther);
	CString operator = (LPCTSTR cOther);
	
	CString operator + (CString& sOther);
	CString operator + (LPTSTR cOther);
	CString operator + (TCHAR cOther);

	CString operator += (CString& sOther);
	CString operator += (LPTSTR cOther);
	CString operator += (TCHAR cOther);

	bool operator == (CString& sOther);
	bool operator == (LPTSTR cOther);

	bool operator != (CString& sOther);
	bool operator != (LPTSTR cOther);
};

typedef CString* PString;	// Новый тип - указатель на строку

template <class T1, class T2> struct SPair{
	T1	First;
	T2	Second;
};

typedef SPair<CString, CString> TStrPair;	// Новый тип - структура из двух строк

///////////////////////////////////////////////////////////////////////////////
// CList
///////////////////////////////////////////////////////////////////////////////
template <class T> struct SListItem{
	T			Elem;
	SListItem*	Next;

	SListItem(): Elem() {};
};

template <class T>
class CList{
private:
	SListItem<T>*	m_pHead;	// "Голова" списка
	WORD			m_wSize;	// Кол-во элементов в списке

public:
	CList()
	{
		m_pHead = NULL;
		m_wSize = 0;
	};

	WORD Size() { return m_wSize; } ;

	SListItem<T>* GetLast()
	{
		SListItem<T>* pListItem = m_pHead;
		for (; pListItem;)
		{
			if (pListItem->Next == NULL)
				return pListItem;
			else
				pListItem = pListItem->Next;
		}
	};

    void Add(T Item)
	{
		SListItem<T>* pListItem = new SListItem<T>;
		pListItem->Elem = Item;
		pListItem->Next = NULL;

		if (m_pHead == NULL)
		{
			m_pHead = pListItem;
		}
		else
		{
			GetLast()->Next = pListItem;			
		}

		m_wSize++;
	};

	void Delete(int iIndex)
	{
		if (m_wSize == 0) return;

		SListItem<T>* pLIDel = m_pHead;

		if (iIndex == 0)
		{
			pLIDel = m_pHead;
			m_pHead = m_pHead->Next;

			delete pLIDel;
			m_wSize--;
			return;
		}

		SListItem<T>* pListItem = m_pHead;
	
		for (int i = 0; i < iIndex - 1; i++)
		{
			pListItem = pListItem->Next;
		}
		
		pLIDel = pListItem->Next;
		pListItem->Next = pListItem->Next->Next;

		delete pLIDel;
		m_wSize--;
	};

	void Clear()
	{
		SListItem<T>* pListItem = m_pHead;
		for (int i = 0; i < m_wSize; i++)
		{
			SListItem<T>* pListItemToDel = pListItem;
			pListItem = pListItem->Next;

			delete pListItemToDel;
		}
		m_pHead = NULL;
		m_wSize = 0;
	};

	T operator [] (int iIndex)
	{
		SListItem<T>* pListItem = m_pHead;
		for (int i = 0; i < iIndex; i++)
		{
			pListItem = pListItem->Next;
		}
		return pListItem->Elem;
	};
};

typedef CList<CString> TStrList;	// Новый тип - лист из строк
typedef CList<PString> TPStrList;	// Новый тип - лист из указателей на строки

///////////////////////////////////////////////////////////////////////////////
// CDateTime
///////////////////////////////////////////////////////////////////////////////
#define DT_MSECOND	10000
#define DT_SECOND	10000000
#define DT_MINUTE	600000000
#define DT_HOUR		36000000000
#define DT_DAY		864000000000

class CDateTime{
private:
	SYSTEMTIME m_SystemTime;

	__int64 Delta(SYSTEMTIME st);

public:
	CDateTime();
	CDateTime(CDateTime& Other);
	CDateTime(SYSTEMTIME Other);
	CDateTime(WORD wYear, WORD wMonth, WORD wDay, WORD wHour, WORD wMinute, WORD wSecond = 0, WORD wMilliseconds = 0);

	SYSTEMTIME GetSystemTime() { return m_SystemTime; };
	SYSTEMTIME* GetPSystemTime() { return &m_SystemTime; };
	FILETIME GetFileTime();

	// "Пакует" дату в строку
	CString PackToString();

	// "Распаковывает" дату из строки
	bool UnpackFromString(CString sDateTime);

	__int64 DeltaMSec(SYSTEMTIME st);
	__int64 DeltaSec(SYSTEMTIME st);
	__int64 DeltaMin(SYSTEMTIME st);
	__int64 DeltaHour(SYSTEMTIME st);

	__int64 DeltaMSec(CDateTime& dt) { return DeltaMSec(dt.GetSystemTime()); } ;
	__int64 DeltaSec(CDateTime& dt) { return DeltaSec(dt.GetSystemTime()); } ;
	__int64 DeltaMin(CDateTime& dt) { return DeltaMin(dt.GetSystemTime()); } ;
	__int64 DeltaHour(CDateTime& dt) { return DeltaHour(dt.GetSystemTime()); } ;

	// "Нулевое" ли время
	bool IsNullTime() { return m_SystemTime.wYear == 0; };

	CDateTime operator = (SYSTEMTIME& st) { m_SystemTime = st; return *this; } ;
	CDateTime operator = (CDateTime& dt) { m_SystemTime = dt.GetSystemTime(); return *this; };

	CDateTime operator + (SYSTEMTIME& st);
	CDateTime operator + (CDateTime& dt);
	CDateTime operator + (__int64 i64);

	CDateTime operator - (SYSTEMTIME& st);
	CDateTime operator - (CDateTime& dt);
	CDateTime operator - (__int64 i64);

	CDateTime operator += (SYSTEMTIME& st);
	CDateTime operator += (CDateTime& dt);
	CDateTime operator += (__int64 i64);

	CDateTime operator -= (SYSTEMTIME& st);
	CDateTime operator -= (CDateTime& dt);
	CDateTime operator -= (__int64 i64);

	bool operator == (SYSTEMTIME& st);
	bool operator == (CDateTime& dt);

	bool operator > (SYSTEMTIME& st);
	bool operator > (CDateTime& dt) { return operator > (dt.GetSystemTime()); };

	bool operator >= (SYSTEMTIME& st);
	bool operator >= (CDateTime& dt) { return operator >= (dt.GetSystemTime()); };

	bool operator < (SYSTEMTIME& st);
	bool operator < (CDateTime& dt) { return operator < (dt.GetSystemTime()); };

	bool operator <= (SYSTEMTIME& st);
	bool operator <= (CDateTime& dt) { return operator <= (dt.GetSystemTime()); };
};

CDateTime DTNow();
CDateTime DTNULL();

#endif