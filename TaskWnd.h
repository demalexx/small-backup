#ifndef _TASKWND_H
#define _TASKWND_H

#include <windows.h>
#include <shlobj.h>

#include "resource.h"
#include "CommonClasses.h"
#include "CtrlWrappers.h"
#include "Global.h"
#include "Utils.h"

#include "Tasks.h"
#include "TaskInfo.h"
#include "Schedule.h"

// Режимы диалога
#define NEW_TASK		0	// новая задача
#define CHANGE_TASK		1	// изменение параметров существующей задачи

class CTaskWnd{
private:
	STaskInfo			m_TaskInfo;

	BYTE				m_bDialogMode;
	PROPSHEETHEADER		m_PropSheetHdr;

	// Хендлы вкладок
	HWND				m_hGeneralSheet;
	HWND				m_hArchiveSheet;
	HWND				m_hScheduleSheet;

	// Дерево расписания
	CTree*				m_pTimeBlocksTree;

	// Флаг что введённое имя папки назначения неверно
	bool				m_bInvalidDestPath;

	static BOOL CALLBACK StaticGeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK StaticArchiveSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK StaticScheduleSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Ф-ии для диалога "Обзор папок"
	static int CALLBACK BFFCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData);
	CString BrowseForFolder(HWND hOwner, CString sText, CString sRoot);

	void UpdateGenResult();

	// По дереву расписания заполняет компонент "дерево"
	bool FillScheduleTree(HTREEITEM htiCurItem, PTBTreeItem pCurItem);

	// По эл-м компонента "дерево" строит дерево расписания
	PTBTreeItem TreeToScheduleTree(HTREEITEM htiCurItem, PTBTreeItem pParent = NULL);

	// Заполняет выпадающие списки значениями в соответствии с типом временного блока
	void ScheduleFillTBPropsCombo(HWND hCombo, BYTE bTBType);

	// Выводит время след. запуска
	void SchedulePrintNextRun();

	// Обработчики событий вкладки "Общие параметры"
	bool GeneralOnInit();
	bool GeneralOnChooseSrcFolder();
	bool GeneralOnChooseDestFolder();
	bool GeneralOnUpdateDestFolder();
	bool GeneralOnUpdateGenName();
	bool GeneralOnSelChangeGenName();
	bool GeneralOnAddTemplate();
	bool GeneralOnDelTemplate();
	bool GeneralOnExit();

	// Обработчики событий вкладки "Архивирование"
	bool ArchiveOnInit();
	bool ArchiveOnExit();

	// Обработчики событий вкладки "Расписание"
	bool ScheduleOnInit();

	bool ScheduleOnTimeBlocksTreeSel();					// выделение эл-та в дереве

	bool ScheduleOnAddTimeBlock();						// добавление временного блока
	bool ScheduleOnDelTimeBlock();						// удаление временного блока

	bool ScheduleOnMainTBTypeChange();					// изменение типа временного блока

	bool ScheduleOnTBPropsFromChange(int iIndex);		// "Св-ва временного блока" изменение значения "от"
	bool ScheduleOnTBPropsToChange(int iIndex);			// "Св-ва временного блока" изменение значения "по"
	bool ScheduleOnTBPropsUsePeriodClick(bool bCheck);	// "Св-ва временного блока" изменение флага "использовать по"

	bool ScheduleOnExecuteChange(int iIndex);
	bool ScheduleOnDoRepeatClick(bool bCheck);
	bool ScheduleOnRepeatChange();	

	bool ScheduleOnExit();

public:
	CTaskWnd(HINSTANCE hInstance, HWND hParentWnd);
	~CTaskWnd();

	BOOL GeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL ArchiveSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL ScheduleSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	PTaskInfo GetPTaskInfo() { return &m_TaskInfo; } ;
	void SetTaskInfo(PTaskInfo TaskInfo) { m_TaskInfo = *TaskInfo; } ;

	bool ShowNewTask();
	bool ShowChangeTask();
};

// Глобальная переменная - класс текущего открытого окна
extern CTaskWnd* CurTaskWnd;

#endif