#include "OptionsWnd.h"

COptionsWnd* CurOptionsWnd;

COptionsWnd::COptionsWnd(HWND hParent)
{
	// Настроим глобальный указатель на этот класс
	CurOptionsWnd = this;

    static PROPSHEETPAGE psp[2];

    psp[0].dwSize = sizeof(PROPSHEETPAGE);
    psp[0].dwFlags = PSP_USETITLE;
	psp[0].lParam = (LPARAM) this;
    psp[0].hInstance = Instance;
	psp[0].pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS_SHEET);
	psp[0].pfnDlgProc = StaticGeneralSheetDlgProc;
    psp[0].pszTitle = TEXT("Общие настройки");

    psp[1].dwSize = sizeof(PROPSHEETPAGE);
    psp[1].dwFlags = PSP_USETITLE;
	psp[1].lParam = (LPARAM) this;
    psp[1].hInstance = Instance;
	psp[1].pszTemplate = MAKEINTRESOURCE(IDD_ARCHIVE_SET);
	psp[1].pfnDlgProc = StaticArchivesSheetDlgProc;
    psp[1].pszTitle = TEXT("Архиваторы");

    m_PropSheetHdr.dwSize = sizeof(PROPSHEETHEADER);
	m_PropSheetHdr.dwFlags = PSH_PROPSHEETPAGE | PSH_NOCONTEXTHELP | PSH_NOAPPLYNOW;
    m_PropSheetHdr.hwndParent = hParent;
    m_PropSheetHdr.hInstance = Instance;
    m_PropSheetHdr.pszCaption = (LPTSTR) TEXT("Настройки");
    m_PropSheetHdr.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    m_PropSheetHdr.nStartPage = 0;
    m_PropSheetHdr.ppsp = (LPCPROPSHEETPAGE) psp;

	m_bLockEdits = false;
}

COptionsWnd::~COptionsWnd()
{
}

BOOL CALLBACK COptionsWnd::StaticGeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CurOptionsWnd->GeneralSheetDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK COptionsWnd::StaticArchivesSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CurOptionsWnd->ArchivesSheetDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL CALLBACK COptionsWnd::StaticLoadPresetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CurOptionsWnd->LoadPresetDlgProc(hwnd, uMsg, wParam, lParam);
}

BOOL COptionsWnd::GeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hGeneralSheet = hwnd;
			GeneralOnInit();

			return TRUE;
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHOOSE_SOUND:
			GeneralOnChooseSound();
			break;

		case IDC_PlAY_SOUND:
			GeneralOnPlayFile();
			break;

		case IDC_STOP_SOUND:
			GeneralOnStopFile();
			break;
		}
		break;

		case WM_NOTIFY:
			switch (((NMHDR*) lParam)->code)
			{
			case PSN_KILLACTIVE:
				{
					SetWindowLong(hwnd, DWL_MSGRESULT, FALSE);
				}

				return TRUE;
				break;

			case PSN_APPLY:
				GeneralOnExit();
				SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, FALSE);
				break;
			}
			break;
	}

	return FALSE;
}

BOOL COptionsWnd::ArchivesSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		m_hArchiveSheet = hwnd;
		ArchOnInit();

		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LOAD_PRESET:
			ArchOnLoadPreset();
			break;

		case IDC_CHOOSE_ARCHIVER:
			{	  
				ArchOnChooseArchiver();
			}
			break;

		case IDC_ADD_PARAM:
			ArchOnAddParam();
			break;

		case IDC_DEL_PARAM:
			ArchOnDelParam();
			break;

		case IDC_PARAMS:
			if (HIWORD(wParam) == LBN_SELCHANGE)
			{
				ArchOnParamSel();
			}
			break;
			
		case IDC_PARAM:
		case IDC_VALUE:
			if (HIWORD(wParam) == EN_UPDATE && !m_bLockEdits)
			{
				ArchOnEditChange();
			}
			break;
		}
		break;

	case WM_DELETEITEM:
		if (wParam == IDC_PARAMS)
		{
			ArchOnDelItem((DELETEITEMSTRUCT*) lParam);
			return TRUE;
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
			ArchOnExit();
			SetWindowLong(m_hGeneralSheet, DWL_MSGRESULT, FALSE);
			break;
		}

		break;
	}

	return FALSE;
}

BOOL COptionsWnd::LoadPresetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		m_hPresets = hwnd;
		PresetsOnInit();
		return TRUE;
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDC_PRESETS:
				if (HIWORD(wParam) == LBN_DBLCLK)
				{
					// двойной щелчок на пресете == нажатие кнопки Ок
					SendMessage(hwnd, WM_COMMAND, MAKELONG(IDOK, 0), 0);
				}
				break;

			case IDOK:
				PresetsOnExit();
				break;

			case IDCANCEL:
				EndDialog(hwnd, IDCANCEL);
				break;
			}
		}
		break;

	default:
		return FALSE;
	}
}

bool COptionsWnd::GeneralOnInit()
{
	SetDlgItemText(m_hGeneralSheet, IDC_SOUND, Settings->sSoundFile.C());

	if (Settings->bUseSound) SendDlgItemMessage(m_hGeneralSheet, IDC_USE_SOUND, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);
	if (Settings->bShowProgress) SendDlgItemMessage(m_hGeneralSheet, IDC_SHOW_PROGRESS, BM_SETCHECK, (WPARAM) BST_CHECKED, 0);

	m_hLogSizeUpDown = CreateWindowEx(0, UPDOWN_CLASS, NULL, WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT |
		UDS_ARROWKEYS, 0, 0, 0, 0, m_hGeneralSheet, NULL, Instance, NULL);
	SendMessage(m_hLogSizeUpDown, UDM_SETRANGE, 0, MAKELONG(10240, 0));
	SendMessage(m_hLogSizeUpDown, UDM_SETBUDDY, (WPARAM) GetDlgItem(m_hGeneralSheet, IDC_LOG_SIZE), 0);

	SetDlgItemInt(m_hGeneralSheet, IDC_LOG_SIZE, Settings->wMaxLogSize, FALSE);

	return true;
}

bool COptionsWnd::GeneralOnChooseSound()
{
	TCHAR cFileName[MAX_PATH] = {0};

	// Получаем имя муз. файла
	CString sSoundPath = GetDlgItemStr(m_hGeneralSheet, IDC_SOUND);
	TCHAR cSoundPath[MAX_PATH];
	
	if (sSoundPath.Empty())
	{
		// если поле ввода с именем файла пустое - записываем туда папку c:\%windir%\Media
		lstrcpy(cSoundPath, TEXT("%WINDIR%\\Media"));
		DoEnvironmentSubst(cSoundPath, MAX_PATH);	// раскроем макрос 'windir'
	}
	else
		lstrcpy(cSoundPath, sSoundPath.C());

	// Настроим диалог открытия файла на wav файлы
	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetParent(m_hGeneralSheet);
	ofn.lpstrInitialDir = cSoundPath;
	ofn.lpstrFilter = TEXT("WAV файлы (*.wav)\0*.wav\0");
	ofn.lpstrFile = cFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_ENABLESIZING | OFN_NOCHANGEDIR;

	// Показываем окно для выбором имени муз. файла
	if (GetOpenFileName(&ofn))
	{
		// запишем выбранный файл в поле ввода
		SetDlgItemText(m_hGeneralSheet, IDC_SOUND, cFileName);
	}

	return true;
}

bool COptionsWnd::GeneralOnPlayFile()
{
	// Получим имя музыкального файла из поля ввода
	TCHAR cFileName[MAX_PATH];
	GetDlgItemText(m_hGeneralSheet, IDC_SOUND, cFileName, MAX_PATH);

	// и проиграем его
	PlaySound(cFileName, NULL, SND_FILENAME | SND_ASYNC);

	return true;
}

bool COptionsWnd::GeneralOnStopFile()
{
	// Остановим проигрывание музыки
	PlaySound(NULL, NULL, 0);
	return true;
}

bool COptionsWnd::GeneralOnExit()
{
	// Запишем параметры с формы в класс настроек
	Settings->bUseSound = (SendDlgItemMessage(m_hGeneralSheet, IDC_USE_SOUND, BM_GETCHECK, 0, 0) == BST_CHECKED);
	Settings->bShowProgress = (SendDlgItemMessage(m_hGeneralSheet, IDC_SHOW_PROGRESS, BM_GETCHECK, 0, 0) == BST_CHECKED);

	Settings->sSoundFile = GetDlgItemStr(m_hGeneralSheet, IDC_SOUND);

	Settings->wMaxLogSize = GetDlgItemStr(m_hGeneralSheet, IDC_LOG_SIZE).ToInt();

	return true;
}

bool COptionsWnd::ArchOnInit()
{
	// Заполним поля ввода данными об архиваторе
	SetDlgItemText(m_hArchiveSheet, IDC_ARCHIVER_EXE, Settings->sArchiverEXE.C());
	SetDlgItemText(m_hArchiveSheet, IDC_COMMAND_LINE, Settings->sArchiverCmdLine.C());

	// Заполним список параметрами архиватора
	for (BYTE i = 0; i < Settings->aArchiverParams.Size(); i++)
	{
		TStrPair* pStrPair = new TStrPair;
		pStrPair->First = Settings->aArchiverParams[i]->First;
		pStrPair->Second = Settings->aArchiverParams[i]->Second;

		TCHAR buf[MAX_PATH];
		lstrcpy(buf, FormatC(TEXT("%%%s%% = %s"), pStrPair->First.C(), pStrPair->Second.C()));
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_ADDSTRING, 0, (LPARAM) buf);

		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, i, (LPARAM) pStrPair); 
	}
	return true;
}

bool COptionsWnd::ArchOnLoadPreset()
{
	// Показываем окно со списком пресетов
	if (DialogBoxParam(Instance, MAKEINTRESOURCE(IDD_LOAD_ARCH_PRESET), GetParent(m_hArchiveSheet),
		StaticLoadPresetDlgProc, (LPARAM) this) == IDOK)
	{
		// выбрали какой-то пресет - очистим информацию о пред. архиаторе
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_RESETCONTENT, 0, 0);

		TPStrList* pArchiver = m_ArchivePresets[m_bSelPreset];

		// получаем имя и путь к файлу архиватора и раскрываем возможные макросы
		CString sEXE = (*pArchiver)[1];
		DoEnvironmentSubst(sEXE.Buf(), MAX_PATH);

		SetDlgItemText(m_hArchiveSheet, IDC_ARCHIVER_EXE, sEXE.C());
		SetDlgItemText(m_hArchiveSheet, IDC_COMMAND_LINE, (*pArchiver)[2]->C());

		// добавим в список параметров параметры из выбранного пресета
		for (BYTE i = 3; i < pArchiver->Size(); i += 2)
		{
			TStrPair* pStrPair = new TStrPair;
			pStrPair->First = (*pArchiver)[i];
			pStrPair->Second = (*pArchiver)[i + 1]; 

			CString sParamValue = FormatC(TEXT("%%%s%% = %s"), pStrPair->First.C(), pStrPair->Second.C());

			int ind = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_ADDSTRING, 0, (LPARAM) sParamValue.C());
			SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, ind, (LPARAM) pStrPair);
		}
	}

	return true;
}

bool COptionsWnd::ArchOnChooseArchiver()
{
	// Покажем диалог выбора файла-архиватора
	TCHAR cFileName[MAX_PATH] = {0};

	OPENFILENAME ofn = {0};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = GetParent(m_hArchiveSheet);
	ofn.lpstrFilter = TEXT("Выполняемые файлы (*.exe)\0*.exe\0");
	ofn.lpstrFile = cFileName;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_ENABLESIZING | OFN_NOCHANGEDIR;
		
	if (GetOpenFileName(&ofn))
	{
		SetDlgItemText(m_hArchiveSheet, IDC_ARCHIVER_EXE, cFileName);
	}

	return true;
}

bool COptionsWnd::ArchOnAddParam()
{
	// Добавим новый параметр "макрос-значение"
	int ind = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_ADDSTRING, 0, (LPARAM) "%param% = value");

	// Создадим для этого параметра пару строк
	TStrPair* pStrPair = new TStrPair;
	pStrPair->First = TEXT("param");
	pStrPair->Second = TEXT("value");

	// Назначим эту пару строк созданному эл-ту в listbox
	SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, ind, (LPARAM) pStrPair); 

	return true;
}

bool COptionsWnd::ArchOnDelParam()
{
	// Узнаем индекс выделенного эл-та
	int ind = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETCURSEL, 0, (LPARAM) TEXT("%param% = value"));
	if (ind != LB_ERR)
	{
		// если индекс верный - удалим эл-т
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_DELETESTRING, ind, 0);
	}

	return true;
}

void COptionsWnd::ArchOnDelItem(DELETEITEMSTRUCT* pdis)
{
	// Вызывается когда происходит удаление эл-та из списка параметров
	// в этом случае надо освободить пару строк, которая связана с эти эл-м
	TStrPair* pStrPair = (TStrPair*) SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETITEMDATA, pdis->itemID, 0);
	if (pStrPair) delete pStrPair;
}

bool COptionsWnd::ArchOnParamSel()
{
	// Узнаем какой эл-т выделен
	int ind = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETCURSEL, 0, 0);
	if (ind != LB_ERR)
	{
		// Получим пару строк - "макрос-значение"
		TStrPair* pStrPair = (TStrPair*) SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETITEMDATA, ind, 0);
	    
		// Заполним этими значениями поля ввода
		m_bLockEdits = true;
		SetDlgItemText(m_hArchiveSheet, IDC_PARAM, pStrPair->First.C());
		SetDlgItemText(m_hArchiveSheet, IDC_VALUE, pStrPair->Second.C());
		m_bLockEdits = false;
	}

	return true;
}

void COptionsWnd::ArchOnEditChange()
{
	int ind = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETCURSEL, 0, 0);

	if (ind != LB_ERR)
	{
		TStrPair* pStrPair = (TStrPair*) SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETITEMDATA, ind, 0);
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, ind, (LPARAM) NULL);

		// Обновим информацию в listview когда вводят новый параметр\значение
		pStrPair->First = GetDlgItemStr(m_hArchiveSheet, IDC_PARAM);
		pStrPair->Second = GetDlgItemStr(m_hArchiveSheet, IDC_VALUE);

		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_DELETESTRING, ind, 0);
		CString s = FormatC(TEXT("%%%s%% = %s"), pStrPair->First.C(), pStrPair->Second.C());
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_INSERTSTRING, ind, (LPARAM) s.C());

		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, ind, (LPARAM) pStrPair);
		SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETCURSEL, ind, 0);
	}
}

bool COptionsWnd::ArchOnExit()
{
	Settings->sArchiverEXE = GetDlgItemStr(m_hArchiveSheet, IDC_ARCHIVER_EXE);
	Settings->sArchiverCmdLine = GetDlgItemStr(m_hArchiveSheet, IDC_COMMAND_LINE);

	Settings->aArchiverParams.Clear();

	int iCount = SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETCOUNT, 0, 0);
	for (BYTE i = 0; i < iCount; i++)
	{
		TStrPair* pStrPair = (TStrPair*) SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_GETITEMDATA, i, 0);
		if (pStrPair)
		{
			Settings->aArchiverParams.Add(pStrPair);
			SendDlgItemMessage(m_hArchiveSheet, IDC_PARAMS, LB_SETITEMDATA, i, (LPARAM) NULL);
		}
	}

	return true;
}

void COptionsWnd::PresetsOnInit()
{
	m_ArchivePresets.Clear();

	// Загружаем пресеты из файла
	CMemSetTextFile* pArchivers = new CMemSetTextFile(TEXT("Archivers.ini"));

	// читаем каждую секцию, в которой записаны параметры конкретного архиватора
	for (BYTE i = 0; i < pArchivers->GetSectionsCount(); i++)
	{
		// название архиватора == название секции
		CString sSect = pArchivers->GetSection(i);

		if (sSect.Empty()) continue;

		// список с параметрами данного архиватора
		TPStrList* pArchiver = new TPStrList;
		pArchiver->Add(new CString(sSect));

		// читаем путь к файлу архиватора и командную строку
		pArchiver->Add(new CString(pArchivers->GetString(sSect.C(), TEXT("program exe"))));
		pArchiver->Add(new CString(pArchivers->GetString(sSect.C(), TEXT("command line"))));

		// читаем оставшиеся в данной секции записи - "параметр-значение"
		for (BYTE p = 2; p < pArchivers->GetSectionValuesCount(sSect.C()); p++)
		{
			CString sKey = pArchivers->GetKey(sSect.C(), p);

			pArchiver->Add(new CString(sKey));
			pArchiver->Add(new CString(pArchivers->GetString(sSect.C(), sKey.C())));
		}

		// добавляем в список пресетов новый архиватор
		m_ArchivePresets.Add(pArchiver);

		// и добавим в список пресетов
		SendDlgItemMessage(m_hPresets, IDC_PRESETS, LB_ADDSTRING, 0, (LPARAM) (*pArchiver)[0]->C());
	}

	delete pArchivers;
}

void COptionsWnd::PresetsOnExit()
{
	// Узнаем индекс выбранного пресета
	int ind = SendDlgItemMessage(m_hPresets, IDC_PRESETS, LB_GETCURSEL, 0, 0);
	
	if (ind == LB_ERR)
		EndDialog(m_hPresets, IDCANCEL);
	else
	{
		m_bSelPreset = ind;
		EndDialog(m_hPresets, IDOK);
	}
}

bool COptionsWnd::Show()
{
	int res = PropertySheet(&m_PropSheetHdr);
	return (res == IDOK);
}