#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include <windows.h>

#include "CommonClasses.h"
#include "Utils.h"

// Типы временных блоков
#define TIME_BLOCK_NONE			0
#define TIME_BLOCK_MONTH		1

#define TIME_BLOCK_WEEK			2
#define TIME_BLOCK_WEEK_CHILD	3

#define TIME_BLOCK_DAY			4
#define TIME_BLOCK_DAY_CHILD	5

#define TIME_BLOCK_HOUR			6
#define TIME_BLOCK_HOUR_CHILD	7

struct STimeBlock{
	BYTE		bType;			// тип временного блока
	bool		bUseTo;			// использовать период (с ... по ...)
	bool		bUseRepeat;		// использовать "повторять каждые"
	WORD		wRepeatMin;		// через сколько минут повторять задачу
	
	// Св-ва недели
	BYTE		bWeekNumber;	// номер недели от начала месяца (начало периода)
	BYTE		bWeekNumberTo;	// конец периода

	// Св-ва дня
	BYTE		bDayDayOfWeek;		// день недели (начало периода)
	BYTE		bDayDayOfWeekTo;

	// Св-ва часа
	BYTE		bHourHour;		// час (начало периода)
	BYTE		bHourHourTo;

	BYTE		bMinExecute;	// минута часа когда начать

	STimeBlock();
};

typedef STimeBlock* PTimeBlock;

struct STBTreeItem{
	STBTreeItem*		pParent;
	CList<STBTreeItem*>	aChilds;

	STimeBlock			TimeBlock;

	STBTreeItem(){
		pParent = NULL;
	};
};

typedef STBTreeItem* PTBTreeItem;

///////////////////////////////////////////////////////////////////////////////
// CSchedule
///////////////////////////////////////////////////////////////////////////////
class CSchedule{
private:
	// Вспомогательные ф-ии для построения дерева расписания
	WORD GetNextID(CString& sSource, WORD wStart, CString& sID, CString& sValue);	// возвращает ID следующего временного блока в строке
	bool GetTwoNums(CString sSource, int& iOne, int& iTwo);	// распознаёт в строке одно\два числа

	PTBTreeItem GetNextSibling_(PTBTreeItem pItem);	// вернёт след. эл-т, родитель которого равен родителю pItem

	CString MakePeriod(BYTE bFirst, BYTE bSecond, bool bFromTo = false);	// формирует строку из одного\двуз целых чисел

	PTBTreeItem GetActiveTB(PTBTreeItem pScheduleTree, CDateTime dTime);	// возвращает "активный" временной блок из указанного дерева расписания

public:
	CSchedule();
	~CSchedule();

	// Строит дерево расписания по строке
	PTBTreeItem BuildScheduleTree(CString sTree);

	// Создаёт строку по дереву расписания
	CString ScheduleTreeToString(PTBTreeItem pRoot);

	// Освобождает память, связанную с деревом
	bool FreeScheduleTree(PTBTreeItem pRoot);

	// Преобразует временной блок к строке
	CString TimeBlockToString(PTimeBlock pTimeBlock);

	// Возвращает дату след. запуска задачи (после указанной даты)
	CDateTime GetNextRun(PTBTreeItem pScheduleTree, CDateTime dFrom);

	// Возвращает кол-во мс до запуска задачи, зная CDateTime запуска
	__int64 GetNextRunMSec(CDateTime dtRun);

	int GetTimeBlockMaxChilds(BYTE bTBType);
};

extern CSchedule* Schedule;

#endif