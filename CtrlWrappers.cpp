#include "CtrlWrappers.h"

///////////////////////////////////////////////////////////////////////////////
// CCombo
///////////////////////////////////////////////////////////////////////////////
int CCombo::AddString(LPCTSTR cText, LPARAM lData)
{
	int res = SendMessage(m_hCombo, CB_ADDSTRING, 0, (LPARAM) cText);
	SendMessage(m_hCombo, CB_SETITEMDATA, res, lData);

	return res;
}

void CCombo::SelItem(int iIndex)
{
	SendMessage(m_hCombo, CB_SETCURSEL, iIndex, 0);
}

void CCombo::SelItemByData(LPARAM lData)
{
	for (WORD i = 0; i < SendMessage(m_hCombo, CB_GETCOUNT, 0, 0); i++)
	{
		if (lData == SendMessage(m_hCombo, CB_GETITEMDATA, i, 0))
		{
			SelItem(i);
			break;
		}
	}
}

int CCombo::GetSel()
{
	return SendMessage(m_hCombo, CB_GETCURSEL, 0, 0);
}

LPARAM CCombo::GetSelData()
{
	return SendMessage(m_hCombo, CB_GETITEMDATA, GetSel(), 0);
}

///////////////////////////////////////////////////////////////////////////////
// CListView
///////////////////////////////////////////////////////////////////////////////
CListView::CListView(HWND hParent, HINSTANCE hInstance, int iID)
{
	m_hParent = hParent;
	m_iLastColumnIndex = 0;

	// Создание listview
	m_hListView = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_CHILD | 
		WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | LVS_NOSORTHEADER,
		0, 0, 0, 0, hParent, (HMENU) iID, hInstance, NULL);

	// Установим св-во - полностью выделять строку
	ListView_SetExtendedListViewStyle(m_hListView, LVS_EX_FULLROWSELECT);
}

int CListView::GetSelIndex()
{
	return ListView_GetNextItem(m_hListView, -1, LVNI_SELECTED);
}

int CListView::FindItem(LPCTSTR cText, int iCol)
{
	int ind = -1;
	while ((ind = ListView_GetNextItem(m_hListView, ind, 0)) != -1)
	{
		CString sText;

		LVITEM lvi = {0};
		lvi.iItem = ind;
		lvi.iSubItem = iCol;
		lvi.pszText = sText.Buf();
		lvi.cchTextMax = sText.BufLen();
		lvi.mask = LVIF_TEXT;

		ListView_GetItem(m_hListView, &lvi);

		CString sFind = cText;
		sText.ToLower();
		sFind.ToLower();

		if (sFind == sText)
			return ind;
	}

	return -1;
}

int CListView::FindItemParam(LPARAM lParam)
{
	LVFINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = lParam;

	return ListView_FindItem(m_hListView, -1, &lvfi);
}


LPARAM CListView::GetItemParam(int iIndex)
{
	LVITEM lvi = {0};
	lvi.mask = LVIF_PARAM;
	lvi.iItem = iIndex;

	if (!ListView_GetItem(m_hListView, &lvi)) return 0;

	return lvi.lParam;
}

CString CListView::GetSubItem(int iItem, int iSubItem)
{
	CString sBuf;

	LVITEM lvi = {0};
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;
	lvi.pszText = sBuf.Buf();
	lvi.cchTextMax = sBuf.BufLen();
	lvi.mask = LVIF_TEXT;

	ListView_GetItem(m_hListView, &lvi);

	return sBuf;
}

bool CListView::AddColumn(LPCTSTR cText, WORD wWidth)
{
	LVCOLUMN lvc = {0};
	lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
	lvc.fmt = LVCF_TEXT;
	
	lvc.pszText = (LPTSTR) cText;
	lvc.cx = wWidth;

	int res = ListView_InsertColumn(m_hListView, m_iLastColumnIndex, &lvc);
	if (res != -1) m_iLastColumnIndex++;

	return (res != -1);
}

int CListView::AddItem(LPCTSTR cText, LPARAM lParam) 
{
	LVITEM lvi = {0};
	lvi.iItem = MAXWORD;
	lvi.mask = LVIF_PARAM;
	lvi.lParam = lParam;

	int res = ListView_InsertItem(m_hListView, &lvi);
	return res;
}

bool CListView::DelItem(int iItem)
{
	return (ListView_DeleteItem(m_hListView, iItem) == TRUE);
}

bool CListView::SetSubItem(int iItem, int iSubItem, LPCTSTR cText)
{
	LVITEM lvi = {0};
	lvi.iItem = iItem;
	lvi.iSubItem = iSubItem;

	if (!ListView_GetItem(m_hListView, &lvi)) return false;

	lvi.mask = LVIF_TEXT;
	lvi.pszText = (LPTSTR) cText;

	return (ListView_SetItem(m_hListView, &lvi) == TRUE);
}

WORD CListView::GetColumnWidth(int iColumns)
{
	return ListView_GetColumnWidth(m_hListView, iColumns);
}

void CListView::Resize()
{
	RECT rcClient;
	GetClientRect(m_hParent, &rcClient);

	SetWindowPos(m_hListView, NULL, 0, 0, rcClient.right, rcClient.bottom, SWP_NOMOVE);
}

///////////////////////////////////////////////////////////////////////////////
// CTree
///////////////////////////////////////////////////////////////////////////////
CTree::CTree(HWND hTree)
{
	m_hParent = ::GetParent(hTree);
	m_hTree = hTree;
}

WORD CTree::GetCount()
{
	return TreeView_GetCount(m_hTree);
}

WORD CTree::GetChildCount(HTREEITEM hti)
{	
	WORD res = 0;
	HTREEITEM htiChild = TreeView_GetChild(m_hTree, hti);
	while (htiChild != NULL)
	{
		htiChild = TreeView_GetNextSibling(m_hTree, htiChild);
		res++;
	}
	return res;
}

HTREEITEM CTree::GetSel()
{
	return TreeView_GetSelection(m_hTree);
}

LPARAM CTree::GetItemParam(HTREEITEM hti)
{
	TVITEM tvi = {0};
	tvi.hItem = hti;
	tvi.mask = TVIF_PARAM;

	TreeView_GetItem(m_hTree, &tvi);
	return tvi.lParam;
}

LPARAM CTree::GetSelItemParam()
{
	return GetItemParam(GetSel());
}

HTREEITEM CTree::GetRoot()
{
	return TreeView_GetRoot(m_hTree);
}

HTREEITEM CTree::GetParent(HTREEITEM hti)
{
	return TreeView_GetParent(m_hTree, hti);
}

HTREEITEM CTree::GetChild(HTREEITEM hti)
{
	return TreeView_GetChild(m_hTree, hti);
}

HTREEITEM CTree::GetNextSibling_(HTREEITEM hti)
{
	return TreeView_GetNextSibling(m_hTree, hti);
}

HTREEITEM CTree::GetNextItem(HTREEITEM hti)
{
	HTREEITEM htiChild = TreeView_GetChild(m_hTree, hti);
	if (htiChild != NULL)
	{
		return htiChild;
	}

	HTREEITEM htiRes = TreeView_GetNextSibling(m_hTree, hti);

	if (htiRes == NULL)
	{
		HTREEITEM htiParent = hti;

		do
		{
			htiParent = TreeView_GetParent(m_hTree, htiParent);
			htiRes = TreeView_GetNextSibling(m_hTree, htiParent);
		} while (htiRes == NULL && htiParent != NULL);
	}

	return htiRes;
}

CString CTree::GetItemText(HTREEITEM hti)
{
	CString res;

	TVITEM tvi = {0};
	tvi.hItem = hti;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = res.Buf();
	tvi.cchTextMax = MAX_PATH;

	TreeView_GetItem(m_hTree, &tvi);

	return res;
}

HTREEITEM CTree::AddItem(LPCTSTR ccText, LPARAM lParam, HTREEITEM htiParent)
{
	TVINSERTSTRUCT tvis = {0};
	TVITEM tvi = {0};

	tvi.mask = TVIF_TEXT | TVIF_PARAM;
	tvi.lParam = lParam;
	tvi.pszText = (LPTSTR) ccText;

	tvis.hParent = htiParent;
	tvis.item = tvi;

	return TreeView_InsertItem(m_hTree, &tvis);
}

void CTree::SetItemText(HTREEITEM hti, LPCTSTR ccText)
{
	TVITEM tvi = {0};
	tvi.hItem = hti;
	tvi.mask = TVIF_TEXT;
	tvi.pszText = (LPTSTR) ccText;

	TreeView_SetItem(m_hTree, &tvi);
}

void CTree::ExpandItem(HTREEITEM hti)
{						
	TreeView_Expand(m_hTree, hti, TVE_EXPAND);
}

void CTree::ExpandAll()
{
	HTREEITEM hti = GetRoot();
	while (hti)
	{
		ExpandItem(hti);
		hti = GetNextItem(hti);
	}
}

void CTree::Select(HTREEITEM hti)
{
	TreeView_Select(m_hTree, hti, TVGN_CARET);
}

void CTree::DelAll()
{								
	TreeView_DeleteAllItems(m_hTree);
}

void CTree::DelItem(HTREEITEM hti)
{
	TreeView_DeleteItem(m_hTree, hti);
}