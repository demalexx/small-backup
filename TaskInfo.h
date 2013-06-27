#ifndef _TASKINFO_H
#define _TASKINFO_H

#include <windows.h>

#include "CommonClasses.h"

// Структура с необходимой информацией о задаче
struct STaskInfo{
	CString				sName;
	CString				sSrcFolder;
	CString				sDestFolder;
	CString				sDestGenName;
	CString				sIncludeMask;
	CString				sExcludeMask;

	bool				bSubFolders;

	bool				bScheduled;	// признак что задачу надо выполнять по расписанию
	CString				sSchedule;

	// Archive options
	bool				bDoArchive;
	BYTE				bArchCompress;
	bool				bArchSFX;
	bool				bArchDelFiles;
	bool				bArchLock;
	CString				sArchTaskCmd;

	// Статистика
	__int64 unsigned	i64FinishedBytes;	// кол-во скопированных (обработанных) байтов

	STaskInfo()
	{
		sIncludeMask = TEXT("*");
		bSubFolders = true;
		
		sSchedule = TEXT("TB(Day)");

		bDoArchive = false;
		bArchCompress = 0;
		bArchSFX = false;
		bArchDelFiles = false;
		bArchLock = false;

		i64FinishedBytes = 0;
	}
};

typedef STaskInfo* PTaskInfo;

#endif