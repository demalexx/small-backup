#include "Application.h"

///////////////////////////////////////////////////////////////////////////////
// Mini CRT
///////////////////////////////////////////////////////////////////////////////
void* __cdecl malloc(size_t n)
{
    void* pv = HeapAlloc(GetProcessHeap(), 0, n);
    return pv;
}
void* __cdecl calloc(size_t n, size_t s)
{
    return malloc(n*s);
}
void* __cdecl realloc(void* p, size_t n)
{
    if (p == NULL) return malloc(n);
    return HeapReAlloc(GetProcessHeap(), 0, p, n);
}
void __cdecl free(void* p)
{
    if (p == NULL) return;
    HeapFree(GetProcessHeap(), 0, p);
}
void* __cdecl operator new(size_t n)
{
    return malloc(n);
}
void __cdecl operator delete(void* p)
{
    free(p);
}

extern "C" int _fltused = 0;

CApplication* Application;

///////////////////////////////////////////////////////////////////////////////
// Точка входа в программу
///////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	STARTUPINFO si = {0};
	GetStartupInfo(&si);

	Application = new CApplication();
	int res = Application->WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), 
		si.dwFlags & STARTF_USESHOWWINDOW ? si.wShowWindow : SW_SHOWDEFAULT);
	delete Application;

	ExitProcess(res);
	return res;
}

///////////////////////////////////////////////////////////////////////////////
// CApplication
///////////////////////////////////////////////////////////////////////////////
CApplication::CApplication()
{
    m_pLVTasks = NULL;
	m_iWaitTasks = 0;
	m_dtLastTasksSave = DTNow();
	m_hTrayAnimation = NULL;
}

CApplication::~CApplication()
{
	delete m_pLVTasks;
}

LRESULT CALLBACK CApplication::StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return Application->WindowProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK CApplication::AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_COMMAND && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL))
	{
		EndDialog(hwndDlg, IDOK);
		return TRUE;
	}

	return FALSE;
}

void CApplication::AddTaskToListView(CTask* pTask)
{
	STaskInfo* pTaskInfo = pTask->GetPTaskInfo();

	int iIndex = m_pLVTasks->AddItem(NULL, (LPARAM) pTask);
	UpdateTaskInListView(iIndex, false);
}

void CApplication::UpdateTaskInListView(int iIndex, bool bStatusOnly, BYTE bPercents)
{
	if (iIndex == -1) return;

	LVITEM lvi = {0};
		
	CTask* pTask = (CTask*) m_pLVTasks->GetItemParam(iIndex);
	if (!pTask) return;

	STaskInfo* pTaskInfo = pTask->GetPTaskInfo();

	// Обновим информацию о состоянии задачи
	BYTE bTaskState = pTask->GetState();
	if (bTaskState == TASK_STATE_NONE)
		m_pLVTasks->SetSubItem(iIndex, 0, TEXT(""));
	else if (bTaskState == TASK_STATE_TERMINATING)
		m_pLVTasks->SetSubItem(iIndex, 0, TEXT("-"));
	else if (bTaskState == TASK_STATE_WORKING)
	{
		if (bPercents == (BYTE) -1)
			m_pLVTasks->SetSubItem(iIndex, 0, TEXT(">"));
		else
		{
			// Преобразуем число - процент выполнения в строку
			CString sPercents(5);
			sPercents.FromInt(bPercents);
			sPercents += TEXT("%");

			// Покажем процент выполнения задачи
			m_pLVTasks->SetSubItem(iIndex, 0, sPercents.C());
		}
	}

	// Если указано - обновим всю информацию о задаче
	if (!bStatusOnly)
	{
		m_pLVTasks->SetSubItem(iIndex, 1, pTaskInfo->sName.C());
		m_pLVTasks->SetSubItem(iIndex, 2, pTaskInfo->sSrcFolder.C());
		m_pLVTasks->SetSubItem(iIndex, 3, (pTaskInfo->sDestFolder + pTaskInfo->sDestGenName).C());
		m_pLVTasks->SetSubItem(iIndex, 4, NextRunToStr(pTask->GetLastRun()).C());

		if (pTask->GetPTaskInfo()->bScheduled)
			m_pLVTasks->SetSubItem(iIndex, 5, NextRunToStr(pTask->GetNextRun()).C());
		else
			m_pLVTasks->SetSubItem(iIndex, 5, NextRunToStr(DTNULL()).C());
	}
}

void CApplication::UpdateTaskInListView(CTask* pTask, bool bStatusOnly, BYTE bPercents)
{
	UpdateTaskInListView(m_pLVTasks->FindItemParam((LPARAM) pTask), bStatusOnly, bPercents);
}

void CApplication::EnDisTaskCommands()
{
	int iSelIndex = m_pLVTasks->GetSelIndex();

	if (iSelIndex == -1)
	{
		EnableMenuItem(m_hMainMenu, ID_TASK_CHANGE, MF_GRAYED);
		EnableMenuItem(m_hMainMenu, ID_TASK_DELETE, MF_GRAYED);
		EnableMenuItem(m_hMainMenu, ID_TASK_START, MF_GRAYED);
		EnableMenuItem(m_hMainMenu, ID_TASK_SHOW_PROGRESS, MF_GRAYED);
	}
	else
	{
		// Получаем задачу как параметр эл-та в listview
		CTask* pTask = (CTask*) m_pLVTasks->GetItemParam(m_pLVTasks->GetSelIndex());

		if (pTask == NULL) return;

		if (pTask->GetState() == TASK_STATE_NONE)
		{
			EnableMenuItem(m_hMainMenu, ID_TASK_CHANGE, MF_ENABLED);
			EnableMenuItem(m_hMainMenu, ID_TASK_DELETE, MF_ENABLED);
			EnableMenuItem(m_hMainMenu, ID_TASK_START, MF_ENABLED);
			EnableMenuItem(m_hMainMenu, ID_TASK_SHOW_PROGRESS, MF_GRAYED);
		}
		else
		{
			EnableMenuItem(m_hMainMenu, ID_TASK_CHANGE, MF_GRAYED);
			EnableMenuItem(m_hMainMenu, ID_TASK_DELETE, MF_GRAYED);
			EnableMenuItem(m_hMainMenu, ID_TASK_START, MF_GRAYED);
			EnableMenuItem(m_hMainMenu, ID_TASK_SHOW_PROGRESS, MF_ENABLED);
		}
	}  
}

CString CApplication::NextRunToStr(CDateTime dtRun)
{
	CString sNextRun(TEXT("-"));

	if (dtRun.IsNullTime())
	{
		return sNextRun;
	}

	CString sBuf;

	// Вернём время согласно настройкам пользователя
	sBuf.Buf();
	sNextRun.Buf();

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIMEFORMAT, sBuf.Buf(), sBuf.BufLen());
	GetTimeFormat(LOCALE_USER_DEFAULT, 0, dtRun.GetPSystemTime(), sBuf.C(), sNextRun.Buf(), sNextRun.BufLen());

	GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SLONGDATE, sBuf.Buf(), sBuf.BufLen());
	CString sNextRunDate;
	sNextRunDate.Buf();
	GetDateFormat(LOCALE_USER_DEFAULT, 0, dtRun.GetPSystemTime(), sBuf.C(), sNextRunDate.Buf(), sNextRunDate.BufLen());

	sNextRun += TEXT(" ");
	sNextRun += sNextRunDate;

	return sNextRun;
}

void CApplication::UpdateSchedule()
{
	// Для начала "убъём" таймер что бы не сработал повторно во время выполнения этой ф-ии
	KillTimer(MainWindow, SCHEDULE_TIMER);

	// Попытаемся запустить нужные запланированные задачи
	for (int i = 0; i < TaskList->GetSize(); i++)
	{
		CTask* pTask = TaskList->GetTask(i);

		// Если задача не запланирована - нечего на неё тратить процессорное время
		if (pTask->GetNextRun().IsNullTime() || !pTask->GetPTaskInfo()->bScheduled) continue;
			
		if (pTask->GetNextRun().DeltaMSec(DTNow()) <= 1500)
			OnTaskStart(pTask, true);
		else
			// если след. запуск раньше текущей даты - непорядок
			if (pTask->GetNextRun() < DTNow())
			{
				// пересчитаем заново время след. запуска
				pTask->CalcNextRun(DTNow());
				UpdateTaskInListView(pTask);	// обновим инфо в listview
			}
	}

	// Узнаем через сколько нужно запустить задачу
	DWORD dwRunAfter = ScheduledTaskAfterMSec();

	// Если нет запланированных задач - выходим
	if (dwRunAfter == MAXDWORD)
		return;

	SetTimer(MainWindow, SCHEDULE_TIMER, dwRunAfter, NULL);
}

DWORD CApplication::ScheduledTaskAfterMSec()
{
	// Ф-я возвращает кол-во мс на которое нужно настроить таймер
	//
	// Сначала она находит время в мс до времени выполнения
	// ближайшей задачи.
	//
	// Если таких задач нет - она возвращает MAXDWORD, что означает
	// что запланированных задач нет и что таймер не надо устанавливать
	//
	// Если ли же есть какая-то заполанированная задача, в зависимости от
	// времени до выполнения задачи ф-я ведёт себя по-разному
	//
	// До выполнения задачи более 35 сек: возвращается 30 сек, что бы как минимум
	//  был запас в 5 сек. Это на тот случай если задача выполняется ровно
	//  через 35 секунд. 
	//
	// Больше 10, но менее 35 сек: от времени до выполнения вычитается 5 сек.
	//  Т.о. таймер опять сработает несколько раньше выполнения задачи
	//
	// От одной до 10 сек: возвращается время в два раза меньшее времени
	//  до начала. На тот случай если система будет сильно загружена и не 
	//  сможет вызвать обработку таймера в нужное время. А мы это время
	//  уменьшаем и увеличиваем вероятность срабатывания таймера
	//
	// Менее секунды: возвращается полторы секунды, что бы
	//  таймер гарантированно сработал через какую-то долю секунды
	//  ПОСЛЕ времени на которое заполанировано выполнение задачи.
	//  И во время этого срабатывания UpdateSchedule() запустит
	//  задачу в том случае, если текущее время будет больше времени
	//  выполнения задачи на значение, меньшее полутора секунд.
	//  Кроме того можно корректно подсчитать время след. запуска
	//  задачи, т.к. текущее время будет больше времени текущего
	//  выполнения задачи
	//
	//   Время через	 Возвращаемое   
	//  которое должна	   значение	     Расчёт
	//   выполниться	  (интервал
	//     задача		   таймера)
	//
	//     59688             30000 	    '30000'
	//     29688             24688  	29688 - 5000
	//      5000              2500	    5000 / 2
	//      2500              1250	    2500 / 2
	//      1250               625	    1250 / 2
	//      625               1500	    '1500'
	//
	// "Побочный эффект" от такой логики - когда приближается время 
	// выполнения задачи, интервалы таймер принимают определённые 
	// значения - 2500, 1250, 625, а время через которое задача должна
	// выполниться - 5000, 2500, 1250, 625

	// Кол-во мс через которое должна запустится ближайшая запланированная задача
	__int64 i64FirstRunAfter = MAXLONGLONG;

	// Просмотрим все задачи и найдём ту которая должна запуститься раньше всех
	for (int t = 0; t < TaskList->GetSize(); t++)
	{
		CTask* pTask = TaskList->GetTask(t);

		if (!pTask->GetPTaskInfo()->bScheduled) continue;

		// Узнаём через сколько мс стартует текущая задача
		__int64 i64TaskRunTimeAfter = Schedule->GetNextRunMSec(pTask->GetNextRun());

		// если больше нуля и меньше чем кол-во мс через которое стартует др. задача
		if (i64TaskRunTimeAfter > 0 && i64TaskRunTimeAfter <= i64FirstRunAfter)
		{
			// запомним это значение
			i64FirstRunAfter = i64TaskRunTimeAfter;
		}
	}

	//CString s = FormatC("След. задача через %d мс", i64FirstRunAfter);

	// Если нет запланированных задач
	if (i64FirstRunAfter == MAXLONGLONG) 
		return MAXDWORD;	// вернём максимальное значение
	else
	{
		// есть запланированная задача - вернём число не большее 30 сек (30 000 мс)
		if (i64FirstRunAfter >= 35000)
			i64FirstRunAfter = 30000;
		// если выполнение через 10...35 сек - уменьшим это значение на 5 сек
		else if (i64FirstRunAfter > 10000 && i64FirstRunAfter < 35000)
			i64FirstRunAfter -= 5000;
		else
		{
			// если задача выполняется через 1..10 сек - уменьшим вдвое этот промежуток
			if (i64FirstRunAfter >= 1000 && i64FirstRunAfter <= 10000)
				i64FirstRunAfter /= 2;
			// если до запуска меньше секунды
			else if (i64FirstRunAfter < 1000)
				// установим срабатывание таймера ровно через одну секунду
				i64FirstRunAfter = 1500;
		}

		//s += CString(FormatC(" таймер на %d мс", i64FirstRunAfter));
		//SetWindowText(MainWindow, s.C());

		return i64FirstRunAfter;
	}
}

void CApplication::HideToTray()
{
	NOTIFYICONDATA nid = {0};
	nid.cbSize = sizeof(nid);
	nid.hWnd = MainWindow;
	nid.uID = 1;
	lstrcpy(nid.szTip, TEXT("Small Backup"));
	nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_APP_SMALL));
	nid.uCallbackMessage = RegisterWindowMessage(Tray_Message);
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;

	Shell_NotifyIcon(NIM_ADD, &nid);

	ShowWindow(MainWindow, SW_MINIMIZE);
	ShowWindow(MainWindow, SW_HIDE);
}

void CApplication::TrayAnimation(bool bTimer)
{
	// Если вызов по таймеру - заменим картинку в трее
	if (bTimer)
	{
		m_bCurTrayIcon++;
		if (m_bCurTrayIcon > TRAY_ANIM_FRAMES)
			m_bCurTrayIcon = 0;

		NOTIFYICONDATA nid = {0};
		nid.cbSize = sizeof(nid);
		nid.hWnd = MainWindow;
		nid.uID = 1;
		nid.hIcon = ImageList_GetIcon(m_hTrayAnimation, m_bCurTrayIcon, 0);
		nid.uFlags = NIF_ICON;

		Shell_NotifyIcon(NIM_MODIFY, &nid);
	}
	else
	{
		// Если есть работающие задачи - запустим анимацию
		if (TaskList->GetRunningTasksCount() > 0)
		{
			m_bCurTrayIcon = 0;
			SetTimer(MainWindow, TRAY_TIMER, TRAY_ANIM_INTERVAL, NULL);
		}
		else
		{
			// если нет работающих задач - удалим таймер, анимации не надо
			KillTimer(MainWindow, TRAY_TIMER);

			// Установим обычную иконку
			NOTIFYICONDATA nid = {0};
			nid.cbSize = sizeof(nid);
			nid.hWnd = MainWindow;
			nid.uID = 1;
			nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_APP_SMALL));;
			nid.uFlags = NIF_ICON;

			Shell_NotifyIcon(NIM_MODIFY, &nid);
		}
	}
}

bool CApplication::OnCreate()
{	 
	// Объект, работающий с расписанием
	Schedule = new CSchedule();

	// Запомним менюшки что бы каждый раз их не узнавать по-новой
	m_hMainMenu = GetMenu(m_hMainWindow);
	m_hTasksMenu = GetSubMenu(m_hMainMenu, 1);

	// Составим список картинок-анимаций для трея
	m_hTrayAnimation = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, TRAY_ANIM_FRAMES, 0);

	// Загрузим 21 иконку из ресурсов и imagelist
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON2)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON3)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON4)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON5)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON6)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON7)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON8)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON9)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON10)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON12)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON13)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON14)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON15)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON16)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON17)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON18)));
	ImageList_AddIcon(m_hTrayAnimation, LoadIcon(Instance, MAKEINTRESOURCE(IDI_ICON19)));

	// Создание listview задач
	m_pLVTasks = new CListView(m_hMainWindow, m_hInstance, IDC_LV_TASKS);

	// 1-я колонка - прогресс
	m_pLVTasks->AddColumn(TEXT("*"), Settings->aColumnSize[0]);

	// 2-я колонка - название задачи
	m_pLVTasks->AddColumn(TEXT("Задача"), Settings->aColumnSize[1]);

	// 3-я колонка - исходная папка
	m_pLVTasks->AddColumn(TEXT("Исходная папка"), Settings->aColumnSize[2]);	

	// 4-я колонка - папка назначения
	m_pLVTasks->AddColumn(TEXT("Папка назначения"), Settings->aColumnSize[3]);	

	// 5-я колонка - последний запуск
	m_pLVTasks->AddColumn(TEXT("Последний запуск"), Settings->aColumnSize[4]);

	// 6-я колонка - следующий запуск
	m_pLVTasks->AddColumn(TEXT("Следующий запуск"), Settings->aColumnSize[5]);

	// Загрузим задачи из файла
	TaskList = new CTaskList();
	TaskList->LoadTasks();

	// Отобразим задачи в listview
	for (WORD i = 0; i < TaskList->GetSize(); i++)
	{
		AddTaskToListView(TaskList->GetTask(i));
	}

	// Поработаем с расписанием
	UpdateSchedule();

	// Установим таймер автосохранения
	SetTimer(MainWindow, AUTOSAVE_TIMER, AUTOSAVE_TIMER_INTERVAL, NULL);

	return true;
}

bool CApplication::OnClose()
{
	// Проверим есть ли выполняющиеся задачи
	WORD wRunningTasks = 0;
	CString sRunningTasks = TEXT("В данный момент выполняются следующие задачи:\n\n");
	for (int i = 0; i < TaskList->GetSize(); i++)
	{
		CTask* pTask = TaskList->GetTask(i);
		pTask->Lock();

		if (pTask->GetState() != TASK_STATE_NONE)
		{
			wRunningTasks++;
			sRunningTasks += pTask->GetPTaskInfo()->sName + TEXT("\n");
		}
	}

	if (wRunningTasks)
	{
		sRunningTasks += TEXT("\nЕсли вы закроете программу, выполнение задач будет прервано\n");
		sRunningTasks += TEXT("Закрыть программу?");

		// Если ответили "не закрывать программу" - вернём false и выйдет из ф-ии
		if (MessageBox(MainWindow, sRunningTasks.C(), MessageCaption, MB_ICONWARNING | MB_YESNOCANCEL) != IDYES)
		{
			for (int i = 0; i < TaskList->GetSize(); i++)
			{
				// т.к. закрывать программу передумали - разблокируем выполнение задач
				TaskList->GetTask(i)->UnLock();
			}

			return false;
		}
		// Если же сказали всё равно закрыть программу - прерываем выполнение всех задач
		else
		{
			for (int i = 0; i < TaskList->GetSize(); i++)
			{
				CTask* pTask = TaskList->GetTask(i);

				// если задача работает
				if (pTask->GetState() != TASK_STATE_NONE)
					pTask->Abort();	// прервём её
			}

			m_iWaitTasks = wRunningTasks;

			// возвращаем false (нельзя закрыть), т.к. нужно дождаться правильного 
			// завершения всех работающих задач
			return false;
		}
	}

	// Запомним параметры
	RECT rc;
	GetWindowRect(MainWindow, &rc);	// узнаём рамку окошка относительно экрана

	Settings->iMainWndX = rc.left;	// и запоминаем x и y
	Settings->iMainWndY = rc.top;

	GetClientRect(MainWindow, &rc);	// теперь узнаем рамку окошка относительно самого окна

	Settings->wMainWndWidth = rc.right;	// и запомним размеры окна
	Settings->wMainWndHeight = rc.bottom;

	Settings->aColumnSize[0] = m_pLVTasks->GetColumnWidth(0);
	Settings->aColumnSize[1] = m_pLVTasks->GetColumnWidth(1);
	Settings->aColumnSize[2] = m_pLVTasks->GetColumnWidth(2);
	Settings->aColumnSize[3] = m_pLVTasks->GetColumnWidth(3);
	Settings->aColumnSize[4] = m_pLVTasks->GetColumnWidth(4);
	Settings->aColumnSize[5] = m_pLVTasks->GetColumnWidth(5);

	// Запишем их на диск
	Settings->SaveSettings();

	// и задачи
	TaskList->SaveTasks();

	return true;
}

bool CApplication::OnSize(WPARAM wFlag)
{
	// При сворачивании программы свернём ей в трей
	if (wFlag == SIZE_MINIMIZED)
	{
		HideToTray();
		return true;
	}

	if (m_pLVTasks) m_pLVTasks->Resize();

	return true;
}

bool CApplication::OnTaskNew()
{
	CTaskWnd NewTask(m_hInstance, m_hMainWindow);
	
	if (NewTask.ShowNewTask())
	{
		CTask* pNewTask = TaskList->AddTask(NewTask.GetPTaskInfo());

        Log->WriteLogLine(L"[%s] Добавлена новая задача", pNewTask->GetPTaskInfo()->sName.C());

		pNewTask->BuildScheduleTree();
		pNewTask->CalcNextRun(DTNow());

		UpdateSchedule();
		AddTaskToListView(pNewTask);
	}

	return true;
}

bool CApplication::OnTaskDel()
{
	int iSel = m_pLVTasks->GetSelIndex();

	CTask* pTask = (CTask*) m_pLVTasks->GetItemParam(iSel);
	if (!pTask) return false;

	// Когда появляется диалог с подтверждением - задачу нельзя выполнить по расписанию
	pTask->Lock();

	if (MessageBox(m_hMainWindow, FormatC(TEXT("Удалить задачу \'%s\'?"), pTask->GetPTaskInfo()->sName.C()),
		MessageCaption, MB_ICONQUESTION | MB_YESNOCANCEL) == IDYES)
	{
		Log->WriteLogLine(TEXT("[%s] Задача удалена"), pTask->GetPTaskInfo()->sName.C());

		TaskList->DelTask(pTask);
		m_pLVTasks->DelItem(iSel);
		pTask = NULL;

		UpdateSchedule();
	}

	if (pTask) pTask->UnLock();

	return true;
}

bool CApplication::OnTaskChange()
{
	int iSelItem = m_pLVTasks->GetSelIndex();
	CTask* pTask = (CTask*) m_pLVTasks->GetItemParam(iSelItem);

	if (pTask == NULL || pTask->GetState() != TASK_STATE_NONE) return false;

	// Сделаем так что выполнять задачу нельзя
	pTask->Lock();

	CTaskWnd ChangeTask(m_hInstance, m_hMainWindow);
	ChangeTask.SetTaskInfo(pTask->GetPTaskInfo());
	if (ChangeTask.ShowChangeTask())
	{
		// В диалоге изменеия задачи нажали "ОК" - применим новые параметры задачи
		pTask->SetTaskInfo(ChangeTask.GetPTaskInfo());
	
		// Заново построим дерево расписания
		pTask->BuildScheduleTree();

		// И пересчитаем время след. запуска
		pTask->CalcNextRun(DTNow());

		Log->WriteLogLine(TEXT("[%s] Параметры задачи изменены"), pTask->GetPTaskInfo()->sName.C());
	}

	// Теперь задачу можно выполнять
	pTask->UnLock();

	UpdateSchedule();
	UpdateTaskInListView(iSelItem);

	return true;
}

bool CApplication::OnTaskStart(CTask* pTask, bool bScheduled)
{
	int iSelItem;

	if (pTask == NULL)
	{
		iSelItem = m_pLVTasks->GetSelIndex();
		pTask = (CTask*) m_pLVTasks->GetItemParam(iSelItem);
	}
	else
	{
		iSelItem = m_pLVTasks->FindItemParam((LPARAM) pTask);
	}

	if (pTask == NULL || iSelItem == -1) return false;

	// Не будем запускать задачу которая уже работает
	//if (pTask->GetState() != TASK_STATE_NONE) return false;

	// Запускаем задачу
	pTask->Start(bScheduled);

	// Подсчитаем когда в след. раз выполнять задачу
	pTask->CalcNextRun(DTNow());

	UpdateTaskInListView(iSelItem, false);
	EnDisTaskCommands();

	// Обновим анимацию в трее
	TrayAnimation();

	return true;
}

bool CApplication::OnTaskShowPrg()
{	
	CTask* pTask = (CTask*) m_pLVTasks->GetItemParam(m_pLVTasks->GetSelIndex());
	pTask->ShowProgressWnd();
	return true;
}

void CApplication::OnScheduleTimer()
{
	UpdateSchedule();
}

void CApplication::OnTrayTimer()
{
	TrayAnimation(true);
}

void CApplication::OnAutoSaveTimer()
{
	CDateTime dtNow;
	if (m_dtLastTasksSave.DeltaMSec(dtNow) >= AUTOSAVE_TIME)
	{
		TaskList->SaveTasks();
		m_dtLastTasksSave = dtNow;
	}
}

void CApplication::OnTray(LPARAM lMessage)
{
	switch (lMessage)
	{
	case WM_LBUTTONDOWN:
		{
			// Восстановим окно
            ShowWindow(MainWindow, SW_SHOW);
			ShowWindow(MainWindow, SW_RESTORE);

			// Удалим иконку из трея
			NOTIFYICONDATA nid = {0};
			nid.cbSize = sizeof(nid);
			nid.hWnd = MainWindow;
			nid.uID = 1;

			Shell_NotifyIcon(NIM_DELETE, &nid);
		}
		break;
	}
}

bool CApplication::OnOptions()
{
	COptionsWnd* pOptionsWnd = new COptionsWnd(m_hMainWindow);
	if (pOptionsWnd->Show())
	{
		Log->SetMaxSize(Settings->wMaxLogSize * 1024);
	}
	delete pOptionsWnd;

	return true;
}

bool CApplication::OnAbout()
{
	DialogBox(m_hInstance, MAKEINTRESOURCE(IDD_ABOUT), m_hMainWindow, AboutDlgProc);
	return true;
}

bool CApplication::OnListViewSel(int iIndex)
{
	EnDisTaskCommands();
	return true;
}

bool CApplication::OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Показываем контекстное меню для listview с задачами
	if ((HWND) wParam == m_pLVTasks->GetHWND())
	{
		// Сначала нажмём левой кнопкой на эл-те что бы его выделить
		SendMessage(m_pLVTasks->GetHWND(), WM_LBUTTONDOWN, 0, lParam);

		// затем покажем меню
		TrackPopupMenuEx(m_hTasksMenu, 0, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), m_hMainWindow, NULL);
	}
	else
		DefWindowProc(hwnd, uMsg, wParam, lParam);

	return true;
}

void CApplication::OnThreadUpdate(PTaskProgress pTaskProgress, CTask* pTask)
{
	// Находим индекс задачи по адресу этого объекта (pTask)
	int iIndex = m_pLVTasks->FindItemParam((LPARAM) pTask);
	UpdateTaskInListView(iIndex, true, pTaskProgress->bPercents);
}

bool CApplication::OnTaskDone(CTask* pTask)
{
	// Обновим информацию в главном окне о выполненной задаче
	UpdateSchedule();

	// Обновим интерфейс
	UpdateTaskInListView(pTask);
	EnDisTaskCommands();

	// если программа находится в режиме закрытия, т.е.
	// ждёт пока не завершатся все задачи - уменьшим счётчик
	// работающих задач
	if (m_iWaitTasks)
	{
		m_iWaitTasks--;

		// если все задачи завершены - закроемся
		if (m_iWaitTasks == 0)
			SendMessage(MainWindow, WM_CLOSE, 0, 0);
	}

	TrayAnimation();

	return true;
}

int WINAPI CApplication::WinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpCmdLine, int nCmdShow)
{
	m_hInstance = hInstance;
	Instance = hInstance;

	// Загружаем настройки
	Settings = new CSettings();
	Settings->LoadSettings();

    Log = new CMemLogUStream(L"Small Backup.log", Settings->wMaxLogSize * 1024);

    Log->WriteLogLine(L"Программа запущена", L"i");

	// Init common controls
	{
		INITCOMMONCONTROLSEX cce;
		cce.dwSize = sizeof(INITCOMMONCONTROLSEX);
		cce.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_DATE_CLASSES | ICC_UPDOWN_CLASS;
	
		InitCommonControlsEx(&cce);
	}

	CoInitialize(NULL);

	// Check command line parameters
	//{
	//	CTask* pTask = ParseCommandLine(lpCmdLine);
	//	if (pTask != NULL)
	//	{
	//		pTask->Start();
	//		return 0;
	//	}
	//}

	// Fill main window class
	WNDCLASSEX wcMainWindow = {0};
    wcMainWindow.cbSize        = sizeof(WNDCLASSEX);
    wcMainWindow.style         = 0;
	wcMainWindow.lpfnWndProc   = StaticWindowProc;
    wcMainWindow.cbClsExtra    = 0;
    wcMainWindow.cbWndExtra    = 0;
	wcMainWindow.hInstance     = m_hInstance;
	wcMainWindow.hIcon         = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_APP));
    wcMainWindow.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wcMainWindow.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wcMainWindow.lpszMenuName  = MAKEINTRESOURCE(IDR_MAINMENU);
    wcMainWindow.lpszClassName = cClassName;
	wcMainWindow.hIconSm       = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAIN_APP_SMALL));

	RegisterClassEx(&wcMainWindow);

	// Creating main window
	m_hMainWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        cClassName,
        TEXT("Small Backup"),
        WS_OVERLAPPEDWINDOW,
		Settings->iMainWndX, Settings->iMainWndY, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL, NULL, m_hInstance, NULL);

	MainWindow = m_hMainWindow;

	// Correct window size
	{
		RECT rc = {0, 0, Settings->wMainWndWidth, Settings->wMainWndHeight};
		AdjustWindowRectEx(&rc, GetWindowLong(m_hMainWindow, GWL_STYLE), TRUE, GetWindowLong(m_hMainWindow, GWL_EXSTYLE));

		SetWindowPos(m_hMainWindow, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE);
	}

	// Загружаем таблицу горячих клавиш
	HACCEL hAccel = LoadAccelerators(m_hInstance, MAKEINTRESOURCE(IDR_ACCELERATORS));

	OnCreate();
	ShowWindow(m_hMainWindow, nCmdShow);

	// Message loop
	MSG Msg;
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		if (!TranslateAccelerator(m_hMainWindow, hAccel, &Msg))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}

    Log->WriteLogLine(L"Выход из программы", L"i");
	
    delete Log;
	delete Settings;

	return 0;
}

LRESULT CApplication::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == RegisterWindowMessage(ThreadUpdateMessage))
	{
		OnThreadUpdate((PTaskProgress) wParam, (CTask*) lParam);
		return 0;
	}
	else if (uMsg == RegisterWindowMessage(TaskDoneMessage))
	{
		OnTaskDone((CTask*) wParam);
		return 0;
	}
	else if (uMsg == RegisterWindowMessage(Tray_Message))
	{
		OnTray(lParam);
		return 0;
	}

	switch (uMsg)
	{
	case WM_CLOSE:
		if (OnClose())
			PostQuitMessage(0);

		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TASK_NEW:
			OnTaskNew();
			break;

		case ID_TASK_DELETE:
			OnTaskDel();
			break;

		case ID_TASK_CHANGE:
			OnTaskChange();
			break;

		case ID_TASK_START:
			OnTaskStart(NULL);
			break;

		case ID_TASK_SHOW_PROGRESS:
			OnTaskShowPrg();
			break;

		case ID_OPTIONS_OPTIONS:
			OnOptions();
			break;

		case ID_FILE_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case IDC_HELP_ABOUT:
			OnAbout();
			break;
		}
		break;

	case WM_NOTIFY:
		{
			NMHDR* nmhdr = (NMHDR*) lParam; 
			switch (nmhdr->idFrom)
			{
			case IDC_LV_TASKS:
				{
					if (nmhdr->code == LVN_ITEMCHANGED)
					{
						NMLISTVIEW* nmlv = (NMLISTVIEW*) lParam;
						if ((nmlv->uChanged & LVIF_STATE) != 0)
							OnListViewSel(nmlv->iItem);
					}
					else if (nmhdr->code == NM_DBLCLK)
					{
						OnTaskChange();
					}
				}
				break;
			}
		}
		break;

	case WM_TIMER:
		if (wParam == SCHEDULE_TIMER)
			OnScheduleTimer();
		else if (wParam == TRAY_TIMER)
			OnTrayTimer();
		else if (wParam == AUTOSAVE_TIMER)
			OnAutoSaveTimer();

		break;

	case WM_SIZE:
		OnSize(wParam);
		break;

	case WM_CONTEXTMENU:
		OnContextMenu(hwnd, uMsg, wParam, lParam);
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}