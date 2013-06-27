#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "Instance.h"

#include "Settings.h"

class COptionsWnd{
private:
	HWND				m_hWindow;

	HWND				m_hLogSizeUpDown;

	PROPSHEETHEADER		m_PropSheetHdr;
	HWND				m_hGeneralSheet;
	HWND				m_hArchiveSheet;
	HWND				m_hPresets;

	bool				m_bLockEdits;	// не обрабатывать сообщения о изменении содержимого edit'ов

	CList<TPStrList*>	m_ArchivePresets;	// конфигурации разных архиваторов
	BYTE				m_bSelPreset;	// выбранный пресет

	static BOOL CALLBACK StaticGeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK StaticArchivesSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static BOOL CALLBACK StaticLoadPresetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	BOOL GeneralSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL ArchivesSheetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	BOOL LoadPresetDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Обработчики событий
	bool GeneralOnInit();
	bool GeneralOnChooseSound();
	bool GeneralOnPlayFile();
	bool GeneralOnStopFile();
	bool GeneralOnExit();

	bool ArchOnInit();
	bool ArchOnLoadPreset();
	bool ArchOnChooseArchiver();
	bool ArchOnAddParam();
	bool ArchOnDelParam();
	void ArchOnDelItem(DELETEITEMSTRUCT* pdis);
	bool ArchOnParamSel();
	void ArchOnEditChange();
	bool ArchOnExit();

	void PresetsOnInit();
	void PresetsOnExit();

public:
	COptionsWnd(HWND hParent);
	~COptionsWnd();

	bool Show();
};

// Глобальная переменная - класс текущего окна настроек
extern COptionsWnd* CurOptionsWnd;

#endif