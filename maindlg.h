#pragma once

#include "searchpattern.h"
#include "maintypes.h"

class CMainDlg : public CModifiedDialogImpl<CMainDlg>,
	public WTL::CDialogResize<CMainDlg>, public CInvokeImpl<CMainDlg>,
	private WTL::CMessageFilter 
{
	enum { IDC_STATUS_BAR = 1100 + 0 };
	enum { COLUMN_INDEX_NAME, COLUMN_INDEX_PATH, COLUMN_INDEX_SIZE, COLUMN_INDEX_SIZE_ON_DISK, COLUMN_INDEX_CREATION_TIME, COLUMN_INDEX_MODIFICATION_TIME, COLUMN_INDEX_ACCESS_TIME };
	static unsigned int const WM_TASKBARCREATED;
	enum { WM_NOTIFYICON = WM_USER + 100, WM_DRIVESETITEMDATA = WM_USER + 101 };

	static unsigned int CALLBACK iocp_worker(void *iocp);

	size_t num_threads;
	CSearchPattern txtPattern;
	WTL::CButton btnOK;
	WTL::CRichEditCtrl richEdit;
	WTL::CStatusBarCtrl statusbar;
	WTL::CAccelerator accel;
	std::map<std::tstring, CacheInfo> cache;
	std::tstring lastRequestedIcon;
	HANDLE hRichEdit;
	Results results;
	WTL::CImageList imgListSmall, imgListLarge, imgListExtraLarge;
	WTL::CComboBox cmbDrive;
	int indices_created;
	CThemedListViewCtrl lvFiles;
	WTL::CMenu menu;
	boost::shared_ptr<BackgroundWorker> iconLoader;
	Handle closing_event;
	Handle iocp;
	Threads threads;
	std::locale loc;
	HANDLE hWait, hEvent;
	CoInit coinit;

private:
	void setupUI();
	void setupDriverComboList();
	void setupFileList();
	void setupStatusbar();

public:
	CMainDlg(HANDLE const hEvent) : num_threads(static_cast<size_t>(get_num_threads())),
		indices_created(), closing_event(CreateEvent(NULL, TRUE, FALSE, NULL)),
		iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)), threads(),
		loc(get_numpunct_locale(std::locale(""))), hWait(), hEvent(hEvent),
		iconLoader(BackgroundWorker::create(true)), lastRequestedIcon(),
		hRichEdit(LoadLibrary(_T("riched20.dll")))
	{
	}

	void OnDestroy();

	
	int CacheIcon(std::tstring path, int const iItem, ULONG fileAttributes, bool lifo);

	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return this->lvFiles.SendMessage(uMsg, wParam, lParam);
	}

	static VOID CALLBACK WaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
	{
		HWND const hWnd = reinterpret_cast<HWND>(lpParameter);
		if (!TimerOrWaitFired) {
			WINDOWPLACEMENT placement = { sizeof(placement) };
			if (::GetWindowPlacement(hWnd, &placement)) {
				::ShowWindowAsync(hWnd, ::IsZoomed(hWnd)
					|| (placement.flags & WPF_RESTORETOMAXIMIZED) != 0 ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
			}
		}
	}

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);

	LRESULT OnFilesListColumnClick(LPNMHDR pnm);

	void Search();

	void OnBrowse(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/);

	void OnOK(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/);

	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void RightClick(std::vector<int> const &indices, POINT const &point, int const focused);

	void DoubleClick(int index);

	LRESULT OnFilesDoubleClick(LPNMHDR pnmh);

	LRESULT OnFileNameArrowKey(LPNMHDR pnmh);

	LRESULT OnFilesKeyDown(LPNMHDR pnmh);

	std::tstring GetSubItemText(Results::const_iterator const result, int const subitem,
		std::tstring &path) const;

		LRESULT OnFilesGetDispInfo(LPNMHDR pnmh);

	void OnCancel(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (this->CheckAndCreateIcon(false)) {
			this->ShowWindow(SW_HIDE);
		}
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (this->accel) {
			if (this->accel.TranslateAccelerator(this->m_hWnd, pMsg)) {
				return TRUE;
			}
		}

		return this->CWindow::IsDialogMessage(pMsg);
	}

	LRESULT OnFilesListCustomDraw(LPNMHDR pnmh);

	void OnClose(UINT /*uNotifyCode*/ = 0, int nID = IDCANCEL, HWND /*hWnd*/ = NULL);

	LRESULT OnDeviceChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam);


	void OnWindowPosChanged(LPWINDOWPOS lpWndPos);

	void DeleteNotifyIcon()
	{
		NOTIFYICONDATA nid = { sizeof(nid), *this, 0 };
		Shell_NotifyIcon(NIM_DELETE, &nid);
		SetPriorityClass(GetCurrentProcess(), 0x200000 /*PROCESS_MODE_BACKGROUND_END*/);
	}

	BOOL CheckAndCreateIcon(bool checkVisible);

	LRESULT OnTaskbarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/);

	LRESULT OnDriveSetItemData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam);

	LRESULT OnNotifyIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam);

	void OnHelpAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	void OnHelpRegex(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	void OnViewLargeIcons(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	void OnViewGridlines(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	void OnViewFitColumns(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	void OnRefresh(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/);

	struct IconLoaderCallback {
		CMainDlg *this_;
		std::tstring path;
		SIZE iconSmallSize, iconLargeSize;
		unsigned long fileAttributes;
		int iItem;

		struct MainThreadCallback {
			CMainDlg *this_;
			std::tstring description, path;
			WTL::CIcon iconSmall, iconLarge;
			int iItem;
			TCHAR szTypeName[80];
			bool operator()()
			{
				WTL::CWaitCursor wait(true, IDC_APPSTARTING);
				CacheInfo &cached = this_->cache[path];
				_tcscpy(cached.szTypeName, this->szTypeName);
				cached.description = description;

				if (cached.iIconSmall < 0) {
					cached.iIconSmall = this_->imgListSmall.AddIcon(iconSmall);
				}
				else {
					this_->imgListSmall.ReplaceIcon(cached.iIconSmall, iconSmall);
				}

				if (cached.iIconLarge < 0) {
					cached.iIconLarge = this_->imgListLarge.AddIcon(iconLarge);
				}
				else {
					this_->imgListLarge.ReplaceIcon(cached.iIconLarge, iconLarge);
				}

				cached.valid = true;

				this_->lvFiles.RedrawItems(iItem, iItem);
				return true;
			}
		};

		BOOL operator()()
		{
			RECT rcItem = { LVIR_BOUNDS };
			RECT rcFiles, intersection;
			this_->lvFiles.GetClientRect(&rcFiles);  // Blocks, but should be fast
			this_->lvFiles.GetItemRect(iItem, &rcItem,
				LVIR_BOUNDS);  // Blocks, but I'm hoping it's fast...
			if (IntersectRect(&intersection, &rcFiles, &rcItem)) {
				std::tstring const normalizedPath = NormalizePath(path);
				SHFILEINFO shfi = { 0 };
				std::tstring description;
#if 0
				{
					std::vector<BYTE> buffer;
					DWORD temp;
					buffer.resize(GetFileVersionInfoSize(normalizedPath.c_str(), &temp));
					if (GetFileVersionInfo(normalizedPath.c_str(), NULL, static_cast<DWORD>(buffer.size()),
						buffer.empty() ? NULL : &buffer[0])) {
						LPVOID p;
						UINT uLen;
						if (VerQueryValue(buffer.empty() ? NULL : &buffer[0],
							_T("\\StringFileInfo\\040904E4\\FileDescription"), &p, &uLen)) {
							description = std::tstring((LPCTSTR)p, uLen);
						}
					}
				}
#endif
				Handle fileTemp;  // To prevent icon retrieval from changing the file time
				{
					std::tstring ntpath = _T("\\??\\") + path;
					winnt::UNICODE_STRING us = { static_cast<unsigned short>(ntpath.size() * sizeof(*ntpath.begin())), static_cast<unsigned short>(ntpath.size() * sizeof(*ntpath.begin())), ntpath.empty() ? NULL : &*ntpath.begin() };
					winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), NULL, &us };
					winnt::IO_STATUS_BLOCK iosb;
					if (winnt::NtOpenFile(&fileTemp.value, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, &oa,
						&iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						0x00200000 /* FILE_OPEN_REPARSE_POINT */ | 0x00000008 /*FILE_NO_INTERMEDIATE_BUFFERING*/)
						== 0) {
						FILETIME preserve = { ULONG_MAX, ULONG_MAX };
						SetFileTime(fileTemp, NULL, &preserve, NULL);
					}
				}
				BOOL success = FALSE;
				SetLastError(0);
				WTL::CIcon iconSmall, iconLarge;
				for (int pass = 0; pass < 2; ++pass) {
					WTL::CSize const size = pass ? iconLargeSize : iconSmallSize;
					ULONG const flags = SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_ADDOVERLAYS
						| //SHGFI_TYPENAME | SHGFI_SYSICONINDEX |
						(pass ? SHGFI_LARGEICON : SHGFI_SMALLICON);
					// CoInit com;  // MANDATORY!  Some files, like '.sln' files, won't work without it!
					success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi),
						flags) != 0;
					if (!success && (flags & SHGFI_USEFILEATTRIBUTES) == 0) {
						success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi),
							flags | SHGFI_USEFILEATTRIBUTES) != 0;
					}
					(pass ? iconLarge : iconSmall).Attach(shfi.hIcon);
				}

				if (success) {
					std::tstring const path(path);
					int const iItem(iItem);
					MainThreadCallback callback = { this_, description, path, iconSmall.Detach(), iconLarge.Detach(), iItem };
					_tcscpy(callback.szTypeName, shfi.szTypeName);
					this_->InvokeAsync(callback);
					callback.iconLarge.Detach();
					callback.iconSmall.Detach();
				}
				else {
					_ftprintf(stderr, _T("Could not get the icon for file: %s\n"), normalizedPath.c_str());
				}
			}
			return true;
		}
	};


	BEGIN_MSG_MAP_EX(CMainDlg)
		CHAIN_MSG_MAP(CInvokeImpl<CMainDlg>)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_WINDOWPOSCHANGED(OnWindowPosChanged)
		MSG_WM_CLOSE(OnClose)
		MESSAGE_HANDLER_EX(WM_DEVICECHANGE,
		OnDeviceChange)  // Don't use MSG_WM_DEVICECHANGE(); it's broken (uses DWORD)
		MESSAGE_HANDLER_EX(WM_NOTIFYICON, OnNotifyIcon)
		MESSAGE_HANDLER_EX(WM_DRIVESETITEMDATA, OnDriveSetItemData)
		MESSAGE_HANDLER_EX(WM_TASKBARCREATED, OnTaskbarCreated)
		MESSAGE_HANDLER_EX(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER_EX(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnHelpAbout)
		COMMAND_ID_HANDLER_EX(ID_HELP_USINGREGULAREXPRESSIONS, OnHelpRegex)
		COMMAND_ID_HANDLER_EX(ID_VIEW_GRIDLINES, OnViewGridlines)
		COMMAND_ID_HANDLER_EX(ID_VIEW_LARGEICONS, OnViewLargeIcons)
		COMMAND_ID_HANDLER_EX(ID_VIEW_FITCOLUMNSTOWINDOW, OnViewFitColumns)
		COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnClose)
		COMMAND_ID_HANDLER_EX(ID_ACCELERATOR40006, OnRefresh)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
		COMMAND_HANDLER_EX(IDC_BUTTON_BROWSE, BN_CLICKED, OnBrowse)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_CUSTOMDRAW, OnFilesListCustomDraw)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_GETDISPINFO, OnFilesGetDispInfo)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_COLUMNCLICK, OnFilesListColumnClick)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_DBLCLK, OnFilesDoubleClick)
		NOTIFY_HANDLER_EX(IDC_EDITFILENAME, CSearchPattern::CUN_KEYDOWN, OnFileNameArrowKey)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_KEYDOWN, OnFilesKeyDown)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_LISTFILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_EDITFILENAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_STATUS_BAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
		DLGRESIZE_CONTROL(IDC_BUTTON_BROWSE, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()
	enum { IDD = IDD_DIALOG1 };
};

