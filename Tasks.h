#ifndef _TASKS_H
#define _TASKS_H

#include <windows.h>
#include <commctrl.h>

#include "INIFile.h"

#include "resource.h"
#include "Instance.h"
#include "Settings.h"

#include "CommonClasses.h"
#include "Streams.h"
#include "Global.h"
#include "TaskInfo.h"
#include "Schedule.h"

#include "Thread.h"

// Имя файла где находятся задачи
#define cTasksFile	TEXT("Tasks.ini")

// Имя сообщение что задача завершена
extern LPCTSTR TaskDoneMessage;

// Этот нужный класс объявлен ниже
class CTaskProgressWnd;

///////////////////////////////////////////////////////////////////////////////
// CTask
///////////////////////////////////////////////////////////////////////////////

// Возможные состояния задачи
#define TASK_STATE_NONE			0
#define TASK_STATE_WORKING		1
#define TASK_STATE_TERMINATING	2

class CTask{
private:
	// Информация о задаче
	STaskInfo			m_TaskInfo;

	// Дерево расписания
	PTBTreeItem			m_pScheduleTree;

	CDateTime			m_dtLastRun;	// Время последнего запуска	
	CDateTime			m_dtNextRun;	// Время след. запуска

	// Состояние задачи
	BYTE				m_bState;

	// Признак что задача прервана пользователем
	bool				m_bAborted;

	// Задача заблокирована - нельзя выпонять (например, открыто окно с параметрами задачи)
	bool				m_bLocked;

	// Окошко которое отображает информацию о выполнении задачи
	CTaskProgressWnd*	m_hProgressWnd;

	// Поток, непосредственно выполняющий задачу
	CThread*			m_pThread;				

public:
	CTask();
	~CTask();

	BYTE GetState() { return m_bState; };
	bool IsAborted() { return m_bAborted; };
	void Lock() { m_bLocked = true; };
	void UnLock() { m_bLocked = false; };

	bool Start(bool bScheduled = false);
	bool Abort();

	void ShowProgressWnd();

	bool OnThreadDone();

	PTaskInfo GetPTaskInfo() { return &m_TaskInfo; };
	void SetTaskInfo(PTaskInfo pTaskInfo) { m_TaskInfo = *pTaskInfo; };

	bool BuildScheduleTree();	// построить дерево расписания
	bool CalcNextRun(CDateTime dtFrom);	// подсчитать время след. запуска считая от переданного значения времени
	CDateTime GetNextRun() { return m_dtNextRun; };	// возвращает время след. запуска
	CDateTime GetLastRun() { return m_dtLastRun; };	// возвращает время последнего запуска
	void SetLastRun(CDateTime dtLastRun) { m_dtLastRun = dtLastRun; };
};

///////////////////////////////////////////////////////////////////////////////
// CTaskList
///////////////////////////////////////////////////////////////////////////////
class CTaskList{
private:
	CList<CTask*>	m_aTasks;

public:
	CTaskList();
	~CTaskList();

	CTask* AddTask(PTaskInfo TaskInfo);
	bool DelTask(CTask* pTask);
	bool DelTask(CString sTask);

    CTask* FindTaskByName(CString sName);

	// Кол-во работающих задач
	WORD GetRunningTasksCount();

	bool LoadTasks();
	bool SaveTasks();

	WORD GetSize() { return m_aTasks.Size(); };
	CTask* GetTask(WORD Index) { return m_aTasks[Index]; };
};

// Глобальный список задач
extern CTaskList* TaskList;

///////////////////////////////////////////////////////////////////////////////
// CTaskProgressWnd
///////////////////////////////////////////////////////////////////////////////
class CTaskProgressWnd{
private:
	HWND	m_hWindow;	// хендл окна

	CTask*	m_pTask;	// выполняемая задача

	static BOOL CALLBACK StaticDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	BOOL CALLBACK DlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

	void ResetInfo();

	// Обработчики событий
	bool OnInit();

	void OnSizing(RECT* pRect);
	void OnSize();

	bool OnHideClick();
	bool OnAbortClick();

	bool OnClose();

	void OnThreadUpdate(PTaskProgress pTaskProgress);
	void OnThreadString(char* Message);
	void OnThreadDone();

public:
	CTaskProgressWnd(CTask* pTask = NULL);
	~CTaskProgressWnd();

	HWND GetHWND() { return m_hWindow; };

	// Создать окно с информацией о выполнении задачи, аргументом - показывать его или оставить скрытым
	bool CreateProgressWnd(bool bHidden = false);

	void Show();
};

#endif