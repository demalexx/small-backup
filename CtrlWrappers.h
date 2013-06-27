#ifndef _CTRLWRAPPERS_H
#define _CTRLWRAPPERS_H

#include <windows.h>
#include <commctrl.h>

#include "CommonClasses.h"

class CCombo{
private:
	HWND	m_hCombo;

public:
	CCombo(HWND hwnd) { m_hCombo = hwnd; };

	int AddString(LPCTSTR cText, LPARAM lData = 0);
	void SelItem(int iIndex);
	void SelItemByData(LPARAM lData);

	int GetSel();
	LPARAM GetSelData();
};

///////////////////////////////////////////////////////////////////////////////
// CListView
///////////////////////////////////////////////////////////////////////////////
class CListView{
private:
	HWND	m_hParent;
	HWND	m_hListView;

	// Индекс последней добавленной колонки для того, что бы следующую колонку добавить после последней
	int		m_iLastColumnIndex;

public:
	CListView(HWND hParent, HINSTANCE hInstance, int iID);

	HWND GetHWND() { return m_hListView; };

	int GetSelIndex();

	int FindItem(LPCTSTR cText, int iCol);
	int FindItemParam(LPARAM lParam);
	LPARAM GetItemParam(int iIndex);

	CString GetSubItem(int iItem, int iSubItem);

	bool AddColumn(LPCTSTR cText, WORD wWidth);

	int AddItem(LPCTSTR cText, LPARAM lParam);
	bool DelItem(int iItem);
	
	bool SetSubItem(int iItem, int iSubItem, LPCTSTR cText);

	WORD GetColumnWidth(int iColumns);

	void Resize();
};

///////////////////////////////////////////////////////////////////////////////
// CTree
///////////////////////////////////////////////////////////////////////////////
class CTree{
private:
	HWND	m_hParent;
	HWND	m_hTree;

public:
	CTree(HWND hTree);
	CTree(HWND hParent, HINSTANCE hInstance, int iID);

	WORD GetCount();
	WORD GetChildCount(HTREEITEM hti);
	HTREEITEM GetSel();
	LPARAM GetItemParam(HTREEITEM hti);
	LPARAM GetSelItemParam();

	HTREEITEM GetRoot();
	HTREEITEM GetParent(HTREEITEM hti);
	HTREEITEM GetChild(HTREEITEM hti);
	HTREEITEM GetNextSibling_(HTREEITEM hti);
	HTREEITEM GetNextItem(HTREEITEM hti);

	CString GetItemText(HTREEITEM hti);

	HTREEITEM AddItem(LPCTSTR ccText, LPARAM lParam, HTREEITEM htiParent = TVI_ROOT);

	void SetItemText(HTREEITEM hti, LPCTSTR ccText);
	void ExpandItem(HTREEITEM hti);
	void ExpandAll();
	void Select(HTREEITEM hti);

	void DelAll();
	void DelItem(HTREEITEM hti);
};

#endif