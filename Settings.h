#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#include <windows.h>

#include "INIFile.h"

#include "CommonClasses.h"
#include "Streams.h"

// Имя файла с настройками
#define cSettingsFile	TEXT("set.ini")

///////////////////////////////////////////////////////////////////////////////
// CSettings
///////////////////////////////////////////////////////////////////////////////
class CSettings{

public:
	// Положение (x, y) главного окна
	int					iMainWndX,
						iMainWndY;

	// Размеры главного окна
	WORD				wMainWndWidth,
						wMainWndHeight;

	// Размеров пяти колонок в listview главного окна
	WORD				aColumnSize[6];

	// Максимальный размер файла журнала
	WORD				wMaxLogSize;

	// Проиграть звук по окончании выполнения задачи?
	bool				bUseSound;

	// Показать окно с инфомрацией о выполненной задаче?
	bool				bShowProgress;

	// Имя файла-звука который следует проиграть если установлен флаг m_bUseSound
	CString				sSoundFile;

	// Список шаблонов для генерирования имени папки назначения
	TPStrList			aGenTemplates;

	CString				sArchiverEXE;
	CString				sArchiverCmdLine;
	CList<TStrPair*>	aArchiverParams;

	bool LoadSettings();
	bool SaveSettings();
};

extern CSettings* Settings;

#endif