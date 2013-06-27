#include "Thread.h"

CThread::CThread(bool bSuspended)
{
	// Создаём поток
	m_hThread = CreateThread(NULL, 0, StaticThreadProc, (void*) this, bSuspended ? CREATE_SUSPENDED : 0, NULL);

	m_bTerminated = false;

	// Обнулим информацию о выполнении задачи
	ZeroMemory(&m_TaskProgress, sizeof(STaskProgress));

	InitializeCriticalSection(&m_csTerminated);

	// Для корректного отсчитывания интервалов в UpdateInfo
	m_dwLastUpdate = 0;
}

CThread::~CThread()
{
	// Закрываем хендл потока
	CloseHandle(m_hThread);

	DeleteCriticalSection(&m_csTerminated);

	// Нужно освободить строки (CString) из списка m_aIncludeList и m_aExcludeList
	for (int i = 0; i < m_aIncludeList.Size(); i++)
		delete (CString*) m_aIncludeList[i];

	for (int i = 0; i < m_aExcludeList.Size(); i++)
		delete (CString*) m_aExcludeList[i];
}

DWORD WINAPI CThread::StaticThreadProc(LPVOID lpParameter)
{
	return ((CThread*) lpParameter)->ThreadProc(0);
}

void CThread::UpdateInfo(bool bImmediate)
{
	// Пошлём сообщения с интервалом не менее 350 мс
	if (GetTickCount() - m_dwLastUpdate >= 350 || bImmediate)
	{
		// Преобразуем проценты из float в BYTE
		VarI1FromR4(m_fPercents, (char*) &m_TaskProgress.bPercents);

		// пошлём сообщения всем подписавшимся слушателям
		for (int i = 0; i < m_ProgressListenrs.Size(); i++)
			SendMessage(m_ProgressListenrs[i], RegisterWindowMessage(ThreadUpdateMessage), (WPARAM) &m_TaskProgress, m_pTask);

		m_dwLastUpdate = GetTickCount();
	}
}

void CThread::AddLog(LPCTSTR cText, ...)
{
	va_list val;
	va_start(val, cText);

	TCHAR buf[MAX_PATH];
	wvsprintf(buf, cText, val);

	va_end(val);

	// пошлём сообщения всем подписавшимся слушателям
	for (int i = 0; i < m_ProgressListenrs.Size(); i++)
		SendMessage(m_ProgressListenrs[i], RegisterWindowMessage(ThreadStringMessage), (WPARAM) buf, NULL);
}

bool CThread::CheckDestFolder()
{
	// Если папка назначения есть - всё ок
	if (PathFileExists(m_sDestFolder.C())) 
		return true;

	// иначе пытаемся создать папка назначения
	return ForceDirectories(m_sDestFolder.C());
}

void CThread::DoPrescan(CString scFolder)
{
	m_bLevel++;

	HANDLE hFind = NULL;
	WIN32_FIND_DATA dFind;

	// Ищем все файлы\папки в переданной папке
	CString sFolder = scFolder + TEXT("\\*");
	if ((hFind = FindFirstFile(sFolder.C(), &dFind)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Если прервали выполнение - выход из цикла
			if (IsTerminated()) break;

			// Пропускаем специальные папки '.' и '..'
			if (lstrcmp(dFind.cFileName, TEXT(".")) == 0 ||
				lstrcmp(dFind.cFileName, TEXT("..")) == 0)
				continue;

			if ((dFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				m_dwPrescanFolders++;

				// Продолжим предсканирование если не превысилы вложенность папок
				if (m_bLevel < PRESCAN_LEVELS - 1)
					DoPrescan(scFolder + TEXT("\\") + dFind.cFileName);
			}

		} while (FindNextFile(hFind, &dFind) != 0);

		FindClose(hFind);
	}

	m_bLevel--;
}

DWORD CThread::FilesInFolder(CString scFolder)
{
	HANDLE hFind = NULL;
	WIN32_FIND_DATA dFind;

	DWORD res = 0;

	// Ищем все файлы\папки в переданной папке
	CString sFolder = scFolder + TEXT("\\*");
	if ((hFind = FindFirstFile(sFolder.C(), &dFind)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			// Если прервали выполнение - выход из цикла
			if (IsTerminated()) break;

			// Пропускаем специальные папки '.' и '..'
			if (lstrcmp(dFind.cFileName, TEXT(".")) == 0 ||
				lstrcmp(dFind.cFileName, TEXT("..")) == 0)
				continue;

			if ((dFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
				res++;

		} while (FindNextFile(hFind, &dFind) != 0);

		FindClose(hFind);
	}

	return res;
}

void CThread::MakeList(CString scString, TPStrList& aMasks)
{
	if (scString.Empty()) return;

	CString sString(scString);
	sString.ToLower();

	WORD wStart = 0;
	WORD wPos = 0;
	while (wPos < sString.Length())
	{		
		if (sString[wPos] == TEXT(';'))
		{
			CString sMask(sString, wStart, wPos - wStart);
			sMask.Trim();
			aMasks.Add(new CString(sMask));

			wPos++;
			wStart = wPos;
		}
		else if (sString[wPos] == TEXT('\r'))
		{
			CString sMask(sString, wStart, wPos - wStart);
			sMask.Trim();
			aMasks.Add(new CString(sMask));

			wPos += 2;
			wStart = wPos;
		}
		else if (wPos == sString.Length() - 1)
		{
			CString sMask(sString, wStart, wPos - wStart + 1);
			sMask.Trim();
			aMasks.Add(new CString(sMask));
			break;
		}
		else
			wPos++;
	}
}

bool CThread::MaskMatch(CString sStr, CString sMask)
{
	sStr.ToLower();

	LPTSTR cp = 0;
	LPTSTR mp = 0;

	LPTSTR s = new TCHAR[sStr.Length() + 1];
	LPTSTR mask = new TCHAR[sMask.Length() + 1];

	lstrcpy(s, sStr.C());
	lstrcpy(mask, sMask.C());

	for (; *s&& *mask != TEXT('*'); mask++, s++)
		if (*mask != *s && *mask != TEXT('?')) return false;

	for (;;)
	{
		if (!*s)
		{
			while (*mask == TEXT('*')) mask++;
			return !*mask;
		}

		if (*mask == TEXT('*'))
		{
			if (!*++mask) return true;
			
			mp = mask;
			cp=s+1;
			continue;
		}

		if (*mask == *s || *mask == TEXT('?'))
		{
			mask++, s++;
			continue;
		}

		mask = mp; s = cp++;
	}
}

bool CThread::MaskListMath(CString sStr, TPStrList& aMasks)
{
	for (int m = 0; m < aMasks.Size(); m++)
		if (MaskMatch(sStr, *aMasks[m]))
			return true;

	return false;
}

void CThread::DoSubFolders(CString sFolder, CString sBackupFolder)
{
	m_bLevel++;	// очередной раз вошли рекурсивно в себя

	HANDLE hFind = NULL;
	WIN32_FIND_DATA dFind;

	CString strFolder = sFolder + TEXT("\\*");

	bool bError = false;
	DWORD dwFiles = 0;		// кол-во файлов в текущей папке
	DWORD dwFolders = 0;	// кол-во папок в текущей папке

	// Папки куда копировать файлы несуществует, попробуем создать
	if (!PathFileExists(sBackupFolder.C()))
	{
		// создать папку не вышло, ошибка
		if (CreateDirectory(sBackupFolder.C(), NULL) == FALSE)
		{
			AddLog(TEXT("[!] Не могу создать папку \'%s\'! Пропуск папки %s\r\n"), sBackupFolder.C(), sFolder.C());
			bError = true;
		}
	}

	// Не работаем дальше на текущем уровне вложенности если произошла ошибка - нельзя создать папку назначения
	if ((!bError) && (hFind = FindFirstFile(strFolder.C(), &dFind)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (IsTerminated()) break;

			// Skip '.' and '..' folders
			if (lstrcmp(dFind.cFileName, TEXT(".")) == 0 ||
				lstrcmp(dFind.cFileName, TEXT("..")) == 0)
				continue;

			// Работаем с папкой
			if ((dFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				if (!m_TaskInfo.bSubFolders) continue;

				// Добавим папку в файл-список
				if (m_bLevel == 1)
				{
					m_pFileList->SetLine(CString(sBackupFolder + TEXT("\\") + dFind.cFileName).C());	
					m_lFileList.Add(sBackupFolder + TEXT("\\") + dFind.cFileName);
				}

				// Формируем имя след. папки для обработки (вложенной)
				CString strNewFolder = sFolder + TEXT("\\") + dFind.cFileName;
				CString sNewBackupFolder = sBackupFolder + TEXT("\\") + dFind.cFileName;

				// Обработали ещё одну папку на текущем уровне вложенности
				dwFolders++;

				// Вызываемся рекурсивно - обрабатываем вложенную папку
				DoSubFolders(strNewFolder, sNewBackupFolder);
			}
			// Работаем с файлом
			else
			{
				// Увеличим кол-во обработанных файлов
				m_TaskProgress.dwProceedFiles++;

				// Сперва проверим удовлетворяет ли имя файла с "маской исключения"
				if (MaskListMath(CString(dFind.cFileName), m_aExcludeList))
					// если да - увеличим число пропущенных файлов
					m_TaskProgress.dwExcludedFiles++;
				// иначе, если имя файла удовлетворяет "маске включения" - работаем с ним
				else if (MaskListMath(CString(dFind.cFileName), m_aIncludeList))
				{
					// Формируем полные имена файлов - откуда и куда копировать
					CString sFileCopyFrom = sFolder + TEXT("\\") + dFind.cFileName;
					CString sFileCopyTo = sBackupFolder + TEXT("\\") + dFind.cFileName;

					// Если файл уже есть в папке назначения - удалим его (как будто он старый)
					if (PathFileExists(sFileCopyTo.C()))
						DeleteFile(sFileCopyTo.C());

					// Скопируем файл из исходной папки в папку назначения
					if (!CopyFile(sFileCopyFrom.C(), sFileCopyTo.C(), TRUE))
					{
						// ошибка при копировании
						m_TaskProgress.dwErrors++;
						AddLog(TEXT("[!] Ошибка при копировании файла \'%s\'!\r\n"), sFileCopyFrom.C());
					}
					else
					{
						HANDLE hFile = CreateFile(sFileCopyFrom.C(), GENERIC_READ, 0, NULL, OPEN_EXISTING, NULL, NULL);
						if (hFile != INVALID_HANDLE_VALUE)
						{
							m_TaskProgress.i64FinishedBytes += GetFileSize(hFile, NULL);
							CloseHandle(hFile);
						}

						// Добавим файл в файл-список, если он скопировался успешно
						if (m_bLevel == 1)
						{
							m_pFileList->SetLine(CString(sBackupFolder + TEXT("\\") + dFind.cFileName).C());
							m_lFileList.Add(sBackupFolder + TEXT("\\") + dFind.cFileName);
						}
					}

					// Обработали ещё один файл - увеличим счётчики
					m_TaskProgress.dwIncludedFiles++;
                    dwFiles++;
				}
				// иначе, если имя файла не удовлетворяет ни одной из масок - записываем его как необработанный
				else
					m_TaskProgress.dwExcludedFiles++;

				// Обработали файл - увеличим процент выполнения
				if (!m_TaskInfo.bSubFolders)
					m_fPercents += m_fPercentInc;

				UpdateInfo();
			}

		} while (FindNextFile(hFind, &dFind) != 0);

		FindClose(hFind);
	}

	// Если нет ошибки и уровень вложенности менее заданного числа - добавим в лог эту папку
	if (!bError && m_bLevel <= 2)
		AddLog(TEXT("[i] Папка \'%s\' обработана; файлов: %d, папок: %d\r\n"), sFolder.C(), dwFiles, dwFolders);

	// Посчитаем процент выполнения задачи
	if (m_TaskInfo.bSubFolders && m_bLevel <= PRESCAN_LEVELS)
		m_fPercents += m_fPercentInc;

	// Обработали очередную папку
	m_TaskProgress.dwProceedFolders++;
	UpdateInfo();

	// поднимаемся на уровень выше
	m_bLevel--;
}

CString CThread::ParseArchiverCmdLine()
{
	WORD wPos = 0;
	WORD wResPos = 0;
	WORD wLen = m_sArchiverCmdLine.Length();

	TCHAR res[MAX_PATH] = {0};

	while (wPos < wLen)
	{
		if (m_sArchiverCmdLine[wPos] != TEXT('%'))
		{
			res[wResPos] = m_sArchiverCmdLine[wPos];
			wPos++;
			wResPos++;
		}
		else if (m_sArchiverCmdLine[wPos] == TEXT('%') && 
			wPos + 1 < wLen && m_sArchiverCmdLine[wPos + 1] == TEXT('%'))
		{
			// Двойной процент ('%%') считаем за один, не макрос
			res[wResPos] = TEXT('%');
			wPos += 2;
			wResPos++;
		}
		else if (m_sArchiverCmdLine[wPos] == TEXT('%'))
		{
			CString sMacro;

			wPos++;
			WORD wMacroStart = wPos;

			while (wPos < wLen)
			{
				if (m_sArchiverCmdLine[wPos] != TEXT('%'))
					wPos++;
                else if (m_sArchiverCmdLine[wPos] == TEXT('%'))
				{	   
					sMacro = m_sArchiverCmdLine.SubStr(wMacroStart, wPos - wMacroStart).ToLowerNew();
					CString sMacroValue;

					while (true)
					{
						if ((sMacro[0] >= TEXT('a') && sMacro[0] <= TEXT('z')) || (sMacro[0] >= TEXT('A') && sMacro[0] <= TEXT('Z')))
						{
							break;
						}
						else
						{
							sMacroValue += sMacro[0];
							sMacro.Delete(0, 1);
						}
					}							

					if (sMacro == TEXT("compress"))
					{
						CString sCompress = FormatC(TEXT("compress%d"), m_TaskInfo.bArchCompress);
						sMacroValue += GetMacroValue(sCompress);

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else if (sMacro == TEXT("sfx") && m_TaskInfo.bArchSFX)
					{
						sMacroValue += GetMacroValue(TEXT("sfx"));

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else if (sMacro == TEXT("lock") && m_TaskInfo.bArchLock)
					{
						sMacroValue += GetMacroValue(TEXT("lock"));

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else if (sMacro == TEXT("taskcmd"))
					{
						sMacroValue += m_TaskInfo.sArchTaskCmd;

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else if (sMacro == TEXT("filelist"))
					{
						sMacroValue += TEXT("\"");
						sMacroValue += m_sFileList;
						sMacroValue += TEXT("\"");

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else if (sMacro == TEXT("archive"))
					{
						sMacroValue += TEXT("\"");
						sMacroValue += m_sDestFolder;
						sMacroValue += TEXT("\"");

						lstrcat(res, sMacroValue.C());
						wResPos = lstrlen(res);
						wPos++;
					}
					else 
						wPos++;

					break;
				}
			}
		}
	}

	return res;
}

CString CThread::GetMacroValue(CString sMacro)
{
	CString slMacro = CString(sMacro).ToLowerNew();

	for (BYTE i = 0; i < m_aArchiverParams.Size(); i++)
	{
		TStrPair* pStrPair = m_aArchiverParams[i];

		if ((pStrPair->First.ToLowerNew()) == slMacro)
			return pStrPair->Second;
	}

	return TEXT("");
}

void CThread::DelFiles(CString sFolder)
{
	if ((GetFileAttributes(sFolder.C()) & FILE_ATTRIBUTE_DIRECTORY) == 0)
	{
		DeleteFile(sFolder.C());
		return;
	}

	HANDLE hFind = NULL;
	WIN32_FIND_DATA dFind;

	CString sFindFolder = sFolder + TEXT("\\*");
	if ((hFind = FindFirstFile(sFindFolder.C(), &dFind)) != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (lstrcmp(dFind.cFileName, TEXT(".")) == 0 ||
				lstrcmp(dFind.cFileName, TEXT("..")) == 0)
				continue;

			DelFiles(sFolder + TEXT("\\") + dFind.cFileName);
		} while (FindNextFile(hFind, &dFind) != 0);

		FindClose(hFind);
	}

	RemoveDirectory(sFolder.C());
	return;
}

DWORD WINAPI CThread::ThreadProc(LPVOID lpParameter)
{
	UpdateInfo(true);

	// Скопируем имена папок в новые переменные что бы не затереть исходные значения
	m_sSrcFolder = m_TaskInfo.sSrcFolder;
	m_sDestFolder = m_TaskInfo.sDestFolder;

	DelSlash(m_sSrcFolder);

	// Отформатируем имя папки назначения (раскроем макросы)
	CString sFormatedGenName = FormatDateTime(m_TaskInfo.sDestGenName.C());

	// Сформируем окончательное имя папки назначения
	m_sDestFolder += sFormatedGenName;
	DelSlash(m_sDestFolder);

	// Начало лога - запишем информацию о выполняемой задаче
	AddLog(TEXT("[i] Начало выполнения задачи, параметры:\r\n"));
	AddLog(TEXT("========================================\r\n"));
	AddLog(TEXT("[i] Задача: %s\r\n"), m_TaskInfo.sName.C());
	AddLog(TEXT("[i] Исходная папка: %s\r\n"), m_TaskInfo.sSrcFolder.C());
	AddLog(TEXT("[i] Папка назначения: %s\r\n"), (m_TaskInfo.sDestFolder + m_TaskInfo.sDestGenName).C());
	AddLog(TEXT("[i] Папка назначения: %s\r\n"), m_sDestFolder.C());
	AddLog(TEXT("[i] Включать файлы: %s\r\n"), m_TaskInfo.sIncludeMask.C());
	AddLog(TEXT("[i] Исключать файлы: %s\r\n"), m_TaskInfo.sExcludeMask.C());
    AddLog(TEXT("========================================\r\n"));

	// Если выполнение задачи не прервали - продолжаем
	if (!IsTerminated())
	{
		// Проверяем что бы папка назначения не была подпапкой исходной папки
		if (IsSubFolder(m_TaskInfo.sSrcFolder, m_sDestFolder))
		{
			AddLog(TEXT("[!] Папка назначения является подпапкой исходной папки\r\n"));
			AddLog(TEXT("[x] Невозможно продолжить выполнение задачи\r\n"));
		}
		// Проверка что папка назначения существует (или успешно создана)
		else if (!CheckDestFolder())
		{
			AddLog(TEXT("[!] Не могу создать папку назначения (%s)\r\n"), m_TaskInfo.sDestFolder.C());
			AddLog(TEXT("[x] Невозможно продолжить выполнение задачи\r\n"));
		}
		else
		{
			if (m_TaskInfo.bDoArchive)
			{
				// Получим временное (и уникальное) имя файла во временной папке
				TCHAR cTempPath[MAX_PATH];
				// получим временную папку
				GetTempPath(MAX_PATH, cTempPath);
				// получим уникальное имя для времнного файла, так же система создаст этот файл
				GetTempFileName(cTempPath, TEXT("smb"), 0, (LPTSTR) m_sFileList.Buf());

				if (!PathFileExists(m_sFileList.C()))
				{
					AddLog(TEXT("[!] Не могу создать файл-список (%s)\r\n"), m_sFileList.C());
					AddLog(TEXT("[!] Задача будет выполнена без архивирования\r\n"));
				}
			}

			// Сделаем предв. сканирование исходной папки
			m_bLevel = 0;
			m_dwPrescanFolders = 1;

			// Если работа с подпапками - предсканируем их
			if (m_TaskInfo.bSubFolders)
			{
				DoPrescan(m_sSrcFolder);

				// Узнаем сколько процентов занимает каждая "предсканированная" папка
				m_fPercentInc = (float) 100 / m_dwPrescanFolders;
			}
			// иначе просто подсчитаем кол-во файлов в исходной папке
			else
			{
				DWORD dwFiles = FilesInFolder(m_sSrcFolder);
				m_fPercentInc = (float) 100 / dwFiles;
			}

			UpdateInfo(true);

			MakeList(m_TaskInfo.sIncludeMask, m_aIncludeList);
			MakeList(m_TaskInfo.sExcludeMask, m_aExcludeList);

			// Создадим объект, в который будем записывать файл-лист для архиватора
			m_pFileList = new CMemTextFile();

			// Запускаем копирование файлов
			m_bLevel = 0;
			m_fPercents = 0;
			DoSubFolders(m_sSrcFolder, m_sDestFolder);

			// Сохраним файл-лист из памяти на диск
			m_pFileList->SaveFile(m_sFileList.C());
			delete m_pFileList;

			// Это последний вызов updateinfo, пускай получатели сообщения знают об этом
			m_TaskProgress.bLastInfo = true;
			UpdateInfo(true);

			// если задача не прервана - выполняем архивацию, если надо
			if (!IsTerminated() && m_TaskInfo.bDoArchive)
			{
				if (!PathFileExists(m_sArchiverEXE.C()))
				{
					AddLog(TEXT("[!] Не найден файл архиватора\r\n"));
					AddLog(TEXT("[x] Отмена запуска архиватора\r\n"));
				}											 
				else if (PathFileExists(m_sFileList.C()))
				{
					CString sArch = FormatC(TEXT("\"%s\" %s"), m_sArchiverEXE.C(), ParseArchiverCmdLine().C());
					AddLog(TEXT("[i] Вызов архиватора: %s\r\n"), sArch.C());

					PROCESS_INFORMATION pi = {0};

					STARTUPINFO si = {0};
					si.cb = sizeof(si);

					TCHAR cmd[MAX_PATH];
					lstrcpy(cmd, sArch.C());

					// Создаём процесс - запускаем архиватор
					if (CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
					{
						// И ждём пока он не завершит работу
						WaitForSingleObject(pi.hProcess, INFINITE);

						DWORD dwExitCode;
						GetExitCodeProcess(pi.hProcess, &dwExitCode);

						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);
    
						AddLog(TEXT("[i] Архиватор завершил работу с кодом %d\r\n"), dwExitCode);

						// Если указано - удалим скопированный файлы (их имена занесены в список)
						if (m_TaskInfo.bArchDelFiles)
						{
							for (int i = 0; i < m_lFileList.Size(); i++)
                                DelFiles(m_lFileList[i]);
						}
					}
					else
						AddLog(TEXT("[!] Ошибка при вызове архиватора\r\n"));
				}

				// Удалим файл-список для архиватора
				DeleteFile(m_sFileList.C());
			}

			AddLog(TEXT("========================================\r\n"));
			if (IsTerminated())
				AddLog(TEXT("[i] Выполнение задачи прервано\r\n"));
			else
				AddLog(TEXT("[i] Выполнение задачи завершено\r\n"));
		}
	}

	// пошлём сообщения всем подписавшимся слушателям что работа завершена
	for (int i = 0; i < m_ProgressListenrs.Size(); i++)
		PostMessage(m_ProgressListenrs[i], RegisterWindowMessage(ThreadDoneMessage), 0, 0);

	return 0;
}

void CThread::Suspend()
{
	SuspendThread(m_hThread);
}

void CThread::Resume()
{
	ResumeThread(m_hThread);
}

void CThread::Terminate()
{
	EnterCriticalSection(&m_csTerminated);
	m_bTerminated = true;
	LeaveCriticalSection(&m_csTerminated);
}

bool CThread::IsTerminated()
{
	// Войдём в крит. секцию, т.к. переменную m_bTerminated могут одновременно читать\писать из разных мест
	EnterCriticalSection(&m_csTerminated);
	bool res = (m_bTerminated == true);
	LeaveCriticalSection(&m_csTerminated);

	return res;
}