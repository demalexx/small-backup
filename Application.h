#ifndef _APPLICATION_H
#define _APPLICATION_H

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include "resource.h"
#include "CommonClasses.h"
#include "CtrlWrappers.h"

#include "Global.h"
#include "Instance.h"
#include "Settings.h"
#include "Schedule.h"

#include "OptionsWnd.h"
#include "TaskWnd.h"

#define cClassName				TEXT("SmallBackup")	// имя класса главного окна приложения
#define IDC_LV_TASKS			100					// идентификатор listview с задачами

#define SCHEDULE_TIMER			1					// номер таймера для работы с расписанием
#define TRAY_TIMER				2					// таймер для рисования анимации в трее

#define AUTOSAVE_TIMER			3					// для автоматического сохранения задач
#define AUTOSAVE_TIME			60000*5				// время через которое выполнять автосохранение	(5 мин)
#define AUTOSAVE_TIMER_INTERVAL	60000				// время через которое будет срабатывать таймер автосохранения

#define Tray_Message			TEXT("WM_TRAY")		// сообщение от трея
#define TRAY_ANIM_FRAMES		17					// кол-во "кадров" в анимации
#define TRAY_ANIM_INTERVAL		100					// интервал через которые нужно менять картинки в трее

///////////////////////////////////////////////////////////////////////////////
// CApplication
///////////////////////////////////////////////////////////////////////////////
class CApplication{
private:
	HINSTANCE		m_hInstance;
	HWND			m_hMainWindow;

	CListView*		m_pLVTasks;

	HMENU			m_hMainMenu;
	HMENU			m_hTasksMenu;

	CDateTime		m_dtLastTasksSave;	// время последнего сохранения задачи

	HIMAGELIST		m_hTrayAnimation;	// картинки которые образуют анимацию в трее
	BYTE			m_bCurTrayIcon;		// номер текущего "кадра" анимации трея

	// Когда польз-ль закрывает окно, но есть выполняющиеся задачи
	// в эту переменную заносится кол-во выполняющихся задач
	// которые нужно прервать и приложение дожидается пока 
	// все задачи не завершатся. Так же это признак что нельзя
	// выходить при закрытии программы - сначала нужно дождаться
	// завершения всех выполняющихся задач
	int				m_iWaitTasks;

	static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Добавляет задачу в listview, т.е. заполняет нужные колонки нужными данными задачи
	void AddTaskToListView(CTask* pTask);

	// Обновляет выводимую в listview информацию о задаче, либо всю информацию, либо только состояние
	void UpdateTaskInListView(int iIndex, bool bStatusOnly = false, BYTE bPercents = (BYTE) -1);
	void UpdateTaskInListView(CTask* pTask, bool bStatusOnly = false, BYTE bPercents = (BYTE) -1);

	// Делает активными\неактивными пункты меню о задаче
	void EnDisTaskCommands();
	
	// Преобразует CDateTime след. запуска в строку
	CString NextRunToStr(CDateTime dtRun);

	// Ф-я работает с расписанием всех задач, которые надо запускает
	void UpdateSchedule();

	// Возвращает время в мс до ближайшего запуска запланированной задачи
	DWORD ScheduledTaskAfterMSec();

	// Прячет главное окно и показывает иконку в трее
	void HideToTray();

	// Реализует анимацию в трее, аргументом - вызов ф-ии по таймеру или вручную
	void TrayAnimation(bool bTimer = false);

	// Обработчики событий
	bool OnCreate();
	bool OnClose();

	bool OnSize(WPARAM wFlag);
	
	bool OnTaskNew();
	bool OnTaskDel();
	bool OnTaskChange();
	bool OnTaskStart(CTask* pTask, bool bScheduled = false);
	bool OnTaskShowPrg();

	void OnScheduleTimer();
	void OnTrayTimer();
	void OnAutoSaveTimer();

	void OnTray(LPARAM lMessage);

	bool OnOptions();
	bool OnAbout();

	bool OnListViewSel(int iIndex);
	bool OnContextMenu(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void OnThreadUpdate(PTaskProgress pTaskProgress, CTask* pTask);	// вызывается когда поток сообщает о прогрессе выполнения
	bool OnTaskDone(CTask* pTask);

public:
	CApplication();
	~CApplication();

	int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPTSTR lpCmdLine, int nCmdShow);
	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern CApplication* Application;

#endif