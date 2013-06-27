#ifndef _THREAD_H
#define _THREAD_H

#include <windows.h>
#include <oleauto.h>

#include "Instance.h"
#include "CommonClasses.h"
#include "Streams.h"
#include "Utils.h"

#include "TaskInfo.h"

// Структура хранит информацию о прогрессе выполнения задачи
struct STaskProgress{
	bool				bLastInfo;			// флаг что это последнее сообщение, задача скоро завершится

	DWORD				dwProceedFiles;		// кол-во обработанных файлов
	DWORD				dwProceedFolders;	// кол-во обработанных директорий
	DWORD				dwIncludedFiles;	// кол-во файлов удовлетворивших "маске включения"
	DWORD				dwExcludedFiles;	// кол-во файлов удовлетворивших "маске исключения"
	DWORD				dwErrors;			// кол-во ошибок
	__int64 unsigned	i64FinishedBytes;	// размер обработанных файлов (Bytes)

	BYTE				bPercents;			// процент выполнения задачи (0..100)
};

typedef STaskProgress* PTaskProgress;

///////////////////////////////////////////////////////////////////////////////
// CThread
///////////////////////////////////////////////////////////////////////////////

#define PRESCAN_LEVELS 3	// кол-во вложенных папок которое будет просмотрено при предварительном сканировании

class CThread{
private:
	HANDLE				m_hThread;

	bool				m_bTerminated;		// флаг, указывающий на то что поток следует завершить
	CRITICAL_SECTION	m_csTerminated;		// крит. секция для обращения к m_bTerminated

	LPARAM				m_pTask;			// адрес где находится класс CTask, работающий с задачей
	STaskInfo			m_TaskInfo;			// локальная копия информации о выполняемой задаче
	CString				m_sSrcFolder;		// исходная папка (не тоже самое что в m_TaskInfo)
	CString				m_sDestFolder;		// папка назначения, с раскрытыми макросами (не тоже самое что в m_TaskInfo)

	STaskProgress		m_TaskProgress;		// информация о процессе выполнения задачи
	float				m_fPercents;		// процент выполнения в формате float, т.к. точности BYTE мало
	float				m_fPercentInc;		// на сколько увеличивать m_fPercents при определённых условиях

	CList<HWND>			m_ProgressListenrs;	// список окон, которые хотят получать сообщения о процессе выполнения задачи
	DWORD				m_dwLastUpdate;		// время последнего обновления

	BYTE				m_bLevel;			// текущий уровень вложженности на котором работают рекурсивные ф-ии
	DWORD				m_dwPrescanFolders;	// кол-во папок найденных при предв. сканировании

	TPStrList			m_aIncludeList;		// список расширений файлов которые нужно обрабатывать
	TPStrList			m_aExcludeList;		// список расширений файлов которые нужно пропустить

	CString				m_sArchiverEXE;		// путь и имя файла архиватора
	CString				m_sArchiverCmdLine;	// командная строка для архиватора
	CList<TStrPair*>	m_aArchiverParams;	// параметры для раскрытия командной строки
	CString				m_sFileList;		// имя файла со списком папок и файлов для передачи архиватору
	CMemTextFile*		m_pFileList;		// для записи файл-листа для архиватора
	CList<CString>		m_lFileList;		// список с именами файлов и папок для удаления (то же что и m_pFileList)

	static DWORD WINAPI StaticThreadProc(LPVOID lpParameter);
	DWORD WINAPI ThreadProc(LPVOID lpParameter);

	// Послать сообщение нужным окнам что "процесс идёт" (передаёт указатель на структуру с нужными данными)
	void UpdateInfo(bool bImmediate = false);

	// Передать текстовое сообщение, в отличие от UpdateInfo
	void AddLog(LPCTSTR cText, ...);

	// Проверяет возможность существования папки назначения
	bool CheckDestFolder();

	// Предварительное сканирование исходной папки - нужно примерно узнать сколько папок\файлов нужно обработать
	void DoPrescan(CString scFolder);

	// Подсчитает кол-во файлов в папке
	DWORD FilesInFolder(CString scFolder);

	// Работа с масками
	void MakeList(CString scString, TPStrList& aMasks);	// разобивает маски разделённые ';' и составляет CList из этих масок
	bool MaskMatch(CString sStr, CString sMask);		// удовлетворяет ли имя файла маске
	bool MaskListMath(CString sStr, TPStrList& aMasks);	// удовлетворяет ли имя файле хотя бы одной маске из списка

	// Рекурсивная ф-я, выполняющая копирование файлов
	void DoSubFolders(CString sFolder, CString sBackupFolder);

	// Раскрывает макросы в командной строке архиватора
	CString ParseArchiverCmdLine();

	// Возвращает значение макроса (по имени)
	CString GetMacroValue(CString sMacro);

	// Удаляет файлы и подпапки
	void DelFiles(CString sFolder);

public:
	CThread(bool bSuspended = true);
	~CThread();

	HANDLE GetHandle() { return m_hThread; };

	void Suspend();
	void Resume();
	void Terminate();

	bool IsTerminated();

	// Добавить слушателя сообщения WMThreadUpdateMessage
	void AddProgressListner(HWND hWindow) { m_ProgressListenrs.Add(hWindow); };

	void SetPTask(LPARAM pTask) { m_pTask = pTask; };
	void SetTaskInfo(PTaskInfo pTaskInfo) { m_TaskInfo = *pTaskInfo; };

	void SetArchiverEXE(CString& sArchiverEXE) { m_sArchiverEXE = sArchiverEXE; };
	void SetArchiverCmdLine(CString& sCmdLine) { m_sArchiverCmdLine = sCmdLine; };
	void SetArchiverParams(CList<TStrPair*>& aParams) { m_aArchiverParams = aParams; };
};

#endif