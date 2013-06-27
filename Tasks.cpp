#include "Tasks.h"

LPCTSTR TaskDoneMessage = TEXT("WM_TaskDone");

CTaskList* TaskList;

///////////////////////////////////////////////////////////////////////////////
// CTask
///////////////////////////////////////////////////////////////////////////////
CTask::CTask()
{
	m_pScheduleTree = NULL;
	m_dtLastRun = DTNULL();
	m_dtNextRun = DTNULL();

	m_bState = TASK_STATE_NONE;
	m_bLocked = false;

	// Создадим заранее класс-окно с информацией о процессе выполнения
	m_hProgressWnd = new CTaskProgressWnd(this);
}

CTask::~CTask()
{
	// Удалим класс
	delete m_hProgressWnd;
}

bool CTask::Start(bool bScheduled)
{
	// Если задача не находится в состоянии бездействия или заблокирована - ошибка
	if (m_bState != TASK_STATE_NONE || m_bLocked) 
	{
		// Задачу пытались запустить по расписанию, но не вышло - запишем это в лог
		if (bScheduled)
			Log->WriteLogLine(TEXT("[%s] Задача не была запущена в назначенное время"), TEXT("s"), m_TaskInfo.sName.C());

		return false;
	}

	m_bState = TASK_STATE_WORKING;	// установим состояние - "работа"
	m_bAborted = false;

	m_dtLastRun = DTNow();	// запомним время последнего запуска

	// Округлим это время до минут
	m_dtLastRun.GetPSystemTime()->wSecond = 0;
	m_dtLastRun.GetPSystemTime()->wMilliseconds = 0;

	// Создадим окошко с информацией о процессе выполнения задачи
	m_hProgressWnd->CreateProgressWnd(bScheduled);

	// Запускаем поток на выполнение задачи
	m_pThread = new CThread();
	m_pThread->AddProgressListner(m_hProgressWnd->GetHWND());	// добавим окошко как слушателя
	m_pThread->AddProgressListner(MainWindow);
	m_pThread->SetPTask((LPARAM) this);	// установишь адрес текущего экземпляра класса потоку
	m_pThread->SetTaskInfo(&m_TaskInfo);	// передадим информацию о задаче
	m_pThread->SetArchiverEXE(Settings->sArchiverEXE);
	m_pThread->SetArchiverCmdLine(Settings->sArchiverCmdLine);
	m_pThread->SetArchiverParams(Settings->aArchiverParams);
	m_pThread->Resume();

	if (bScheduled)
		Log->WriteLogLine(TEXT("[%s] Задача запущена"), TEXT("s"), m_TaskInfo.sName.C());
	else
		Log->WriteLogLine(TEXT("[%s] Задача запущена"), TEXT("i"), m_TaskInfo.sName.C());

	return true;
}

bool CTask::Abort()
{
	if (m_bState == TASK_STATE_WORKING)
	{
		m_bState = TASK_STATE_TERMINATING;
		m_bAborted = true;
		
		m_pThread->Terminate();
        		
		return true;
	}
	else
		return false;
}

void CTask::ShowProgressWnd()
{
	if (m_bState == TASK_STATE_NONE) return;

	m_hProgressWnd->Show();
}

bool CTask::OnThreadDone()
{
	// Когда поток послал сообщение что он завершил работу
	// мы ждём когда его хендл просигнализирует
	WaitForSingleObject(m_pThread->GetHandle(), INFINITE);

	// и удалим класс, отвечающий за работу с потоком (CThread)
	delete m_pThread;
	m_pThread = NULL;

	// Установим состояние задачи в "бездействие"
	m_bState = TASK_STATE_NONE;

	// Скажем главному окну что задача завершена
	SendMessage(MainWindow, RegisterWindowMessage(TaskDoneMessage), (WPARAM) this, NULL);

	// Запишем в лог что задача завершена
	if (m_bAborted)
		Log->WriteLogLine(TEXT("[%s] Выполнение задачи прервано"), TEXT("i"), m_TaskInfo.sName);
	else
		Log->WriteLogLine(TEXT("[%s] Выполнение задачи завершено"), TEXT("i"), m_TaskInfo.sName);

	// Покажем окно с информацией о прогрессе
	if (Settings->bShowProgress)
	{
		m_hProgressWnd->Show();
		SetForegroundWindow(GetLastActivePopup(MainWindow));
	}

	return true;
}

bool CTask::BuildScheduleTree()
{
	// Если дерево уже создано - удалим его
	if (m_pScheduleTree)
		Schedule->FreeScheduleTree(m_pScheduleTree);

	// Построим дерево расписания
	m_pScheduleTree = Schedule->BuildScheduleTree(m_TaskInfo.sSchedule);

	return true;
}

bool CTask::CalcNextRun(CDateTime dtFrom)
{
	// Узнаём время след. запуска задачи
	m_dtNextRun = Schedule->GetNextRun(m_pScheduleTree, dtFrom);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CTaskList
///////////////////////////////////////////////////////////////////////////////
CTaskList::CTaskList()
{
}

CTask* CTaskList::AddTask(PTaskInfo TaskInfo)
{
	// Создадим новый объект-задачу
	CTask* pTask = new CTask();
	m_aTasks.Add(pTask);

	// Присвоим переданную информацию о задаче только что созданному объекту
	*(pTask->GetPTaskInfo()) = *TaskInfo;

	return pTask;
}

bool CTaskList::DelTask(CTask* pTask)
{
	if (pTask)
	{
		for (int i = 0; i < m_aTasks.Size(); i++)
		{
			if (m_aTasks[i] == pTask)
			{
				m_aTasks.Delete(i);
				return true;
			}
		}
	}

	return false;
}

bool CTaskList::DelTask(CString sTask)
{
	return true;
}

CTask* CTaskList::FindTaskByName(CString sName)
{
	for (int i = 0; i < m_aTasks.Size(); i++)
	{
		if (m_aTasks[i]->GetPTaskInfo()->sName == sName)
			return m_aTasks[i];
	}
	return NULL;
}

WORD CTaskList::GetRunningTasksCount()
{
	WORD res = 0;

	for (int i = 0; i < m_aTasks.Size(); i++)
	{
		if (m_aTasks[i]->GetState() != TASK_STATE_NONE)
			res++;
	}

	return res;
}

bool CTaskList::LoadTasks()
{
    CINIFile* pTasksIni = new CINIFile(cTasksFile);

	CTask* pTask = NULL;

	CDateTime dtNow = DTNow();

    int iSections = pTasksIni->GetSectionsCount();
    for (int t = 0; t < iSections; t++)
	{
        CString sSect = pTasksIni->GetNextSection();

        if (sSect.Empty())
            continue;

		pTask = new CTask();
		m_aTasks.Add(pTask);

        // Получим указатель на структуру, содержащую данные задачи
		PTaskInfo pTaskInfo = pTask->GetPTaskInfo();

        // Заполним данные задачи из INI файла
		pTaskInfo->sName            = sSect;
        pTaskInfo->sSrcFolder       = pTasksIni->GetString(sSect.C(), TEXT("Source folder"));
        pTaskInfo->sDestFolder      = pTasksIni->GetString(sSect.C(), TEXT("Dest folder"));
		pTaskInfo->sDestGenName     = pTasksIni->GetString(sSect.C(), TEXT("Generate name"));
		pTaskInfo->sIncludeMask     = pTasksIni->GetString(sSect.C(), TEXT("Incl mask"));
		pTaskInfo->sExcludeMask     = pTasksIni->GetString(sSect.C(), TEXT("Excl mask"));
		pTaskInfo->bSubFolders      = pTasksIni->GetBool(sSect.C(), TEXT("Subfolders"));

		pTaskInfo->bScheduled       = pTasksIni->GetBool(sSect.C(), TEXT("Scheduled"));
		pTaskInfo->sSchedule        = pTasksIni->GetString(sSect.C(), TEXT("Schedule"));

		pTaskInfo->bDoArchive       = pTasksIni->GetBool(sSect.C(), TEXT("Do archive"));
        pTaskInfo->bArchCompress    = pTasksIni->GetNumeric<BYTE>(sSect.C(), TEXT("Compress level"));
		pTaskInfo->bArchSFX         = pTasksIni->GetBool(sSect.C(), TEXT("SFX"));
		pTaskInfo->bArchDelFiles    = pTasksIni->GetBool(sSect.C(), TEXT("Del files"));
		pTaskInfo->bArchLock        = pTasksIni->GetBool(sSect.C(), TEXT("Lock"));
		pTaskInfo->sArchTaskCmd     = pTasksIni->GetString(sSect.C(), TEXT("Archiver cmd"));

        pTaskInfo->i64FinishedBytes = pTasksIni->GetNumeric<__int64>(sSect.C(), TEXT("Finished bytes"));

        // "Распакуем" строку последнего запуска в CDateTime
        CDateTime dtLastRun;
		dtLastRun.UnpackFromString(pTasksIni->GetString(sSect.C(), TEXT("Last run"), TEXT("00000000000000")));
		pTask->SetLastRun(dtLastRun);

		// построим дерево расписания для задачи
		pTask->BuildScheduleTree();

		// и подсчитаем время след. запуска
		pTask->CalcNextRun(dtNow);
	}

    delete pTasksIni;

	return true;
}

bool CTaskList::SaveTasks()
{
	//CMemTextFile* pTasks = new CMemTextFile();
    //CMemOutUStream* pTasks = new CMemOutUStream(cTasksFile);
    CMemIniOutW* pTasks = new CMemIniOutW(cTasksFile);

    pTasks->AddString(L"Desc         ", L"Small Backup tasks file");
	pTasks->AddString(L"Version      ", L"1");
	pTasks->WriteLine();

	for (int t = 0; t < m_aTasks.Size(); t++)
	{
		CTask* pTask = m_aTasks[t];
		PTaskInfo pTaskInfo = pTask->GetPTaskInfo();

		pTasks->AddSection( pTaskInfo->sName.C()                                         );
		pTasks->AddString(  L"Source folder  ", pTaskInfo->sSrcFolder.C()                );
		
		pTasks->AddString(  L"Dest folder    ", pTaskInfo->sDestFolder.C()               );
		
		pTasks->AddString(  L"Generate name  ", pTaskInfo->sDestGenName.C()              );
		pTasks->AddString(  L"Incl mask      ", pTaskInfo->sIncludeMask.C()              );
		pTasks->AddString(  L"Excl mask      ", pTaskInfo->sExcludeMask.C()              );
		pTasks->AddBoolean( L"Subfolders     ", pTaskInfo->bSubFolders                   );

		pTasks->AddBoolean( L"Scheduled      ", pTaskInfo->bScheduled                    );
		pTasks->AddString(  L"Schedule       ", pTaskInfo->sSchedule.C()                 );
		pTasks->AddString(  L"Last run       ", pTask->GetLastRun().PackToString().C()   );

		pTasks->AddBoolean( L"Do archive     ", pTaskInfo->bDoArchive                    );
		pTasks->AddNumeric( L"Compress level ", pTaskInfo->bArchCompress                 );
		pTasks->AddBoolean( L"SFX            ", pTaskInfo->bArchSFX                      );
		pTasks->AddBoolean( L"Del files      ", pTaskInfo->bArchDelFiles                 );
		pTasks->AddBoolean( L"Lock           ", pTaskInfo->bArchLock                     );
		pTasks->AddString(  L"Archiver cmd   ", pTaskInfo->sArchTaskCmd.C()              );
		pTasks->WriteLine();

		pTasks->AddNumeric( L"Finished bytes ", pTaskInfo->i64FinishedBytes              );
	}

	pTasks->SaveFile();
	delete pTasks;

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// CTaskProgressWnd
///////////////////////////////////////////////////////////////////////////////
CTaskProgressWnd::CTaskProgressWnd(CTask* pTask)
{
	// Запомним задачу, о которой отображается информация
	m_pTask = pTask;
	m_hWindow = NULL;
}

CTaskProgressWnd::~CTaskProgressWnd()
{
	//DestroyWindow(m_hWindow);
}

BOOL CALLBACK CTaskProgressWnd::StaticDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == WM_INITDIALOG)
		SetWindowLong(hwnd, DWL_USER, (LONG) lParam);

	CTaskProgressWnd* pProgressWnd = (CTaskProgressWnd*) GetWindowLong(hwnd, DWL_USER);

	if (pProgressWnd)
		return pProgressWnd->DlgProc(hwnd, Message, wParam, lParam);
	else
		return FALSE;
}

BOOL CALLBACK CTaskProgressWnd::DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	if (Message == RegisterWindowMessage(ThreadUpdateMessage))
	{
		OnThreadUpdate((PTaskProgress) wParam);
		return TRUE;
	}
	else if (Message == RegisterWindowMessage(ThreadStringMessage))
	{
		OnThreadString((char*) wParam);
		return TRUE;
	}
	else if (Message == RegisterWindowMessage(ThreadDoneMessage))
	{
		OnThreadDone();
		return TRUE;
	}

	switch (Message)
	{
	case WM_INITDIALOG:
		m_hWindow = hwnd;
		OnInit();
		return FALSE;
		break;

	case WM_SIZING: 
		OnSizing((RECT*) lParam); 
		break;

	case WM_SIZE:
		OnSize();
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			OnHideClick();
			break;

		case IDCANCEL:
			OnAbortClick();
			break;
		}
		break;

	default:
		return FALSE;
	}

	return TRUE;
}

void CTaskProgressWnd::ResetInfo()
{
	// Зададим заголовок
	SetWindowText(m_hWindow, FormatC(TEXT("[%s] выполнение"), m_pTask->GetPTaskInfo()->sName.C()));

	// Покажем кнопку "Прервать"
	ShowWindow(GetDlgItem(m_hWindow, IDOK), SW_SHOW);

	// Заменим заголовок кнопки
	SetDlgItemText(m_hWindow, IDCANCEL, TEXT("Прервать"));
}

bool CTaskProgressWnd::OnInit()
{
	// Сделаем иконку у диалога
	SendMessage(m_hWindow, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(Instance, MAKEINTRESOURCE(IDI_MAIN_APP_SMALL)));

	// Зададим границы у progress bar  (0..100)
    SendDlgItemMessage(m_hWindow, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 100));    

	// Сбросим надписи в начальное положение
	ResetInfo();

	return true;
}

void CTaskProgressWnd::OnSizing(RECT* pRect)
{
	// Наложим ограничение на минимальный размер окна
	pRect->right = max(pRect->left + DialogToScreen(m_hWindow, 270), pRect->right);
	pRect->bottom = max(pRect->top + DialogToScreen(m_hWindow, 160), pRect->bottom);
}

void CTaskProgressWnd::OnSize()
{
	RECT rcParent, rcControl;
	GetWindowRect(m_hWindow, &rcParent);

	GetWindowRect(GetDlgItem(m_hWindow, IDC_PROGRESS), &rcControl);
	rcControl.right = rcParent.right - DialogToScreen(m_hWindow, 8, true);
	SetWindowPos(GetDlgItem(m_hWindow, IDC_PROGRESS), NULL, 0, 0, rcControl.right - rcControl.left, rcControl.bottom - rcControl.top, SWP_NOMOVE);

	GetWindowRect(GetDlgItem(m_hWindow, IDC_LOG), &rcControl);
	rcControl.right = rcParent.right - DialogToScreen(m_hWindow, 8, true);
	rcControl.bottom = rcParent.bottom - DialogToScreen(m_hWindow, 25, false);
	SetWindowPos(GetDlgItem(m_hWindow, IDC_LOG), NULL, 0, 0, rcControl.right - rcControl.left, rcControl.bottom - rcControl.top, SWP_NOMOVE);

	GetWindowRect(GetDlgItem(m_hWindow, IDC_AUTO_CLOSE), &rcControl);
	rcControl.top = rcParent.bottom - (rcControl.bottom - rcControl.top) - DialogToScreen(m_hWindow, 10, false);
	POINT p = {rcControl.left, rcControl.top};
	ScreenToClient(m_hWindow, &p);
	SetWindowPos(GetDlgItem(m_hWindow, IDC_AUTO_CLOSE), NULL, p.x, p.y, 0, 0, SWP_NOSIZE);

	GetWindowRect(GetDlgItem(m_hWindow, IDCANCEL), &rcControl);
	rcControl.left = rcParent.right - (rcControl.right - rcControl.left) - DialogToScreen(m_hWindow, 8, true);
	rcControl.top = rcParent.bottom - (rcControl.bottom - rcControl.top) - DialogToScreen(m_hWindow, 8, false);
	p.x = rcControl.left;
	p.y = rcControl.top;
	ScreenToClient(m_hWindow, &p);
	SetWindowPos(GetDlgItem(m_hWindow, IDCANCEL), NULL, p.x, p.y, 0, 0, SWP_NOSIZE);

	GetWindowRect(GetDlgItem(m_hWindow, IDOK), &rcControl);
	rcControl.left = rcParent.right - DialogToScreen(m_hWindow, 115, true);
	rcControl.top = rcParent.bottom - (rcControl.bottom - rcControl.top) - DialogToScreen(m_hWindow, 8, false);
	p.x = rcControl.left;
	p.y = rcControl.top;			
	ScreenToClient(m_hWindow, &p);
	SetWindowPos(GetDlgItem(m_hWindow, IDOK), NULL, p.x, p.y, 0, 0, SWP_NOSIZE);
}

bool CTaskProgressWnd::OnHideClick()
{
	ShowWindow(m_hWindow, SW_HIDE);
	return true;
}

bool CTaskProgressWnd::OnAbortClick()
{
	// При нажатии на кнопке "прервать"
	if (m_pTask->GetState() == TASK_STATE_WORKING)
	{
		// если задача выполняется - спросим прерывать ли задачу
		if (MessageBox(m_hWindow, FormatC(TEXT("Прервать выполнение задачи \'%s\'?"), m_pTask->GetPTaskInfo()->sName.C()), 
			MessageCaption, MB_ICONQUESTION | MB_YESNOCANCEL) == IDYES)
		{
			// если ответили прерывать - прерываем
			m_pTask->Abort();
		}
	}
	else if (m_pTask->GetState() == TASK_STATE_NONE)
	{
		// а если задача не выполняется просто закроем окно
		OnClose();
		EndDialog(m_hWindow, 0);
		m_hWindow = NULL;
	}

	return true;
}

bool CTaskProgressWnd::OnClose()
{
	return true;
}

void CTaskProgressWnd::OnThreadUpdate(PTaskProgress pTaskProgress)
{
	// Обновим надписи на форме в соответствии с прогрессом выполнения задачи
	SetDlgItemText(m_hWindow, IDC_PROCEED_FOLDERS, FormatC(TEXT("Обработано папок: %d"), pTaskProgress->dwProceedFolders));
	SetDlgItemText(m_hWindow, IDC_PROCEED_FILES, FormatC(TEXT("Обработано файлов: %d"), pTaskProgress->dwProceedFiles));
	SetDlgItemText(m_hWindow, IDC_INCLUDED_FILES, FormatC(TEXT("Скопировано файлов: %d"), pTaskProgress->dwIncludedFiles));
	SetDlgItemText(m_hWindow, IDC_EXCLUDED_FILES, FormatC(TEXT("Пропущено файлов: %d"), pTaskProgress->dwExcludedFiles));
	SetDlgItemText(m_hWindow, IDC_ERRORS, FormatC(TEXT("Ошибок: %d"), pTaskProgress->dwErrors));
	SetDlgItemText(m_hWindow, IDC_FINISHED_BYTES, FormatC(TEXT("Скопировано: %s"), FormatSize(pTaskProgress->i64FinishedBytes).C()));

	// Отобразим процент выполнения в виде progress bar
	SendDlgItemMessage(m_hWindow, IDC_PROGRESS, PBM_SETPOS, pTaskProgress->bPercents, 0);

	// Если это последнее обновление информации - запомним кол-во скопированных байт для статистики
	if (pTaskProgress->bLastInfo)
		m_pTask->GetPTaskInfo()->i64FinishedBytes += pTaskProgress->i64FinishedBytes;
}

void CTaskProgressWnd::OnThreadString(char* Message)
{
	SendDlgItemMessage(m_hWindow, IDC_LOG, EM_SETSEL, -1, -1);	// Переместимся в конец лога
	SendDlgItemMessage(m_hWindow, IDC_LOG, EM_REPLACESEL, TRUE, (LPARAM) Message);	// добавим сообщение
}

void CTaskProgressWnd::OnThreadDone()
{
	// Сообщим задаче что поток завершил работу
	m_pTask->OnThreadDone();

	// и узнаем у задачи - прервал ли её пользователь
	if (m_pTask->IsAborted())
	{
		SetWindowText(m_hWindow, FormatC(TEXT("[%s] прервано пользователем"), m_pTask->GetPTaskInfo()->sName.C()));
	}
	else
	{
		// Поток завершил работу - изменим заголовок окна
		SetWindowText(m_hWindow, FormatC(TEXT("[%s] завершено"), m_pTask->GetPTaskInfo()->sName.C()));
	}

	// Спрячем кнопку "Спрятать" (которая есть IDOK)
	ShowWindow(GetDlgItem(m_hWindow, IDOK), SW_HIDE);

	// Изменим название кнопки с "Прервать" на "Закрыть", т.к. задача выполнена, её нельзя прервать
	SetDlgItemText(m_hWindow, IDCANCEL, TEXT("Закрыть"));

	// Проиграем звук
	if (Settings->bUseSound)
	{
		PlaySound(Settings->sSoundFile.C(), NULL, SND_ASYNC | SND_FILENAME);
	}
}

bool CTaskProgressWnd::CreateProgressWnd(bool bHidden)
{
	// Если окно уже создано, и не закрыто, то заново создавать ничего не надо
	if (m_hWindow)
	{
		// но сбросим надписи в начальное положение
		ResetInfo();
	}
	else
	{
		// Создаём новое окно
		CreateDialogParam(Instance, MAKEINTRESOURCE(IDD_TASK_PROGRESS), 
			MainWindow, StaticDlgProc, (LPARAM) this);
	}

	if (!bHidden)
		ShowWindow(m_hWindow, SW_SHOW);

	return true;
}

void CTaskProgressWnd::Show()
{
	ShowWindow(m_hWindow, SW_SHOW);
}