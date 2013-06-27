#include "Settings.h"

CSettings* Settings;

bool CSettings::LoadSettings()
{
	// Устанавливаем флаг что нужно брать настройки по-умолчанию
	bool bDefault = true;

	// Проверяем что файл с насчтройками существует
	if (PathFileExists(cSettingsFile))
	{
		//CMemSetTextFile* pSet = new CMemSetTextFile(cSettingsFile);
        CINIFile* pSet = new CINIFile(cSettingsFile);

		// Проверка типа файла и его версии ('small backup settings file', 1)
        LPCTSTR sFileDesc = pSet->GetString(TEXT(""), TEXT("File desc"));
        LPCTSTR sFileVersion = pSet->GetString(TEXT(""), TEXT("Version"));

        if (lstrcmpi(sFileDesc, TEXT("small backup settings file")) != 0 ||
            lstrcmpi(sFileVersion, TEXT("1")) != 0)
		{
			// Нужно взять настройки по-умолчанию
			bDefault = true;
		}
		else
		{
			// Не берём настройки по-умолчанию, а загружаем их из файла
			bDefault = false;

            iMainWndX = pSet->GetNumeric<int>(TEXT("Main window"), TEXT("X"), CW_USEDEFAULT);
			iMainWndY = pSet->GetNumeric<int>(TEXT("Main window"), TEXT("Y"), CW_USEDEFAULT);

			wMainWndWidth = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Width"), 400);
			wMainWndHeight = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Height"), 300);
                                                 
			aColumnSize[0] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 1"), 20);
			aColumnSize[1] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 2"), 100);
			aColumnSize[2] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 3"), 140);
			aColumnSize[3] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 4"), 140);
			aColumnSize[4] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 5"), 160);
			aColumnSize[5] = pSet->GetNumeric<WORD>(TEXT("Main window"), TEXT("Col 6"), 160);

	//		//g_pOptions->iPrgWndX = pSet->GetInt("Progress window", "X", CW_USEDEFAULT);
	//		//g_pOptions->iPrgWndY = pSet->GetInt("Progress window", "Y", CW_USEDEFAULT);

	//		g_pOptions->wPrgWndWidth = pSet->GetInt("Progress window", "Width", 270);
	//		g_pOptions->wPrgWndHeight = pSet->GetInt("Progress window", "Height", 170);

			wMaxLogSize = pSet->GetNumeric<WORD>(TEXT("Log"), TEXT("Max size"), 1024);

			bUseSound = pSet->GetBool(TEXT("Notify on end task"), TEXT("Use sound"), false);
			bShowProgress = pSet->GetBool(TEXT("Notify on end task"), TEXT("Show progress"), true);
			sSoundFile = pSet->GetString(TEXT("Notify on end task"), TEXT("Sound file"));
						
            int iGenNameTemplates = pSet->GetParamsCount(TEXT("GenName templates"));
			for (int i = 0; i < iGenNameTemplates; i++)
			{
                CString* sTempl = new CString(pSet->GetNextParamKey());
				aGenTemplates.Add(sTempl);

                // Необходимо вызвать, что бы переместиться на след. параметр
                pSet->GetNextParamValue();
			}

			sArchiverEXE = pSet->GetString(TEXT("Archiver"), TEXT("Program EXE"));
			sArchiverCmdLine = pSet->GetString(TEXT("Archiver"), TEXT("Command line"));

            int iArchivers = pSet->GetParamsCount(TEXT("Archiver"));

            // Пропустим первые два параметра в секции
            pSet->GetNextParamValue();
            pSet->GetNextParamValue();

			for (int i = 2; i < iArchivers; i++)
			{
				TStrPair* pStrPair = new TStrPair;
                pStrPair->First = pSet->GetNextParamKey();
				pStrPair->Second = pSet->GetNextParamValue();

				aArchiverParams.Add(pStrPair);
			}
		}

		delete pSet;
	}

	// Apply default settings
	if (bDefault)
	{
		iMainWndX = CW_USEDEFAULT;
		iMainWndY = CW_USEDEFAULT;

		wMainWndWidth = 400;
		wMainWndHeight = 300;

		aColumnSize[0] = 30;
		aColumnSize[1] = 100;
		aColumnSize[2] = 140;
		aColumnSize[3] = 140;
		aColumnSize[4] = 160;
		aColumnSize[5] = 160;

		//g_pOptions->iPrgWndX = CW_USEDEFAULT;
		//g_pOptions->iPrgWndY = CW_USEDEFAULT;
		//wPrgWndWidth = 270;
		//wPrgWndHeight = 170;

		wMaxLogSize = 1024;

		bUseSound = true;
		bShowProgress = true;
		sSoundFile = TEXT("");
	}

	return true;
}

bool CSettings::SaveSettings()
{
    CMemIniOutW* pSet = new CMemIniOutW(cSettingsFile);

	pSet->AddString(L"File desc", L"Small Backup settings file");
	pSet->AddString(L"Version  ", L"1");
	pSet->WriteLine();

	pSet->AddSection(L"Main window");
	pSet->AddNumeric(L"X", iMainWndX);
	pSet->AddNumeric(L"Y", iMainWndY);
	pSet->AddNumeric(L"Width", wMainWndWidth);
	pSet->AddNumeric(L"Height", wMainWndHeight);
	pSet->WriteLine();

	pSet->AddNumeric(L"Col 1", aColumnSize[0]);
	pSet->AddNumeric(L"Col 2", aColumnSize[1]);
	pSet->AddNumeric(L"Col 3", aColumnSize[2]);
	pSet->AddNumeric(L"Col 4", aColumnSize[3]);
	pSet->AddNumeric(L"Col 5", aColumnSize[4]);
	pSet->AddNumeric(L"Col 6", aColumnSize[5]);
    pSet->WriteLine();

	pSet->AddSection(L"Log");
	pSet->AddNumeric(L"Max size", wMaxLogSize);

	pSet->AddSection(L"Notify on end task");
    pSet->AddBoolean(L"Use sound", bUseSound);
	pSet->AddNumeric(L"Show progress", bShowProgress);
	pSet->AddString(L"Sound file", sSoundFile.C());

	pSet->AddSection(L"GenName templates");
	for (int i = 0; i < aGenTemplates.Size(); i++)
	{
    	pSet->AddString(aGenTemplates[i]->C());
	}
	pSet->WriteLine();

	pSet->AddSection(L"Archiver");
	pSet->AddString(L"Program EXE", sArchiverEXE.C());
	pSet->AddString(L"Command line", sArchiverCmdLine.C());
	pSet->WriteLine();

	for (BYTE i = 0; i < aArchiverParams.Size(); i++)
	{
		TStrPair* pStrPair = (TStrPair*) aArchiverParams[i];

		pSet->AddString(pStrPair->First.C(), pStrPair->Second.C());		
	}

	pSet->SaveFile();
	delete pSet;

	return true;
}