#include "stdafx.h"
#include "targetver.h"

#include "GUI.hpp"

#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include <boost/bind/bind.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/algorithm/equal.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/range/algorithm/for_each.hpp>
#include <boost/range/algorithm/reverse.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#if 0
#include <boost/xpressive/detail/dynamic/matchable.hpp>
#define clear() resize(0)
#define push_back(x) operator +=(x)
#include <boost/xpressive/detail/dynamic/parser_traits.hpp>
#undef  push_back
#undef clear
#include <boost/xpressive/match_results.hpp>
#include <boost/xpressive/xpressive_dynamic.hpp>
#endif

#include <Windows.h>
#include <WinUser.h>
#include <ComDef.h>
#include <ShellApi.h>
#include <Shlwapi.h>
#include <ShlObj.h>
#include <ShlGuid.h>
#include <CommCtrl.h>
#include <ntddscsi.h>
#include <WinIoCtl.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlmisc.h>
extern ATL::CComModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atltheme.h>

#include "resource.h"
#include "BackgroundWorker.hpp"
#include "CModifiedDialogImpl.hpp"
#include "nformat.hpp"
#include "NtObjectMini.hpp"
#include "path.hpp"

#include "NtfsIndex.hpp"
#include "NtfsIndexThread.hpp"
#include "relative_iterator.hpp"
#include "ShellItemIDList.hpp"

inline TCHAR totupper(TCHAR c) { return c < SCHAR_MAX ? (_T('A') <= c && c <= _T('Z')) ? c + (_T('a') - _T('A')) : c : _totupper(c); }
#ifdef _totupper
#undef _totupper
#endif
#define _totupper totupper

EXTERN_C NTSYSAPI VOID NTAPI RtlSecondsSince1980ToTime(IN ULONG ElapsedSeconds, OUT PLARGE_INTEGER Time);
EXTERN_C NTSYSAPI NTSTATUS NTAPI RtlSystemTimeToLocalTime(IN LARGE_INTEGER const *SystemTime, OUT PLARGE_INTEGER LocalTime);

LONGLONG RtlSecondsSince1980ToTime(ULONG time)
{
	LARGE_INTEGER time2 = {0};
	RtlSecondsSince1980ToTime(time, &time2);
	return time2.QuadPart;
}

struct DataRow
{
	NtfsIndex const *pIndex;
	size_t i;
	DataRow(NtfsIndex const *pIndex = NULL, size_t const i = static_cast<size_t>(-1)) : pIndex(pIndex), i(i) { }
	NtfsIndex::CombinedRecord const &record() const { return this->pIndex->at(this->i); }
	std::basic_string<TCHAR> name() const
	{
		boost::iterator_range<TCHAR const *> const name = this->pIndex->get_name(this->record().second.second.first.first);
		return std::basic_string<TCHAR>(name.begin(), name.end());
	}
	unsigned long parent() const { return this->record().second.second.first.second; }
	long long creationTime() const { return RtlSecondsSince1980ToTime(this->record().second.first.first.first); }
	long long modificationTime() const { return RtlSecondsSince1980ToTime(this->record().second.first.first.second); }
	long long accessTime() const { return RtlSecondsSince1980ToTime(this->record().second.first.second.first); }
	unsigned long attributes() const { return this->record().second.first.second.second; }
	long long size() const { return this->record().second.second.second.first; }
	long long sizeOnDisk() const { return this->record().second.second.second.second; }
};

struct Wow64Disable
{
	void *cookie;
	BOOL success;
	Wow64Disable() : cookie(NULL), success(FALSE)
	{
		typedef BOOL (WINAPI *PWow64DisableWow64FsRedirection)(OUT PVOID *OldValue);
		HMODULE hKernel32 = NULL;
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)&GetSystemInfo, &hKernel32))
		{ reinterpret_cast<PWow64DisableWow64FsRedirection>(GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection"))(&this->cookie); }
	}
	~Wow64Disable()
	{
		typedef BOOL (WINAPI *PWow64RevertWow64FsRedirection)(IN PVOID OldValue);
		HMODULE hKernel32 = NULL;
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)&GetSystemInfo, &hKernel32))
		{ reinterpret_cast<PWow64RevertWow64FsRedirection>(GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection"))(&this->cookie); }
	}
};

template<class It1, class It2, class Tr>
bool wildcard(It1 patBegin, It1 const patEnd, It2 strBegin, It2 const strEnd, Tr const &tr = std::char_traits<typename std::iterator_traits<It2>::value_type>())
{
	(void)tr;
	if (patBegin == patEnd) { throw std::domain_error("Invalid string!"); }
	if (strBegin == strEnd) { throw std::domain_error("Invalid string!"); }
	//http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
	It2 s(strBegin);
	It1 p(patBegin);
	bool star = false;

loopStart:
	for (s = strBegin, p = patBegin; s != strEnd; ++s, ++p)
	{
		if (tr.eq(*p, _T('*')))
		{
			star = true;
			strBegin = s, patBegin = p;
			if (++patBegin == patEnd)
			{ return true; }
			goto loopStart;
		}
		else if (tr.eq(*p, _T('?')))
		{
			if (tr.eq(*s, _T('.')))
			{ goto starCheck; }
		}
		else
		{
			if (!tr.eq(*s, *p))
			{ goto starCheck; }
		}
	}
	if (p != patEnd && tr.eq(*p, _T('*'))) { ++p; }
	return p == patEnd;

starCheck:
	if (!star) { return false; }
	strBegin++;
	goto loopStart;
}


struct tchar_ci_traits : public std::char_traits<TCHAR>
{
	static bool eq(TCHAR c1, TCHAR c2) { return _totupper(c1) == _totupper(c2); }
	static bool ne(TCHAR c1, TCHAR c2) { return _totupper(c1) != _totupper(c2); }
	static bool lt(TCHAR c1, TCHAR c2) { return _totupper(c1) <  _totupper(c2); }
	static int compare(TCHAR const *s1, TCHAR const *s2, size_t n)
	{
		while (n-- != 0)
		{
			if (lt(*s1, *s2)) { return -1; }
			if (lt(*s2, *s1)) { return +1; }
			++s1; ++s2;
		}
		return 0;
	}
	static TCHAR const *find(TCHAR const *s, size_t n, TCHAR a)
	{
		while (n > 0 && ne(*s, a)) { ++s; --n; }
		return s;
	}
};

LONGLONG RtlSystemTimeToLocalTime(LONGLONG systemTime)
{
	LARGE_INTEGER time2, localTime;
	time2.QuadPart = systemTime;
	NTSTATUS status = RtlSystemTimeToLocalTime(&time2, &localTime);
	if (status != 0) { RaiseException(status, 0, 0, NULL); }
	return localTime.QuadPart;
}

LPCTSTR SystemTimeToString(LONGLONG systemTime, LPTSTR buffer, size_t cchBuffer)
{
	LONGLONG time = RtlSystemTimeToLocalTime(systemTime);
	SYSTEMTIME sysTime = {0};
	if (FileTimeToSystemTime(&reinterpret_cast<FILETIME &>(time), &sysTime))
	{
		size_t const cchDate = int_cast<size_t>(GetDateFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, &buffer[0], int_cast<int>(cchBuffer)));
		if (cchDate > 0)
		{
			// cchDate INCLUDES null-terminator
			buffer[cchDate - 1] = _T(' ');
			size_t const cchTime = int_cast<size_t>(GetTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, &buffer[cchDate], int_cast<int>(cchBuffer - cchDate)));
			buffer[cchDate + cchTime - 1] = _T('\0');
		}
		else { memset(&buffer[0], 0, sizeof(buffer[0]) * cchBuffer); }
	}
	else { *buffer = _T('\0'); }
	return buffer;
}

template<typename TWindow>
class CUpDownNotify : public ATL::CWindowImpl<CUpDownNotify<TWindow>, TWindow>
{
	BEGIN_MSG_MAP_EX(CCustomDialogCode)
		MESSAGE_RANGE_HANDLER_EX(WM_KEYDOWN, WM_KEYUP, OnKey)
	END_MSG_MAP()

public:
	struct KeyNotify { NMHDR hdr; TCHAR vkey; };
	enum { CUN_KEYDOWN, CUN_KEYUP };

	LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if (wParam == VK_UP || wParam == VK_DOWN)
		{
			int id = this->GetWindowLong(GWL_ID);
			KeyNotify msg = { { *this, id, uMsg == WM_KEYUP ? CUN_KEYUP : CUN_KEYDOWN }, (TCHAR)wParam };
			HWND hWndParent = this->GetParent();
			return hWndParent == NULL || this->SendMessage(hWndParent, WM_NOTIFY, id, (LPARAM)&msg) ? this->DefWindowProc(uMsg, wParam, lParam) : 0;
		}
		else { return this->DefWindowProc(uMsg, wParam, lParam); }
	}
};

std::basic_string<TCHAR> GetDisplayName(HWND hWnd, const std::basic_string<TCHAR> &path, DWORD shgdn)
{
	ATL::CComPtr<IShellFolder> desktop;
	STRRET ret;
	LPITEMIDLIST shidl;
	ATL::CComBSTR bstr;
	ULONG attrs = SFGAO_ISSLOW | SFGAO_HIDDEN;
	std::basic_string<TCHAR> result = (
		SHGetDesktopFolder(&desktop) == S_OK
		&& desktop->ParseDisplayName(hWnd, NULL, path.empty() ? NULL : const_cast<LPWSTR>(path.c_str()), NULL, &shidl, &attrs) == S_OK
		&& (attrs & SFGAO_ISSLOW) == 0
		&& desktop->GetDisplayNameOf(shidl, shgdn, &ret) == S_OK
		&& StrRetToBSTR(&ret, shidl, &bstr) == S_OK
		) ? static_cast<LPCTSTR>(bstr) : std::basic_string<TCHAR>(basename(path.begin(), path.end()), path.end());
	return result;
}

std::basic_string<TCHAR> NormalizePath(std::basic_string<TCHAR> const &path)
{
	std::basic_string<TCHAR> result;
	bool wasSep = false;
	bool isCurrentlyOnPrefix = true;
	for (size_t i = 0; i < path.size(); i++)
	{
		TCHAR const &c = path[i];
		isCurrentlyOnPrefix &= isdirsep(c);
		if (isCurrentlyOnPrefix || !wasSep || !isdirsep(c)) { result += c; }
		wasSep = isdirsep(c);
	}
	if (!isrooted(result.begin(), result.end()))
	{
		std::basic_string<TCHAR> currentDir(32 * 1024, _T('\0'));
		currentDir.resize(GetCurrentDirectory(static_cast<DWORD>(currentDir.size()), &currentDir[0]));
		adddirsep(currentDir);
		result = currentDir + result;
	}
	return result;
}

class CProgressDialog : private CModifiedDialogImpl<CProgressDialog>, private WTL::CDialogResize<CProgressDialog>
{
	typedef CProgressDialog This;
	typedef CModifiedDialogImpl<CProgressDialog> Base;
	friend CDialogResize<This>;
	friend CDialogImpl<This>;
	friend CModifiedDialogImpl<This>;
	enum { IDD = IDD_DIALOGPROGRESS };
	class CUnselectableWindow : public ATL::CWindowImpl<CUnselectableWindow>
	{
		BEGIN_MSG_MAP(CUnselectableWindow)
			MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
		END_MSG_MAP()
		LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
		{
			LRESULT result = this->DefWindowProc(uMsg, wParam, lParam);
			return result == HTCLIENT ? HTTRANSPARENT : result;
		}
	};

	CUnselectableWindow progressText;
	WTL::CProgressBarCtrl progressBar;
	bool canceled;
	bool invalidated;
	DWORD shownTime;
	DWORD creationTime;
	DWORD lastUpdateTime;
	HWND parent;
	std::basic_string<TCHAR> lastProgressText, lastProgressTitle;
	int lastProgress, lastProgressTotal;

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		(this->progressText.SubclassWindow)(this->GetDlgItem(IDC_RICHEDITPROGRESS));
		this->progressText.SendMessage(EM_SETBKGNDCOLOR, FALSE, GetSysColor(COLOR_3DFACE));

		this->progressBar.Attach(this->GetDlgItem(IDC_PROGRESS1));
		this->DlgResize_Init(false, false, 0);

		this->creationTime = GetTickCount();
		return TRUE;
	}

	void OnPause(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		UNREFERENCED_PARAMETER((uNotifyCode, nID, wndCtl));
		__debugbreak();
	}

	void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		UNREFERENCED_PARAMETER((uNotifyCode, nID, wndCtl));
		PostQuitMessage(ERROR_CANCELLED);
	}

	BEGIN_MSG_MAP_EX(This)
		CHAIN_MSG_MAP(CDialogResize<This>)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDRETRY, BN_CLICKED, OnPause)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(This)
		DLGRESIZE_CONTROL(IDC_PROGRESS1, DLSZ_MOVE_Y | DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_RICHEDITPROGRESS, DLSZ_SIZE_Y | DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
	END_DLGRESIZE_MAP()

	static BOOL EnableWindowRecursive(HWND hWnd, BOOL enable, BOOL includeSelf = true)
	{
		struct Callback
		{
			static BOOL CALLBACK EnableWindowRecursiveEnumProc(HWND hWnd, LPARAM lParam)
			{
				EnableWindowRecursive(hWnd, static_cast<BOOL>(lParam), TRUE);
				return TRUE;
			}
		};
		if (enable)
		{
			EnumChildWindows(hWnd, &Callback::EnableWindowRecursiveEnumProc, enable);
			return includeSelf && ::EnableWindow(hWnd, enable);
		}
		else
		{
			BOOL result = includeSelf && ::EnableWindow(hWnd, enable);
			EnumChildWindows(hWnd, &Callback::EnableWindowRecursiveEnumProc, enable);
			return result;
		}
	}

	static bool WaitWithMessageLoop(HANDLE const handle, DWORD timeout = INFINITE)
	{
		for (;;)
		{
			MSG msg;
			DWORD result = MsgWaitForMultipleObjectsEx(handle == NULL ? 0 : 1, handle == NULL ? NULL : &handle, timeout, QS_INPUT | QS_PAINT | QS_SENDMESSAGE, MWMO_INPUTAVAILABLE);
			if (result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
			{ return result != WAIT_TIMEOUT; }
			else if (result == WAIT_OBJECT_0 + 1)
			{
				if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_QS_INPUT | PM_QS_PAINT | PM_QS_SENDMESSAGE))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			else { RaiseException(GetLastError(), 0, 0, NULL); }
		}
	}

	DWORD GetMinDelay() const
	{
		LONG delay = 0;
		SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &delay, 0);
		using std::max;
		delay = max(delay, IsDebuggerPresent() ? 0 : 750);
		return delay;
	}

public:
	enum { UPDATE_INTERVAL = 20 };
	CProgressDialog(ATL::CWindow parent)
		: Base(true), parent(parent), shownTime(0 /*NOT GetTickCount()!*/), lastUpdateTime(GetTickCount()), creationTime(GetTickCount()), lastProgress(0), lastProgressTotal(1), invalidated(false), canceled(false)
	{
	}

	~CProgressDialog()
	{
		if (false)
		{
			DWORD now = GetTickCount(), minDelay = this->GetMinDelay();
			if (now - this->shownTime < minDelay)
			{
				WaitWithMessageLoop(NULL, minDelay - (now - this->shownTime));
			}
		}
		if (this->shownTime)
		{
			EnableWindowRecursive(parent, TRUE);
		}
		if (this->IsWindow())
		{
			this->DestroyWindow();
		}
	}

	bool ShouldUpdate()
	{
		unsigned long const tickCount = GetTickCount();
		return tickCount - this->lastUpdateTime >= UPDATE_INTERVAL;
	}

	bool HasUserCancelled()
	{
		if (this->ShouldUpdate())
		{
			if (this->shownTime == 0 && abs(static_cast<int>(GetTickCount() - this->creationTime)) >= static_cast<int>(this->GetMinDelay()))
			{
				this->Create(parent);
				this->ShowWindow(SW_SHOW);
				this->shownTime = GetTickCount();
				EnableWindowRecursive(parent, FALSE);
			}
			if (this->invalidated)
			{ this->Flush(); }
			if (this->IsWindow())
			{
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (!this->IsDialogMessage(&msg))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
					if (msg.message == WM_QUIT)
					{ this->canceled = true; }
				}
			}
		}
		return this->canceled;
	}

	void Flush()
	{
		if (this->IsWindow())
		{
			this->SetWindowText(this->lastProgressTitle.c_str());
			this->progressBar.SetRange32(0, this->lastProgressTotal);
			this->progressBar.SetPos(this->lastProgress);
			this->progressText.SetWindowText(this->lastProgressText.c_str());
		}
		this->invalidated = false;
		this->SetUpdated();
	}

	void SetUpdated()
	{
		this->lastUpdateTime = GetTickCount();
	}

	void SetProgress(long long current, long long total)
	{
		if (total > INT_MAX)
		{
			current = static_cast<long long>((static_cast<double>(current) / total) * INT_MAX);
			total = INT_MAX;
		}
		bool const flush = current == total && this->lastProgress != this->lastProgressTotal;
		this->invalidated |= this->lastProgress != current || this->lastProgressTotal != total;
		this->lastProgressTotal = static_cast<int>(total);
		this->lastProgress = static_cast<int>(current);

		if (flush) { this->Flush(); }
	}

	void SetProgressTitle(LPCTSTR title)
	{
		this->invalidated |= this->lastProgressTitle != title;
		this->lastProgressTitle = title;
	}

	void SetProgressText(boost::iterator_range<TCHAR const *> const text)
	{
		this->invalidated |= !boost::equal(this->lastProgressText, text);
		this->lastProgressText.assign(text.begin(), text.end());
	}
};

template<class F>
struct CancellableComparator
{
	F f;
	CancellableComparator(F const &f) : f(f) { }
	template<class T1, class T2>
	bool operator()(T1 const &a, T2 const &b)
	{
		if (GetAsyncKeyState(VK_ESCAPE) < 0)
		{ throw CStructured_Exception(ERROR_CANCELLED, NULL); }
		return f(a, b);
	}
};

template<class F>
CancellableComparator<F> cancellable_comparator(F const &f) { return f; }

inline std::basic_string<TCHAR> vtformat(TCHAR const *format, va_list argptr)
{
	std::basic_string<TCHAR> s(_tcslen(format) * 2, _T('\0'));
	if (*format != _T('\0'))
	{
		for (;;)
		{
			int r = _vsntprintf(&*s.begin(), s.size(), format, argptr);
			if (r != -1)
			{
				s.resize(static_cast<size_t>(r));
				break;
			}
			s.resize(s.size() * 2);
		}
	}
	return s;
}


// WARNING:   O(n^2)
template<typename Str, typename S1, typename S2>
void replace_all(Str &str, S1 from, S2 to)
{
	size_t const nFrom = Str(from).size();  // SLOW but OK because this function is O(n^2) anyway
	size_t const nTo = Str(to).size();  // SLOW but OK because this function is O(n^2) anyway
	for (size_t i = 0; (i = str.find_first_of(from, i)) != Str::npos; i += nTo)
	{
		str.erase(i, nFrom);
		str.insert(i, to);
	}
}

inline std::basic_string<TCHAR> tformat(TCHAR const *format, ...) { va_list a; va_start(a, format); return vtformat(format, a); }

class iless
{
	std::ctype<TCHAR> const &ctype;
	struct iless_ch
	{
		iless const *p;
		iless_ch(iless const &l) : p(&l) { }
		bool operator()(TCHAR a, TCHAR b) const
		{
			return _totupper(a) < _totupper(b);
			//return p->ctype.toupper(a) < p->ctype.toupper(b);
		}
	};

	template<class T>
	static T const &use_facet(std::locale const &loc)
	{
		return std::
#if defined(_USEFAC)
			_USE(loc, T)
#else
			use_facet<T>(loc)
#endif
			;
	}

public:
	iless(std::locale const &loc) : ctype(static_cast<std::ctype<TCHAR> const &>(use_facet<std::ctype<TCHAR> >(loc))) { }
	bool operator()(std::basic_string<TCHAR> const &a, std::basic_string<TCHAR> const &b) const
	{
		return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
		//return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), iless_ch(*this));
	}
};
template<typename F, typename C> struct Comparator { F f; C c; Comparator(F f, C c) : f(f), c(c) { } template<class T> bool operator()(T const &a, T const &b) const { return c(f(a), f(b)); } };
template<typename F> struct Comparator<F, void> { F f; Comparator(F f) : f(f) { } template<class T> bool operator()(T const &a, T const &b) const { return f(a) < f(b); } };
template<typename F> Comparator<F, void> comparator(F f) { return Comparator<F, void>(f); }
template<typename C, typename F> Comparator<F, C> comparator(F f, C c) { return Comparator<F, C>(f, c); }

class CMainDlg : public WTL::CDialogResize<CMainDlg>, public CModifiedDialogImpl<CMainDlg>, public CInvokeImpl<CMainDlg>, public CMainDlgBase
{
	enum { IDC_STATUS_BAR = 1100 + 0 };
	enum { COLUMN_INDEX_NAME, COLUMN_INDEX_PATH, COLUMN_INDEX_SIZE, COLUMN_INDEX_SIZE_ON_DISK, COLUMN_INDEX_MODIFICATION_TIME, COLUMN_INDEX_CREATION_TIME, COLUMN_INDEX_ACCESS_TIME };
	struct CThemedListViewCtrl : public WTL::CListViewCtrl, public WTL::CThemeImpl<CThemedListViewCtrl> { using WTL::CListViewCtrl::Attach; };
	CThemedListViewCtrl lvFiles;
	WTL::CComboBox cmbDrive;
	WTL::CStatusBarCtrl statusbar;
	WTL::CProgressBarCtrl statusbarProgress;
	CUpDownNotify<WTL::CEdit> txtFileName;
	boost::ptr_vector<NtfsIndexThread> drives;
	class CoInit
	{
		CoInit(CoInit const &) : hr(S_FALSE) { }
	public:
		HRESULT hr;
		CoInit(bool initialize = true) : hr(initialize ? CoInitialize(NULL) : S_FALSE) { }
		~CoInit() { if (this->hr == S_OK) { CoUninitialize(); } }
	};
	struct CacheInfo
	{
		CacheInfo() : valid(false), iIconSmall(-1), iIconLarge(-1), iIconExtraLarge(-1) { this->szTypeName[0] = _T('\0'); }
		bool valid;
		int iIconSmall, iIconLarge, iIconExtraLarge;
		TCHAR szTypeName[80];
		std::basic_string<TCHAR> description, displayName;
	};
	WTL::CImageList imgListSmall, imgListLarge, imgListExtraLarge;
	WTL::CRichEditCtrl richEdit;
	std::map<std::basic_string<TCHAR>, CacheInfo> cache;
	INT lastSortColumn;  // -1 if unset
	INT lastSortIsDescending;  // -1 if unset
	boost::shared_ptr<BackgroundWorker> iconLoader;
	CoInit coInit;
	HMODULE hRichEdit;

	std::vector<DataRow> rows;

public:
	enum { IDD = IDD_DIALOGMAIN };
	CMainDlg() : CModifiedDialogImpl(true), lastSortIsDescending(-1), lastSortColumn(-1), iconLoader(BackgroundWorker::create(true)), hRichEdit(LoadLibrary(_T("riched20.dll"))) { }

	~CMainDlg()
	{
		FreeLibrary(this->hRichEdit);
	}

private:
	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		this->cmbDrive.Attach(this->GetDlgItem(IDC_COMBODRIVE));
		this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
		this->txtFileName.SubclassWindow(this->GetDlgItem(IDC_EDITFILENAME));
		this->richEdit.Create(this->lvFiles, NULL, 0, ES_MULTILINE, WS_EX_TRANSPARENT);
		this->richEdit.SetFont(this->lvFiles.GetFont());
		
		this->statusbar = CreateStatusWindow(WS_CHILD | SBT_TOOLTIPS, NULL, *this, IDC_STATUS_BAR);
		int const rcStatusPaneWidths[] = { 240, -1 };
		if ((this->statusbar.GetStyle() & WS_VISIBLE) != 0)
		{
			RECT rect; this->lvFiles.GetWindowRect(&rect);
			this->ScreenToClient(&rect);
			{
				RECT sbRect; this->statusbar.GetWindowRect(&sbRect);
				rect.bottom -= (sbRect.bottom - sbRect.top);
			}
			this->lvFiles.MoveWindow(&rect);
		}
		this->statusbar.SetParts(sizeof(rcStatusPaneWidths) / sizeof(*rcStatusPaneWidths), const_cast<int *>(rcStatusPaneWidths));
		this->statusbar.SetText(0, _T("Type in a file name and press Search."), 0);
		RECT rcStatusPane1; this->statusbar.GetRect(1, &rcStatusPane1);
		this->statusbarProgress.Create(this->statusbar, rcStatusPane1, NULL, WS_CHILD | PBS_SMOOTH);

		this->DlgResize_Init(true, false);

		{
			const int IMAGE_LIST_INCREMENT = 0x100;
			this->imgListSmall.Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON), ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
			this->imgListLarge.Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON), ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
			this->imgListExtraLarge.Create(48, 48, ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
		}

		{
			this->lvFiles.SetImageList(this->imgListLarge, LVSIL_SMALL);
			this->lvFiles.SetImageList(this->imgListLarge, LVSIL_NORMAL);
			this->lvFiles.SetImageList(this->imgListExtraLarge, LVSIL_NORMAL);
		}

		this->lvFiles.SetWindowTheme(L"Explorer", NULL);
		this->txtFileName.SetCueBannerText(_T("Enter the file name here"));

		std::basic_string<TCHAR> logicalDrives(GetLogicalDriveStrings(0, NULL) + 1, _T('\0'));
		logicalDrives.resize(GetLogicalDriveStrings(static_cast<DWORD>(logicalDrives.size()) - 1, &logicalDrives[0]));
		this->cmbDrive.SetCurSel(-1);
		int iSel = 0;
		TCHAR windowsDir[MAX_PATH] = {0};
		windowsDir[GetWindowsDirectory(windowsDir, sizeof(windowsDir) / sizeof(*windowsDir))] = _T('\0');
		for (LPCTSTR drive = logicalDrives.c_str(); *drive != _T('\0'); drive += _tcslen(drive) + 1)
		{
			//if (IsDriveReady(drive))
			{
				TCHAR fsName[MAX_PATH];
				if (GetVolumeInformation(drive, NULL, 0, NULL, 0, NULL, fsName, sizeof(fsName) / sizeof(*fsName)) && _tcsicmp(fsName, _T("NTFS")) == 0)
				{
					int index = this->cmbDrive.AddString(drive);
					if (drive[0] != _T('\0') && drive[0] == windowsDir[0] && drive[1] == _T(':'))
					{
						iSel = index;
					}
				}
			}
		}
		this->cmbDrive.SetCurSel(iSel);

		this->lvFiles.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | 0x80000000 /*LVS_EX_COLUMNOVERFLOW*/ /*| 0x10000000 LVS_EX_AUTOSIZECOLUMNS*/);
		this->lvFiles.InsertColumn(COLUMN_INDEX_NAME, _T("Name"), LVCFMT_LEFT, 170);
		this->lvFiles.InsertColumn(COLUMN_INDEX_PATH, _T("Directory"), LVCFMT_LEFT, 468);
		this->lvFiles.InsertColumn(COLUMN_INDEX_SIZE, _T("Size (bytes)"), LVCFMT_RIGHT, 96);
		this->lvFiles.InsertColumn(COLUMN_INDEX_SIZE_ON_DISK, _T("Size on Disk"), LVCFMT_RIGHT, 96);
		this->lvFiles.InsertColumn(COLUMN_INDEX_MODIFICATION_TIME, _T("Modified"), LVCFMT_LEFT, 80);
		this->lvFiles.InsertColumn(COLUMN_INDEX_CREATION_TIME, _T("Created"), LVCFMT_LEFT, 80);
		this->lvFiles.InsertColumn(COLUMN_INDEX_ACCESS_TIME, _T("Accessed"), LVCFMT_LEFT, 80);

		HINSTANCE hInstance = GetModuleHandle(NULL);
		this->SetIcon((HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICONMAIN), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0), FALSE);
		this->SetIcon((HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICONMAIN), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0), TRUE);

		{
			typedef DWORD (WINAPI *PGetCurrentProcessorNumber)(VOID);
			HMODULE hModule = NULL;
			GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(&GetProcessAffinityMask), &hModule);
			PGetCurrentProcessorNumber const GetCurrentProcessorNumber = reinterpret_cast<PGetCurrentProcessorNumber>(GetProcAddress(hModule, "GetCurrentProcessorNumber"));
			if (LOBYTE(LOWORD(GetVersion())) < 6)
			{
				SetProcessAffinityMask(GetCurrentProcess(), 1 << (GetCurrentProcessorNumber != NULL ? GetCurrentProcessorNumber() : 0));
			}
		}
		for (LPCTSTR drive = logicalDrives.c_str(); *drive != _T('\0'); drive += _tcslen(drive) + 1)
		{
			this->drives.push_back(std::auto_ptr<NtfsIndexThread>(NtfsIndexThread::create(drive)));
		}

		this->OnSearchParamsChange(CBN_SELCHANGE, IDC_COMBODRIVE, this->cmbDrive);
		return TRUE;
	}

	void OnDestroy()
	{
		WTL::CWaitCursor wait;
		this->iconLoader->clear();
		this->drives.clear();
	}

	LRESULT OnFileNameArrowKey(LPNMHDR pnmh)
	{
		int i = this->cmbDrive.GetCurSel() + (((CUpDownNotify<WTL::CEdit>::KeyNotify *)pnmh)->vkey == VK_DOWN ? 1 : -1);
		if (i >= this->cmbDrive.GetCount()) { i = this->cmbDrive.GetCount() - 1; }
		if (i < 0) { i = 0; }
		this->cmbDrive.SetCurSel(i);
		this->OnSearchParamsChange(CBN_SELCHANGE, IDC_COMBODRIVE, this->cmbDrive);
		return 0;
	}

	static DWORD WINAPI SHOpenFolderAndSelectItemsThread(IN LPVOID lpParameter)
	{
		CShellItemIDList pItemIdList((LPITEMIDLIST)lpParameter);
		// This is in a separate thread because of a BUG:
		// Try this with RmMetadata:
		// 1. Double-click it.
		// 2. Press OK when the error comes up.
		// 3. Now you can't access the main window, because SHOpenFolderAndSelectItems() hasn't returned!
		// So we put this in a separate thread to solve that problem.

		CoInit coInit;
		return SHOpenFolderAndSelectItems(pItemIdList, 0, NULL, 0);
	}

	LRESULT OnFilesRightClick(LPNMHDR pnmh)
	{
		int index;
		WTL::CWaitCursor wait;
		Wow64Disable wow64Disabled;
		LPNMITEMACTIVATE pnmItem = (LPNMITEMACTIVATE)pnmh;
		index = this->lvFiles.HitTest(pnmItem->ptAction, 0);
		if (index >= 0)
		{
			std::basic_string<TCHAR> path = GetSubItemText(this->lvFiles, index, COLUMN_INDEX_PATH);
			path += GetSubItemText(this->lvFiles, index, COLUMN_INDEX_NAME);
			path.erase(std::find(basename(path.begin(), path.end()), path.end(), _T(':')), path.end());
			CShellItemIDList pItemIdList;
			SFGAOF sfgao;
			HRESULT hr = SHParseDisplayName(path.c_str(), NULL, &pItemIdList.m_pidl, 0, &sfgao);
			if (hr == S_OK)
			{
				CoInit const coInit;
				ATL::CComPtr<IShellFolder> folder;
				LPCITEMIDLIST lastItemIdList;
				if (SHBindToParent(pItemIdList, IID_IShellFolder, &reinterpret_cast<void *&>(folder), &lastItemIdList) == S_OK)
				{
					ATL::CComPtr<IContextMenu> contextMenu;
					if (folder->GetUIObjectOf(*this, 1, &lastItemIdList, IID_IContextMenu, NULL, &reinterpret_cast<void *&>(contextMenu.p)) == S_OK)
					{
						WTL::CMenu menu;
						menu.CreatePopupMenu();
						UINT const minID = 1000;
						hr = contextMenu->QueryContextMenu(menu, 0, minID, UINT_MAX, 0x80 /*CMF_ITEMMENU*/);
						if (SUCCEEDED(hr))
						{
							{
								MENUITEMINFO mii = { sizeof(mii), 0, MFT_MENUBREAK };
								menu.InsertMenuItem(0, TRUE, &mii);
							}
							{
								MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_STRING, MFT_STRING, MFS_DEFAULT | MFS_HILITE, minID - 1, NULL, NULL, NULL, NULL, _T("Open &Containing Folder") };
								menu.InsertMenuItem(0, TRUE, &mii);
								menu.SetMenuDefaultItem(0, TRUE);
							}
							WTL::CPoint cursorPos;
							GetCursorPos(&cursorPos);
							BOOL id = menu.TrackPopupMenu(
								TPM_RETURNCMD | TPM_NONOTIFY | (GetKeyState(VK_SHIFT) < 0 ? CMF_EXTENDEDVERBS : 0) |
								(GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION : TPM_LEFTALIGN | TPM_HORPOSANIMATION),
								cursorPos.x, cursorPos.y, *this);
							if (!id)
							{
								// User cancelled
							}
							else if (id == minID - 1)
							{
								if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, pItemIdList.m_pidl, WT_EXECUTEINUITHREAD))
								{
									pItemIdList.Detach();
								}
							}
							else
							{
								CMINVOKECOMMANDINFO cmd = { sizeof(cmd), CMIC_MASK_ASYNCOK, *this, reinterpret_cast<LPCSTR>(id - minID), NULL, NULL, SW_SHOW };
								hr = contextMenu->InvokeCommand(&cmd);
								if (hr == S_OK)
								{
								}
								else
								{
									this->MessageBox(_com_error(hr).ErrorMessage(), _T("Error"), MB_OK | MB_ICONERROR);
								}
							}
						}
						else { this->MessageBox(_com_error(hr).ErrorMessage(), _T("Error"), MB_OK | MB_ICONERROR); }
					}
				}
			}
		}
		return 0;
	}

	LRESULT OnFilesDoubleClick(LPNMHDR pnmh)
	{
		int index;
		Wow64Disable wow64Disabled;
		WTL::CWaitCursor wait;
		LPNMITEMACTIVATE pnmItem = (LPNMITEMACTIVATE)pnmh;
		index = this->lvFiles.HitTest(pnmItem->ptAction, 0);
		if (index >= 0)
		{
			std::basic_string<TCHAR> path = GetSubItemText(this->lvFiles, index, COLUMN_INDEX_PATH);
			path += GetSubItemText(this->lvFiles, index, COLUMN_INDEX_NAME);
			path.erase(std::find(basename(path.begin(), path.end()), path.end(), _T(':')), path.end());
			CShellItemIDList pItemIdList;
			SFGAOF sfgao;
			HRESULT hr = SHParseDisplayName(path.c_str(), NULL, &pItemIdList.m_pidl, 0, &sfgao);
			if (hr == S_OK)
			{
				if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, pItemIdList.m_pidl, WT_EXECUTEINUITHREAD))
				{
					pItemIdList.Detach();
				}
			}
			else
			{
				this->MessageBox(GetAnyErrorText(hr), _T("Error"), MB_ICONERROR);
			}
		}
		return 0;
	}

	LRESULT OnFilesListCustomDraw(LPNMHDR pnmh)
	{
		LRESULT result;
		LPNMLVCUSTOMDRAW const pLV = (LPNMLVCUSTOMDRAW)pnmh;
		if (true)
		{
			switch (pLV->nmcd.dwDrawStage)
			{
			case CDDS_PREPAINT:
				result = CDRF_NOTIFYITEMDRAW;
				break;
			case CDDS_ITEMPREPAINT:
				result = CDRF_NOTIFYSUBITEMDRAW;
				break;
			case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
				if (pLV->iSubItem == COLUMN_INDEX_PATH) { result = 0x8 /*CDRF_DOERASE*/ | CDRF_NOTIFYPOSTPAINT; }
				else { result = CDRF_DODEFAULT; }
				break;
			case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
				{
					std::basic_string<TCHAR> itemText = GetSubItemText(this->lvFiles, int_cast<int>(pLV->nmcd.dwItemSpec), pLV->iSubItem);
					WTL::CDCHandle dc(pLV->nmcd.hdc);
					int const savedDC = dc.SaveDC();
					{
						std::replace(itemText.begin(), itemText.end(), _T(' '), _T('\u00A0'));
						replace_all(itemText, _T("\\"), _T("\\\u200B"));
#ifdef _UNICODE
						this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, 1200);
#else
						this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, CP_ACP);
#endif
						TCHAR const boundary = pLV->iSubItem == 1 ? _T('\\') : _T('\r');
						CHARFORMAT format = { sizeof(format), CFM_COLOR, 0, 0, 0, GetSysColor(COLOR_WINDOWTEXT) };
						this->richEdit.SetSel(0, static_cast<int>(itemText.find_first_not_of(boundary, itemText.find_last_of(boundary))));
						this->richEdit.SetSelectionCharFormat(format);
						RECT rcTwips = pLV->nmcd.rc;
						rcTwips.left = (int)((rcTwips.left + 6) * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
						rcTwips.right = (int)(rcTwips.right * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
						rcTwips.top = (int)(rcTwips.top * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
						rcTwips.bottom = (int)(rcTwips.bottom * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
						FORMATRANGE formatRange = { dc, dc, rcTwips, rcTwips, { 0, -1 } };
						this->richEdit.FormatRange(formatRange, FALSE);
						LONG height = formatRange.rc.bottom - formatRange.rc.top;
						formatRange.rc = formatRange.rcPage;
						formatRange.rc.top += (formatRange.rc.bottom - formatRange.rc.top - height) / 2;
						this->richEdit.FormatRange(formatRange, TRUE);

						this->richEdit.FormatRange(NULL);
					}
					dc.RestoreDC(savedDC);
				}
				result = CDRF_SKIPDEFAULT;
				break;
			default:
				result = CDRF_DODEFAULT;
				break;
			}
		}
		else { result = CDRF_DODEFAULT; }
		return result;
	}

	static std::basic_string<TCHAR> &GetPath(NtfsIndex const &index, unsigned long segmentNumber, std::basic_string<TCHAR> &path)
	{
		path.erase(path.begin(), path.end());
		try
		{
			for (;;)
			{
				size_t const cchPrev = path.size();
				unsigned long const parentSegmentNumber = index.get_name(segmentNumber, path);
				if (path.size() > cchPrev)
				{ std::reverse(&path[cchPrev], &path[0] + path.size()); }
				if (parentSegmentNumber == segmentNumber)
				{ break; }
				segmentNumber = parentSegmentNumber;
				path.append(1, _T('\\'));
			}
		}
		catch (std::domain_error const &ex)
		{
			path.append(1, _T('\\'));
			std::string msg = ex.what();
			msg = "<" + ("error: " + msg) + ">";
			std::copy(msg.rbegin(), msg.rend(), std::inserter(path, path.end()));
		}
		std::reverse(path.begin(), path.end());
		return path;
	}

	LRESULT OnFilesGetDispInfo(LPNMHDR pnmh)
	{
		NMLVDISPINFO *pLV = (NMLVDISPINFO *)pnmh;
		DataRow const &row = this->rows[pLV->item.iItem];

		if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0 && (pLV->item.mask & LVIF_TEXT) != 0)
		{
			std::basic_string<TCHAR> path;
			switch (pLV->item.iSubItem)
			{
			case COLUMN_INDEX_NAME:
				_tcsncpy(pLV->item.pszText, row.name().c_str(), pLV->item.cchTextMax);
				{
					int iImage = this->CacheIcon(adddirsep(GetPath(*row.pIndex, row.parent(), path)) + row.name(), pLV->item.iItem, row.attributes(), true);
					if (iImage >= 0) { pLV->item.iImage = iImage; }
				}
				break;
			case COLUMN_INDEX_PATH: _tcsncpy(pLV->item.pszText, adddirsep(GetPath(*row.pIndex, row.parent(), path)).c_str(), pLV->item.cchTextMax); break;
			case COLUMN_INDEX_SIZE: _tcsncpy(pLV->item.pszText, (row.attributes() & FILE_ATTRIBUTE_DIRECTORY) == 0 ? nformat(row.size()).c_str() : _T(""), pLV->item.cchTextMax); break;
			case COLUMN_INDEX_SIZE_ON_DISK: _tcsncpy(pLV->item.pszText, (row.attributes() & FILE_ATTRIBUTE_DIRECTORY) == 0 ? nformat(row.sizeOnDisk()).c_str() : _T(""), pLV->item.cchTextMax); break;
			case COLUMN_INDEX_MODIFICATION_TIME: SystemTimeToString(row.modificationTime(), pLV->item.pszText, pLV->item.cchTextMax - 2 /*to be safe*/); break;
			case COLUMN_INDEX_CREATION_TIME: SystemTimeToString(row.creationTime(), pLV->item.pszText, pLV->item.cchTextMax - 2 /*to be safe*/); break;
			case COLUMN_INDEX_ACCESS_TIME: SystemTimeToString(row.accessTime(), pLV->item.pszText, pLV->item.cchTextMax - 2 /*to be safe*/); break;
			default:
				__debugbreak();
				break;
			}
		}

		return 0;
	}

	int CacheIcon(std::basic_string<TCHAR> const &path, int const iItem, ULONG fileAttributes, bool lifo)
	{
		if (this->cache.find(path) == this->cache.end())
		{
			this->cache[path] = CacheInfo();
		}

		CacheInfo const &entry = this->cache[path];

		if (!entry.valid)
		{
			SIZE size; this->lvFiles.GetImageList(LVSIL_SMALL).GetIconSize(size);
			this->iconLoader->add([this, path, size, fileAttributes, iItem]() -> BOOL
			{
				RECT rcItem = { LVIR_BOUNDS };
				RECT rcFiles, intersection;
				this->lvFiles.GetClientRect(&rcFiles);  // Blocks, but should be fast
				this->lvFiles.GetItemRect(iItem, &rcItem, LVIR_BOUNDS);  // Blocks, but I'm hoping it's fast...
				if (IntersectRect(&intersection, &rcFiles, &rcItem))
				{
					std::basic_string<TCHAR> const normalizedPath = NormalizePath(path);
					SHFILEINFO shfi = {0};
					std::basic_string<TCHAR> description;
#if 0
					{
						std::vector<BYTE> buffer;
						DWORD temp;
						buffer.resize(GetFileVersionInfoSize(normalizedPath.c_str(), &temp));
						if (GetFileVersionInfo(normalizedPath.c_str(), NULL, static_cast<DWORD>(buffer.size()), buffer.empty() ? NULL : &buffer[0]))
						{
							LPVOID p;
							UINT uLen;
							if (VerQueryValue(buffer.empty() ? NULL : &buffer[0], _T("\\StringFileInfo\\040904E4\\FileDescription"), &p, &uLen))
							{ description = std::basic_string<TCHAR>((LPCTSTR)p, uLen); }
						}
					}
#endif
					BOOL success = FALSE;
					SetLastError(0);
					if (shfi.hIcon == NULL)
					{
						CoInit com;  // MANDATORY!  Some files, like '.sln' files, won't work without it!
						ULONG const flags = SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_ADDOVERLAYS | //SHGFI_TYPENAME | SHGFI_SYSICONINDEX |
							((std::max)(size.cx, size.cy) <= (std::max)(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) ? SHGFI_SMALLICON : SHGFI_LARGEICON);
						success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi), flags) != 0;
						if (!success && (flags & SHGFI_USEFILEATTRIBUTES) == 0)
						{ success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0; }
					}
					if (success)
					{
						std::basic_string<TCHAR> const path(path), displayName(GetDisplayName(*this, path, SHGDN_NORMAL));
						CMainDlg *this_(this);
						int const iItem(iItem);
						this->InvokeAsync([this_, description, displayName, path, shfi, iItem]() -> bool
						{
							WTL::CWaitCursor wait(true, IDC_APPSTARTING);
							CMainDlg::CacheInfo &cached = this_->cache[path];
							WTL::CIcon iconSmall(shfi.hIcon);
							_tcscpy(cached.szTypeName, shfi.szTypeName);
							cached.displayName = displayName;
							cached.description = description;

							if (cached.iIconSmall < 0) { cached.iIconSmall = this_->imgListSmall.AddIcon(iconSmall); }
							else { this_->imgListSmall.ReplaceIcon(cached.iIconSmall, iconSmall); }

							if (cached.iIconLarge < 0) { cached.iIconLarge = this_->imgListLarge.AddIcon(iconSmall); }
							else { this_->imgListLarge.ReplaceIcon(cached.iIconLarge, iconSmall); }

							cached.valid = true;

							this_->lvFiles.RedrawItems(iItem, iItem);
							return true;
						});
					}
					else
					{
						_ftprintf(stderr, _T("Could not get the icon for file: %s\n"), normalizedPath.c_str());
					}
				}
				return true;
			}, lifo);
		}
		return entry.iIconSmall;
	}

	template<class StrCmp>
	class PathComparator
	{
		std::basic_string<TCHAR> path1, path2;
		StrCmp cmp;
	public:
		PathComparator(StrCmp const &cmp) : cmp(cmp) { }
		bool operator()(DataRow const &a, DataRow const &b)
		{ return this->cmp(GetPath(*a.pIndex, a.parent(), path1), GetPath(*b.pIndex, b.parent(), path2)); }
	};

	template<class StrCmp>
	PathComparator<StrCmp> path_comparator(StrCmp const &cmp)
	{
		return PathComparator<StrCmp>(cmp);
	}

	LRESULT OnFilesListColumnClick(LPNMHDR pnmh)
	{
		WTL::CWaitCursor wait;
		LPNM_LISTVIEW pLV = (LPNM_LISTVIEW)pnmh;
		WTL::CHeaderCtrl header = this->lvFiles.GetHeader();
		HDITEM hditem = {0};
		hditem.mask = HDI_LPARAM;
		header.GetItem(pLV->iSubItem, &hditem);
		bool cancelled = false;
		if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0)
		{
			std::locale loc;

			try
			{
				// TODO: Compare paths case-insensitively
				switch (pLV->iSubItem)
				{
				case COLUMN_INDEX_NAME: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::name, _1), iless(loc)))); break;
				case COLUMN_INDEX_PATH: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(path_comparator(iless(loc)))); break;
				case COLUMN_INDEX_SIZE: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::size, _1)))); break;
				case COLUMN_INDEX_SIZE_ON_DISK: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::sizeOnDisk, _1)))); break;
				case COLUMN_INDEX_CREATION_TIME: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::creationTime, _1)))); break;
				case COLUMN_INDEX_MODIFICATION_TIME: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::modificationTime, _1)))); break;
				case COLUMN_INDEX_ACCESS_TIME: std::stable_sort(this->rows.begin(), this->rows.end(), cancellable_comparator(comparator(boost::bind(&DataRow::accessTime, _1)))); break;
				default: __debugbreak(); break;
				}
				if (hditem.lParam)
				{
					std::reverse(this->rows.begin(), this->rows.end());
				}
			}
			catch (CStructured_Exception &ex)
			{
				cancelled = true;
				if (ex.GetSENumber() != ERROR_CANCELLED)
				{
					throw;
				}
			}
			this->lvFiles.SetItemCount(this->lvFiles.GetItemCount());
		}
		else
		{
			pLV->iItem = this->lastSortColumn == pLV->iSubItem ? !this->lastSortIsDescending : 0;
			struct Sorting
			{
				CMainDlg *me; LPARAM lParamSort;
				static int CALLBACK compare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
				{
					Sorting *p = (Sorting *)lParamSort;
					return p->me->OnFilesSort(lParam1, lParam2, p->lParamSort);
				}
			} lParam = { this, (LPARAM)pnmh };
			this->lvFiles.SortItemsEx(Sorting::compare, (LPARAM)&lParam);
			this->lastSortIsDescending = pLV->iItem;
			this->lastSortColumn = pLV->iSubItem;
		}
		if (!cancelled)
		{
			hditem.lParam = !hditem.lParam;
			header.SetItem(pLV->iSubItem, &hditem);
		}
		return TRUE;
	}

	int CALLBACK OnFilesSort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
	{
		(void)lParam1;
		(void)lParam2;
		(void)lParamSort;
		throw std::logic_error("not implemented");
	}

	static std::basic_string<TCHAR> GetSubItemText(WTL::CListViewCtrl lv, int iItem, int iSubItem)
	{
		std::basic_string<TCHAR> result(256, _T('\0'));
		size_t cch;
		for (; ; )
		{
			cch = int_cast<size_t>(lv.GetItemText(iItem, iSubItem, &result[0], static_cast<int>(result.size())));
			if (cch + 1 < result.size() - 1) { break; }
			result.resize(result.size() * 2);
		}
		result.resize(cch);
		return result;
	}

	void OnClose(UINT /*uNotifyCode*/ = 0, int nID = IDCANCEL, HWND /*hWnd*/ = NULL) { this->EndDialog(nID); }

	std::basic_string<TCHAR> GetCurrentDrive()
	{
		int const i = this->cmbDrive.GetCurSel();
		std::basic_string<TCHAR> drive(this->cmbDrive.GetLBTextLen(i), _T('\0'));
		if (!drive.empty()) { this->cmbDrive.GetLBText(i, &drive[0]); }
		if (!drive.empty() && !isdirsep(drive[drive.size() - 1]))
		{ drive += _T('\\'); }
		return drive;
	}

	void OnSearch(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (this->txtFileName.GetWindowTextLength() > 0)
		{
			ATL::CComBSTR name;
			this->txtFileName.GetWindowText(*&name);

			WTL::CWaitCursor wait;

			std::basic_string<TCHAR> 
				driveLetter = this->GetCurrentDrive(),
				pattern = _T("*") + std::basic_string<TCHAR>(name) + _T("*");

#if 0
			replace_all(pattern, _T("#"), _T("\\#")); replace_all(pattern, _T("\\"), _T("\\\\"));
			replace_all(pattern, _T("("), _T("\\(")); replace_all(pattern, _T(")"), _T("\\)"));
			replace_all(pattern, _T("["), _T("\\[")); replace_all(pattern, _T("]"), _T("\\]"));
			replace_all(pattern, _T("{"), _T("\\{")); replace_all(pattern, _T("}"), _T("\\}"));
			replace_all(pattern, _T("^"), _T("\\^")); replace_all(pattern, _T("$"), _T("\\$"));
			replace_all(pattern, _T("+"), _T("\\+")); replace_all(pattern, _T("*"), _T(".*"));
			replace_all(pattern, _T("."), _T("\\.")); replace_all(pattern, _T("?"), _T("."));
			replace_all(pattern, _T("|"), _T("\\|"));
			typedef boost::xpressive::basic_regex<std::basic_string<TCHAR>::const_iterator> tregex;
			tregex patternRegex;
			try { patternRegex = tregex::compile(pattern.begin(), pattern.end(), boost::xpressive::regex_constants::icase); }
			catch (boost::xpressive::regex_error &ex)
			{
				std::basic_string<TCHAR> msg;
				std::copy(ex.what(), ex.what() + strlen(ex.what()), std::inserter(msg, msg.end()));
				this->MessageBox(tformat(_T("You entered an invalid regular expression pattern.\nIf you're using wildcards, change '*' to '.*' and change '?' to '.'.\n\nError: %s"), msg.c_str()).c_str(), _T("Invalid Pattern"), MB_OK | MB_ICONERROR);
				return;
			}
#endif

			bool const ownerData = (this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0;
			this->iconLoader->clear();
			if (ownerData) { this->lvFiles.SetItemCount(0); }
			else { this->lvFiles.DeleteAllItems(); }
			this->rows.erase(this->rows.begin(), this->rows.end());
			this->UpdateWindow();

			class SetUncached
			{
				bool prev;
				bool volatile *pCached;
				SetUncached(SetUncached const &) { throw std::logic_error(""); }
				SetUncached &operator =(SetUncached const &) { throw std::logic_error(""); }
			public:
				SetUncached(bool volatile *pCached = NULL)
					: pCached(pCached), prev(pCached ? *pCached : false) { if (pCached) { *pCached = false; } }
				~SetUncached() { if (pCached) { *pCached = prev; } }
				void swap(SetUncached &other) { using std::swap; swap(this->prev, other.prev); swap(this->pCached, other.pCached); }
			};
			class SetBackground
			{
				NtfsIndexThread *pThread;
				SetBackground(SetBackground const &) { throw std::logic_error(""); }
				SetBackground &operator =(SetBackground const &) { throw std::logic_error(""); }
			public:
				SetBackground(NtfsIndexThread *pThread = NULL)
					: pThread(pThread)
				{
					if (pThread)
					{
						ResetEvent(reinterpret_cast<HANDLE>(pThread->event()));
						SetThreadPriority(reinterpret_cast<HANDLE>(pThread->thread()), 0x20000 /*THREAD_MODE_BACKGROUND_END*/);
					}
				}
				~SetBackground()
				{
					if (pThread)
					{
						SetEvent(reinterpret_cast<HANDLE>(pThread->event()));
						SetThreadPriority(reinterpret_cast<HANDLE>(pThread->thread()), 0x10000 /*THREAD_MODE_BACKGROUND_BEGIN*/);
					}
				}
				void swap(SetBackground &other) { using std::swap; swap(this->pThread, other.pThread); }
			};
			boost::scoped_array<SetBackground> suspendedThreads(new SetBackground[this->drives.size()]);
			SetUncached setUncached;
			NtfsIndexThread *pThread = NULL;
			for (size_t i = 0; i < this->drives.size(); i++)
			{
				if (this->drives[i].drive() == driveLetter) { pThread = &this->drives[i]; }
				else { SetBackground(&this->drives[i]).swap(suspendedThreads[i]); }
			}
			if (pThread)
			{
				SetUncached(&pThread->cached()).swap(setUncached);
				CProgressDialog dlg(*this);
				NtfsIndex *index = pThread->index();
				if (!index)
				{
					dlg.SetProgressTitle(_T("Reading drive..."));
					while (!dlg.HasUserCancelled() && (index = pThread->index(), index == NULL))
					{
						CProgressDialog::WaitWithMessageLoop(NULL, CProgressDialog::UPDATE_INTERVAL);
						if (dlg.ShouldUpdate())
						{
							TCHAR text[256];
							_stprintf(text, _T("Reading file table... %3u%% done"), static_cast<unsigned long>(100 * static_cast<unsigned long long>(pThread->progress()) / (std::numeric_limits<unsigned long>::max)()));
							dlg.SetProgressText(boost::make_iterator_range(text, text + std::char_traits<TCHAR>::length(text)));
							dlg.SetProgress(pThread->progress(), (std::numeric_limits<unsigned long>::max)());
						}
					}
				}
				if (index)
				{
					dlg.SetProgressTitle(_T("Searching..."));
					std::basic_string<TCHAR> tempName;
					tempName.reserve(1024);
					try
					{
						this->rows.reserve(index->size());
						for (size_t i = 0; i < index->size(); i++)
						{
							if (dlg.HasUserCancelled())
							{ throw CStructured_Exception(ERROR_CANCELLED, NULL); }
							boost::iterator_range<TCHAR const *> const name = index->get_name(index->at(i).second.second.first.first);
							if (dlg.ShouldUpdate())
							{
								dlg.SetProgress(i, index->size());
								dlg.SetProgressText(name);
							}
							if (wildcard(pattern.begin(), pattern.end(), name.begin(), name.end(), tchar_ci_traits()))
							{
								this->rows.push_back(DataRow(index, i));
							}
						}
					}
					catch (CStructured_Exception &ex)
					{
						if (ex.GetSENumber() != ERROR_CANCELLED)
						{
							throw;
						}
					}
				}
			}
			if (ownerData)
			{
				this->lvFiles.SetItemCountEx(int_cast<int>(this->rows.size()), LVSICF_NOINVALIDATEALL);
			}
		}
		else { this->MessageBox(_T("Please enter a file name."), _T("No Input"), MB_OK | MB_ICONERROR); }
	}

	void OnShowWindow(BOOL bShow, UINT /*nStatus*/)
	{
		if (bShow)
		{
			this->txtFileName.SetFocus();
		}
	}

	void OnSearchParamsChange(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		if (false)
		{
			WTL::CWaitCursor wait;
			this->iconLoader->clear();
			this->lvFiles.DeleteAllItems();
		}
		this->lastSortIsDescending = -1;
		this->lastSortColumn = -1;
        }

	intptr_t operator()(uintptr_t hWndParent)
	{
		return this->DoModal(reinterpret_cast<HWND>(hWndParent));
	}

	void OnHelpAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		this->MessageBox(_T("© Mehrdad Niknami"), _T("About"), MB_ICONINFORMATION);
	}

	void OnFileFitColumns(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		WTL::CListViewCtrl &wndListView = this->lvFiles;

		RECT rect;
		wndListView.GetClientRect(&rect);
		DWORD width;
		width = (std::max)(1, (int)(rect.right - rect.left) - GetSystemMetrics(SM_CXVSCROLL));
		WTL::CHeaderCtrl wndListViewHeader = wndListView.GetHeader();
		int oldTotalColumnsWidth;
		oldTotalColumnsWidth = 0;
		int columnCount;
		columnCount = wndListViewHeader.GetItemCount();
		for (int i = 0; i < columnCount; i++)
		{ oldTotalColumnsWidth += wndListView.GetColumnWidth(i); }
		for (int i = 0; i < columnCount; i++)
		{
			int colWidth = wndListView.GetColumnWidth(i);
			int newWidth = MulDiv(colWidth, width, oldTotalColumnsWidth);
			newWidth = (std::max)(newWidth, 1);
			wndListView.SetColumnWidth(i, newWidth);
		}
	}

	BEGIN_MSG_MAP_EX(CMainDlg)
		CHAIN_MSG_MAP(CInvokeImpl<CMainDlg>)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_SHOWWINDOW(OnShowWindow)
		MSG_WM_CLOSE(OnClose)
		COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnClose)
		COMMAND_ID_HANDLER_EX(ID_FILE_FITCOLUMNS, OnFileFitColumns)
		COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnHelpAbout)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnSearch)
		COMMAND_HANDLER_EX(IDC_COMBODRIVE, CBN_SELCHANGE, OnSearchParamsChange)
		COMMAND_HANDLER_EX(IDC_COMBODRIVE, CBN_EDITCHANGE, OnSearchParamsChange)
		COMMAND_HANDLER_EX(IDC_EDITFILENAME, EN_CHANGE, OnSearchParamsChange)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_GETDISPINFO, OnFilesGetDispInfo)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_COLUMNCLICK, OnFilesListColumnClick)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_CUSTOMDRAW, OnFilesListCustomDraw)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_DBLCLK, OnFilesDoubleClick)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_RCLICK, OnFilesRightClick)
		NOTIFY_HANDLER_EX(IDC_EDITFILENAME, CUpDownNotify<WTL::CEdit>::CUN_KEYDOWN, OnFileNameArrowKey)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_COMBODRIVE, 0)
		DLGRESIZE_CONTROL(IDC_STATUS_BAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		DLGRESIZE_CONTROL(IDC_LISTFILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_EDITFILENAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()
};

CMainDlgBase *CMainDlgBase::create() { return new CMainDlg(); }

#pragma comment(lib, "ShlWAPI.lib")
