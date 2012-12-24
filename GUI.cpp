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
#include <boost/range/algorithm/stable_sort.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/join.hpp>
#include <boost/range/sub_range.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/scoped_array.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

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
#include <Dbt.h>

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
#include "Matcher.hpp"
#include "nformat.hpp"
#include "NtObjectMini.hpp"
#include "NtfsIndex.hpp"
#include "NtfsIndexThread.hpp"
#include "path.hpp"
#include "relative_iterator.hpp"
#include "ShellItemIDList.hpp"

//#include <boost/xpressive/regex_error.hpp>

inline TCHAR totupper(TCHAR c) { return c < SCHAR_MAX ? (_T('A') <= c && c <= _T('Z')) ? c + (_T('a') - _T('A')) : c : _totupper(c); }
#ifdef _totupper
#undef _totupper
#endif
#define _totupper totupper

struct DataRow
{
	boost::shared_ptr<NtfsIndex const> pIndex;
	size_t i;
	DataRow(boost::shared_ptr<NtfsIndex const> const pIndex = boost::shared_ptr<NtfsIndex const>(), size_t const i = static_cast<size_t>(-1)) : pIndex(pIndex), i(i) { }
	NtfsIndex::CombinedRecord const &record() const { return this->pIndex->at(this->i); }
	boost::iterator_range<TCHAR const *> file_name() const
	{ return this->pIndex->get_name_by_index(this->i).first; }
	boost::iterator_range<TCHAR const *> stream_name() const { return this->pIndex->get_name_by_index(this->i).second; }
	std::basic_string<TCHAR> combined_name() const
	{
		std::basic_string<TCHAR> s;
		boost::iterator_range<TCHAR const *> name = this->file_name();
		s.append(name.begin(), name.end());
		name = this->stream_name();
		if (!name.empty())
		{
			s.append(1, _T(':'));
			s.append(name.begin(), name.end());
		}
		return s;
	}
	unsigned long parent() const { return this->record().second.second.second.first; }
	long long creationTime() const { return this->record().second.first.first.first; }
	long long modificationTime() const { return this->record().second.first.first.second; }
	long long accessTime() const { return this->record().second.first.second.first; }
	unsigned long attributes() const { return this->record().second.first.second.second; }
	long long size() const { return this->record().second.second.second.second.first; }
	long long sizeOnDisk() const { return this->record().second.second.second.second.second; }
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
		{
			if (PWow64DisableWow64FsRedirection Wow64DisableWow64FsRedirection =
				reinterpret_cast<PWow64DisableWow64FsRedirection>(GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection")))
			{
				Wow64DisableWow64FsRedirection(&this->cookie);
			}
		}
	}
	~Wow64Disable()
	{
		typedef BOOL (WINAPI *PWow64RevertWow64FsRedirection)(IN PVOID OldValue);
		HMODULE hKernel32 = NULL;
		if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCTSTR)&GetSystemInfo, &hKernel32))
		{
			if (PWow64RevertWow64FsRedirection Wow64RevertWow64FsRedirection =
				reinterpret_cast<PWow64RevertWow64FsRedirection>(GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection")))
			{
				Wow64RevertWow64FsRedirection(&this->cookie);
			}
		}
	}
};

template<class It1, class It2, class Tr>
bool wildcard(It1 patBegin, It1 const patEnd, It2 strBegin, It2 const strEnd, Tr const &tr = std::char_traits<typename std::iterator_traits<It2>::value_type>())
{
	(void)tr;
	if (patBegin == patEnd) { return strBegin == strEnd; }
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
	while (p != patEnd && tr.eq(*p, _T('*'))) { ++p; }
	return p == patEnd;

starCheck:
	if (!star) { return false; }
	strBegin++;
	goto loopStart;
}

template<class R1, class R2, class Tr>
bool wildcard(R1 const &pat, R2 const &str, Tr const &tr = std::char_traits<typename boost::range_value<R1>::type>())
{
	return wildcard(pat.begin(), pat.end(), str.begin(), str.end(), tr);
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

template<class T>
inline T const &use_facet(std::locale const &loc)
{
	return std::
#if defined(_USEFAC)
		_USE(loc, T)
#else
		use_facet<T>(loc)
#endif
		;
}

class iless
{
	mutable std::basic_string<TCHAR> s1, s2;
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
	bool operator()(boost::iterator_range<TCHAR const *> const a, boost::iterator_range<TCHAR const *> const b) const
	{
		s1.assign(a.begin(), a.end());
		s2.assign(b.begin(), b.end());
		return StrCmpLogicalW(s1.c_str(), s2.c_str()) < 0;
		//return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), iless_ch(*this));
	}
};

template<class Pred>
struct indirect_pred
{
	Pred *p;
	indirect_pred(Pred *p) : p(p) { }
	template<class T1, class T2> bool operator()(T1 const &a, T2 const &b) const { return (*p)(a, b); }
};

template<class R, class Pred> R &const_pred_stable_sort(R &range, Pred const &pred) { return boost::stable_sort(range, indirect_pred<Pred const>(&pred)); }

template<class Ch>
class natural_less
{
	natural_less &operator =(natural_less const &);
	struct to_upper
	{
		std::ctype<Ch> const &ctype;
		to_upper(std::ctype<Ch> const &ctype) : ctype(ctype) { }
		Ch operator()(Ch const c) const
		{
			Ch r;
			switch (c)
			{
			case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
			case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
			case 'm': case 'n': case 'o': case 'p': case 'q': case 'r':
			case 's': case 't': case 'u': case 'v': case 'w': case 'x':
			case 'y': case 'z':
				r = c + ('A' - 'a');
				break;
			default:
				r = c <= SCHAR_MAX ? c : ctype.toupper(c);
				break;
			}
			return r;
		}
	};

	static std::locale get_numpunct_locale(std::locale const &loc)
	{
		std::locale result(loc);
#if defined(_MSC_VER) && defined(_ADDFAC)
		std::_ADDFAC(result, new std::numpunct<TCHAR>());
#else
		result = std::locale(locale, new std::numpunct<TCHAR>());
#endif
		return result;
	}

	std::locale const loc, numpunct_loc;
	std::ctype<Ch> const &ctype;
	std::numpunct<Ch> const &numpunct;
	std::num_get<Ch> const &num_get;
	boost::shared_ptr<std::basic_stringstream<Ch> > ss1, ss2;
	to_upper _to_upper;
	bool case_insensitive;

	struct mystreambuf : public std::basic_stringbuf<Ch>
	{
		using std::basic_stringbuf<Ch>::eback;
		using std::basic_stringbuf<Ch>::gbump;
	};

	static std::basic_stringstream<Ch> &set_char(std::basic_stringstream<Ch> &ss, Ch const c)
	{
		ss.seekg(0);
		ss.seekp(0);
		ss << c;
		return ss;
	}

public:
	natural_less(std::locale const &loc = std::locale(""), bool const case_insensitive = true) :
		loc(loc),
		numpunct_loc(get_numpunct_locale(loc)),
		ctype(::use_facet<std::ctype<Ch> >(loc)),
		numpunct(::use_facet<std::numpunct<Ch> >(numpunct_loc)),
		num_get(::use_facet<std::num_get<Ch> >(loc)),
		ss1(new std::basic_stringstream<Ch>()),
		ss2(new std::basic_stringstream<Ch>()),
		_to_upper(ctype),
		case_insensitive(case_insensitive)
	{
		ss1->imbue(loc);
		ss2->imbue(loc);
		*ss1 << Ch();
		*ss2 << Ch();
	}

	std::basic_string<TCHAR> s1, s2;
	bool operator()(Ch const *const begin1, Ch const *const end1, Ch const *const begin2, Ch const *const end2)
	{
		std::istreambuf_iterator<Ch> const isbiEnd;
		Ch const *it1(begin1);
		Ch const *it2(begin2);
		while (it1 != end1 && it2 != end2)
		{
			Ch const c1 = case_insensitive ? this->_to_upper(*it1) : *it1;
			Ch const c2 = case_insensitive ? this->_to_upper(*it2) : *it2;
			size_t nConsumed1, nSignificantDigits1 = 0, nConsumed2, nSignificantDigits2 = 0;

			Ch const *it1EndDigits(it1);
			do
			{
				nConsumed1 = 0;
				{
					Ch const *it1EndDigitsNew(ctype.scan_not(std::ctype_base::digit, it1EndDigits, end1));
					while (it1EndDigits != it1EndDigitsNew)
					{
						unsigned int digit;
						std::ios_base::iostate iostate;
						if (num_get.get(set_char(*ss1, *it1EndDigits), isbiEnd, *ss1, iostate, digit) == isbiEnd)
						{
							++nConsumed1;
							if (digit != 0 || nSignificantDigits1 > 0)
							{ ++nSignificantDigits1; }
						}
						++it1EndDigits;
					}
				}
				if (nConsumed1 > 0)
				{
					while (it1EndDigits != end1 && *it1EndDigits == numpunct.thousands_sep())
					{ ++it1EndDigits; }
				}
			} while (nConsumed1 > 0);

			Ch const *it2EndDigits(it2);
			do
			{
				nConsumed2 = 0;
				{
					Ch const *it2EndDigitsNew(ctype.scan_not(std::ctype_base::digit, it2EndDigits, end2));
					while (it2EndDigits != it2EndDigitsNew)
					{
						unsigned int digit;
						std::ios_base::iostate iostate;
						if (num_get.get(set_char(*ss2, *it2EndDigits), isbiEnd, *ss2, iostate, digit) == isbiEnd)
						{
							++nConsumed2;
							if (digit != 0 || nSignificantDigits2 > 0)
							{ ++nSignificantDigits2; }
						}
						++it2EndDigits;
					}
				}
				if (nConsumed2 > 0)
				{
					while (it2EndDigits != end2 && *it2EndDigits == numpunct.thousands_sep())
					{ ++it2EndDigits; }
				}
			} while (nConsumed2 > 0);

			if (it1 == it1EndDigits)
			{
				if (it2 == it2EndDigits)
				{
					return c1 < c2;
				}
				else { return *it1 < *it2; }
			}
			else
			{
				if (it2 == it2EndDigits)
				{ return *it1 < *it2; }
				else
				{
					if (nSignificantDigits1 < nSignificantDigits2)
					{ return true; }
					else if (nSignificantDigits1 > nSignificantDigits2)
					{ return false; }
					else
					{
						std::ios_base::iostate iostate;
						unsigned int digit1 = 0, digit2 = 0;
						
						while (it1 != it1EndDigits && (*it1 == numpunct.thousands_sep() || num_get.get(set_char(*ss1, *it1), isbiEnd, *ss1, iostate, digit1) == isbiEnd && digit1 == 0))
						{ ++it1; }
						
						while (it2 != it2EndDigits && (*it2 == numpunct.thousands_sep() || num_get.get(set_char(*ss2, *it2), isbiEnd, *ss2, iostate, digit2) == isbiEnd && digit2 == 0))
						{ ++it2; }

						while (it1 != it1EndDigits && it2 != it2EndDigits)
						{
							if (*it1 != numpunct.thousands_sep())
							{
								if (*it2 != numpunct.thousands_sep())
								{
									digit1 = 0;
									digit2 = 0;
									num_get.get(set_char(*ss1, *it1), isbiEnd, *ss1, iostate, digit1);
									num_get.get(set_char(*ss2, *it2), isbiEnd, *ss2, iostate, digit2);
									if (digit1 < digit2)
									{ return true; }
									else if (digit1 > digit2)
									{ return false; }
									++it1;
								}
								++it2;
							}
							else
							{
								if (*it2 != numpunct.thousands_sep())
								{ }
								else
								{ ++it2; }
								++it1;
							}
						}
					}
				}
			}
		}
		if (it2 != end2)
		{
			return true;
		}
		return false;
	}

	template<class R1, class R2>
	bool operator()(R1 const &range1, R2 const &range2)
	{
		return (*this)(
			range1.begin() == range1.end() ? NULL : &range1[0],
			range1.begin() == range1.end() ? NULL : &range1[0] + static_cast<ptrdiff_t>(range1.size()),
			range2.begin() == range2.end() ? NULL : &range2[0],
			range2.begin() == range2.end() ? NULL : &range2[0] + static_cast<ptrdiff_t>(range2.size())
		);
	}
};

struct Tester
{
	Tester()
	{
#if 0
		natural_less<char> less(std::locale(""));
		std::string s1, s2;
		for (;;)
		{
			std::getline(std::cin, s1);
			std::getline(std::cin, s2);
			std::cout << (less(s1, s2) ? "less" : less(s2, s1) ? "greater" : "equal") << std::endl;
		}
#endif
	}
} tester;

EXTERN_C NTSYSAPI NTSTATUS NTAPI RtlSystemTimeToLocalTime(IN LARGE_INTEGER const *SystemTime, OUT PLARGE_INTEGER LocalTime);

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
	DWORD creationTime;
	DWORD lastUpdateTime;
	HWND parent;
	std::basic_string<TCHAR> lastProgressText, lastProgressTitle;
	bool windowCreated;
	int lastProgress, lastProgressTotal;

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		(this->progressText.SubclassWindow)(this->GetDlgItem(IDC_RICHEDITPROGRESS));
		this->progressText.SendMessage(EM_SETBKGNDCOLOR, FALSE, GetSysColor(COLOR_3DFACE));

		this->progressBar.Attach(this->GetDlgItem(IDC_PROGRESS1));
		this->DlgResize_Init(false, false, 0);

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

	static bool WaitMessageLoop()
	{
		for (;;)
		{
			MSG msg;
			unsigned long const nMessages = 0;
			DWORD result = MsgWaitForMultipleObjectsEx(nMessages, NULL, UPDATE_INTERVAL, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
			if (result == WAIT_OBJECT_0 || result == WAIT_TIMEOUT)
			{ return result != WAIT_TIMEOUT; }
			else if (result == WAIT_OBJECT_0 + nMessages)
			{
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
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
		return IsDebuggerPresent() ? 0 : 100;
	}

public:
	enum { UPDATE_INTERVAL = 30 };
	CProgressDialog(ATL::CWindow parent)
		: Base(true), parent(parent), lastUpdateTime(0), creationTime(GetTickCount()), lastProgress(0), lastProgressTotal(1), invalidated(false), canceled(false), windowCreated(false)
	{
	}

	~CProgressDialog()
	{
		if (this->windowCreated)
		{
			EnableWindowRecursive(parent, TRUE);
			this->DestroyWindow();
		}
	}

	bool ShouldUpdate() const
	{
		unsigned long const tickCount = GetTickCount();
		return tickCount - this->lastUpdateTime >= UPDATE_INTERVAL;
	}

	bool HasUserCancelled()
	{
		bool justCreated = false;
		if (!this->windowCreated && abs(static_cast<int>(GetTickCount() - this->creationTime)) >= static_cast<int>(this->GetMinDelay()))
		{
			this->windowCreated = !!this->Create(parent);
			EnableWindowRecursive(parent, FALSE);
			true;
			this->Flush();
		}
		if (this->windowCreated && (justCreated || this->ShouldUpdate()))
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
		return this->canceled;
	}

	void Flush()
	{
		if (this->invalidated && this->windowCreated)
		{
			this->invalidated = false;
			this->SetWindowText(this->lastProgressTitle.c_str());
			this->progressBar.SetRange32(0, this->lastProgressTotal);
			this->progressBar.SetPos(this->lastProgress);
			this->progressText.SetWindowText(this->lastProgressText.c_str());
			this->progressBar.UpdateWindow();
			this->progressText.UpdateWindow();
			this->UpdateWindow();
			this->lastUpdateTime = GetTickCount();
		}
	}

	void SetProgress(long long current, long long total)
	{
		if (total > INT_MAX)
		{
			current = static_cast<long long>((static_cast<double>(current) / total) * INT_MAX);
			total = INT_MAX;
		}
		this->invalidated |= this->lastProgress != current || this->lastProgressTotal != total;
		this->lastProgressTotal = static_cast<int>(total);
		this->lastProgress = static_cast<int>(current);
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
	static void check_key()
	{
		if (GetAsyncKeyState(VK_ESCAPE) < 0)
		{ throw CStructured_Exception(ERROR_CANCELLED, NULL); }
	}
	template<class T1, class T2> bool operator()(T1 const &a, T2 const &b) { check_key(); return f(a, b); }
	template<class T1, class T2> bool operator()(T1 const &a, T2 const &b) const { check_key(); return f(a, b); }
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

template<typename F, typename C> struct Comparator { F f; C c; Comparator(F f, C c) : f(f), c(c) { } template<class T> bool operator()(T const &a, T const &b) const { return c(f(a), f(b)); } };
template<typename F> struct Comparator<F, void> { F f; Comparator(F f) : f(f) { } template<class T> bool operator()(T const &a, T const &b) const { return f(a) < f(b); } };
template<typename F> Comparator<F, void> comparator(F f) { return Comparator<F, void>(f); }
template<typename C, typename F> Comparator<F, C> comparator(F f, C c) { return Comparator<F, C>(f, c); }

class CMainDlg : public WTL::CDialogResize<CMainDlg>, public CModifiedDialogImpl<CMainDlg>, public CInvokeImpl<CMainDlg>, public CMainDlgBase
{
	enum { IDC_STATUS_BAR = 1100 + 0 };
	enum { COLUMN_INDEX_NAME, COLUMN_INDEX_PATH, COLUMN_INDEX_SIZE, COLUMN_INDEX_SIZE_ON_DISK, COLUMN_INDEX_MODIFICATION_TIME, COLUMN_INDEX_CREATION_TIME, COLUMN_INDEX_ACCESS_TIME };
	enum { WM_NOTIFYICON = WM_USER + 100, WM_THREADMESSAGE = WM_USER + 101 };
	struct CThemedListViewCtrl : public WTL::CListViewCtrl, public WTL::CThemeImpl<CThemedListViewCtrl> { using WTL::CListViewCtrl::Attach; };
	CThemedListViewCtrl lvFiles;
	WTL::CComboBox cmbDrive;
	WTL::CStatusBarCtrl statusbar;
	WTL::CProgressBarCtrl statusbarProgress;
	CUpDownNotify<WTL::CEdit> txtFileName;
	static UINT const WM_TASKBARCREATED;
	//boost::ptr_vector<NtfsIndexThread> drives;
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
	HANDLE hWait, hEvent;

	std::vector<DataRow> rows;

public:
	enum { IDD = IDD_DIALOGMAIN };
	CMainDlg(HANDLE hEvent)
		: CModifiedDialogImpl(true), lastSortIsDescending(-1), lastSortColumn(-1), iconLoader(BackgroundWorker::create(true)), hRichEdit(LoadLibrary(_T("riched20.dll"))), hWait(NULL), hEvent(hEvent) { }

	~CMainDlg()
	{
		FreeLibrary(this->hRichEdit);
	}

private:
	static VOID CALLBACK WaitCallback(PVOID lpParameter, BOOLEAN TimerOrWaitFired)
	{
		HWND const hWnd = reinterpret_cast<HWND>(lpParameter);
		if (!TimerOrWaitFired)
		{
			WINDOWPLACEMENT placement = { sizeof(placement) };
			if (::GetWindowPlacement(hWnd, &placement))
			{
				::ShowWindowAsync(hWnd, ::IsZoomed(hWnd) || (placement.flags & WPF_RESTORETOMAXIMIZED) != 0 ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
			}
		}
	}

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		this->cmbDrive.Attach(this->GetDlgItem(IDC_COMBODRIVE));
		this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
		this->txtFileName.SubclassWindow(this->GetDlgItem(IDC_EDITFILENAME));
		this->richEdit.Create(this->lvFiles, NULL, 0, ES_MULTILINE, WS_EX_TRANSPARENT);
		this->richEdit.SetFont(this->lvFiles.GetFont());
		
		this->statusbar = CreateStatusWindow(WS_CHILD | SBT_TOOLTIPS, NULL, *this, IDC_STATUS_BAR);
		int const rcStatusPaneWidths[] = { 360, -1 };
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
		WTL::CRect rcStatusPane1; this->statusbar.GetRect(1, &rcStatusPane1);
		//this->statusbarProgress.Create(this->statusbar, rcStatusPane1, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0);
		//this->statusbarProgress.SetRange(0, INT_MAX);
		//this->statusbarProgress.SetPos(INT_MAX / 2);
		this->statusbar.ShowWindow(SW_SHOW);

		WTL::CRect clientRect;
		if (this->lvFiles.GetWindowRect(&clientRect))
		{
			this->ScreenToClient(&clientRect);
			this->lvFiles.SetWindowPos(NULL, 0, 0, clientRect.Width(), clientRect.Height() - rcStatusPane1.Height(), SWP_NOMOVE | SWP_NOZORDER);
		}

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

		RegisterWaitForSingleObject(&this->hWait, hEvent, &WaitCallback, this->m_hWnd, INFINITE, WT_EXECUTEINUITHREAD);

		this->cmbDrive.SetCurSel(-1);
		this->cmbDrive.SetCurSel(this->RefreshVolumes());

		this->OnSearchParamsChange(CBN_SELCHANGE, IDC_COMBODRIVE, this->cmbDrive);
		return TRUE;
	}

	int RefreshVolumes()
	{
		std::basic_string<TCHAR> logicalDrives(GetLogicalDriveStrings(0, NULL) + 1, _T('\0'));
		logicalDrives.resize(GetLogicalDriveStrings(static_cast<DWORD>(logicalDrives.size()) - 1, &logicalDrives[0]));
		int iSel = this->cmbDrive.GetCurSel();
		TCHAR windowsDir[MAX_PATH] = {0};
		windowsDir[GetWindowsDirectory(windowsDir, sizeof(windowsDir) / sizeof(*windowsDir))] = _T('\0');
		for (LPCTSTR drive = logicalDrives.c_str(); *drive != _T('\0'); drive += _tcslen(drive) + 1)
		{
			if (this->cmbDrive.FindStringExact(-1, drive) == CB_ERR)
			{
				TCHAR fsName[MAX_PATH];
				if (GetVolumeInformation(drive, NULL, 0, NULL, 0, NULL, fsName, sizeof(fsName) / sizeof(*fsName)) && _tcsicmp(fsName, _T("NTFS")) == 0)
				{
					boost::intrusive_ptr<NtfsIndexThread> const p(NtfsIndexThread::create(this->m_hWnd, WM_THREADMESSAGE, drive));
					int const index = this->cmbDrive.AddString(drive);
					if (drive[0] != _T('\0') && drive[0] == windowsDir[0] && drive[1] == _T(':'))
					{ iSel = index; }
					if (this->cmbDrive.SetItemDataPtr(index, p.get()) != CB_ERR)
					{ intrusive_ptr_add_ref(p.get()); }
				}
			}
		}
		return iSel;
	}

	void OnParentNotify(UINT message, UINT nChildID, LPARAM lParam)
	{
		(void)nChildID;
		if (message == WM_DESTROY && reinterpret_cast<HWND>(lParam) == this->cmbDrive)
		{
			for (int i = this->cmbDrive.GetCount() - 1; i >= 0; i--)
			{
				boost::intrusive_ptr<NtfsIndexThread> const p = static_cast<NtfsIndexThread *>(this->cmbDrive.GetItemDataPtr(i));
				if (this->cmbDrive.SetItemDataPtr(i, NULL) != CB_ERR)
				{
					this->cmbDrive.DeleteString(static_cast<UINT>(i));
					intrusive_ptr_release(p.get());
				}
			}
		}
	}

	void OnDestroy()
	{
		WTL::CWaitCursor wait;
		this->iconLoader->clear();
		UnregisterWait(this->hWait);
		this->DeleteNotifyIcon();
	}

	std::vector<boost::intrusive_ptr<NtfsIndexThread> > GetDrives()
	{
		std::vector<boost::intrusive_ptr<NtfsIndexThread> > drives;
		for (int i = 0; i < this->cmbDrive.GetCount(); i++)
		{ drives.push_back(static_cast<NtfsIndexThread *>(this->cmbDrive.GetItemDataPtr(i))); }
		return drives;
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
			DataRow const &row = this->rows[index];
			std::basic_string<TCHAR> path;
			adddirsep(GetPath(*row.pIndex, row.parent(), path));
			boost::iterator_range<TCHAR const *> const fileName = row.file_name();
			if (fileName != boost::as_literal(_T("."))) { path.append(fileName.begin(), fileName.end()); }

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
							UINT openContainingFolderId;
							{
								MENUITEMINFO mii = { sizeof(mii), 0, MFT_MENUBREAK };
								menu.InsertMenuItem(0, TRUE, &mii);
							}
							{
								std::basic_stringstream<TCHAR> ssName;
								ssName.imbue(std::locale(""));
								ssName << _T("File #") << static_cast<unsigned long>(row.record().first);
								std::basic_string<TCHAR> name = ssName.str();
								MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_DISABLED, minID - 2, NULL, NULL, NULL, NULL, (name.c_str(), &name[0]) };
								menu.InsertMenuItem(0, TRUE, &mii);
							}
							{
								MENUITEMINFO mii = { sizeof(mii), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_ENABLED, (openContainingFolderId = minID - 1), NULL, NULL, NULL, NULL, _T("Open &Containing Folder") };
								menu.InsertMenuItem(0, TRUE, &mii);
								menu.SetMenuDefaultItem(openContainingFolderId, FALSE);
							}
							WTL::CPoint cursorPos;
							GetCursorPos(&cursorPos);
							UINT id = menu.TrackPopupMenu(
								TPM_RETURNCMD | TPM_NONOTIFY | (GetKeyState(VK_SHIFT) < 0 ? CMF_EXTENDEDVERBS : 0) |
								(GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION : TPM_LEFTALIGN | TPM_HORPOSANIMATION),
								cursorPos.x, cursorPos.y, *this);
							if (!id)
							{
								// User cancelled
							}
							else if (id == openContainingFolderId)
							{
								if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, pItemIdList.m_pidl, WT_EXECUTEINUITHREAD))
								{
									pItemIdList.Detach();
								}
							}
							else if (id >= minID)
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
			while (segmentNumber != 5)
			{
				size_t const cchPrev = path.size();
				segmentNumber = index.get_name(segmentNumber, path);
				if (path.size() > cchPrev)
				{ std::reverse(&path[cchPrev], &path[0] + path.size()); }
				path.append(1, _T('\\'));
			}
			{
				size_t const cchPrev = path.size();
				std::basic_string<TCHAR> const &drive = index.drive();
				path.append(drive.begin(), trimdirsep(drive.begin(), drive.end()));
				if (path.size() > cchPrev)
				{ std::reverse(&path[cchPrev], &path[0] + path.size()); }
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
				_tcsncpy(pLV->item.pszText, row.combined_name().c_str(), pLV->item.cchTextMax);
				{
					int iImage = this->CacheIcon(adddirsep(GetPath(*row.pIndex, row.parent(), path)) + (boost::equal(row.file_name(), boost::as_literal(_T("."))) ? _T("") : row.combined_name()), pLV->item.iItem, row.attributes(), true);
					if (iImage >= 0) { pLV->item.iImage = iImage; }
				}
				break;
			case COLUMN_INDEX_PATH: _tcsncpy(pLV->item.pszText, adddirsep(GetPath(*row.pIndex, row.parent(), path)).c_str(), pLV->item.cchTextMax); break;
			case COLUMN_INDEX_SIZE: _tcsncpy(pLV->item.pszText, nformat(row.size()).c_str(), pLV->item.cchTextMax); break;
			case COLUMN_INDEX_SIZE_ON_DISK: _tcsncpy(pLV->item.pszText, nformat(row.sizeOnDisk()).c_str(), pLV->item.cchTextMax); break;
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
					winnt::NtFile fileTemp;  // To prevent icon retrieval from changing the file time
					try
					{
						std::basic_string<TCHAR> ntPath = winnt::NtFile::RtlDosPathNameToNtPathName((_T("\\??\\") + normalizedPath).c_str());
						fileTemp = winnt::NtFile::NtOpenFile(ntPath, winnt::Access::QueryAttributes | winnt::Access::SetAttributes | winnt::Access::Read, 0 /*exclusive access*/, FILE_NO_INTERMEDIATE_BUFFERING | FILE_OPEN_REPARSE_POINT);
						FILE_BASIC_INFORMATION fbi = fileTemp.NtQueryInformationFile<FILE_BASIC_INFORMATION>();
						fbi.ChangeTime.QuadPart = fbi.LastAccessTime.QuadPart = fbi.LastWriteTime.QuadPart = -1;
						fileTemp.NtSetInformationFile(fbi);
					}
					catch (CStructured_Exception &) { }
					BOOL success = FALSE;
					SetLastError(0);
					if (shfi.hIcon == NULL)
					{
						ULONG const flags = SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_ADDOVERLAYS | //SHGFI_TYPENAME | SHGFI_SYSICONINDEX |
							((std::max)(size.cx, size.cy) <= (std::max)(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) ? SHGFI_SMALLICON : SHGFI_LARGEICON);
						CoInit com;  // MANDATORY!  Some files, like '.sln' files, won't work without it!
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
	class NameComparator
	{
		StrCmp less;
	public:
		NameComparator(StrCmp const &less) : less(less) { }
		bool operator()(DataRow const &a, DataRow const &b)
		{
			bool less = this->less(a.file_name(), b.file_name());
			if (!less && !this->less(b.file_name(), a.file_name()))
			{ less = this->less(a.stream_name(), b.stream_name()); }
			return less;
		}
		bool operator()(DataRow const &a, DataRow const &b) const
		{
			bool less = this->less(a.file_name(), b.file_name());
			if (!less && !this->less(b.file_name(), a.file_name()))
			{ less = this->less(a.stream_name(), b.stream_name()); }
			return less;
		}
	};

	template<class StrCmp>
	NameComparator<StrCmp> name_comparator(StrCmp const &cmp) { return NameComparator<StrCmp>(cmp); }

	template<class StrCmp>
	class PathComparator
	{
		mutable std::basic_string<TCHAR> path1, path2;
		StrCmp cmp;
	public:
		PathComparator(StrCmp const &cmp) : cmp(cmp) { }
		bool operator()(DataRow const &a, DataRow const &b)
		{
			GetPath(*a.pIndex, a.parent(), path1);
			GetPath(*b.pIndex, b.parent(), path2);
			return this->cmp(
				boost::make_iterator_range(path1.data(), path1.data() + static_cast<ptrdiff_t>(path1.size())),
				boost::make_iterator_range(path2.data(), path2.data() + static_cast<ptrdiff_t>(path2.size())));
		}
		bool operator()(DataRow const &a, DataRow const &b) const
		{
			GetPath(*a.pIndex, a.parent(), path1);
			GetPath(*b.pIndex, b.parent(), path2);
			return this->cmp(
				boost::make_iterator_range(path1.data(), path1.data() + static_cast<ptrdiff_t>(path1.size())),
				boost::make_iterator_range(path2.data(), path2.data() + static_cast<ptrdiff_t>(path2.size())));
		}
	};

	template<class StrCmp>
	PathComparator<StrCmp> path_comparator(StrCmp const &cmp) { return PathComparator<StrCmp>(cmp); }

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
			//natural_less<TCHAR> less;
			std::locale loc("");
			iless less(loc);
			try
			{
				// TODO: Compare paths case-insensitively
				switch (pLV->iSubItem)
				{
				case COLUMN_INDEX_NAME: const_pred_stable_sort(this->rows, cancellable_comparator(name_comparator(less))); break;
				case COLUMN_INDEX_PATH: const_pred_stable_sort(this->rows, cancellable_comparator(path_comparator(less))); break;
				case COLUMN_INDEX_SIZE: const_pred_stable_sort(this->rows, cancellable_comparator(comparator(boost::bind(&DataRow::size, _1)))); break;
				case COLUMN_INDEX_SIZE_ON_DISK: const_pred_stable_sort(this->rows, cancellable_comparator(comparator(boost::bind(&DataRow::sizeOnDisk, _1)))); break;
				case COLUMN_INDEX_CREATION_TIME: const_pred_stable_sort(this->rows, cancellable_comparator(comparator(boost::bind(&DataRow::creationTime, _1)))); break;
				case COLUMN_INDEX_MODIFICATION_TIME: const_pred_stable_sort(this->rows, cancellable_comparator(comparator(boost::bind(&DataRow::modificationTime, _1)))); break;
				case COLUMN_INDEX_ACCESS_TIME: const_pred_stable_sort(this->rows, cancellable_comparator(comparator(boost::bind(&DataRow::accessTime, _1)))); break;
				default: __debugbreak(); break;
				}
				if (hditem.lParam)
				{
					boost::reverse(this->rows);
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
		int const cch = this->cmbDrive.GetLBTextLen(i);
		std::basic_string<TCHAR> drive;
		if (cch >= 0)
		{
			drive.resize(cch, _T('\0'));
			if (!drive.empty()) { this->cmbDrive.GetLBText(i, &drive[0]); }
			if (!drive.empty() && !isdirsep(drive[drive.size() - 1]))
			{ drive += _T('\\'); }
		}
		return drive;
	}

	void OnCancel(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (this->CheckAndCreateIcon(false))
		{
			this->ShowWindow(SW_HIDE);
		}
	}

	BOOL CheckAndCreateIcon(bool checkVisible)
	{
		NOTIFYICONDATA nid = { sizeof(nid), *this, 0, NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_NOTIFYICON, this->GetIcon(FALSE), _T("SwiftSearch") };
		return (!checkVisible || !this->IsWindowVisible()) && Shell_NotifyIcon(NIM_ADD, &nid);
	}

	LRESULT OnTaskbarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		this->CheckAndCreateIcon(true);
		return 0;
	}

	LRESULT OnNotifyIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		if (lParam == WM_LBUTTONUP || lParam == WM_KEYUP)
		{
			this->ShowWindow(SW_SHOW);
		}
		return 0;
	}

	void RemoveDevice(HANDLE handle)
	{
		for (int i = this->cmbDrive.GetCount() - 1; i >= 0; i--)
		{
			boost::intrusive_ptr<NtfsIndexThread> const p = static_cast<NtfsIndexThread *>(this->cmbDrive.GetItemDataPtr(i));
			if (reinterpret_cast<HANDLE>(p->volume()) == handle)
			{
				int const curSel = this->cmbDrive.GetCurSel();
				if (this->cmbDrive.DeleteString(static_cast<UINT>(i)) != CB_ERR)
				{
					intrusive_ptr_release(p.get());
					HANDLE const hThread = reinterpret_cast<HANDLE>(p->thread());
					p->close();
					WaitForSingleObject(hThread, INFINITE);
					if (curSel == i)
					{ this->OnSearchParamsChange(CBN_SELCHANGE, IDC_COMBODRIVE, this->cmbDrive); }
				}
			}
		}
	}

	LRESULT OnThreadError(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
	{
		int const errorCode = static_cast<int>(wParam);
		(void)errorCode;
		std::auto_ptr<std::basic_string<TCHAR> > const msg(reinterpret_cast<std::basic_string<TCHAR> *>(lParam));
		this->statusbar.SetWindowText(msg->c_str());
		return 0;
	}

	LRESULT OnDeviceChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
	{
		switch (wParam)
		{
		case DBT_DEVICEQUERYREMOVEFAILED:
			{
				this->RefreshVolumes();
			}
			break;
		case DBT_DEVICEQUERYREMOVE:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_HANDLE)
				{ this->RemoveDevice(reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header).dbch_handle); }
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_HANDLE)
				{ this->RemoveDevice(reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header).dbch_handle); }
			}
			break;
		case DBT_DEVICEARRIVAL:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
					this->RefreshVolumes();
				}
			}
			break;
		default: break;
		}
		return TRUE;
	}

	void OnSearch(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		ATL::CComBSTR name;
		this->txtFileName.GetWindowText(*&name);

		WTL::CWaitCursor wait;

		std::basic_string<TCHAR> 
			driveLetter = this->GetCurrentDrive(),
			pattern = name ? std::basic_string<TCHAR>(name) : std::basic_string<TCHAR>();
		if (!driveLetter.empty())
		{
			bool const isRegex = pattern.find(_T('>')) == 0;
			bool isPath = isRegex || pattern.find(_T('\\')) != std::basic_string<TCHAR>::npos;
			try
			{
				bool const isRegex = !pattern.empty() && pattern[0] == _T('>');
				if (!isRegex)
				{
					if (pattern.find(_T('"')) != std::basic_string<TCHAR>::npos)
					{ replace_all(pattern, _T("\""), _T("")); }
					else if (pattern.find(_T('*')) == std::basic_string<TCHAR>::npos)
					{ pattern = _T("*") + pattern + _T("*"); }
				}
				std::auto_ptr<Matcher> const matcher(Matcher::compile(boost::make_iterator_range(pattern.data() + (isRegex ? 1 : 0), pattern.data() + pattern.size()), isRegex));

				bool const ownerData = (this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0;
				this->iconLoader->clear();
				if (ownerData) { this->lvFiles.SetItemCount(0); }
				else { this->lvFiles.DeleteAllItems(); }
				this->rows.erase(this->rows.begin(), this->rows.end());
				this->UpdateWindow();

				class MoveToForeground
				{
					bool prevCached, prevBackground;
					boost::intrusive_ptr<NtfsIndexThread> pThread;
					MoveToForeground(MoveToForeground const &) { throw std::logic_error(""); }
					MoveToForeground &operator =(MoveToForeground const &) { throw std::logic_error(""); }
				public:
					MoveToForeground(boost::intrusive_ptr<NtfsIndexThread> pThread = NULL)
						: pThread(pThread), prevBackground(pThread ? pThread->background() : false)
					{
						if (pThread)
						{ pThread->background() = false; }
					}
					~MoveToForeground()
					{
						if (pThread)
						{ pThread->background() = prevBackground; }
					}
					void swap(MoveToForeground &other)
					{
						using std::swap;
						swap(this->pThread, other.pThread);
						swap(this->prevBackground, other.prevBackground);
					}
				};
				class BlockIo
				{
					boost::intrusive_ptr<NtfsIndexThread> pThread;
					BlockIo(BlockIo const &) { throw std::logic_error(""); }
					BlockIo &operator =(BlockIo const &) { throw std::logic_error(""); }
				public:
					BlockIo(boost::intrusive_ptr<NtfsIndexThread> pThread = NULL) : pThread(pThread) { if (pThread) { ResetEvent(reinterpret_cast<HANDLE>(pThread->event())); } }
					~BlockIo() { if (pThread) { SetEvent(reinterpret_cast<HANDLE>(pThread->event())); } }
					void swap(BlockIo &other) { using std::swap; swap(this->pThread, other.pThread); }
				};
				std::vector<boost::intrusive_ptr<NtfsIndexThread> > const threads = this->GetDrives();
				boost::scoped_array<BlockIo> suspendedThreads(new BlockIo[threads.size()]);
				MoveToForeground setUncached;
				boost::intrusive_ptr<NtfsIndexThread> pThread;
				for (size_t i = 0; i < threads.size(); i++)
				{
					if (threads[i]->drive() == driveLetter) { pThread = threads[i]; }
					else { BlockIo(threads[i]).swap(suspendedThreads[i]); }
				}
				if (pThread)
				{
					MoveToForeground(pThread).swap(setUncached);
					CProgressDialog dlg(*this);
					boost::shared_ptr<NtfsIndex> index = pThread->index();
					if (!index)
					{
						dlg.SetProgressTitle(_T("Reading drive..."));
						while (!dlg.HasUserCancelled() && (index = pThread->index(), index == NULL))
						{
							CProgressDialog::WaitMessageLoop();
							if (dlg.ShouldUpdate())
							{
								TCHAR text[256];
								_stprintf(text, _T("Reading file table... %3u%% done"), static_cast<unsigned long>(100 * static_cast<unsigned long long>(pThread->progress()) / (std::numeric_limits<unsigned long>::max)()));
								dlg.SetProgressText(boost::make_iterator_range(text, text + std::char_traits<TCHAR>::length(text)));
								dlg.SetProgress(pThread->progress(), (std::numeric_limits<unsigned long>::max)());
								dlg.Flush();
							}
						}
					}
					if (index)
					{
						std::basic_stringstream<TCHAR> title;
						title << _T("Searching ") << (isPath ? _T("file paths, please wait") : _T("file names")) << _T("...");
						dlg.SetProgressTitle(title.str().c_str());
						long volatile i = 0;
						class ResizeRows
						{
							ResizeRows &operator =(ResizeRows const &) { throw std::logic_error(""); }
						public:
							long volatile nRows;
							std::vector<DataRow> &rows;
							ResizeRows(std::vector<DataRow> &rows, size_t tempSize)
								: nRows(0), rows(rows)
							{
								rows.resize(tempSize);
							}
							~ResizeRows() { rows.resize(static_cast<size_t>(nRows)); }
						} resizeRows(rows, index->size());
						winnt::NtEvent startEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
						class MatcherThread
						{
							MatcherThread &operator =(MatcherThread const &) { throw std::logic_error(""); }
						public:
							uintptr_t volatile h;
							winnt::NtEvent startEvent;
							boost::shared_ptr<NtfsIndex const> index;
							CProgressDialog &dlg;
							long volatile &i;
							bool const isPath, isRegex;
							tchar_ci_traits const traits;
							Matcher &matcher;
							std::vector<DataRow> &rows;
							bool volatile stop;
							long volatile &nRows;
							MatcherThread(boost::shared_ptr<NtfsIndex const> index, winnt::NtEvent const &startEvent, CProgressDialog &progressDlg, long volatile &i, bool isPath, bool isRegex, Matcher &matcher, std::vector<DataRow> &rows, long volatile &nRows)
								: h(NULL), startEvent(startEvent), index(index), dlg(progressDlg), i(i), isPath(isPath), isRegex(isRegex), matcher(matcher), rows(rows), stop(false), nRows(nRows)
							{ }
							~MatcherThread()
							{
								while (!dlg.HasUserCancelled())
								{
									CProgressDialog::WaitMessageLoop();
									size_t const i(static_cast<size_t>(i));
									if (i >= index->size()) { break; }
									if (dlg.ShouldUpdate())
									{
										dlg.SetProgress(i, index->size());
										dlg.SetProgressText(i < index->size() ? index->get_name_by_index(i).first : boost::as_literal(_T("")));
										dlg.Flush();
									}
								}
								this->stop = true;
								WaitForSingleObject(reinterpret_cast<HANDLE>(h), INFINITE);
							}
							static unsigned int __stdcall invoke(void *p)
							{
								return (*static_cast<MatcherThread *>(p))();
							}
							unsigned int operator()()
							{
								try
								{
									std::basic_string<TCHAR> tempPath;
									tempPath.reserve(32 * 1024);
									startEvent.NtWaitForSingleObject();
									while (!this->stop)
									{
										size_t const i(static_cast<size_t>(InterlockedIncrement(&i)) - 1);
										if (i >= index->size()) { break; }
										DataRow const row(index, i);
										boost::iterator_range<TCHAR const *> const
											fileName = row.file_name(), streamName = row.stream_name();
										bool match;
										if (isPath)
										{
											tempPath.erase(tempPath.begin(), tempPath.end());
											adddirsep(GetPath(*index, row.parent(), tempPath));
											tempPath.append(fileName.begin(), fileName.end());
											if (!streamName.empty())
											{
												tempPath.append(1, _T(':'));
												tempPath.append(streamName.begin(), streamName.end());
											}
											match = matcher(boost::make_iterator_range(tempPath.data(), tempPath.data() + static_cast<ptrdiff_t>(tempPath.size())), boost::iterator_range<TCHAR const *>());
										}
										else { match = matcher(fileName, streamName); }
										if (match) { rows[static_cast<size_t>(InterlockedIncrement(&nRows)) - 1] = row; }
									}
									return 0;
								}
								catch (CStructured_Exception &ex)
								{
									return ex.GetSENumber();
								}
							}
						};
						SYSTEM_INFO si = { };
						GetSystemInfo(&si);
						{
							boost::ptr_vector<MatcherThread> threads;
							using std::max;
							for (unsigned long k = 0; k < si.dwNumberOfProcessors - (si.dwNumberOfProcessors > 2 ? 1 : 0); k++)
							{
								std::auto_ptr<MatcherThread> p(new MatcherThread(index, startEvent, dlg, i, isPath, isRegex, *matcher.get(), rows, resizeRows.nRows));
								unsigned int tid;
								p->h = _beginthreadex(NULL, 0, &MatcherThread::invoke, p.get(), CREATE_SUSPENDED, &tid);
								if (ResumeThread(reinterpret_cast<HANDLE>(p->h)) != static_cast<DWORD>(-1))
								{ threads.push_back(p); }
							}
							SetEvent(startEvent.get());
							// 'threads' destroyed here
						}
					}
				}
				if (ownerData)
				{
					this->lvFiles.SetItemCountEx(int_cast<int>(this->rows.size()), LVSICF_NOINVALIDATEALL);
				}
				std::basic_stringstream<TCHAR> ss;
				ss << _T("Found ") << nformat(static_cast<int>(this->lvFiles.GetItemCount())) << _T(" file(s) on ") << driveLetter;
				this->statusbar.SetWindowText(ss.str().c_str());
			}
			catch (std::domain_error &ex)
			{
				std::basic_string<TCHAR> msg;
				std::copy(ex.what(), ex.what() + strlen(ex.what()), std::inserter(msg, msg.end()));
				this->MessageBox(tformat(_T("Invalid regular expression: %s\nRemove the leading '>' if you intended to use wildcards."), msg.c_str()).c_str(), _T("Invalid Regex"), MB_OK | MB_ICONERROR);
				return;
			}
		}
	}

	void DeleteNotifyIcon()
	{
		NOTIFYICONDATA nid = { sizeof(nid), *this, 0 };
		Shell_NotifyIcon(NIM_DELETE, &nid);
		SetPriorityClass(GetCurrentProcess(), 0x200000 /*PROCESS_MODE_BACKGROUND_END*/);
	}

	void OnShowWindow(BOOL bShow, UINT /*nStatus*/)
	{
		if (bShow)
		{
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			this->DeleteNotifyIcon();
			this->txtFileName.SetFocus();
		}
		else
		{
			SetPriorityClass(GetCurrentProcess(), 0x100000 /*PROCESS_MODE_BACKGROUND_BEGIN*/);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
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
		this->MessageBox(_T("© 2012 Mehrdad N.\r\nAll rights reserved."), _T("About"), MB_ICONINFORMATION);
	}

	void OnHelpRegex(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		this->MessageBox(
			_T("To find a file, select the drive you want to search, enter part of the file name or path, and click Search.\r\n\r\n")
			_T("You can either use wildcards, which are the default, or regular expressions, which require starting the pattern with a '>' character.\r\n\r\n")
			_T("Wildcards work the same as in Windows; regular expressions are implemented using the Boost.Xpressive library.\r\n\r\n")
			_T("Some common regular expressions:\r\n")
			_T(".\t= A single character\r\n")
			_T("\\+\t= A plus symbol (backslash is the escape character)\r\n")
			_T("[a-cG-K]\t= A single character from a to c or from G to K\r\n")
			_T("(abc|def)\t= Either \"abc\" or \"def\"\r\n\r\n")
			_T("\"Quantifiers\" can follow any expression:\r\n")
			_T("*\t= Zero or more occurrences\r\n")
			_T("+\t= One or more occurrences\r\n")
			_T("{m,n}\t= Between m and n occurrences (n is optional)\r\n")
			_T("Examples of regular expressions:\r\n")
			_T("Hi{2,}.*Bye= At least two occurrences of \"Hi\", followed by any number of characters, followed by \"Bye\"\r\n")
			_T(".*\t= At least zero characters\r\n")
			_T("Hi.+\\+Bye\t= At least one character between \"Hi\" and \"+Bye\"\r\n")
		, _T("Regular expressions"), MB_ICONINFORMATION);
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
		MSG_WM_PARENTNOTIFY(OnParentNotify)
		MESSAGE_HANDLER_EX(WM_THREADMESSAGE, OnThreadError)
		MESSAGE_HANDLER_EX(WM_DEVICECHANGE, OnDeviceChange)  // Don't use MSG_WM_DEVICECHANGE(); it's broken (uses DWORD)
		MESSAGE_HANDLER_EX(WM_NOTIFYICON, OnNotifyIcon)
		MESSAGE_HANDLER_EX(WM_TASKBARCREATED, OnTaskbarCreated)
		COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnClose)
		COMMAND_ID_HANDLER_EX(ID_FILE_FITCOLUMNS, OnFileFitColumns)
		COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnHelpAbout)
		COMMAND_ID_HANDLER_EX(ID_HELP_USINGREGULAREXPRESSIONS, OnHelpRegex)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
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
UINT const CMainDlg::WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));

CMainDlgBase *CMainDlgBase::create(HANDLE const hEvent) { return new CMainDlg(hEvent); }

#pragma comment(lib, "ShlWAPI.lib")
