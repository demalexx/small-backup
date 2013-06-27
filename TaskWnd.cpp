#include "TaskWnd.h"

CTaskWnd* CurTaskWnd;

// Массив идентификаторов контролов, которые отображают св-ва ВБ
int aTBProps[] = {IDC_TB_PROPS_FROM, IDC_TB_PROPS_USE_PERIOD, IDC_TB_PROPS_TO};

// Массив ид-ов контролов, отвечающих за отображение св-в запуска задачи
int aExecuteProps[] = {IDC_EXECUTE_MIN_ST, IDC_EXECUTE_MIN, IDC_EXECUTE_DO_REPEAT, IDC_EXECUTE_REPEAT_MIN, IDC_EXECUTE_REPEAT_MIN_UD};

///////////////////////////////////////////////////////////////////////////////
// CTaskWnd
///////////////////////////////////////////////////////////////////////////////
CTaskWnd::CTaskWnd(HINSTANCE hInstance, HWND hParentWnd)
{
	// Теперь глобальная переменная ссылается на текущее окошко
	CurTaskWnd = this;

    static PROPSHEETPAGE psp[3];

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
	psp[0].lParam = (LPARAM) this;
    psp[0].hInstance = hInstance;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_NEW_TASK_SHEET);
	psp[0].pfnDlgProc = StaticGeneralSheetDlgProc;
    psp[0].pszTitle = TEXT("Общие параметры");

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
	psp[1].lParam = (LPARAM) this;
    psp[1].hInstance = hInstance;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_ARCHIVE_SHEET);
	psp[1].pfnDlgProc = StaticArchiveSheetDlgProc;
    psp[1].pszTitle = TEXT("Архивирование");

    psp[2].dwSize = sizeof(PROPSHEETPAGE);
    psp[2].dwFlags = PSP_USETITLE;
	psp[2].lParam = (LPARAM) this;
    psp[2].hInstance = hInstance;
	psp[2].pszTemplate = MAKEINTRESOURCE(IDD_SCHEDULE_SHEET);
	psp[2].pfnDlgProc = StaticScheduleSheetDlgProc;
    psp[2].pszTitle = TEXT("Расписание");

    m_PropSheetHdr.dwSize = sizeof(PROPSHEETHEADER);
	m_PropSheetHdr.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP | PSH_NOAPPLYNOW;
    m_PropSheetHdr.hwndParent = hParentWnd;
    m_PropSheetHdr.hInstance = hInstance;
    m_PropSheetHdr.pszCaption = (LPTSTR) TEXT("Новая задача");
    m_PropSheetHdr.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    m_PropSheetHdr.nStartPage = 0;
    m_PropSheetHdr.ppsp = (LPCPROPSHEETPAGE) psp;
}

CTaskWnd::~CTaskWnd()
{
	CurTaskWnd = NULL;
}

BOOL CALLBACK CTaskWnd::StaticGeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
	return CurTaskWnd->GeneralSheetDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK CTaskWnd::StaticArchiveSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
	return CurTaskWnd->ArchiveSheetDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK CTaskWnd::StaticScheduleSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{ 
	return CurTaskWnd->ScheduleSheetDlgProc(hwnd, uMsg, wParam, lParam);
}

int CALLBACK CTaskWnd::BFFCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED)
		SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM) true, 
		(LPARAM) ((CString*) lpData)->C());

	return 0;
}

CString CTaskWnd::BrowseForFolder(HWND hOwner, CString sText, CString sRoot)
{
	BROWSEINFO bi = {0};
	bi.hwndOwner = hOwner;
	bi.lpszTitle = sText.C();
	bi.lpfn = BFFCallbackProc;
	bi.lParam = (LPARAM) &sRoot;
	bi.ulFlags = BIF_NEWDIALOGSTYLE | /*BIF_NONEWFOLDERBUTTON |*/
		BIF_RETURNONLYFSDIRS | BIF_EDITBOX;

	IMalloc* pim = NULL;
	SHGetMalloc(&pim);

	ITEMIDLIST* pidl = SHBrowseForFolder(&bi);
	if (pidl != NULL)
	{
		TCHAR buf[MAX_PATH];
		SHGetPathFromIDList(pidl, buf);
		CString sSelected = buf;

		pim->Free(pidl);
		pim->Release();

		return sSelected;
	}

	return TEXT("");
}

void CTaskWnd::UpdateGenResult()
{
	CString sGenPath = GetDlgItemStr(m_hGeneralSheet, IDC_DEST_FOLDER) + 
		GetDlgItemStr(m_hGeneralSheet, IDC_GENERATE_NAME);

	m_bInvalidDestPath = !IsPathValid(sGenPath.C());
	CString sFormatedPath = FormatDateTime(sGenPath.C());

	RECT rc;
	GetWindowRect(GetDlgItem(m_hGeneralSheet, IDC_GENERATE_RESULT), &rc);

	TCHAR buf[MAX_PATH];
	lstrcpy(buf, sFormatedPath.C());
	PathCompactPath(GetDC(GetDlgItem(m_hGeneralSheet, IDC_GENERATE_RESULT)), buf, rc.right - rc.left);

	SetDlgItemText(m_hGeneralSheet, IDC_GENERATE_RESULT, buf);
}

bool CTaskWnd::FillScheduleTree(HTREEITEM htiCurItem, PTBTreeItem pCurItem)
{
	// Выделим память и скопируем информацию о временном блоке
	PTimeBlock ptb = new STimeBlock();
	CopyMemory(ptb, &pCurItem->TimeBlock, sizeof(STimeBlock));

	HTREEITEM hti = m_pTimeBlocksTree->AddItem(Schedule->TimeBlockToString(ptb).C(), (LPARAM) ptb, htiCurItem);

	// Добавим детей
	for (BYTE i = 0; i < pCurItem->aChilds.Size(); i++)
	{
		FillScheduleTree(hti, pCurItem->aChilds[i]);
	}

	return true;
}

PTBTreeItem CTaskWnd::TreeToScheduleTree(HTREEITEM htiCurItem, PTBTreeItem pParent)
{
	// Создадим новый эл-т дерева расписания
	PTBTreeItem pCurItem = new STBTreeItem();
	pCurItem->pParent = pParent;
	
	// Заполним его данными с текущего эл-та компонента "дерево"
	pCurItem->TimeBlock = *((PTimeBlock) m_pTimeBlocksTree->GetItemParam(htiCurItem));

	WORD wChilds = m_pTimeBlocksTree->GetChildCount(htiCurItem);

	// Получим первого ребёнка
	HTREEITEM htiCurChild = m_pTimeBlocksTree->GetChild(htiCurItem);
    for (WORD i = 0; i < wChilds; i++)
	{
		// Вызываем себя рекурсивно для каждого ребёнка
		pCurItem->aChilds.Add(TreeToScheduleTree(htiCurChild, pCurItem));

		// Получим след. ребёнка
		htiCurChild = m_pTimeBlocksTree->GetNextSibling_(htiCurChild);
	}

	// Возвращаем созданный выше эл-т дерева расписания
	return pCurItem;
}

void CTaskWnd::ScheduleFillTBPropsCombo(HWND hCombo, BYTE bTBType)
{
	SendMessage(hCombo, CB_RESETCONTENT, 0, 0);
	CCombo cb(hCombo);

	switch (bTBType)
	{
	case TIME_BLOCK_WEEK_CHILD:
		{
			for (BYTE i = 1; i <= 4; i++)
			{
				CString sWeekNum(3);
				cb.AddString(sWeekNum.FromInt(i).C(), i);
			}
		}
		break;

	case TIME_BLOCK_DAY_CHILD:
		{
			for (BYTE i = 1; i <= 7; i++)
			{
				cb.AddString(DayOfWeekToStr(i).C(), i);	
			}
		}
		break;

	case TIME_BLOCK_HOUR_CHILD:
		{
			for (BYTE i = 0; i <= 23; i++)
			{
				CString sHour(3);
				cb.AddString(sHour.FromInt(i).C(), i);
			}
		}
		break;
	}
}

void CTaskWnd::SchedulePrintNextRun()
{
	CString sNextRun(TEXT("-"));
	PTBTreeItem ptbt = TreeToScheduleTree(m_pTimeBlocksTree->GetRoot());

	SYSTEMTIME stNextRun = Schedule->GetNextRun(ptbt, DTNow()).GetSystemTime();
	if (stNextRun.wYear == 0)
	{
		SetDlgItemText(m_hScheduleSheet, IDC_NEXT_RUN_ST, sNextRun.C());
		return;
	}

	CString sBuf;
	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, sBuf.Buf(), sBuf.BufLen());
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, &stNextRun, sBuf.C(), sNextRun.Buf(), sNextRun.BufLen());

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, sBuf.Buf(), sBuf.BufLen());
	CString sNextRunDate;
	GetDateFormat(LOCALE_USER_DEFAULT, 0, &stNextRun, sBuf.C(), sNextRunDate.Buf(), sNextRunDate.BufLen());

	sNextRun += TEXT(" ");
	sNextRun += sNextRunDate;
	SetDlgItemText(m_hScheduleSheet, IDC_NEXT_RUN_ST, sNextRun.C());
}

bool CTaskWnd::GeneralOnInit()
{
	if (m_bDialogMode == NEW_TASK)
		SetDlgItemText(m_hGeneralSheet, IDC_TASK_NAME, TEXT("Новая задача"));
	else if (m_bDialogMode == CHANGE_TASK)
		SetDlgItemText(m_hGeneralSheet, IDC_TASK_NAME, m_TaskInfo.sName.C());
    
 	SetDlgItemText(m_hGeneralSheet, IDC_SRC_FOLDER, m_TaskInfo.sSrcFolder.C());
	SetDlgItemText(m_hGeneralSheet, IDC_DEST_FOLDER, m_TaskInfo.sDestFolder.C());
	SetDlgItemText(m_hGeneralSheet, IDC_GENERATE_NAME, m_TaskInfo.sDestGenName.C());
	SetDlgItemText(m_hGeneralSheet, IDC_INCLUDE_MASK, m_TaskInfo.sIncludeMask.C());
	SetDlgItemText(m_hGeneralSheet, IDC_EXCLUDE_MASK, m_TaskInfo.sExcludeMask.C());

	SendDlgItemMessage(m_hGeneralSheet, IDC_SUBFOLDERS, BM_SETCHECK, m_TaskInfo.bSubFolders ? BST_CHECKED : BST_UNCHECKED, 0);

	for (BYTE t = 0; t < Settings->aGenTemplates.Size(); t++)
		SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_ADDSTRING, 0, (LPARAM) Settings->aGenTemplates[t]->C());

	UpdateGenResult();
	return true;
}

bool CTaskWnd::GeneralOnChooseSrcFolder()
{
	CString sSrcFolder = GetDlgItemStr(m_hGeneralSheet, IDC_SRC_FOLDER);	

	CString sSel = BrowseForFolder(m_hGeneralSheet, CString(TEXT("Выберите исходную папку")), sSrcFolder);
	if (!sSel.Empty())
		SetDlgItemText(m_hGeneralSheet, IDC_SRC_FOLDER, sSel.C());
	return true;
}

bool CTaskWnd::GeneralOnChooseDestFolder()
{
	CString sDestFolder = GetDlgItemStr(m_hGeneralSheet, IDC_DEST_FOLDER);

	CString sSel = BrowseForFolder(m_hGeneralSheet, CString(TEXT("Выберите папку назначения")), sDestFolder);
	AddSlash(sSel);

	if (!sSel.Empty())
		SetDlgItemText(m_hGeneralSheet, IDC_DEST_FOLDER, sSel.C());
	return true;
}

bool CTaskWnd::GeneralOnUpdateDestFolder()
{
	UpdateGenResult();
	return true;
}

bool CTaskWnd::GeneralOnUpdateGenName()
{
	UpdateGenResult();
	return true;
}

bool CTaskWnd::GeneralOnSelChangeGenName()
{
	TCHAR buf[MAX_PATH];
	int iSel = SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_GETCURSEL, 0, 0);
	
	if (iSel != CB_ERR)
	{
		SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_GETLBTEXT, iSel, (LPARAM) buf);
		SetDlgItemText(m_hGeneralSheet, IDC_GENERATE_NAME, buf);
	}
	UpdateGenResult();
	return true;
}

bool CTaskWnd::GeneralOnAddTemplate()
{
	TCHAR buf[MAX_PATH];
	GetDlgItemText(m_hGeneralSheet, IDC_GENERATE_NAME, buf, MAX_PATH);
	SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_ADDSTRING, 0, (LPARAM) buf);

	Settings->aGenTemplates.Add(new CString(buf));

	return true;
}

bool CTaskWnd::GeneralOnDelTemplate()
{
	int iSel = SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_GETCURSEL, 0, 0);
	if (iSel != CB_ERR)
	{
		SendDlgItemMessage(m_hGeneralSheet, IDC_GENERATE_NAME, CB_DELETESTRING, (WPARAM) iSel, 0);
		Settings->aGenTemplates.Delete(iSel);
	}
	return true;
}

bool CTaskWnd::GeneralOnExit()
{
	bool res = true;

	if (m_bDialogMode == CHANGE_TASK && GetDlgItemStr(m_hGeneralSheet, IDC_TASK_NAME) == m_TaskInfo.sName)
		res = true;
	else if (TaskList->FindTaskByName(GetDlgItemStr(m_hGeneralSheet, IDC_TASK_NAME)) != NULL)
	{
		MessageBox(GetParent(m_hGeneralSheet), TEXT("Задача с таким именем уже существует. Укажите другое имя"),
			MessageCaption, MB_OK | MB_ICONINFORMATION);
		res = false;
	}

	if (res)
	{
		m_TaskInfo.sName = GetDlgItemStr(m_hGeneralSheet, IDC_TASK_NAME);
		m_TaskInfo.sSrcFolder = GetDlgItemStr(m_hGeneralSheet, IDC_SRC_FOLDER);
		m_TaskInfo.sDestFolder = GetDlgItemStr(m_hGeneralSheet, IDC_DEST_FOLDER);
		m_TaskInfo.sDestGenName = GetDlgItemStr(m_hGeneralSheet, IDC_GENERATE_NAME);
		m_TaskInfo.sIncludeMask = GetDlgItemStr(m_hGeneralSheet, IDC_INCLUDE_MASK);
		m_TaskInfo.sExcludeMask = GetDlgItemStr(m_hGeneralSheet, IDC_EXCLUDE_MASK);
		m_TaskInfo.bSubFolders = (SendDlgItemMessage(m_hGeneralSheet, IDC_SUBFOLDERS, BM_GETCHECK, 0, 0) == BST_CHECKED);
	}


	// Проверим что бы папка назначения не была подпапкой иходной папки
	// например, src: 'c:\', dest: 'c:\backup'
	if (res)
	{
		CString sDestFolder = m_TaskInfo.sDestFolder + FormatDateTime(m_TaskInfo.sDestGenName.C());

		if (IsSubFolder(m_TaskInfo.sSrcFolder, sDestFolder))
		{
			MessageBox(GetParent(m_hGeneralSheet), TEXT("Нельзя выбирать папку назначения как подпапку исходной папки!\n\
Например, так делать нельзя:\n\
Исходная папка: C:\\\n\
Папка назначения: C:\\Backup"),
				MessageCaption, MB_OK | MB_ICONWARNING);

			res = false;
		}
	}

	return res;
}

bool CTaskWnd::ArchiveOnInit()
{
	// Заполняем выпадающий список методами сжатия архиватора
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Без сжатия"));
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Скоростной"));
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Быстрый"));
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Обычный"));
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Хороший"));
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_ADDSTRING, 0, (LPARAM) TEXT("Наилучший"));

	// Устанавливаем галочки согласно информации о задаче
	SendDlgItemMessage(m_hArchiveSheet, IDC_DO_ARCHIVE, BM_SETCHECK, m_TaskInfo.bDoArchive ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_SETCURSEL, m_TaskInfo.bArchCompress, 0);
	SendDlgItemMessage(m_hArchiveSheet, IDC_SFX, BM_SETCHECK, m_TaskInfo.bArchSFX ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(m_hArchiveSheet, IDC_DEL_FILES, BM_SETCHECK, m_TaskInfo.bArchDelFiles ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage(m_hArchiveSheet, IDC_LOCK, BM_SETCHECK, m_TaskInfo.bArchLock ? BST_CHECKED : BST_UNCHECKED, 0);
	SetDlgItemText(m_hArchiveSheet, IDC_TASK_CMD, m_TaskInfo.sArchTaskCmd.C());

	return true;
}

bool CTaskWnd::ArchiveOnExit()
{
	m_TaskInfo.bDoArchive = (SendDlgItemMessage(m_hArchiveSheet, IDC_DO_ARCHIVE, BM_GETCHECK, 0, 0) == BST_CHECKED);
	m_TaskInfo.bArchCompress = SendDlgItemMessage(m_hArchiveSheet, IDC_COMPRESS_LEVEL, CB_GETCURSEL, 0, 0);
	m_TaskInfo.bArchSFX = (SendDlgItemMessage(m_hArchiveSheet, IDC_SFX, BM_GETCHECK, 0, 0) == BST_CHECKED);
	m_TaskInfo.bArchDelFiles = (SendDlgItemMessage(m_hArchiveSheet, IDC_DEL_FILES, BM_GETCHECK, 0, 0) == BST_CHECKED);
	m_TaskInfo.bArchLock = (SendDlgItemMessage(m_hArchiveSheet, IDC_LOCK, BM_GETCHECK, 0, 0) == BST_CHECKED);
	m_TaskInfo.sArchTaskCmd = GetDlgItemStr(m_hArchiveSheet, IDC_TASK_CMD);

	return true;
}

bool CTaskWnd::ScheduleOnInit()
{
	SendDlgItemMessage(m_hScheduleSheet, IDC_SCHEDULE_ACTIVE, BM_SETCHECK, m_TaskInfo.bScheduled ? BST_CHECKED : BST_UNCHECKED, 0);

	SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN_UD, UDM_SETBUDDY, (WPARAM) GetDlgItem(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN), 0);
	SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN_UD, UDM_SETRANGE, 0, MAKELONG(1440, 0));

	// Заполним минуты для времени выполнения и для повторения (0-60)
	for (BYTE i = 0; i < 60; i++)
	{
		CString sMin(3);
		SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_MIN, CB_ADDSTRING, 0, (LPARAM) sMin.FromInt(i).C());
		if (i > 0) SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN, CB_ADDSTRING, 0, (LPARAM) sMin.FromInt(i).C());
	}								   

	m_pTimeBlocksTree = new CTree(GetDlgItem(m_hScheduleSheet, IDC_TIME_BLOCKS_TREE));

	// Заполним список возможных типов главного ВБ
	CCombo cb(GetDlgItem(m_hScheduleSheet, IDC_MAIN_TB_TYPE));
	cb.AddString(TEXT("Неделя"), TIME_BLOCK_WEEK);
	cb.AddString(TEXT("День"), TIME_BLOCK_DAY);
	cb.AddString(TEXT("Час"), TIME_BLOCK_HOUR);
				
	// Построим дерево расписания
	PTBTreeItem pTree = Schedule->BuildScheduleTree(m_TaskInfo.sSchedule);
	
	// Перенесём это дерево в контрол "дерево"
	FillScheduleTree(TVI_ROOT, pTree);
	
	// Выделим соотв. эл-т выпадающего списка типа главного временного блока
	cb.SelItemByData(pTree->TimeBlock.bType);

	// Освободим ненужную память
	Schedule->FreeScheduleTree(pTree);

	m_pTimeBlocksTree->ExpandAll();
	return true;
}

bool CTaskWnd::ScheduleOnTimeBlocksTreeSel()
{
	STimeBlock* pTimeBlock = (STimeBlock*) m_pTimeBlocksTree->GetItemParam(m_pTimeBlocksTree->GetSel());

	// Настроим активность кнопки "Добавить"\"Удалить"
	EnableWindow(GetDlgItem(m_hScheduleSheet, IDC_TIME_BLOCK_ADD), 
		(pTimeBlock->bType != TIME_BLOCK_HOUR_CHILD && pTimeBlock->bType != TIME_BLOCK_HOUR));

	EnableWindow(GetDlgItem(m_hScheduleSheet, IDC_TIME_BLOCK_DEL), 
		(m_pTimeBlocksTree->GetParent(m_pTimeBlocksTree->GetSel()) != NULL));

	// Заполним поля "от" и "до" значениями
	ScheduleFillTBPropsCombo(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_FROM), pTimeBlock->bType);
	ScheduleFillTBPropsCombo(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_TO), pTimeBlock->bType);

	// Скроем ненужные для данного временного блока параметры
	bool bHideTBProps = (pTimeBlock->bType == TIME_BLOCK_MONTH || pTimeBlock->bType == TIME_BLOCK_WEEK || 
		pTimeBlock->bType == TIME_BLOCK_DAY || pTimeBlock->bType == TIME_BLOCK_HOUR);

	if (bHideTBProps) SetDlgItemText(m_hScheduleSheet, IDC_TB_PROPS_FROM_ST, TEXT("Нет свойств"));

	// Скроем\покажем "параметры временного блока" (контролы относящиеся к этой группе)
	for (BYTE i = 0; i < sizeof(aTBProps) / sizeof(aTBProps[0]); i++)
		ShowWindow(GetDlgItem(m_hScheduleSheet, aTBProps[i]), bHideTBProps ? SW_HIDE : SW_SHOW);

	// Скроем\покажем св-ва "выполнение задачи"
	bool bHideExecuteProps = (pTimeBlock->bType != TIME_BLOCK_HOUR && pTimeBlock->bType != TIME_BLOCK_HOUR_CHILD);
	for (BYTE i = 0; i < sizeof(aExecuteProps) / sizeof(aExecuteProps[0]); i++)
		ShowWindow(GetDlgItem(m_hScheduleSheet, aExecuteProps[i]), bHideExecuteProps ? SW_HIDE : SW_SHOW);

	CCombo cbFrom(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_FROM));
	CCombo cbTo(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_TO));

	// Зададим контролам значения параметров временного блока
	if (pTimeBlock->bType == TIME_BLOCK_WEEK_CHILD)
	{
		SetDlgItemText(m_hScheduleSheet, IDC_TB_PROPS_FROM_ST, TEXT("Неделя"));
		cbFrom.SelItemByData(pTimeBlock->bWeekNumber);
		cbTo.SelItemByData(pTimeBlock->bWeekNumberTo);
	}
	else if (pTimeBlock->bType == TIME_BLOCK_DAY_CHILD)
	{
		SetDlgItemText(m_hScheduleSheet, IDC_TB_PROPS_FROM_ST, TEXT("День недели"));
		cbFrom.SelItemByData(pTimeBlock->bDayDayOfWeek);
		cbTo.SelItemByData(pTimeBlock->bDayDayOfWeekTo);
	}
	else if (pTimeBlock->bType == TIME_BLOCK_HOUR || pTimeBlock->bType == TIME_BLOCK_HOUR_CHILD)
	{
		if (pTimeBlock->bType == TIME_BLOCK_HOUR_CHILD)
		{
			SetDlgItemText(m_hScheduleSheet, IDC_TB_PROPS_FROM_ST, TEXT("Час"));

			cbFrom.SelItemByData(pTimeBlock->bHourHour);
			cbTo.SelItemByData(pTimeBlock->bHourHourTo);
		}

		SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_MIN, CB_SETCURSEL, pTimeBlock->bMinExecute, 0);
		SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN, CB_SETCURSEL, pTimeBlock->wRepeatMin - 1, 0);

		SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_DO_REPEAT, BM_SETCHECK, pTimeBlock->bUseRepeat ? BST_CHECKED : BST_UNCHECKED, 0);
		SetDlgItemInt(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN, pTimeBlock->wRepeatMin, FALSE);
		ScheduleOnDoRepeatClick(pTimeBlock->bUseRepeat);
	}

	SendDlgItemMessage(m_hScheduleSheet, IDC_TB_PROPS_USE_PERIOD, BM_SETCHECK, pTimeBlock->bUseTo ? BST_CHECKED : BST_UNCHECKED, 0);
	ScheduleOnTBPropsUsePeriodClick(pTimeBlock->bUseTo);
	return true;
}

bool CTaskWnd::ScheduleOnAddTimeBlock()
{
	if (m_pTimeBlocksTree->GetSel() == NULL && m_pTimeBlocksTree->GetCount() > 0)
		return false;

	PTimeBlock pTBParent = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();

	STimeBlock* pTimeBlock = new STimeBlock();
	
	BYTE bTBChilds = m_pTimeBlocksTree->GetChildCount(m_pTimeBlocksTree->GetSel());

	if (pTBParent == NULL)
		pTimeBlock->bType = TIME_BLOCK_WEEK;
	else if (bTBChilds < Schedule->GetTimeBlockMaxChilds(pTBParent->bType))
	{
		if (pTBParent->bType == TIME_BLOCK_MONTH)
		{
			pTimeBlock->bType = TIME_BLOCK_WEEK_CHILD;
		}
		if (pTBParent->bType == TIME_BLOCK_WEEK || pTBParent->bType == TIME_BLOCK_WEEK_CHILD)
		{
			pTimeBlock->bType = TIME_BLOCK_DAY_CHILD;
		}
		else if (pTBParent->bType == TIME_BLOCK_DAY || pTBParent->bType == TIME_BLOCK_DAY_CHILD)
		{
			pTimeBlock->bType = TIME_BLOCK_HOUR_CHILD;
		}		
	}
	else
	{
		delete pTimeBlock;
		return false;
	}

	m_pTimeBlocksTree->AddItem(Schedule->TimeBlockToString(pTimeBlock).C(), (LPARAM) pTimeBlock, m_pTimeBlocksTree->GetSel());
	m_pTimeBlocksTree->ExpandItem(m_pTimeBlocksTree->GetSel());
	SchedulePrintNextRun();

	return true;
}

bool CTaskWnd::ScheduleOnDelTimeBlock()
{
	HTREEITEM htiSel = m_pTimeBlocksTree->GetSel();

	// Нельзя удалить корневой эл-т
	if (m_pTimeBlocksTree->GetParent(htiSel) == NULL)
		return false;

	if (MessageBox(GetParent(m_hScheduleSheet), TEXT("Удалить выделенный временной блок?"), MessageCaption, MB_ICONQUESTION | MB_YESNOCANCEL) == IDYES)
	{
		m_pTimeBlocksTree->DelItem(htiSel);
		SchedulePrintNextRun();
	}
	return true;
}

bool CTaskWnd::ScheduleOnMainTBTypeChange()
{
	if (MessageBox(GetParent(m_hScheduleSheet),
		TEXT("При изменении типа главного блока всё дерево расписания будет потеряно\nИзменить тип главного временного блока?"),
		MessageCaption, MB_ICONQUESTION | MB_YESNOCANCEL) == IDYES)
	{
		m_pTimeBlocksTree->DelAll();

		PTimeBlock pTB = new STimeBlock();	 
		BYTE bCurSel = SendDlgItemMessage(m_hScheduleSheet, IDC_TIME_BLOCK_TYPE, CB_GETCURSEL, 0, 0);
		pTB->bType = SendDlgItemMessage(m_hScheduleSheet, IDC_TIME_BLOCK_TYPE, CB_GETITEMDATA, bCurSel, 0);

		HTREEITEM hti = m_pTimeBlocksTree->AddItem(Schedule->TimeBlockToString(pTB).C(), (LPARAM) pTB, NULL);
		m_pTimeBlocksTree->Select(hti);
		SchedulePrintNextRun();
	}

	return true;
}

bool CTaskWnd::ScheduleOnTBPropsFromChange(int iIndex)
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();

	CCombo cb(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_FROM));

	switch (ptb->bType)
	{
	case TIME_BLOCK_WEEK_CHILD: ptb->bWeekNumber = cb.GetSelData(); break;
	case TIME_BLOCK_DAY_CHILD: ptb->bDayDayOfWeek = cb.GetSelData(); break;
	case TIME_BLOCK_HOUR_CHILD: ptb->bHourHour = cb.GetSelData(); break;
	}
	
	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C());
	SchedulePrintNextRun();
	return true;
}

bool CTaskWnd::ScheduleOnTBPropsToChange(int iIndex)
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();
	CCombo cb(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_TO));

	switch (ptb->bType)
	{
	case TIME_BLOCK_WEEK_CHILD: ptb->bWeekNumberTo = cb.GetSelData(); break;
	case TIME_BLOCK_DAY_CHILD: ptb->bDayDayOfWeekTo = cb.GetSelData(); break;
	case TIME_BLOCK_HOUR_CHILD: ptb->bHourHourTo = cb.GetSelData(); break;
	}
	
	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C());
	SchedulePrintNextRun();
	return true;
}

bool CTaskWnd::ScheduleOnTBPropsUsePeriodClick(bool bCheck)
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();
	ptb->bUseTo = bCheck;

	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C());
	EnableWindow(GetDlgItem(m_hScheduleSheet, IDC_TB_PROPS_TO), bCheck);	
	SchedulePrintNextRun();
	return true;
}

bool CTaskWnd::ScheduleOnExecuteChange(int iIndex)
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();
	ptb->bMinExecute = iIndex;

	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C());
	SchedulePrintNextRun();
	return true;
}

bool CTaskWnd::ScheduleOnDoRepeatClick(bool bCheck)
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();
	ptb->bUseRepeat = bCheck;

	EnableWindow(GetDlgItem(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN), bCheck);
	RedrawWindow(GetDlgItem(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN_UD), NULL, NULL, RDW_INVALIDATE);

	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C());
	SchedulePrintNextRun();

	return true;
}

bool CTaskWnd::ScheduleOnRepeatChange()
{
	PTimeBlock ptb = (PTimeBlock) m_pTimeBlocksTree->GetSelItemParam();

	CString sValue(10);
	GetDlgItemText(m_hScheduleSheet, IDC_EXECUTE_REPEAT_MIN, sValue.Buf(10), sValue.BufLen());

	ptb->wRepeatMin = sValue.ToInt();

	m_pTimeBlocksTree->SetItemText(m_pTimeBlocksTree->GetSel(), Schedule->TimeBlockToString(ptb).C()); 
	SchedulePrintNextRun();

	return true;
}

bool CTaskWnd::ScheduleOnExit()
{
	m_TaskInfo.bScheduled = (SendDlgItemMessage(m_hScheduleSheet, IDC_SCHEDULE_ACTIVE, BM_GETCHECK, 0, 0) == BST_CHECKED);

	// Создаём дерево расписания на основе эл-ов компонента "дерево"
	PTBTreeItem pRoot = TreeToScheduleTree(m_pTimeBlocksTree->GetRoot());

	// Преобразуем полученное дерево в строку
	m_TaskInfo.sSchedule = Schedule->ScheduleTreeToString(pRoot);

	return true;
}

BOOL CTaskWnd::GeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		m_hGeneralSheet = hwnd;
		GeneralOnInit();

		return TRUE;
		break;

	case WM_CTLCOLORSTATIC:
		{
			if ((HWND) lParam == GetDlgItem(m_hGeneralSheet, IDC_GENERATE_RESULT))
			{
				HDC hdc = (HDC) wParam;

				if (m_bInvalidDestPath)
					SetTextColor(hdc, RGB(255, 0, 0));

				SetBkMode(hdc, TRANSPARENT);

				return (BOOL) NULL;
			}
			else
				return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHOOSE_SRC_FOLDER:
			GeneralOnChooseSrcFolder();
			break;

		case IDC_CHOOSE_DEST_FOLDER:
			GeneralOnChooseDestFolder();
			break;

		case IDC_DEST_FOLDER:
			if (HIWORD(wParam) == EN_UPDATE)
				GeneralOnUpdateDestFolder();	
			break;

		case IDC_GENERATE_NAME:
			{
				if (HIWORD(wParam) == CBN_EDITUPDATE)
				{
					GeneralOnUpdateGenName();
				}
				else if (HIWORD(wParam) == CBN_SELCHANGE)
				{
					GeneralOnSelChangeGenName();
				}
			}
			break;

		case IDC_ADD_TEMPLATE:
			GeneralOnAddTemplate();

			break;

		case IDC_DEL_TEMPLATE:
			GeneralOnDelTemplate();
			break;
		}	

		break;	

	case WM_NOTIFY:
		switch (((NMHDR*) lParam)->code)
		{
		case PSN_KILLACTIVE:
			SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);

			return TRUE;
			break;

		case PSN_APPLY:
			if (GeneralOnExit())
				SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, FALSE);
			else
                SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, TRUE);

			return TRUE;
			break;
		}
        break;

	default:
		return FALSE;
	}
}

BOOL CTaskWnd::ArchiveSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hArchiveSheet = hwnd;
			ArchiveOnInit();
		}

		return TRUE;
		break;

	case WM_NOTIFY:
		switch (((NMHDR*) lParam)->code)
		{
		case PSN_KILLACTIVE:
			SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);
			return TRUE;
			break;

		case PSN_APPLY:
			ArchiveOnExit();
			SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, FALSE);
			break;
		}
        break;

	default:
		return FALSE;
	}
}

BOOL CTaskWnd::ScheduleSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
			m_hScheduleSheet = hwnd;
			ScheduleOnInit();

		return TRUE;
		break;

	case WM_COMMAND:										  
		if (LOWORD(wParam) == IDC_TIME_BLOCK_TYPE && HIWORD(wParam) == LBN_SELCHANGE)
			ScheduleOnMainTBTypeChange();

		else if (LOWORD(wParam) == IDC_TB_PROPS_FROM && HIWORD(wParam) == LBN_SELCHANGE)
			ScheduleOnTBPropsFromChange(SendDlgItemMessage(m_hScheduleSheet, IDC_TB_PROPS_FROM, CB_GETCURSEL, 0, 0));
		else if (LOWORD(wParam) == IDC_TB_PROPS_TO && HIWORD(wParam) == LBN_SELCHANGE)
			ScheduleOnTBPropsToChange(SendDlgItemMessage(m_hScheduleSheet, IDC_TB_PROPS_TO, CB_GETCURSEL, 0, 0));

		else if (LOWORD(wParam) == IDC_TB_PROPS_USE_PERIOD && HIWORD(wParam) == BN_CLICKED)
			ScheduleOnTBPropsUsePeriodClick(SendDlgItemMessage(m_hScheduleSheet, IDC_TB_PROPS_USE_PERIOD, BM_GETCHECK, 0, 0) == BST_CHECKED);

		else if (LOWORD(wParam) == IDC_EXECUTE_MIN && HIWORD(wParam) == LBN_SELCHANGE)
			ScheduleOnExecuteChange(SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_MIN, CB_GETCURSEL, 0, 0));
		else if (LOWORD(wParam) == IDC_EXECUTE_DO_REPEAT && HIWORD(wParam) == BN_CLICKED)
			ScheduleOnDoRepeatClick(SendDlgItemMessage(m_hScheduleSheet, IDC_EXECUTE_DO_REPEAT, BM_GETCHECK, 0, 0) == BST_CHECKED);
		else if (LOWORD(wParam) == IDC_EXECUTE_REPEAT_MIN && HIWORD(wParam) == EN_UPDATE)
			ScheduleOnRepeatChange();

		else if (LOWORD(wParam) == IDC_TIME_BLOCK_ADD)
			ScheduleOnAddTimeBlock();
		else if (LOWORD(wParam) == IDC_TIME_BLOCK_DEL)
			ScheduleOnDelTimeBlock();

	//	else if (HIWORD(wParam) == EN_UPDATE && LOWORD(wParam) == IDC_DAY)
	//	{
	//		UpdateScheduleDateTime(); 
	//	}
		break;

	case WM_NOTIFY:
		switch (((NMHDR*) lParam)->code)
		{
	//	case DTN_DATETIMECHANGE:
	//		UpdateScheduleDateTime();
	//		break;

		case TVN_SELCHANGED:
			ScheduleOnTimeBlocksTreeSel();
			break;
	//		
		case TVN_DELETEITEM:
			{
				// Удалим данные, связанные с каждым эл-ом дерева
				NMTREEVIEW* nmtv = (NMTREEVIEW*) lParam;
				delete (PTimeBlock) nmtv->itemOld.lParam;
			}
			break;

		case PSN_KILLACTIVE:
			SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);
			return TRUE;
			break;

		case PSN_APPLY:
			ScheduleOnExit();
			SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, FALSE);
			break;
		}
        break;

	default:
		return FALSE;
	}
}



bool CTaskWnd::ShowNewTask()
{
	m_bDialogMode = NEW_TASK;
	m_PropSheetHdr.pszCaption = TEXT("Новая задача");
	int res = PropertySheet(&m_PropSheetHdr);

	return (res == IDOK);
}

bool CTaskWnd::ShowChangeTask()
{
	m_bDialogMode = CHANGE_TASK;
	m_PropSheetHdr.pszCaption = TEXT("Изменить задачу");
	int res = PropertySheet(&m_PropSheetHdr);

	return (res == IDOK);
}