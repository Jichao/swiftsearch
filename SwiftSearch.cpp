#include <process.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif

#include <algorithm>
#include <map>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/atomic/atomic.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

namespace WTL { using std::min; using std::max; }

#include <Windows.h>
#include <Dbt.h>
#include <ProvExce.h>
#include <ShlObj.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlmisc.h>
extern WTL::CAppModule _Module;
#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atlctrlx.h>
#include <atltheme.h>
#include <comdef.h>

#include "nformat.hpp"
#include "path.hpp"

#include "BackgroundWorker.hpp"
#include "ShellItemIDList.hpp"
#include "CModifiedDialogImpl.hpp"

#include "resource.h"

#ifndef _DEBUG
#include <boost/xpressive/xpressive_dynamic.hpp>
#endif

#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DYNAMIC_HPP_EAN BOOST_XPRESSIVE_DYNAMIC_HPP_EAN_10_04_2005
#endif

namespace std { typedef basic_string<TCHAR> tstring; }
namespace std
{
	template<class> struct is_scalar;
#ifdef _XMEMORY_
	template<class T1, class T2> struct is_scalar<std::pair<T1, T2> > : integral_constant<bool, is_pod<T1>::value && is_pod<T2>::value>{};
	template<class T1, class T2, class _Diff, class _Valty>
	inline void _Uninit_def_fill_n(std::pair<T1, T2> *_First, _Diff _Count, _Wrap_alloc<allocator<std::pair<T1, T2> > >&, _Valty *, _Scalar_ptr_iterator_tag)
	{ _Fill_n(_First, _Count, _Valty()); }
#endif
}

class mutex
{
	void init()
	{
#if defined(_WIN32)
		InitializeCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_init_lock(&p);
#endif
	}
	void term()
	{
#if defined(_WIN32)
		DeleteCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_destroy_lock(&p);
#endif
	}
public:
	mutex &operator =(mutex const &) { return *this; }
#if defined(_WIN32)
	CRITICAL_SECTION p;
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
	omp_lock_t p;
#elif defined(BOOST_THREAD_MUTEX_HPP)
	boost::mutex m;
#else
	std::mutex m;
#endif
	mutex(mutex const &) { this->init(); }
	mutex() { this->init(); }
	~mutex() { this->term(); }
	void lock()
	{
#if defined(_WIN32)
		EnterCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_set_lock(&p);
#else
		return m.lock();
#endif
	}
	void unlock()
	{
#if defined(_WIN32)
		LeaveCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		omp_unset_lock(&p);
#else
		return m.unlock();
#endif
	}
	bool try_lock()
	{
#if defined(_WIN32)
		return !!TryEnterCriticalSection(&p);
#elif defined(_OPENMP) || defined(_OMP_LOCK_T)
		return !!omp_test_lock(&p);
#else
		return m.try_lock();
#endif
	}
};
template<class Mutex>
struct lock_guard
{
	typedef Mutex mutex_type;
	mutex_type *p;
	~lock_guard() { if (p) { p->unlock(); } }
	lock_guard() : p() { }
	explicit lock_guard(mutex_type *const mutex, bool const already_locked = false) : p(mutex) { if (p && !already_locked) { p->lock(); } }
	explicit lock_guard(mutex_type &mutex, bool const already_locked = false) : p(&mutex) { if (!already_locked) { p->lock(); } }
	lock_guard(lock_guard const &other) : p(other.p) { if (p) { p->lock(); } }
	lock_guard &operator =(lock_guard other) { return this->swap(other), *this; }
	void swap(lock_guard &other) { using std::swap; swap(this->p, other.p); }
	friend void swap(lock_guard &a, lock_guard &b) { return a.swap(b); }
};

struct tchar_ci_traits : public std::char_traits<TCHAR>
{
	typedef std::char_traits<TCHAR> Base;
	static bool eq(TCHAR c1, TCHAR c2)
	{
		return c1 < SCHAR_MAX
			? c1 == c2 ||
				_T('A') <= c1 && c1 <= _T('Z') && c1 - c2 == _T('A') - _T('a') ||
				_T('A') <= c2 && c2 <= _T('Z') && c2 - c1 == _T('A') - _T('a')
			: _totupper(c1) == _totupper(c2);
	}
	static bool ne(TCHAR c1, TCHAR c2) { return !eq(c1, c2); }
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

template<class It1, class It2, class Tr>
inline bool wildcard(It1 patBegin, It1 const patEnd, It2 strBegin, It2 const strEnd, Tr const &tr = std::char_traits<typename std::iterator_traits<It2>::value_type>())
{
	(void)tr;
	if (patBegin == patEnd) { return strBegin == strEnd; }
	//http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
	It2 s(strBegin);
	It1 p(patBegin);
	bool star = false;

loop:
	for (s = strBegin, p = patBegin; s != strEnd && p != patEnd; ++s, ++p)
	{
		if (tr.eq(*p, _T('*')))
		{
			star = true;
			strBegin = s, patBegin = p;
			if (++patBegin == patEnd)
			{ return true; }
			goto loop;
		}
		else if (!tr.eq(*p, _T('?')) && !tr.eq(*s, *p))
		{
			if (!star) { return false; }
			strBegin++;
			goto loop;
		}
	}
	while (p != patEnd && tr.eq(*p, _T('*'))) { ++p; }
	return p == patEnd && s == strEnd;
}

namespace winnt
{
	struct IO_STATUS_BLOCK { union { NTSTATUS Status; PVOID Pointer; }; ULONG_PTR Information; };

	struct UNICODE_STRING
	{
		unsigned short Length, MaximumLength;
		wchar_t *Buffer;
	};

	struct OBJECT_ATTRIBUTES
	{
		unsigned long Length;
		HANDLE RootDirectory;
		UNICODE_STRING *ObjectName;
		unsigned long Attributes;
		void *SecurityDescriptor;
		void *SecurityQualityOfService;
	};

	typedef VOID NTAPI IO_APC_ROUTINE(IN PVOID ApcContext, IN IO_STATUS_BLOCK *IoStatusBlock, IN ULONG Reserved);

	enum IO_PRIORITY_HINT { IoPriorityVeryLow = 0, IoPriorityLow, IoPriorityNormal, IoPriorityHigh, IoPriorityCritical, MaxIoPriorityTypes };
	struct FILE_FS_SIZE_INFORMATION { long long TotalAllocationUnits, ActualAvailableAllocationUnits; unsigned long SectorsPerAllocationUnit, BytesPerSector; };
	struct FILE_FS_ATTRIBUTE_INFORMATION { unsigned long FileSystemAttributes; unsigned long MaximumComponentNameLength; unsigned long FileSystemNameLength; wchar_t FileSystemName[1]; };
	union FILE_IO_PRIORITY_HINT_INFORMATION { IO_PRIORITY_HINT PriorityHint; unsigned long long _alignment; };

	template<class T> struct identity { typedef T type; };
#define X(F, T) identity<T>::type &F = *reinterpret_cast<identity<T>::type *>(GetProcAddress(GetModuleHandle(_T("NTDLL.dll")), #F))
	X(NtOpenFile, NTSTATUS NTAPI(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, OUT IO_STATUS_BLOCK *IoStatusBlock, IN ULONG ShareAccess, IN ULONG OpenOptions));
	X(NtReadFile, NTSTATUS NTAPI(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN IO_APC_ROUTINE *ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL));
	X(NtQueryVolumeInformationFile, NTSTATUS NTAPI(HANDLE FileHandle, IO_STATUS_BLOCK *IoStatusBlock, PVOID FsInformation, unsigned long Length, unsigned long FsInformationClass));
	X(NtQueryInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle, OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN unsigned long FileInformationClass));
	X(NtSetInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle, OUT IO_STATUS_BLOCK *IoStatusBlock, IN PVOID FileInformation, IN ULONG Length, IN unsigned long FileInformationClass));
	X(RtlInitUnicodeString, VOID NTAPI(UNICODE_STRING * DestinationString, PCWSTR SourceString));
	X(RtlNtStatusToDosError, unsigned long NTAPI(IN NTSTATUS NtStatus));
	X(RtlSystemTimeToLocalTime, NTSTATUS NTAPI(IN LARGE_INTEGER const *SystemTime, OUT PLARGE_INTEGER LocalTime));
#undef  X
}

LONGLONG RtlSystemTimeToLocalTime(LONGLONG systemTime)
{
	LARGE_INTEGER time2, localTime;
	time2.QuadPart = systemTime;
	NTSTATUS status = winnt::RtlSystemTimeToLocalTime(&time2, &localTime);
	if (status != 0) { RaiseException(status, 0, 0, NULL); }
	return localTime.QuadPart;
}

LPCTSTR SystemTimeToString(LONGLONG systemTime, LPTSTR buffer, size_t cchBuffer, LPCTSTR dateFormat = NULL, LPCTSTR timeFormat = NULL, LCID lcid = GetThreadLocale())
{
	LONGLONG time = RtlSystemTimeToLocalTime(systemTime);
	SYSTEMTIME sysTime = { 0 };
	if (FileTimeToSystemTime(&reinterpret_cast<FILETIME &>(time), &sysTime))
	{
		size_t const cchDate = static_cast<size_t>(GetDateFormat(lcid, 0, &sysTime, dateFormat, &buffer[0], static_cast<int>(cchBuffer)));
		if (cchDate > 0)
		{
			// cchDate INCLUDES null-terminator
			buffer[cchDate - 1] = _T(' ');
			size_t const cchTime = static_cast<size_t>(GetTimeFormat(lcid, 0, &sysTime, timeFormat, &buffer[cchDate], static_cast<int>(cchBuffer - cchDate)));
			buffer[cchDate + cchTime - 1] = _T('\0');
		}
		else { memset(&buffer[0], 0, sizeof(buffer[0]) * cchBuffer); }
	}
	else { *buffer = _T('\0'); }
	return buffer;
}

namespace ntfs
{
#pragma pack(push, 1)
	struct NTFS_BOOT_SECTOR
	{
		unsigned char Jump[3];
		unsigned char Oem[8];
		unsigned short BytesPerSector;
		unsigned char SectorsPerCluster;
		unsigned short ReservedSectors;
		unsigned char Padding1[3];
		unsigned short Unused1;
		unsigned char MediaDescriptor;
		unsigned short Padding2;
		unsigned short SectorsPerTrack;
		unsigned short NumberOfHeads;
		unsigned long HiddenSectors;
		unsigned long Unused2;
		unsigned long Unused3;
		long long TotalSectors;
		long long MftStartLcn;
		long long Mft2StartLcn;
		signed char ClustersPerFileRecordSegment;
		unsigned char Padding3[3];
		unsigned long ClustersPerIndexBlock;
		long long VolumeSerialNumber;
		unsigned long Checksum;

		unsigned char BootStrap[0x200 - 0x54];
		unsigned int file_record_size() const { return this->ClustersPerFileRecordSegment >= 0 ? this->ClustersPerFileRecordSegment * this->SectorsPerCluster * this->BytesPerSector : 1U << static_cast<int>(-this->ClustersPerFileRecordSegment); }
		unsigned int cluster_size() const { return this->SectorsPerCluster * this->BytesPerSector; }
	};
#pragma pack(pop)
	struct MULTI_SECTOR_HEADER
	{
		unsigned long Magic;
		unsigned short USAOffset;
		unsigned short USACount;
		
		bool unfixup(size_t max_size)
		{
			unsigned short *usa = reinterpret_cast<unsigned short *>(&reinterpret_cast<unsigned char *>(this)[this->USAOffset]);
			bool result = true;
			for (unsigned short i = 1; i < this->USACount; i++)
			{
				const size_t offset = i * 512 - sizeof(unsigned short);
				unsigned short *const check = (unsigned short *) ((unsigned char*)this + offset);
				if (offset < max_size)
				{
					if (usa[0] != *check)
					{
						result = false;
					}
					*check = usa[i];
				}
				else { break; }
			}
			return result;
		}
	};
	enum AttributeTypeCode
	{
		AttributeStandardInformation = 0x10,
		AttributeAttributeList = 0x20,
		AttributeFileName = 0x30,
		AttributeObjectId = 0x40,
		AttributeSecurityDescriptor = 0x50,
		AttributeVolumeName = 0x60,
		AttributeVolumeInformation = 0x70,
		AttributeData = 0x80,
		AttributeIndexRoot = 0x90,
		AttributeIndexAllocation = 0xA0,
		AttributeBitmap = 0xB0,
		AttributeReparsePoint = 0xC0,
		AttributeEAInformation = 0xD0,
		AttributeEA = 0xE0,
		AttributePropertySet = 0xF0,
		AttributeLoggedUtilityStream = 0x100,
		AttributeEnd = -1,
	};
	struct ATTRIBUTE_RECORD_HEADER
	{
		AttributeTypeCode Type;
		unsigned long Length;
		unsigned char IsNonResident;
		unsigned char NameLength;
		unsigned short NameOffset;
		unsigned short Flags;
		unsigned short Instance;
		union
		{
			struct RESIDENT
			{
				unsigned long ValueLength;
				unsigned short ValueOffset;
				unsigned short Flags;
				inline void *GetValue() { return reinterpret_cast<void *>(reinterpret_cast<char *>(CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset); }
				inline void const *GetValue() const { return reinterpret_cast<const void *>(reinterpret_cast<const char *>(CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset); }
			} Resident;
			struct NONRESIDENT
			{
				unsigned long long LowestVCN;
				unsigned long long HighestVCN;
				unsigned short MappingPairsOffset;
				unsigned char CompressionUnit;
				unsigned char Reserved[5];
				long long AllocatedSize;
				long long DataSize;
				long long InitializedSize;
				long long CompressedSize;
			} NonResident;
		};
		ATTRIBUTE_RECORD_HEADER *next() { return reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<unsigned char *>(this) + this->Length); }
		ATTRIBUTE_RECORD_HEADER const *next() const { return reinterpret_cast<ATTRIBUTE_RECORD_HEADER const *>(reinterpret_cast<unsigned char const *>(this) + this->Length); }
		wchar_t *name() { return reinterpret_cast<wchar_t *>(reinterpret_cast<unsigned char *>(this) + this->NameOffset); }
		wchar_t const *name() const { return reinterpret_cast<wchar_t const *>(reinterpret_cast<unsigned char const *>(this) + this->NameOffset); }
	};
	enum FILE_RECORD_HEADER_FLAGS
	{
		FRH_IN_USE = 0x0001,    /* Record is in use */
		FRH_DIRECTORY = 0x0002,    /* Record is a directory */
	};
	struct FILE_RECORD_SEGMENT_HEADER
	{
		MULTI_SECTOR_HEADER MultiSectorHeader;
		unsigned long long LogFileSequenceNumber;
		unsigned short SequenceNumber;
		unsigned short LinkCount;
		unsigned short FirstAttributeOffset;
		unsigned short Flags /* FILE_RECORD_HEADER_FLAGS */;
		unsigned long BytesInUse;
		unsigned long BytesAllocated;
		unsigned long long BaseFileRecordSegment;
		unsigned short NextAttributeNumber;
		//http://blogs.technet.com/b/joscon/archive/2011/01/06/how-hard-links-work.aspx
		unsigned short SegmentNumberUpper_or_USA_or_UnknownReserved;  // WARNING: This does NOT seem to be the actual "upper" segment number of anything! I found it to be 0x26e on one of my drives... and checkdisk didn't say anything about it
		unsigned long SegmentNumberLower;
		ATTRIBUTE_RECORD_HEADER *begin() { return reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<unsigned char *>(this) + this->FirstAttributeOffset); }
		ATTRIBUTE_RECORD_HEADER const *begin() const { return reinterpret_cast<ATTRIBUTE_RECORD_HEADER const *>(reinterpret_cast<unsigned char const *>(this) + this->FirstAttributeOffset); }
		void *end(size_t const max_buffer_size = ~size_t()) { return reinterpret_cast<unsigned char *>(this) + (max_buffer_size < this->BytesInUse ? max_buffer_size : this->BytesInUse); }
		void const *end(size_t const max_buffer_size = ~size_t()) const { return reinterpret_cast<unsigned char const *>(this) + (max_buffer_size < this->BytesInUse ? max_buffer_size : this->BytesInUse); }
	};
	struct FILENAME_INFORMATION
	{
		unsigned long long ParentDirectory;
		long long CreationTime;
		long long LastModificationTime;
		long long LastChangeTime;
		long long LastAccessTime;
		long long AllocatedLength;
		long long FileSize;
		unsigned long FileAttributes;
		unsigned short PackedEaSize;
		unsigned short Reserved;
		unsigned char FileNameLength;
		unsigned char Flags;
		WCHAR FileName[1];
	};
	struct STANDARD_INFORMATION
	{
		long long CreationTime;
		long long LastModificationTime;
		long long LastChangeTime;
		long long LastAccessTime;
		unsigned long FileAttributes;
		// There's more, but only in newer versions
	};
	struct INDEX_HEADER
	{
		unsigned long FirstIndexEntry;
		unsigned long FirstFreeByte;
		unsigned long BytesAvailable;
		unsigned char Flags;  // '1' == has INDEX_ALLOCATION
		unsigned char Reserved[3];
	};
	struct INDEX_ROOT
	{
		AttributeTypeCode Type;
		unsigned long CollationRule;
		unsigned long BytesPerIndexBlock;
		unsigned char ClustersPerIndexBlock;
		INDEX_HEADER Header;
	};
	struct ATTRIBUTE_LIST
	{
		AttributeTypeCode AttributeType;
		unsigned short Length;
		unsigned char NameLength;
		unsigned char NameOffset;
		unsigned long long StartVcn; // LowVcn
		unsigned long long FileReferenceNumber;
		unsigned short AttributeNumber;
		unsigned short AlignmentOrReserved[3];
	};
	static TCHAR const *attribute_names[] =
	{
		NULL,
		_T("$STANDARD_INFORMATION"),
		_T("$ATTRIBUTE_LIST"),
		_T("$FILE_NAME"),
		_T("$OBJECT_ID"),
		_T("$SECURITY_DESCRIPTOR"),
		_T("$VOLUME_NAME"),
		_T("$VOLUME_INFORMATION"),
		_T("$DATA"),
		_T("$INDEX_ROOT"),
		_T("$INDEX_ALLOCATION"),
		_T("$BITMAP"),
		_T("$REPARSE_POINT"),
		_T("$EA_INFORMATION"),
		_T("$EA"),
		_T("$PROPERTY_SET"),
		_T("$LOGGED_UTILITY_STREAM"),
	};
}

std::tstring NormalizePath(std::tstring const &path)
{
	std::tstring result;
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
		std::tstring currentDir(32 * 1024, _T('\0'));
		currentDir.resize(GetCurrentDirectory(static_cast<DWORD>(currentDir.size()), &currentDir[0]));
		adddirsep(currentDir);
		result = currentDir + result;
	}
	return result;
}

std::tstring GetDisplayName(HWND hWnd, const std::tstring &path, DWORD shgdn)
{
	ATL::CComPtr<IShellFolder> desktop;
	STRRET ret;
	LPITEMIDLIST shidl;
	ATL::CComBSTR bstr;
	ULONG attrs = SFGAO_ISSLOW | SFGAO_HIDDEN;
	std::tstring result = (
		SHGetDesktopFolder(&desktop) == S_OK
		&& desktop->ParseDisplayName(hWnd, NULL, path.empty() ? NULL : const_cast<LPWSTR>(path.c_str()), NULL, &shidl, &attrs) == S_OK
		&& (attrs & SFGAO_ISSLOW) == 0
		&& desktop->GetDisplayNameOf(shidl, shgdn, &ret) == S_OK
		&& StrRetToBSTR(&ret, shidl, &bstr) == S_OK
		) ? static_cast<LPCTSTR>(bstr) : std::tstring(basename(path.begin(), path.end()), path.end());
	return result;
}

void CheckAndThrow(int const success) { if (!success) { RaiseException(GetLastError(), 0, 0, NULL); } }

LPTSTR GetAnyErrorText(DWORD errorCode, va_list* pArgList = NULL)
{
	static TCHAR buffer[1 << 15];
	ZeroMemory(buffer, sizeof(buffer));
	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | (pArgList == NULL ? FORMAT_MESSAGE_IGNORE_INSERTS : 0), NULL, errorCode, 0, buffer, sizeof(buffer) / sizeof(*buffer), pArgList))
	{
		if (!FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | (pArgList == NULL ? FORMAT_MESSAGE_IGNORE_INSERTS : 0), GetModuleHandle(_T("NTDLL.dll")), errorCode, 0, buffer, sizeof(buffer) / sizeof(*buffer), pArgList))
		{ _stprintf(buffer, _T("%#x"), errorCode); }
	}
	return buffer;
}

class Handle
{
	static bool valid(void *const value) { return value && value != reinterpret_cast<void *>(-1); }
public:
	void *value;
	Handle() : value() { }
	explicit Handle(void *const value) : value(value)
	{
		if (!valid(value))
		{ throw std::invalid_argument("invalid handle"); }
	}
	Handle(Handle const &other) : value(other.value)
	{
		if (valid(this->value))
		{ CheckAndThrow(DuplicateHandle(GetCurrentProcess(), this->value, GetCurrentProcess(), &this->value, MAXIMUM_ALLOWED, TRUE, DUPLICATE_SAME_ACCESS)); }
	}
	~Handle() { if (valid(this->value)) { CloseHandle(value); } }
	Handle &operator =(Handle other) { return other.swap(*this), *this; }
	operator void *() const { return this->value; }
	void swap(Handle &other) { using std::swap; swap(this->value, other.value); }
	friend void swap(Handle &a, Handle &b) { return a.swap(b); }
};

std::vector<char> vector_bool(std::vector<bool> const &v)
{
	std::vector<char> r;
	r.resize(v.empty() ? 0 : 1 + (v.size() - 1) / (CHAR_BIT * sizeof(*r.begin())));
#if defined(_CPPLIB_VER) && _CPPLIB_VER >= 600
	bool b = sizeof(*r.begin()) > sizeof(*v._Myvec.begin());
	if (b) { throw std::logic_error("fix buffer underflow on the next line"); }
	memcpy(r.empty() ? NULL : &*r.begin(), v.empty() ? NULL : &*v._Myvec.begin(), r.size() * sizeof(*r.begin()));
#else
	std::vector_bool_specializations<>::deprecate();
	for (size_t j = 0; j != v.size(); ++j)
	{
		size_t const k = j / (CHAR_BIT * sizeof(*r.begin()));
		r[k] = static_cast<char>(r[k] | (static_cast<char>(v[j]) << (j % (CHAR_BIT * sizeof(*r.begin())))));
	}
#endif
	return r;
}

std::vector<bool> vector_bool(std::vector<char> const &v)
{
	std::vector<bool> r(v.size() * sizeof(*v.begin()) * CHAR_BIT);
#if defined(_CPPLIB_VER) && _CPPLIB_VER >= 600
	bool b = sizeof(*v.begin()) > sizeof(*r._Myvec.begin());
	if (b) { throw std::logic_error("fix buffer overflow on the next line"); }
	memcpy(reinterpret_cast<char *>(r.empty() ? NULL : &*r._Myvec.begin()), v.empty() ? NULL : &*v.begin(), v.size() * sizeof(*v.begin()));
#else
	std::vector_bool_specializations<>::deprecate();
	for (size_t j = 0; j != v.size(); ++j)
	{
		for (size_t k = 0; k != sizeof(v[j]) * CHAR_BIT; ++k)
		{ r[sizeof(v[j]) * CHAR_BIT + k] = !!((v[j] >> k) & 1); }
	}
#endif
	return r;
}

void read(void *const file, unsigned long long const offset, void *const buffer, size_t const size, HANDLE const event_handle = NULL)
{
	if (!event_handle || event_handle == INVALID_HANDLE_VALUE)
	{
		read(file, offset, buffer, size, Handle(CreateEvent(NULL, TRUE, FALSE, NULL)));
	}
	else
	{
		OVERLAPPED overlapped = {};
		overlapped.hEvent = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(static_cast<void *>(event_handle)) | 1);
		overlapped.Offset = static_cast<unsigned int>(offset);
		overlapped.OffsetHigh = static_cast<unsigned int>(offset >> (CHAR_BIT * sizeof(overlapped.Offset)));
		unsigned long nr;
		CheckAndThrow(ReadFile(file, buffer, static_cast<unsigned long>(size), &nr, &overlapped) || (GetLastError() == ERROR_IO_PENDING && GetOverlappedResult(file, &overlapped, &nr, TRUE)));
	}
}

unsigned int get_cluster_size(void *const volume)
{
	winnt::IO_STATUS_BLOCK iosb;
	winnt::FILE_FS_SIZE_INFORMATION info = {};
	if (winnt::NtQueryVolumeInformationFile(volume, &iosb, &info, sizeof(info), 3))
	{ SetLastError(ERROR_INVALID_FUNCTION), CheckAndThrow(false); }
	return info.BytesPerSector * info.SectorsPerAllocationUnit;
}

std::vector<std::pair<unsigned long long, long long> > get_mft_retrieval_pointers(void *const volume, TCHAR const path[], long long *const size, long long const mft_start_lcn, unsigned int const cluster_size, unsigned int const file_record_size)
{
	typedef std::vector<std::pair<unsigned long long, long long> > Result;
	Result result;
	if (false)
	{
		std::vector<unsigned char> file_record(file_record_size);
		read(volume, static_cast<unsigned long long>(mft_start_lcn) * cluster_size, file_record.empty() ? NULL : &*file_record.begin(), file_record.size() * sizeof(*file_record.begin()));
		ntfs::FILE_RECORD_SEGMENT_HEADER const *const frsh = reinterpret_cast<ntfs::FILE_RECORD_SEGMENT_HEADER const *>(&*file_record.begin());
		for (ntfs::ATTRIBUTE_RECORD_HEADER const *ah = frsh->begin(); ah < frsh->end(file_record.size()) && ah->Type != ntfs::AttributeTypeCode() && ah->Type != ntfs::AttributeEnd; ah = ah->next())
		{
			if (ah->Type == ntfs::AttributeAttributeList)
			{
				if (ah->IsNonResident)
				{
					// ah->n
				}
			}
			// if (ah->Type == (data ? ntfs::AttributeData : ntfs::AttributeBitmap) && !ah->NameLength)
			{
				// TODO: Get retrieval pointers directly from volume
			}
		}
		throw std::logic_error("not implemented");
	}
	else
	{
		Handle handle;
		{
			Handle root_dir;
			{
				unsigned long long root_dir_id = 0x0005000000000005;
				winnt::UNICODE_STRING us = { sizeof(root_dir_id), sizeof(root_dir_id), reinterpret_cast<wchar_t *>(&root_dir_id) };
				winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), volume, &us };
				winnt::IO_STATUS_BLOCK iosb;
				unsigned long const error = winnt::RtlNtStatusToDosError(winnt::NtOpenFile(&root_dir.value, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0x00002000 /* FILE_OPEN_BY_FILE_ID */ | 0x00000020 /* FILE_SYNCHRONOUS_IO_NONALERT */));
				if (error) { SetLastError(error); CheckAndThrow(!error); }
			}
			{
				size_t const cch = path ? std::char_traits<TCHAR>::length(path) : 0;
				winnt::UNICODE_STRING us = { static_cast<unsigned short>(cch * sizeof(*path)), static_cast<unsigned short>(cch * sizeof(*path)), const_cast<TCHAR *>(path) };
				winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), root_dir, &us };
				winnt::IO_STATUS_BLOCK iosb;
				unsigned long const error = winnt::RtlNtStatusToDosError(winnt::NtOpenFile(&handle.value, FILE_READ_ATTRIBUTES | SYNCHRONIZE, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0x00200000 /* FILE_OPEN_REPARSE_POINT */ | 0x00000020 /* FILE_SYNCHRONOUS_IO_NONALERT */));
				if (error) { SetLastError(error); CheckAndThrow(!error); }
			}
		}
		result.resize(1 + (sizeof(RETRIEVAL_POINTERS_BUFFER) - 1) / sizeof(Result::value_type));
		STARTING_VCN_INPUT_BUFFER input = {};
		BOOL success;
		for (unsigned long nr; !(success = DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &input, sizeof(input), &*result.begin(), static_cast<unsigned long>(result.size()) * sizeof(*result.begin()), &nr, NULL), success) && GetLastError() == ERROR_MORE_DATA;)
		{
			size_t const n = result.size();
			Result(/* free old memory */).swap(result);
			Result(n * 2).swap(result);
		}
		CheckAndThrow(success);
		if (size)
		{
			LARGE_INTEGER large_size;
			CheckAndThrow(GetFileSizeEx(handle, &large_size));
			*size = large_size.QuadPart;
		}
		result.erase(result.begin() + 1 + reinterpret_cast<unsigned long const &>(*result.begin()), result.end());
		result.erase(result.begin(), result.begin() + 1);
		for (Result::iterator i = result.begin(); i != result.end(); ++i)
		{
			i->first *= cluster_size;
			i->second *= static_cast<long long>(cluster_size);
		}
	}
	return result;
}

class Overlapped : public OVERLAPPED
{
	Overlapped(Overlapped const &);
	Overlapped &operator =(Overlapped const &);
public:
	virtual ~Overlapped() { }
	explicit Overlapped() : OVERLAPPED() { }
	virtual int /* > 0 if re-queue requested, = 0 if no re-queue but no destruction, < 0 if destruction requested */ operator()(size_t const size, uintptr_t const key) = 0;
};

static clock_t const begin_time = clock();

static struct negative_one
{
	template<class T>
	operator T() const { return static_cast<T>(~T()); }
} const negative_one;

template<class Derived>
class RefCounted
{
	mutable boost::atomic<unsigned int> refs;
	friend void intrusive_ptr_add_ref(RefCounted const volatile *p) { p->refs.fetch_add(1, boost::memory_order_acq_rel); }
	friend void intrusive_ptr_release(RefCounted const volatile *p)
	{
		if (p->refs.fetch_sub(1, boost::memory_order_acq_rel) - 1 == 0)
		{
			delete static_cast<Derived const volatile *>(p);
		}
	}

protected:
	RefCounted() : refs(0) { }
	RefCounted(RefCounted const &) : refs(0) { }
	~RefCounted() { }
	RefCounted &operator =(RefCounted const &) { }
	void swap(RefCounted &) { }
};

class NtfsIndex : public RefCounted<NtfsIndex>
{
	typedef NtfsIndex this_type;
	mutable mutex _mutex;
	std::tstring _root_path;
	Handle _volume;
	typedef std::codecvt<std::tstring::value_type, char, std::mbstate_t> CodeCvt;
	NtfsIndex *unvolatile() volatile { return const_cast<NtfsIndex *>(this); }
	NtfsIndex const *unvolatile() const volatile { return const_cast<NtfsIndex *>(this); }
	std::tstring names;
#pragma pack(push)
#pragma pack(2)
	struct StandardInfo
	{
		long long created, written, accessed;
		unsigned long attributes;
	};
	friend struct std::is_scalar<StandardInfo>;
	struct NameInfo
	{
		unsigned int offset;
		unsigned char length;
	};
	friend struct std::is_scalar<NameInfo>;
	struct LinkInfo
	{
		NameInfo name;
		unsigned int parent;
	};
	friend struct std::is_scalar<LinkInfo>;
	struct StreamInfo
	{
		NameInfo name;
		TCHAR const *type_name;
		unsigned long long length, allocated;
		bool is_index;
	};
	friend struct std::is_scalar<StreamInfo>;
#pragma pack(pop)
	typedef std::vector<std::pair<LinkInfo, size_t /* next */> > LinkInfos;
	typedef std::vector<std::pair<StreamInfo, size_t /* next */> > StreamInfos;
	struct Record;
	typedef std::vector<Record> Records;
	typedef std::vector<std::pair<std::pair<Records::size_type, LinkInfos::size_type>, size_t /* next */> > ChildInfos;
	struct Record
	{
		StandardInfo stdinfo;
		unsigned short name_count, stream_count;
		LinkInfos::size_type first_name;
		StreamInfos::value_type first_stream;
		ChildInfos::value_type first_child;
	};
	friend struct std::is_scalar<Record>;
	Records records;
	LinkInfos nameinfos;
	StreamInfos streaminfos;
	ChildInfos childinfos;
	Handle _finished_event;
	bool _atomic_unfinished;
	size_t loads_so_far;
	size_t total_loads;
	size_t _total_items;
	boost::atomic<bool> _cancelled;
	boost::atomic<unsigned int> _records_so_far;

	// IF YOU ADD FIELDS: update clear()!!

	mutex *get_mutex() const
	{
		return static_cast<bool const /* WRONG USE OF VOLATILE! but it works on x86 */ volatile &>(this->_atomic_unfinished) ? &this->_mutex : NULL;
	}
public:
	unsigned int mft_record_size;
	unsigned int total_records;
	typedef std::pair<std::pair<unsigned int, unsigned short>, unsigned short> key_type;
	NtfsIndex(std::tstring value) : _finished_event(CreateEvent(NULL, TRUE, FALSE, NULL)), _atomic_unfinished(true), loads_so_far(), total_loads(), _total_items(), _records_so_far(0), _cancelled(false), mft_record_size(), total_records()
	{
		bool success = false;
		std::tstring dirsep;
		dirsep.append(1, _T('\\'));
		dirsep.append(1, _T('/'));
		try
		{
			std::tstring path_name = value;
			if (false)
			{
				path_name.erase(path_name.begin() + static_cast<ptrdiff_t>(path_name.size() - std::min(path_name.find_last_not_of(dirsep), path_name.size())), path_name.end());
				if (!path_name.empty() && *path_name.begin() != _T('\\') && *path_name.begin() != _T('/')) { path_name.insert(0, _T("\\\\.\\")); }
			}
			else
			{
				path_name.append(_T("$Volume"));
			}
			Handle(CreateFile(path_name.c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)).swap(this->_volume);
			winnt::IO_STATUS_BLOCK iosb;
			winnt::FILE_IO_PRIORITY_HINT_INFORMATION io_priority = { winnt::IoPriorityLow };
			winnt::NtSetInformationFile(this->_volume, &iosb, &io_priority, sizeof(io_priority), 43);
			success = true;
		}
		catch (std::invalid_argument &) {}
		if (success) { using std::swap; swap(this->_root_path, value); }
		if (!success) { SetEvent(this->_finished_event); }
	}
	~NtfsIndex()
	{
	}
	size_t total_items() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->total_items();
	}
	size_t total_items() const { return this->_total_items; }
	size_t records_so_far() const volatile { return this->_records_so_far.load(boost::memory_order_acquire); }
	size_t records_so_far() const { return this->_records_so_far.load(boost::memory_order_relaxed); }
	void *volume() const volatile { return this->_volume.value; }
	static Records::iterator at(Records &records, size_t const frs, Records::iterator *const existing_to_revalidate = NULL)
	{
		if (frs >= records.size())
		{
			ptrdiff_t const j = (existing_to_revalidate ? *existing_to_revalidate : records.end()) - records.begin();
			records.resize(frs + 1, empty_record());
			if (existing_to_revalidate) { *existing_to_revalidate = records.begin() + j; }
		}
		return records.begin() + static_cast<ptrdiff_t>(frs);
	}
	LinkInfos::value_type const *nameinfo(size_t const i) const { return i < this->nameinfos.size() ? &this->nameinfos[i] : NULL; }
	std::tstring const &root_path() const { return this->_root_path; }
	std::tstring const &root_path() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->root_path();
	}
	bool cancelled() const volatile
	{
		this_type const *const me = this->unvolatile();
		return me->_cancelled.load(boost::memory_order_acquire);
	}
	void cancel() volatile
	{
		this_type *const me = this->unvolatile();
		me->_cancelled.store(true, boost::memory_order_release);
	}
	bool is_finished() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->is_finished();
	}
	bool is_finished() const { return this->loads_so_far == this->total_loads; }
	uintptr_t finished_event() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->finished_event();
	}
	uintptr_t finished_event() const
	{
		return reinterpret_cast<uintptr_t>(this->_finished_event.value);
	}
	static Record empty_record()
	{
		Record empty_record = Record();
		empty_record.name_count = 0;
		empty_record.stream_count = 0;
		empty_record.stdinfo.attributes = negative_one;
		empty_record.first_stream.second = negative_one;
		empty_record.first_name = negative_one;
		empty_record.first_child.second = negative_one;
		empty_record.first_child.first.first = negative_one;
		empty_record.first_stream.first.length = negative_one;
		empty_record.first_stream.first.name.offset = negative_one;
		return empty_record;
	}
	void sort(key_type::first_type::first_type const frs, Records &new_records, LinkInfos &new_nameinfos, StreamInfos &new_streaminfos, ChildInfos &new_childinfos)
	{
		if (frs < this->records.size())
		{
			unsigned short ji = 0;
			for (LinkInfos::value_type const *j = this->nameinfo(this->records[frs].first_name); j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji)
			{
				this->sort(key_type::first_type(frs, ji), new_records, new_nameinfos, new_streaminfos, new_childinfos);
			}
		}
	}
	void sort(key_type::first_type const key_first, Records &new_records, LinkInfos &new_nameinfos, StreamInfos &new_streaminfos, ChildInfos &new_childinfos)
	{
		Records::const_iterator const fr = at(this->records, static_cast<ptrdiff_t>(key_first.first));
		Records::iterator const new_fr = at(new_records, static_cast<ptrdiff_t>(key_first.first));
		if (!~new_fr->stdinfo.attributes) { *new_fr = *fr; }
		unsigned short ii = 0;
		for (ChildInfos::value_type const *i = &fr->first_child; i && ~i->first.first; i = ~i->second ? &this->childinfos[i->second] : NULL, ++ii)
		{
			ChildInfos::iterator const new_i = (new_childinfos.push_back(*i), new_childinfos.end() - 1);
			unsigned short ji = 0;
			for (LinkInfos::value_type const *j = this->nameinfo(this->records[i->first.first].first_name); j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji)
			{
				key_type::first_type const subkey_first(static_cast<unsigned int>(i->first.first), ji);
				if (subkey_first != key_first)
				{
					this->sort(subkey_first, new_records, new_nameinfos, new_streaminfos, new_childinfos);
				}
			}
			unsigned short ki = 0;
			for (StreamInfos::value_type const *k = &fr->first_stream; k; k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki)
			{
				
			}
		}
	}
	bool check_finished()
	{
		bool const b = this->is_finished() && !this->_root_path.empty();
		if (b)
		{
			if (false)
			{
				Records new_records; new_records.reserve(this->records.size());
				LinkInfos new_nameinfos; new_nameinfos.reserve(this->nameinfos.size());
				StreamInfos new_streaminfos; new_streaminfos.reserve(this->streaminfos.size());
				ChildInfos new_childinfos; new_childinfos.reserve(this->childinfos.size());
				this->sort(0x000000000005, new_records, new_nameinfos, new_streaminfos, new_childinfos);
				// using std::swap; swap(this->records, new_records);
				// using std::swap; swap(this->nameinfos, new_nameinfos);
				// using std::swap; swap(this->streaminfos, new_streaminfos);
				// using std::swap; swap(this->childinfos, new_childinfos);
			}
			this->_atomic_unfinished = false;
			Handle().swap(this->_volume);
			_ftprintf(stderr, _T("Finished: %s (%u ms)\n"), this->_root_path.c_str(), (clock() - begin_time) * 1000U / CLOCKS_PER_SEC);
		}
		this->is_finished() ? SetEvent(this->_finished_event) : ResetEvent(this->_finished_event);
		return b;
	}
	bool set_total_loads(size_t const n) volatile
	{
		this_type *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->set_total_loads(n);
	}
	bool set_total_loads(size_t const n)
	{
		this->total_loads = n;
		return this->check_finished();
	}
	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size) volatile
	{
		this_type *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		me->load(virtual_offset, buffer, size);
	}
	void reserve(unsigned int const records)
	{
		if (this->records.size() < records)
		{
			this->nameinfos.reserve(records * 2);
			this->streaminfos.reserve(records);
			this->childinfos.reserve(records + records / 2);
			this->records.resize(records, NtfsIndex::empty_record());
		}
	}
	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size)
	{
		Record const empty_record = NtfsIndex::empty_record();
		if (size % this->mft_record_size)
		{ throw std::runtime_error("cluster size is smaller than MFT record size; split MFT records not supported"); }
		for (size_t i = virtual_offset % this->mft_record_size ? this->mft_record_size - virtual_offset % this->mft_record_size : 0; i + this->mft_record_size <= size; i += this->mft_record_size, this->_records_so_far.fetch_add(1, boost::memory_order_acq_rel))
		{
			unsigned int const frs = static_cast<unsigned int>((virtual_offset + i) / this->mft_record_size);
			ntfs::FILE_RECORD_SEGMENT_HEADER *const frsh = reinterpret_cast<ntfs::FILE_RECORD_SEGMENT_HEADER *>(&static_cast<unsigned char *>(buffer)[i]);
			if (frsh->MultiSectorHeader.Magic == 'ELIF' && frsh->MultiSectorHeader.unfixup(this->mft_record_size) && !!(frsh->Flags & ntfs::FRH_IN_USE))
			{
				unsigned int const frs_base = frsh->BaseFileRecordSegment ? static_cast<unsigned int>(frsh->BaseFileRecordSegment) : frs;
				Records::iterator base_record = at(this->records, frs_base);
				for (ntfs::ATTRIBUTE_RECORD_HEADER const
					*ah = frsh->begin(); ah < frsh->end(this->mft_record_size) && ah->Type != ntfs::AttributeTypeCode() && ah->Type != ntfs::AttributeEnd; ah = ah->next())
				{
					switch (ah->Type)
					{
					case ntfs::AttributeStandardInformation:
						if (ntfs::STANDARD_INFORMATION const *const fn = static_cast<ntfs::STANDARD_INFORMATION const *>(ah->Resident.GetValue()))
						{
							base_record->stdinfo.created = fn->CreationTime;
							base_record->stdinfo.written = fn->LastModificationTime;
							base_record->stdinfo.accessed = fn->LastAccessTime;
							base_record->stdinfo.attributes = fn->FileAttributes | ((frsh->Flags & ntfs::FRH_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0);
						}
						break;
					case ntfs::AttributeFileName:
						if (ntfs::FILENAME_INFORMATION const *const fn = static_cast<ntfs::FILENAME_INFORMATION const *>(ah->Resident.GetValue()))
						{
							unsigned int const frs_parent = static_cast<unsigned int>(fn->ParentDirectory);
							if (fn->Flags != 0x02 /* FILE_NAME_DOS */)
							{
								LinkInfo info = LinkInfo();
								info.name.offset = static_cast<unsigned int>(this->names.size());
								info.name.length = fn->FileNameLength;
								info.parent = frs_parent;
								this->names.append(fn->FileName, fn->FileNameLength);
								size_t const link_index = this->nameinfos.size();
								this->nameinfos.push_back(LinkInfos::value_type(info, base_record->first_name));
								base_record->first_name = link_index;
								Records::iterator const parent = at(this->records, frs_parent, &base_record);
								if (~parent->first_child.first.first)
								{
									size_t const ichild = this->childinfos.size();
									this->childinfos.push_back(parent->first_child);
									parent->first_child.second = ichild;
								}
								parent->first_child.first.first = frs_base;
								parent->first_child.first.second = base_record->name_count;
								this->_total_items += base_record->stream_count;
								++base_record->name_count;
							}
						}
						break;
					case ntfs::AttributeIndexRoot:
					case ntfs::AttributeData:
						if (!ah->IsNonResident || !ah->NonResident.LowestVCN)
						{
							StreamInfo info = StreamInfo();
							bool const isI30 = ah->NameLength == 4 && memcmp(ah->name(), _T("$I30"), sizeof(*ah->name()) * 4) == 0;
							if ((ah->Type == ntfs::AttributeIndexRoot || ah->Type == ntfs::AttributeIndexAllocation || ah->Type == ntfs::AttributeBitmap) && isI30)
							{
								// Suppress name
							}
							else
							{
								info.name.offset = static_cast<unsigned int>(this->names.size());
								info.name.length = ah->NameLength;
								this->names.append(ah->name(), ah->name() + ah->NameLength);
							}
							info.is_index = ah->Type == ntfs::AttributeIndexRoot && isI30;
							info.type_name = ah->Type == ntfs::AttributeData || ah->Type == ntfs::AttributeIndexRoot && isI30 ? NULL : ntfs::attribute_names[ah->Type >> (CHAR_BIT / 2)];
							info.length = ah->IsNonResident ? static_cast<unsigned long long>(frs_base == 0x000000000008 /* $BadClus */ ? ah->NonResident.InitializedSize /* actually this is still wrong... */ : ah->NonResident.DataSize) : ah->Resident.ValueLength;
							info.allocated = ah->IsNonResident ? ah->NonResident.CompressionUnit ? static_cast<unsigned long long>(ah->NonResident.CompressedSize) : static_cast<unsigned long long>(frs_base == 0x000000000008 /* $BadClus */ ? ah->NonResident.InitializedSize /* actually this is still wrong... should be looking at VCNs */ : ah->NonResident.AllocatedSize) : 0;
							if (~base_record->first_stream.first.name.offset)
							{
								size_t const stream_index = this->streaminfos.size();
								this->streaminfos.push_back(base_record->first_stream);
								base_record->first_stream.second = stream_index;
							}
							base_record->first_stream.first = info;
							this->_total_items += base_record->name_count;
							++base_record->stream_count;
						}
						break;
					}
				}
				// fprintf(stderr, "%llx\n", frsh->BaseFileRecordSegment);
			}
		}
		{
#ifdef _OPENMP
#pragma omp atomic
#endif
			++this->loads_so_far;
		}
		this->check_finished();
	}

	size_t get_path(key_type key, std::tstring &result, bool const name_only) const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->get_path(key, result, name_only);
	}

	size_t get_path(key_type key, std::tstring &result, bool const name_only) const
	{
		size_t const old_size = result.size();
		bool leaf = true;
		while (~key.first.first)
		{
			bool found = false;
			unsigned short ji = 0;
			for (LinkInfos::value_type const *j = this->nameinfo(this->records[key.first.first].first_name); !found && j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji)
			{
				if (key.first.second == (std::numeric_limits<unsigned short>::max)() || ji == key.first.second)
				{
					unsigned short ki = 0;
					for (StreamInfos::value_type const *k = &this->records[key.first.first].first_stream; !found && k; k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki)
					{
						if (key.second == (std::numeric_limits<unsigned short>::max)() ? k->first.is_index : ki == key.second)
						{
							found = true;
							size_t const old_size = result.size();
							if (key.first.first != 0x000000000005)
							{
								result.append(&this->names[j->first.name.offset], j->first.name.length);
								if (leaf)
								{
									if (k->first.name.length || k->first.type_name) { result += _T(':'); }
									result.append(&this->names[k->first.name.offset], k->first.name.length);
									if (k->first.type_name) { result += _T(':'); result.append(k->first.type_name); }
								}
								if (k->first.is_index) { result += _T('\\'); }
							}
							std::reverse(result.begin() + static_cast<ptrdiff_t>(old_size), result.end());
							key = key_type(key_type::first_type(j->first.parent /* ... | 0 | 0 (since we want the first name of all ancestors)*/, ~key_type::first_type::second_type()), ~key_type::second_type());
						}
					}
				}
			}
			if (!found)
			{
				throw std::logic_error("could not find a file attribute");
				break;
			}
			leaf = false;
			if (name_only || key.first.first == 0x000000000005) { break; }
		}
		std::reverse(result.begin() + static_cast<ptrdiff_t>(old_size), result.end());
		return result.size() - old_size;
	}

	std::pair<std::pair<unsigned long, unsigned long long>, std::pair<unsigned long long, unsigned long long> > get_stdinfo(unsigned int const frn) const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->get_stdinfo(frn);
	}

	std::pair<std::pair<unsigned long, unsigned long long>, std::pair<unsigned long long, unsigned long long> > get_stdinfo(unsigned int const frn) const
	{
		std::pair<std::pair<unsigned int, unsigned long long>, std::pair<unsigned long long, unsigned long long> > result;
		if (frn < this->records.size())
		{
			result.first.first = this->records[frn].stdinfo.attributes;
			result.first.second = this->records[frn].stdinfo.created;
			result.second.first = this->records[frn].stdinfo.written;
			result.second.second = this->records[frn].stdinfo.accessed;
		}
		return result;
	}

	template<class F>
	std::pair<unsigned long long, unsigned long long> matches(F func, std::tstring &path) const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->get_mutex());
		return me->matches<F>(func, path);
	}

	template<class F>
	std::pair<unsigned long long, unsigned long long> matches(F func, std::tstring &path) const
	{
		return this->matches<F>(func, path, 0x000000000005);
	}

	template<class F>
	std::pair<unsigned long long, unsigned long long> matches(F func, std::tstring &path, key_type::first_type::first_type const frs) const
	{
		std::pair<unsigned long long, unsigned long long> result = std::pair<unsigned long long, unsigned long long>();
		if (frs < this->records.size())
		{
			unsigned short const jn = this->records[frs].name_count;
			unsigned short ji = 0;
			for (LinkInfos::value_type const *j = this->nameinfo(this->records[frs].first_name); j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji)
			{
				std::pair<unsigned long long, unsigned long long> const subresult = this->matches(func, path, key_type::first_type(frs, ji), jn);
				result.first += subresult.first;
				result.second += subresult.second;
				++ji;
			}
		}
		return result;
	}

	template<class F>
	std::pair<unsigned long long, unsigned long long> matches(F func, std::tstring &path, key_type::first_type const key_first, unsigned short const total_names) const
	{
		std::pair<unsigned long long, unsigned long long> result;
		if (key_first.first < this->records.size())
		{
			Records::const_iterator const fr = this->records.begin() + static_cast<ptrdiff_t>(key_first.first);
			std::pair<unsigned long long, unsigned long long> children_size = std::pair<unsigned long long, unsigned long long>();
			unsigned short ii = 0;
			for (ChildInfos::value_type const *i = &fr->first_child; i && ~i->first.first; i = ~i->second ? &this->childinfos[i->second] : NULL, ++ii)
			{
				unsigned short const jn = this->records[i->first.first].name_count;
				unsigned short ji = 0;
				for (LinkInfos::value_type const *j = this->nameinfo(this->records[i->first.first].first_name); j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji)
				{
					if (j->first.parent == key_first.first && i->first.second == jn - static_cast<size_t>(1) - ji)
					{
						size_t const old_size = path.size();
						path += _T('\\');
						path.append(&this->names[j->first.name.offset], j->first.name.length);
						key_type::first_type const subkey_first(static_cast<unsigned int>(i->first.first), ji);
						if (subkey_first != key_first)
						{
							std::pair<unsigned long long, unsigned long long> const subresult = this->matches<F>(func, path, subkey_first, jn);
							children_size.first += subresult.first;
							children_size.second += subresult.second;
						}
						path.resize(old_size);
					}
				}
			}
			result = children_size;
			unsigned short ki = 0;
			for (StreamInfos::value_type const *k = &fr->first_stream; k; k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki)
			{
				size_t const old_size = path.size();
				if (k->first.name.length)
				{
					path += _T(':');
					path.append(&this->names[k->first.name.offset], k->first.name.length);
				}
				else if (fr->stdinfo.attributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					path += _T('\\');
				}
				func(
					std::pair<std::tstring::const_iterator, std::tstring::const_iterator>(path.begin(), path.end()),
					std::pair<unsigned long long, unsigned long long>((k->first.is_index ? children_size.first : 0) + k->first.length, (k->first.is_index ? children_size.second : 0) + k->first.allocated),
					key_type(key_first, ki));
				result.first += k->first.length * (key_first.second + 1) / total_names - k->first.length * key_first.second / total_names;
				result.second += k->first.allocated * (key_first.second + 1) / total_names - k->first.allocated * key_first.second / total_names;
				path.resize(old_size);
			}
		}
		return result;
	}
};

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


namespace std
{
#ifdef _XMEMORY_
#define X(...) template<> struct is_scalar<__VA_ARGS__> : is_pod<__VA_ARGS__>{}
	X(NtfsIndex::StandardInfo);
	X(NtfsIndex::NameInfo);
	X(NtfsIndex::StreamInfo);
	X(NtfsIndex::LinkInfo);
	X(NtfsIndex::Record);
#undef X
#endif
}

class CoInit
{
	CoInit(CoInit const &) : hr(S_FALSE) { }
	CoInit &operator =(CoInit const &) { }
public:
	HRESULT hr;
	CoInit(bool initialize = true) : hr(initialize ? CoInitialize(NULL) : S_FALSE) { }
	~CoInit() { if (this->hr == S_OK) { CoUninitialize(); } }
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
	bool logical;
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
	iless(std::locale const &loc, bool const logical) : ctype(static_cast<std::ctype<TCHAR> const &>(use_facet<std::ctype<TCHAR> >(loc))), logical(logical) {}
	bool operator()(boost::iterator_range<TCHAR const *> const a, boost::iterator_range<TCHAR const *> const b) const
	{
		s1.assign(a.begin(), a.end());
		s2.assign(b.begin(), b.end());
		return logical ? StrCmpLogicalW(s1.c_str(), s2.c_str()) < 0 : std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(), iless_ch(*this));
	}
};

template<class It, class ItBuf, class Pred>
void mergesort_level(It const begin, ptrdiff_t const n, ItBuf const buf, Pred comp, bool const in_buf, ptrdiff_t const m, ptrdiff_t const j)
{
	using std::merge;
	using std::min;
	if (in_buf)
	{
		merge(
			buf + min(n, j), buf + min(n, j + m),
			buf + min(n, j + m), buf + min(n, j + m + m),
			begin + min(n, j),
			comp);
	}
	else
	{
		merge(
			begin + min(n, j), begin + min(n, j + m),
			begin + min(n, j + m), begin + min(n, j + m + m),
			buf + min(n, j),
			comp);
	}
}

template<class It, class ItBuf, class Pred>
bool mergesort(It const begin, It const end, ItBuf const buf, Pred comp, bool const parallel)  // MUST check the return value!
{
	bool in_buf = false;
	for (ptrdiff_t m = 1, n = end - begin; m < n; in_buf = !in_buf, m += m)
	{
		ptrdiff_t const k = n + n - m;
#define X() for (ptrdiff_t j = 0; j < k; j += m + m) { mergesort_level<It, ItBuf, Pred>(begin, n, buf, comp, in_buf, m, j); }
		if (parallel)
		{
#ifdef _OPENMP
#pragma omp parallel for
#endif
			X();
		}
		else
		{
			X();
		}
#undef X
	}
	// if (in_buf) { using std::swap_ranges; swap_ranges(begin, end, buf), in_buf = !in_buf; }
	return in_buf;
}

template<class It, class Pred>
void inplace_mergesort(It const begin, It const end, Pred const &pred, bool const parallel)
{
	class Buffer
	{
		Buffer(Buffer const &) { }
		void operator =(Buffer const &) { }
		typedef typename std::iterator_traits<It>::value_type value_type;
		typedef value_type *pointer;
		typedef size_t size_type;
		pointer p;
	public:
		typedef pointer iterator;
		~Buffer() { delete [] this->p; }
		explicit Buffer(size_type const n) : p(new value_type[n]) { }
		iterator begin() { return this->p; }
	} buf(static_cast<size_t>(std::distance(begin, end)));
	if (mergesort<It, typename Buffer::iterator, Pred>(begin, end, buf.begin(), pred, parallel))
	{
		using std::swap_ranges; swap_ranges(begin, end, buf.begin());
	}
}

class CProgressDialog : private CModifiedDialogImpl<CProgressDialog>, private WTL::CDialogResize<CProgressDialog>
{
	typedef CProgressDialog This;
	typedef CModifiedDialogImpl<CProgressDialog> Base;
	friend CDialogResize<This>;
	friend CDialogImpl<This>;
	friend CModifiedDialogImpl<This>;
	enum { IDD = IDD_DIALOGPROGRESS, BACKGROUND_COLOR = COLOR_WINDOW };
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
		// SetClassLongPtr(this->m_hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
		this->progressBar.Attach(this->GetDlgItem(IDC_PROGRESS1));
		this->DlgResize_Init(false, false, 0);
		ATL::CComBSTR bstr;
		this->progressText.GetWindowText(&bstr);
		this->lastProgressText = bstr;

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

	BOOL OnEraseBkgnd(WTL::CDCHandle dc)
	{
		RECT rc = {};
		this->GetClientRect(&rc);
		dc.FillRect(&rc, BACKGROUND_COLOR);
		return TRUE;
	}

	HBRUSH OnCtlColorStatic(WTL::CDCHandle dc, WTL::CStatic /*wndStatic*/)
	{
		return GetSysColorBrush(BACKGROUND_COLOR);
	}

	BEGIN_MSG_MAP_EX(This)
		CHAIN_MSG_MAP(CDialogResize<This>)
		MSG_WM_INITDIALOG(OnInitDialog)
		// MSG_WM_ERASEBKGND(OnEraseBkgnd)
		// MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
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

	unsigned long WaitMessageLoop(HANDLE const handles[], size_t const nhandles)
	{
		for (;;)
		{
			unsigned long result = MsgWaitForMultipleObjectsEx(static_cast<unsigned int>(nhandles), handles, UPDATE_INTERVAL, QS_ALLINPUT, MWMO_INPUTAVAILABLE);
			if (WAIT_OBJECT_0 <= result && result < WAIT_OBJECT_0 + nhandles || result == WAIT_TIMEOUT)
			{ return result; }
			else if (result == WAIT_OBJECT_0 + static_cast<unsigned int>(nhandles))
			{
				this->ProcessMessages();
			}
			else
			{
				RaiseException(GetLastError(), 0, 0, NULL);
			}
		}
	}

	DWORD GetMinDelay() const
	{
		return IsDebuggerPresent() ? 0 : 400;
	}

public:
	enum { UPDATE_INTERVAL = 20 };
	CProgressDialog(ATL::CWindow parent)
		: Base(true), parent(parent), lastUpdateTime(0), creationTime(GetTickCount()), lastProgress(0), lastProgressTotal(1), invalidated(false), canceled(false), windowCreated(false)
	{
	}

	~CProgressDialog()
	{
		if (this->windowCreated)
		{
			::EnableWindow(parent, TRUE);
			this->DestroyWindow();
		}
	}

	unsigned long Elapsed() const { return GetTickCount() - this->lastUpdateTime; }

	bool ShouldUpdate() const
	{
		return this->Elapsed() >= UPDATE_INTERVAL;
	}

	void ProcessMessages()
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
			{
				this->canceled = true;
			}
		}
	}

	bool HasUserCancelled()
	{
		bool justCreated = false;
		unsigned long const now = GetTickCount();
		if (!this->windowCreated && abs(static_cast<int>(now - this->creationTime)) >= static_cast<int>(this->GetMinDelay()))
		{
			this->windowCreated = !!this->Create(parent);
			::EnableWindow(parent, FALSE);
			this->Flush();
		}
		if (this->windowCreated && (justCreated || this->ShouldUpdate()))
		{
			this->ProcessMessages();
		}
		return this->canceled;
	}

	void Flush()
	{
		if (this->invalidated)
		{
			if (this->windowCreated)
			{
				this->invalidated = false;
				this->SetWindowText(this->lastProgressTitle.c_str());
				this->progressBar.SetRange32(0, this->lastProgressTotal);
				this->progressBar.SetPos(this->lastProgress);
				this->progressText.SetWindowText(this->lastProgressText.c_str());
				this->progressBar.UpdateWindow();
				this->progressText.UpdateWindow();
				this->UpdateWindow();
			}
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

class CMainDlg : public CModifiedDialogImpl<CMainDlg>, public WTL::CDialogResize<CMainDlg>, public CInvokeImpl<CMainDlg>, private WTL::CMessageFilter
{
	typedef std::vector<std::pair<unsigned long long, long long> > RetPtrs;
	enum { IDC_STATUS_BAR = 1100 + 0 };
	enum { COLUMN_INDEX_NAME, COLUMN_INDEX_PATH, COLUMN_INDEX_SIZE, COLUMN_INDEX_SIZE_ON_DISK, COLUMN_INDEX_CREATION_TIME, COLUMN_INDEX_MODIFICATION_TIME, COLUMN_INDEX_ACCESS_TIME };
	struct CThemedListViewCtrl : public WTL::CListViewCtrl, public WTL::CThemeImpl<CThemedListViewCtrl> { using WTL::CListViewCtrl::Attach; };
	class Threads : public std::vector<uintptr_t>
	{
		Threads(Threads const &) { }
		Threads &operator =(Threads const &) { return *this; }
	public:
		Threads() { }
		~Threads()
		{
			while (!this->empty())
			{
				size_t n = std::min(this->size(), static_cast<size_t>(MAXIMUM_WAIT_OBJECTS));
				WaitForMultipleObjects(static_cast<unsigned long>(n), reinterpret_cast<void *const *>(&*this->begin() + this->size() - n), TRUE, INFINITE);
				while (n) { this->pop_back(); --n; }
			}
		}
	};
	class OverlappedNtfsMftRead : public Overlapped
	{
		class Buffer
		{
			Buffer(Buffer const &);
			Buffer &operator =(Buffer const &);
			void *p;
			size_t n;
		public:
			~Buffer() { if (p) { operator delete(p); } }
			Buffer() : p(), n() { }
			explicit Buffer(size_t const n) : p(operator new(n)), n(n) { }
			void *data() { return this->p; }
			size_t size() const { return this->n; }
			void swap(Buffer &other) { using std::swap; swap(this->p, other.p); swap(this->n, other.n); }
		};
		std::tstring path_name;
		Handle iocp;
		HWND m_hWnd;
		Handle closing_event;
		unsigned int cluster_size;
		unsigned int num_reads;
		boost::atomic<unsigned int> records_so_far;
		unsigned long long virtual_offset, next_virtual_offset;
		OVERLAPPED next_overlapped;
		RetPtrs ret_ptrs;
		RetPtrs::const_iterator j;
		boost::intrusive_ptr<NtfsIndex> p;
		Buffer buf;
	public:
		explicit OverlappedNtfsMftRead(Handle const &iocp, std::tstring const &path_name, HWND const m_hWnd, Handle const &closing_event)
			: Overlapped(), path_name(path_name), iocp(iocp), m_hWnd(m_hWnd), closing_event(closing_event), cluster_size(), num_reads(), records_so_far(0), virtual_offset(), next_virtual_offset(), next_overlapped(), ret_ptrs(), j(ret_ptrs.begin()) {}
		~OverlappedNtfsMftRead()
		{
		}
		int operator()(size_t const size, uintptr_t const key)
		{
			int result = -1;
			if (!this->p)
			{
				this->p.reset(new NtfsIndex(path_name));
				if (void *const volume = this->p->volume())
				{
					if (::PostMessage(m_hWnd, WM_DRIVESETITEMDATA, static_cast<WPARAM>(key), reinterpret_cast<LPARAM>(&*this->p))) { intrusive_ptr_add_ref(this->p.get()); }
					DEV_BROADCAST_HANDLE dev = { sizeof(dev), DBT_DEVTYP_HANDLE, 0, this->p->volume(), reinterpret_cast<HDEVNOTIFY>(m_hWnd) };
					dev.dbch_hdevnotify = RegisterDeviceNotification(m_hWnd, &dev, DEVICE_NOTIFY_WINDOW_HANDLE);
					long long mft_start_lcn;
					{
						unsigned long br;
						NTFS_VOLUME_DATA_BUFFER volume_data;
						CheckAndThrow(DeviceIoControl(this->p->volume(), FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &volume_data, sizeof(volume_data), &br, NULL));
						this->cluster_size = static_cast<unsigned int>(volume_data.BytesPerCluster);
						mft_start_lcn = volume_data.MftStartLcn.QuadPart;
						this->p->mft_record_size = volume_data.BytesPerFileRecordSegment;
						this->p->total_records = static_cast<unsigned int>(volume_data.MftValidDataLength.QuadPart / this->p->mft_record_size);
					}
					CheckAndThrow(!!CreateIoCompletionPort(this->p->volume(), iocp, reinterpret_cast<uintptr_t>(&*this->p), 0));
					long long llsize = 0;
					get_mft_retrieval_pointers(this->p->volume(), _T("$MFT::$DATA"), &llsize, mft_start_lcn, this->cluster_size, this->p->mft_record_size).swap(this->ret_ptrs);
					this->j = this->ret_ptrs.begin();
					this->p->reserve(this->p->total_records);
					result = +1;
				}
				else
				{
					::PostMessage(m_hWnd, WM_DRIVESETITEMDATA, static_cast<WPARAM>(key), NULL);
				}
			}
			if (this->p)
			{
				Buffer old_buffer; old_buffer.swap(this->buf);
				unsigned long long const old_virtual_offset = this->virtual_offset;
				if (size)
				{
					static_cast<NtfsIndex volatile *>(this->p.get())->load(old_virtual_offset, old_buffer.data(), size);
					this->records_so_far.fetch_add(static_cast<unsigned int>(size / this->p->mft_record_size));
				}
				this->virtual_offset = this->next_virtual_offset;
				while (j != this->ret_ptrs.end() && this->virtual_offset >= j->first)
				{
					++j;
				}
				bool const more = j != this->ret_ptrs.end() && WaitForSingleObject(this->closing_event, 0) != WAIT_OBJECT_0 && !this->p->cancelled();
				if (more)
				{
					long long const lbn = j->second + (this->virtual_offset - (j == this->ret_ptrs.begin() ? 0 : (j - 1)->first));
					this->OVERLAPPED::Offset = static_cast<unsigned long>(lbn);
					this->OVERLAPPED::OffsetHigh = static_cast<unsigned long>(lbn >> (CHAR_BIT * sizeof(this->OVERLAPPED::Offset)));
					unsigned long const nb = static_cast<unsigned long>(std::min(std::max(1ULL * cluster_size, 1ULL << 20), j->first - this->virtual_offset));
					Buffer(nb).swap(this->buf);
					++this->num_reads;
					if (ReadFile(this->p->volume(), this->buf.data(), nb, NULL, this))
					{
						/* Completed synchronously... call the method */
						result = +1;
					}
					else if (GetLastError() == ERROR_IO_PENDING)
					{ result = 0; }
					else { CheckAndThrow(false); }
					this->next_virtual_offset = this->virtual_offset + nb;
				}
				if (!more)
				{
					this->p->set_total_loads(this->num_reads);
				}
			}
			if (this->p->cancelled() && result > 0)
			{
				result = -1;
			}
			return result;
		}
	};
	static unsigned long get_num_threads()
	{
		unsigned long num_threads = 0;
#ifdef _OPENMP
#pragma omp parallel
#endif
		{
#ifdef _OPENMP
#pragma omp atomic
#endif
			++num_threads;
		}
		return num_threads;
	}
	static unsigned int CALLBACK iocp_worker(void *iocp)
	{
		ULONG_PTR key;
		OVERLAPPED *overlapped_ptr;
		for (unsigned long nr; GetQueuedCompletionStatus(iocp, &nr, &key, &overlapped_ptr, INFINITE);)
		{
			std::auto_ptr<Overlapped> overlapped(static_cast<Overlapped *>(overlapped_ptr));
			if (overlapped.get())
			{
				int r = (*overlapped)(static_cast<size_t>(nr), key);
				if (r > 0) { r = PostQueuedCompletionStatus(iocp, nr, key, overlapped_ptr) ? 0 : -1; }
				if (r >= 0) { overlapped.release(); }
			}
			else if (!key) { break; }
		}
		return 0;
	}

	template<typename TWindow>
	class CUpDownNotify : public ATL::CWindowImpl<CUpDownNotify<TWindow>, TWindow>
	{
		BEGIN_MSG_MAP_EX(CCustomDialogCode)
			MESSAGE_RANGE_HANDLER_EX(WM_KEYDOWN, WM_KEYUP, OnKey)
		END_MSG_MAP()

	public:
		struct KeyNotify { NMHDR hdr; WPARAM vkey; LPARAM lParam; };
		enum { CUN_KEYDOWN, CUN_KEYUP };

		LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			if ((wParam == VK_UP || wParam == VK_DOWN) || (wParam == VK_PRIOR || wParam == VK_NEXT))
			{
				int id = this->GetWindowLong(GWL_ID);
				KeyNotify msg = { { *this, id, uMsg == WM_KEYUP ? CUN_KEYUP : CUN_KEYDOWN }, wParam, lParam };
				HWND hWndParent = this->GetParent();
				return hWndParent == NULL || this->SendMessage(hWndParent, WM_NOTIFY, id, (LPARAM)&msg) ? this->DefWindowProc(uMsg, wParam, lParam) : 0;
			}
			else { return this->DefWindowProc(uMsg, wParam, lParam); }
		}
	};
	struct CacheInfo
	{
		CacheInfo() : valid(false), iIconSmall(-1), iIconLarge(-1), iIconExtraLarge(-1) { this->szTypeName[0] = _T('\0'); }
		bool valid;
		int iIconSmall, iIconLarge, iIconExtraLarge;
		TCHAR szTypeName[80];
		std::tstring description;
	};
	static unsigned int const WM_TASKBARCREATED;
	enum { WM_NOTIFYICON = WM_USER + 100, WM_DRIVESETITEMDATA = WM_USER + 101 };

	typedef std::vector<std::pair<boost::intrusive_ptr<NtfsIndex volatile const>, std::pair<NtfsIndex::key_type, std::pair<unsigned long long, unsigned long long> > > > Results;
	
	template<class StrCmp>
	class NameComparator
	{
		StrCmp less;
	public:
		NameComparator(StrCmp const &less) : less(less) { }
		bool operator()(Results::value_type const &a, Results::value_type const &b)
		{
			bool less = this->less(a.file_name(), b.file_name());
			if (!less && !this->less(b.file_name(), a.file_name()))
			{ less = this->less(a.stream_name(), b.stream_name()); }
			return less;
		}
		bool operator()(Results::value_type const &a, Results::value_type const &b) const
		{
			bool less = this->less(a.file_name(), b.file_name());
			if (!less && !this->less(b.file_name(), a.file_name()))
			{ less = this->less(a.stream_name(), b.stream_name()); }
			return less;
		}
	};

	class RaiseIoPriority
	{
		HANDLE const *wait_volumes;
		size_t wait_handles_size;
		std::vector<winnt::FILE_IO_PRIORITY_HINT_INFORMATION> old_io_priorities;
		RaiseIoPriority(RaiseIoPriority const &);
		RaiseIoPriority &operator =(RaiseIoPriority const &);
	public:
		explicit RaiseIoPriority(HANDLE const wait_volumes[], size_t const wait_handles_size)
			: wait_volumes(wait_volumes), wait_handles_size(wait_handles_size), old_io_priorities(wait_handles_size)
		{
			for (size_t i = 0; i != this->wait_handles_size; ++i)
			{
				winnt::IO_STATUS_BLOCK iosb;
				winnt::NtQueryInformationFile(wait_volumes[i], &iosb, &old_io_priorities[i], sizeof(old_io_priorities[i]), 43);
				winnt::FILE_IO_PRIORITY_HINT_INFORMATION io_priority = { winnt::IoPriorityNormal };
				winnt::NtSetInformationFile(wait_volumes[i], &iosb, &io_priority, sizeof(io_priority), 43);
			}
		}
		~RaiseIoPriority()
		{
			for (size_t i = 0; i != this->wait_handles_size; ++i)
			{
				winnt::IO_STATUS_BLOCK iosb;
				winnt::NtSetInformationFile(wait_volumes[i], &iosb, &old_io_priorities[i], sizeof(old_io_priorities[i]), 43);
			}
		}
	};

	template<class StrCmp>
	static NameComparator<StrCmp> name_comparator(StrCmp const &cmp) { return NameComparator<StrCmp>(cmp); }

	size_t num_threads;
	CUpDownNotify<WTL::CEdit> txtPattern;
	WTL::CRichEditCtrl richEdit;
	WTL::CStatusBarCtrl statusbar;
	WTL::CAccelerator accel;
	std::map<std::tstring, CacheInfo> cache;
	std::tstring lastRequestedIcon;
	HANDLE hRichEdit;
	Results results;
	WTL::CImageList imgListSmall, imgListLarge, imgListExtraLarge;
	WTL::CComboBox cmbDrive;
	CThemedListViewCtrl lvFiles;
	int lastSortColumn, lastSortIsDescending;
	boost::shared_ptr<BackgroundWorker> iconLoader;
	Handle closing_event;
	Handle iocp;
	Threads threads;
	std::locale loc;
	static DWORD WINAPI SHOpenFolderAndSelectItemsThread(IN LPVOID lpParameter)
	{
		std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > > p(
			static_cast<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > *>(lpParameter));
		// This is in a separate thread because of a BUG:
		// Try this with RmMetadata:
		// 1. Double-click it.
		// 2. Press OK when the error comes up.
		// 3. Now you can't access the main window, because SHOpenFolderAndSelectItems() hasn't returned!
		// So we put this in a separate thread to solve that problem.

		CoInit coInit;
		std::vector<LPCITEMIDLIST> relative_item_ids(p->second.size());
		for (size_t i = 0; i < p->second.size(); ++i)
		{
			relative_item_ids[i] = ILFindChild(p->first.first, p->second[i]);
		}
		return SHOpenFolderAndSelectItems(p->first.first, static_cast<UINT>(relative_item_ids.size()), relative_item_ids.empty() ? NULL : &relative_item_ids[0], 0);
	}
public:
	CMainDlg() : num_threads(static_cast<size_t>(get_num_threads())), lastSortIsDescending(-1), lastSortColumn(-1), closing_event(CreateEvent(NULL, TRUE, FALSE, NULL)), iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)), threads(), loc(""), iconLoader(BackgroundWorker::create(true)), lastRequestedIcon(), hRichEdit(LoadLibrary(_T("riched20.dll")))
	{
	}
	void OnDestroy()
	{
		this->DeleteNotifyIcon();
		this->iconLoader->clear();
		for (size_t i = 0; i != this->threads.size(); ++i)
		{
			PostQueuedCompletionStatus(this->iocp, 0, 0, NULL);
		}
	}

	struct IconLoaderCallback
	{
		CMainDlg *this_;
		std::tstring path;
		SIZE size;
		unsigned long fileAttributes;
		int iItem;

		struct MainThreadCallback
		{
			CMainDlg *this_;
			std::tstring description, path;
			SHFILEINFO shfi;
			int iItem;
			bool operator()()
			{
				WTL::CWaitCursor wait(true, IDC_APPSTARTING);
				CMainDlg::CacheInfo &cached = this_->cache[path];
				WTL::CIcon iconSmall(shfi.hIcon);
				_tcscpy(cached.szTypeName, shfi.szTypeName);
				cached.description = description;

				if (cached.iIconSmall < 0) { cached.iIconSmall = this_->imgListSmall.AddIcon(iconSmall); }
				else { this_->imgListSmall.ReplaceIcon(cached.iIconSmall, iconSmall); }

				if (cached.iIconLarge < 0) { cached.iIconLarge = this_->imgListLarge.AddIcon(iconSmall); }
				else { this_->imgListLarge.ReplaceIcon(cached.iIconLarge, iconSmall); }

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
			this_->lvFiles.GetItemRect(iItem, &rcItem, LVIR_BOUNDS);  // Blocks, but I'm hoping it's fast...
			if (IntersectRect(&intersection, &rcFiles, &rcItem))
			{
				std::tstring const normalizedPath = NormalizePath(path);
				SHFILEINFO shfi = {0};
				std::tstring description;
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
						{ description = std::tstring((LPCTSTR)p, uLen); }
					}
				}
#endif
				Handle fileTemp;  // To prevent icon retrieval from changing the file time
				{
					std::tstring ntpath = _T("\\??\\") + path;
					winnt::UNICODE_STRING us = { static_cast<unsigned short>(ntpath.size() * sizeof(*ntpath.begin())), static_cast<unsigned short>(ntpath.size() * sizeof(*ntpath.begin())), ntpath.empty() ? NULL : &*ntpath.begin() };
					winnt::OBJECT_ATTRIBUTES oa = { sizeof(oa), NULL, &us };
					winnt::IO_STATUS_BLOCK iosb;
					if (winnt::NtOpenFile(&fileTemp.value, FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, &oa, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, 0x00200000 /* FILE_OPEN_REPARSE_POINT */ | 0x00000008 /*FILE_NO_INTERMEDIATE_BUFFERING*/) == 0)
					{
						FILETIME preserve = { ULONG_MAX, ULONG_MAX };
						SetFileTime(fileTemp, NULL, &preserve, NULL);
					}
				}
				BOOL success = FALSE;
				SetLastError(0);
				if (shfi.hIcon == NULL)
				{
					ULONG const flags = SHGFI_ICON | SHGFI_SHELLICONSIZE | SHGFI_ADDOVERLAYS | //SHGFI_TYPENAME | SHGFI_SYSICONINDEX |
						((std::max)(size.cx, size.cy) <= (std::max)(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) ? SHGFI_SMALLICON : SHGFI_LARGEICON);
					// CoInit com;  // MANDATORY!  Some files, like '.sln' files, won't work without it!
					success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi), flags) != 0;
					if (!success && (flags & SHGFI_USEFILEATTRIBUTES) == 0)
					{ success = SHGetFileInfo(normalizedPath.c_str(), fileAttributes, &shfi, sizeof(shfi), flags | SHGFI_USEFILEATTRIBUTES) != 0; }
				}

				if (success)
				{
					std::tstring const path(path);
					int const iItem(iItem);
					MainThreadCallback callback = { this_, description, path, shfi, iItem };
					this_->InvokeAsync(callback);
				}
				else
				{
					_ftprintf(stderr, _T("Could not get the icon for file: %s\n"), normalizedPath.c_str());
				}
			}
			return true;
		}
	};

	static void remove_path_stream_and_trailing_sep(std::tstring &path)
	{
		for (;;)
		{
			size_t colon_or_sep = path.find_last_of(_T("\\:"));
			if (!~colon_or_sep || path[colon_or_sep] != _T(':')) { break; }
			path.erase(colon_or_sep, path.size() - colon_or_sep);
		}
		while (!path.empty() && isdirsep(*(path.end() - 1)) && path.find_first_of(_T('\\')) != path.size() - 1)
		{
			path.erase(path.end() - 1);
		}
	}

	int CacheIcon(std::tstring path, int const iItem, ULONG fileAttributes, bool lifo)
	{
		remove_path_stream_and_trailing_sep(path);
		if (this->cache.find(path) == this->cache.end())
		{
			this->cache[path] = CacheInfo();
		}

		CacheInfo const &entry = this->cache[path];

		if (!entry.valid && this->lastRequestedIcon != path)
		{
			SIZE size; this->lvFiles.GetImageList(LVSIL_SMALL).GetIconSize(size);

			IconLoaderCallback callback = { this, path, size, fileAttributes, iItem };
			this->iconLoader->add(callback, lifo);
			this->lastRequestedIcon = path;
		}
		return entry.iIconSmall;
	}
	
	LRESULT OnMouseWheel(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		return this->lvFiles.SendMessage(uMsg, wParam, lParam);
	}

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		_Module.GetMessageLoop()->AddMessageFilter(this);

		this->txtPattern.SubclassWindow(this->GetDlgItem(IDC_EDITFILENAME));
		this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
		this->cmbDrive.Attach(this->GetDlgItem(IDC_LISTVOLUMES));
		this->richEdit.Create(this->lvFiles, NULL, 0, ES_MULTILINE, WS_EX_TRANSPARENT);
		this->richEdit.SetFont(this->lvFiles.GetFont());
		this->accel.LoadAccelerators(IDR_ACCELERATOR1);
		this->txtPattern.SetCueBannerText(_T("File name or path"));
		// this->txtPattern.SetWindowText(_T("*"));
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT,  200, _T("Name") }; this->lvFiles.InsertColumn(0, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT,  340, _T("Directory") }; this->lvFiles.InsertColumn(1, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_RIGHT, 100, _T("Size") }; this->lvFiles.InsertColumn(2, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_RIGHT, 100, _T("Size on Disk") }; this->lvFiles.InsertColumn(3, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Created") }; this->lvFiles.InsertColumn(4, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Written") }; this->lvFiles.InsertColumn(5, &column); }
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Accessed") }; this->lvFiles.InsertColumn(6, &column); }

		this->cmbDrive.SetCueBannerText(_T("Search where?"));
		HINSTANCE hInstance = GetModuleHandle(NULL);
		this->SetIcon((HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0), FALSE);
		this->SetIcon((HICON) LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0), TRUE);

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

		this->lvFiles.OpenThemeData(VSCLASS_LISTVIEW);
		SetWindowTheme(this->lvFiles, _T("Explorer"), NULL);
		{
			WTL::CFontHandle font = this->txtPattern.GetFont();
			LOGFONT logFont;
			if (font.GetLogFont(logFont))
			{
				logFont.lfHeight = logFont.lfHeight * 100 / 85;
				this->txtPattern.SetFont(WTL::CFontHandle().CreateFontIndirect(&logFont));
			}
		}
		this->lvFiles.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | LVS_EX_GRIDLINES | LVS_EX_COLUMNOVERFLOW);
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
		this->statusbar.SetText(0, _T("Type in a file name and press Enter."));
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

		this->DlgResize_Init(false, false);
		// this->SetTimer(0, 15 * 60 * 60, );

		std::vector<std::tstring> path_names;
		{
			std::tstring buf;
			size_t prev;
			do
			{
				prev = buf.size();
				buf.resize(std::max(static_cast<size_t>(GetLogicalDriveStrings(static_cast<unsigned long>(buf.size()), buf.empty() ? NULL : &*buf.begin())), buf.size()));
			} while (prev < buf.size());
			for (size_t i = 0, n; n = _tcsnlen(&buf[i], buf.size() - i), i < buf.size() && buf[i]; i += n + 1)
			{
				path_names.push_back(std::tstring(&buf[i], n));
			}
		}

		this->cmbDrive.SetCurSel(this->cmbDrive.AddString(_T("(All drives)")));
		for (size_t j = 0; j != path_names.size(); ++j)
		{
			// Do NOT capture 'this'!! It is volatile!!
			typedef OverlappedNtfsMftRead T;
			int const index = this->cmbDrive.AddString(path_names[j].c_str());
			std::auto_ptr<T> p(new T(this->iocp, path_names[j], this->m_hWnd, this->closing_event));
			if (PostQueuedCompletionStatus(this->iocp, 0, static_cast<uintptr_t>(index), &*p)) { p.release(); }
		}

		for (size_t i = 0; i != this->num_threads; ++i)
		{
			unsigned int id;
			this->threads.push_back(_beginthreadex(NULL, 0, iocp_worker, this->iocp, 0, &id));
		}
		return TRUE;
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
			try
			{
				std::vector<std::pair<std::tstring, std::tstring> > vnames(
#ifdef _OPENMP
					omp_get_max_threads()
#else
					1
#endif
					);
				int const subitem = pLV->iSubItem;
				inplace_mergesort(this->results.begin(), this->results.end(), [this, subitem, &vnames](Results::value_type const &a, Results::value_type const &b)
				{
					std::pair<std::tstring, std::tstring> &names = *(vnames.begin()
#ifdef _OPENMP
						+ omp_get_thread_num()
#endif
						);
					if (GetAsyncKeyState(VK_ESCAPE) < 0)
					{
						throw CStructured_Exception(ERROR_CANCELLED, NULL);
					}
					Results::value_type::first_type const
						&index1 = a.first,
						&index2 = b.first;
					bool less = false;
					switch (subitem)
					{
					case COLUMN_INDEX_NAME:
						names.first = index1->root_path(); index1->get_path(a.second.first, names.first, true);
						names.second = index2->root_path(); index2->get_path(b.second.first, names.second, true);
						less = std::less<std::tstring>()(names.first, names.second);
						break;
					case COLUMN_INDEX_PATH:
						names.first = index1->root_path(); index1->get_path(a.second.first, names.first, false);
						names.second = index2->root_path(); index2->get_path(b.second.first, names.second, false);
						less = std::less<std::tstring>()(names.first, names.second);
						break;
					case COLUMN_INDEX_SIZE:
						less = a.second.second.first < b.second.second.first;
						break;
					case COLUMN_INDEX_SIZE_ON_DISK:
						less = a.second.second.second < b.second.second.second;
						break;
					case COLUMN_INDEX_CREATION_TIME:
						less = index1->get_stdinfo(a.second.first.first.first).first.second < index2->get_stdinfo(b.second.first.first.first).first.second;
						break;
					case COLUMN_INDEX_MODIFICATION_TIME:
						less = index1->get_stdinfo(a.second.first.first.first).second.first < index2->get_stdinfo(b.second.first.first.first).second.first;
						break;
					case COLUMN_INDEX_ACCESS_TIME:
						less = index1->get_stdinfo(a.second.first.first.first).second.second < index2->get_stdinfo(b.second.first.first.first).second.second;
						break;
					default: break;
					}
					return less;
				}, false /* parallelism BREAKS exception handling, and therefore user-cancellation */);
				if (hditem.lParam)
				{
					std::reverse(this->results.begin(), this->results.end());
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

	void Search()
	{
		WTL::CWaitCursor wait;
		int const selected = this->cmbDrive.GetCurSel();
		if (selected != 0 && !this->cmbDrive.GetItemDataPtr(selected))
		{
			this->MessageBox(_T("This does not appear to be a valid NTFS volume."), _T("Error"), MB_OK | MB_ICONERROR);
			return;
		}
		CProgressDialog dlg(*this);
		dlg.SetProgressTitle(_T("Searching..."));
		if (dlg.HasUserCancelled()) { return; }
		this->lastRequestedIcon.resize(0);
		this->lvFiles.SetItemCount(0);
		this->results.clear();
		std::tstring pattern;
		{
			ATL::CComBSTR bstr;
			if (this->txtPattern.GetWindowText(bstr.m_str))
			{ pattern.assign(bstr, bstr.Length()); }
		}
		bool is_regex = false;
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
		typedef boost::xpressive::basic_regex<std::tstring::const_iterator> RE;
		boost::xpressive::match_results<RE::iterator_type> mr;
		RE re;
		try
		{
			if (!pattern.empty() && *pattern.begin() == _T('>'))
			{
				re = RE::compile(pattern.begin() + 1, pattern.end(), boost::xpressive::regex_constants::nosubs | boost::xpressive::regex_constants::optimize | boost::xpressive::regex_constants::single_line | boost::xpressive::regex_constants::icase | boost::xpressive::regex_constants::collate);
				is_regex = true;
			}
		}
		catch (boost::xpressive::regex_error const &ex) { this->MessageBox(ATL::CA2T(ex.what()), _T("Regex Error"), MB_ICONERROR); return; }
#else
		if (!pattern.empty() && *pattern.begin() == _T('>'))
		{ this->MessageBox(_T("Regex support not included."), _T("Regex Error"), MB_ICONERROR); return; }
#endif
		bool const is_path_pattern = is_regex || ~pattern.find(_T('\\'));
		if (!is_path_pattern && !~pattern.find(_T('*')) && !~pattern.find(_T('?'))) { pattern.insert(pattern.begin(), _T('*')); pattern.insert(pattern.end(), _T('*')); }
		clock_t const start = clock();
		std::vector<HANDLE> wait_handles;
		std::vector<HANDLE> wait_volumes;
		std::vector<Results::value_type::first_type> wait_indices;
		// TODO: What if they exceed maximum wait objects?
		bool any_io_pending = true;
		size_t overall_progress_numerator = 0, overall_progress_denominator = 0;
		for (int ii = 0; ii < this->cmbDrive.GetCount(); ++ii)
		{
			boost::intrusive_ptr<NtfsIndex> const p = static_cast<NtfsIndex *>(this->cmbDrive.GetItemDataPtr(ii));
			if (p && (selected == ii || selected == 0))
			{
				wait_handles.push_back(reinterpret_cast<HANDLE>(p->finished_event()));
				wait_indices.push_back(p);
				wait_volumes.push_back(p->volume());
				size_t const records_so_far = p->records_so_far();
				any_io_pending |= records_so_far < p->total_records;
				overall_progress_denominator += p->total_records * 2;
			}
		}
		if (!any_io_pending) { overall_progress_denominator /= 2; }
		RaiseIoPriority const raise_io_priorities(wait_volumes.empty() ? NULL : &*wait_volumes.begin(), wait_volumes.size());
		while (!dlg.HasUserCancelled() && !wait_handles.empty())
		{
			unsigned long const wait_result = dlg.WaitMessageLoop(wait_handles.empty() ? NULL : &*wait_handles.begin(), wait_handles.size());
			if (wait_result == WAIT_TIMEOUT)
			{
				if (dlg.ShouldUpdate())
				{
					std::basic_ostringstream<TCHAR> ss;
					ss << _T("Reading file tables...");
					bool any = false;
					size_t temp_overall_progress_numerator = overall_progress_numerator;
					for (size_t i = 0; i != wait_indices.size(); ++i)
					{
						Results::value_type::first_type const j = wait_indices[i];
						size_t const records_so_far = j->records_so_far();
						temp_overall_progress_numerator += records_so_far;
						if (records_so_far != j->total_records)
						{
							if (any) { ss << _T(", "); }
							else { ss << _T(" "); }
							ss << j->root_path() << _T(" ") << _T("(") << nformat(records_so_far, this->loc) << _T(" of ") << nformat(j->total_records, this->loc) << _T(")");
							any = true;
						}
					}
					std::tstring const text = ss.str();
					dlg.SetProgressText(boost::iterator_range<TCHAR const *>(text.data(), text.data() + text.size()));
					dlg.SetProgress(static_cast<long long>(temp_overall_progress_numerator), static_cast<long long>(overall_progress_denominator));
					dlg.Flush();
				}
			}
			else
			{
				if (wait_result < wait_handles.size())
				{
					Results::value_type::first_type const i = wait_indices[wait_result];
					size_t const old_size = this->results.size();
					size_t current_progress_numerator = 0;
					size_t const current_progress_denominator = i->total_items();
					std::tstring const root_path = i->root_path();
					std::tstring current_path = root_path;
					while (!current_path.empty() && *(current_path.end() - 1) == _T('\\')) { current_path.erase(current_path.end() - 1); }
					try
					{
						i->matches([&dlg, is_path_pattern, &root_path, &pattern, is_regex, this, i, &wait_indices,
							&current_progress_numerator, current_progress_denominator,
							overall_progress_numerator, overall_progress_denominator
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
							, &re
							, &mr
#endif
						](std::pair<std::tstring::const_iterator, std::tstring::const_iterator> const path, std::pair<unsigned long long, unsigned long long> const sizes, NtfsIndex::key_type const key)
						{
							if (dlg.ShouldUpdate() || current_progress_denominator - current_progress_numerator <= 1)
							{
								if (dlg.HasUserCancelled()) { throw CStructured_Exception(ERROR_CANCELLED, NULL); }
								size_t temp_overall_progress_numerator = overall_progress_numerator;
								for (size_t k = 0; k != wait_indices.size(); ++k)
								{
									Results::value_type::first_type const j = wait_indices[k];
									size_t const records_so_far = j->records_so_far();
									temp_overall_progress_numerator += records_so_far;
								}
								std::tstring text(0x100 + root_path.size() + static_cast<ptrdiff_t>(path.second - path.first), _T('\0'));
								text.resize(static_cast<size_t>(_stprintf(&*text.begin(), _T("Searching %.*s (%s of %s)...\r\n%.*s"),
									static_cast<int>(root_path.size()), root_path.c_str(),
									nformat(current_progress_numerator, this->loc).c_str(),
									nformat(current_progress_denominator, this->loc).c_str(),
									static_cast<int>(path.second - path.first), path.first == path.second ? NULL : &*path.first)));
								dlg.SetProgressText(boost::iterator_range<TCHAR const *>(text.data(), text.data() + text.size()));
								dlg.SetProgress(temp_overall_progress_numerator + static_cast<unsigned long long>(i->total_records) * static_cast<unsigned long long>(current_progress_numerator) / static_cast<unsigned long long>(current_progress_denominator), static_cast<long long>(overall_progress_denominator));
								dlg.Flush();
							}
							++current_progress_numerator;
							std::pair<std::tstring::const_iterator, std::tstring::const_iterator> needle = path;
							if (!is_path_pattern)
							{
								while (needle.first != needle.second && *(needle.second - 1) == _T('\\')) { --needle.second; }
								needle.first = basename(needle.first, needle.second);
							}
							bool const match =
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
								is_regex ? boost::xpressive::regex_match(needle.first, needle.second, mr, re) :
#endif
								wildcard(pattern.begin(), pattern.end(), needle.first, needle.second, tchar_ci_traits())
								;
							if (match)
							{ this->results.push_back(Results::value_type(i, Results::value_type::second_type(key, sizes))); }
						}, current_path);
					}
					catch (CStructured_Exception &ex)
					{
						if (ex.GetSENumber() != ERROR_CANCELLED) { throw; }
					}
					if (any_io_pending) { overall_progress_numerator += i->total_records; }
					if (current_progress_denominator) { overall_progress_numerator += static_cast<size_t>(static_cast<unsigned long long>(i->total_records) * static_cast<unsigned long long>(current_progress_numerator) / static_cast<unsigned long long>(current_progress_denominator)); }
					std::reverse(this->results.begin() + static_cast<ptrdiff_t>(old_size), this->results.end());
					this->lvFiles.SetItemCountEx(static_cast<int>(this->results.size()), LVSICF_NOINVALIDATEALL);
				}
				using std::swap;
				swap(wait_indices[wait_result], wait_indices.back()), wait_indices.pop_back();
				swap(wait_handles[wait_result], wait_handles.back()), wait_handles.pop_back();
			}
		}
		clock_t const end = clock();
		TCHAR buf[0x100];
		_stprintf(buf, _T("%s results in %.2lf seconds"), nformat(this->results.size(), this->loc).c_str(), (end - start) * 1.0 / CLOCKS_PER_SEC);
		this->statusbar.SetText(0, buf);
	}

	void OnOK(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (GetFocus() == this->lvFiles)
		{
			int const index = this->lvFiles.GetNextItem(-1, LVNI_FOCUSED);
			if (index >= 0 && (this->lvFiles.GetItemState(index, LVNI_SELECTED) & LVNI_SELECTED))
			{
				this->DoubleClick(index);
			}
			else
			{
				this->Search();
				if (index >= 0)
				{
					this->lvFiles.EnsureVisible(index, FALSE);
					this->lvFiles.SetItemState(index, LVNI_FOCUSED, LVNI_FOCUSED);
				}
			}
		}
		else if (GetFocus() == this->txtPattern)
		{
			this->Search();
		}
	}

	LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		(void)uMsg;
		LRESULT result = 0;
		POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
		if ((HWND)wParam == this->lvFiles)
		{
			std::vector<int> indices;
			int index;
			if (point.x == -1 && point.y == -1)
			{
				index = this->lvFiles.GetSelectedIndex();
				RECT bounds = { };
				this->lvFiles.GetItemRect(index, &bounds, LVIR_SELECTBOUNDS);
				this->lvFiles.ClientToScreen(&bounds);
				point.x = bounds.left;
				point.y = bounds.top;
				indices.push_back(index);
			}
			else
			{
				POINT clientPoint = point;
				this->lvFiles.ScreenToClient(&clientPoint);
				index = this->lvFiles.HitTest(clientPoint, 0);
				if (index >= 0)
				{
					int i = -1;
					for (;;)
					{
						i = this->lvFiles.GetNextItem(i, LVNI_SELECTED);
						if (i < 0) { break; }
						indices.push_back(i);
					}
				}
			}
			int const focused = this->lvFiles.GetNextItem(-1, LVNI_FOCUSED);
			if (!indices.empty())
			{
				this->RightClick(indices, point, focused);
			}
		}
		return result;
	}

	void RightClick(std::vector<int> const &indices, POINT const &point, int const focused)
	{
		std::vector<Results::const_iterator> results;
		for (size_t i = 0; i < indices.size(); ++i)
		{
			if (indices[i] < 0) { continue; }
			results.push_back(this->results.begin() + indices[i]);
		}
		HRESULT volatile hr = S_OK;
		UINT const minID = 1000;
		WTL::CMenu menu;
		menu.CreatePopupMenu();
		ATL::CComPtr<IContextMenu> contextMenu;
		std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > > p(
			new std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> >());
		p->second.reserve(results.size());  // REQUIRED, to avoid copying CShellItemIDList objects (they're not copyable!)
		SFGAOF sfgao = 0;
		std::tstring common_ancestor_path;
		for (size_t i = 0; i < results.size(); ++i)
		{
			Results::const_iterator const row = results[i];
			Results::value_type::first_type const &index = row->first;
			std::tstring path = index->root_path();
			if (index->get_path(row->second.first, path, false))
			{
				remove_path_stream_and_trailing_sep(path);
			}
			if (i == 0)
			{
				common_ancestor_path = path;
			}
			CShellItemIDList itemIdList;
			if (SHParseDisplayName(path.c_str(), NULL, &itemIdList, sfgao, &sfgao) == S_OK)
			{
				p->second.push_back(CShellItemIDList());
				p->second.back().Attach(itemIdList.Detach());
				if (i != 0)
				{
					common_ancestor_path = path;
					size_t j;
					for (j = 0; j < (path.size() < common_ancestor_path.size() ? path.size() : common_ancestor_path.size()); j++)
					{
						if (path[j] != common_ancestor_path[j])
						{
							break;
						}
					}
					common_ancestor_path.erase(common_ancestor_path.begin() + static_cast<ptrdiff_t>(j), common_ancestor_path.end());
				}
			}
		}
		common_ancestor_path.erase(dirname(common_ancestor_path.begin(), common_ancestor_path.end()), common_ancestor_path.end());
		if (hr == S_OK)
		{
			hr = SHParseDisplayName(common_ancestor_path.c_str(), NULL, &p->first.first, sfgao, &sfgao);
		}
		if (hr == S_OK)
		{
			ATL::CComPtr<IShellFolder> desktop;
			hr = SHGetDesktopFolder(&desktop);
			if (hr == S_OK)
			{
				if (p->first.first.m_pidl->mkid.cb)
				{
					hr = desktop->BindToObject(p->first.first, NULL, IID_IShellFolder, reinterpret_cast<void **>(&p->first.second));
				}
				else
				{
					hr = desktop.QueryInterface(&p->first.second);
				}
			}
		}

		if (hr == S_OK)
		{
			std::vector<LPCITEMIDLIST> relative_item_ids(p->second.size());
			for (size_t i = 0; i < p->second.size(); ++i)
			{
				relative_item_ids[i] = ILFindChild(p->first.first, p->second[i]);
			}
			hr = p->first.second->GetUIObjectOf(
				*this,
				static_cast<UINT>(relative_item_ids.size()),
				relative_item_ids.empty() ? NULL : &relative_item_ids[0],
				IID_IContextMenu,
				NULL,
				&reinterpret_cast<void *&>(contextMenu.p));
		}
		if (hr == S_OK)
		{
			hr = contextMenu->QueryContextMenu(menu, 0, minID, UINT_MAX, 0x80 /*CMF_ITEMMENU*/);
		}

		unsigned int ninserted = 0;
		UINT const openContainingFolderId = minID - 1;

		if (results.size() == 1)
		{
			MENUITEMINFO mii2 = { sizeof(mii2), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_ENABLED, openContainingFolderId, NULL, NULL, NULL, NULL, _T("Open &Containing Folder") };
			menu.InsertMenuItem(ninserted++, TRUE, &mii2);

			menu.SetMenuDefaultItem(openContainingFolderId, FALSE);
		}
		if (0 <= focused && static_cast<size_t>(focused) < this->results.size())
		{
			std::basic_stringstream<TCHAR> ssName;
			ssName.imbue(this->loc);
			ssName << _T("File #") << (this->results.begin() + focused)->second.first.first.first;
			std::tstring name = ssName.str();
			if (!name.empty())
			{
				MENUITEMINFO mii1 = { sizeof(mii1), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_DISABLED, minID - 2, NULL, NULL, NULL, NULL, (name.c_str(), &name[0]) };
				menu.InsertMenuItem(ninserted++, TRUE, &mii1);
			}
		}
		if (contextMenu && ninserted)
		{
			MENUITEMINFO mii = { sizeof(mii), 0, MFT_MENUBREAK };
			menu.InsertMenuItem(ninserted, TRUE, &mii);
		}
		UINT id = menu.TrackPopupMenu(
			TPM_RETURNCMD | TPM_NONOTIFY | (GetKeyState(VK_SHIFT) < 0 ? CMF_EXTENDEDVERBS : 0) |
			(GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION : TPM_LEFTALIGN | TPM_HORPOSANIMATION),
			point.x, point.y, *this);
		if (!id)
		{
			// User cancelled
		}
		else if (id == openContainingFolderId)
		{
			if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, p.get(), WT_EXECUTEINUITHREAD))
			{
				p.release();
			}
		}
		else if (id >= minID)
		{
			CMINVOKECOMMANDINFO cmd = { sizeof(cmd), CMIC_MASK_ASYNCOK, *this, reinterpret_cast<LPCSTR>(id - minID), NULL, NULL, SW_SHOW };
			hr = contextMenu ? contextMenu->InvokeCommand(&cmd) : S_FALSE;
			if (hr == S_OK)
			{
			}
			else
			{
				this->MessageBox(_com_error(hr).ErrorMessage(), _T("Error"), MB_OK | MB_ICONERROR);
			}
		}
	}

	void DoubleClick(int index)
	{
		Results::const_iterator const result = this->results.begin() + static_cast<ptrdiff_t>(index);
		Results::value_type::first_type const &i = result->first;
		std::tstring path;
		path = i->root_path(), i->get_path(result->second.first, path, false);
		remove_path_stream_and_trailing_sep(path);
		std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > > p(
			new std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> >());
		SFGAOF sfgao = 0;
		HRESULT hr = SHParseDisplayName(std::tstring(path.begin(), dirname(path.begin(), path.end())).c_str(), NULL, &p->first.first, 0, &sfgao);
		if (hr == S_OK)
		{
			ATL::CComPtr<IShellFolder> desktop;
			hr = SHGetDesktopFolder(&desktop);
			if (hr == S_OK)
			{
				if (p->first.first.m_pidl->mkid.cb)
				{
					hr = desktop->BindToObject(p->first.first, NULL, IID_IShellFolder, reinterpret_cast<void **>(&p->first.second));
				}
				else
				{
					hr = desktop.QueryInterface(&p->first.second);
				}
			}
		}
		if (hr == S_OK && basename(path.begin(), path.end()) != path.end())
		{
			p->second.resize(1);
			hr = SHParseDisplayName((path.c_str(), path.empty() ? NULL : &path[0]), NULL, &p->second.back().m_pidl, sfgao, &sfgao);
		}
		if (hr == S_OK)
		{
			if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, p.get(), WT_EXECUTEINUITHREAD))
			{
				p.release();
			}
		}
		else { this->MessageBox(GetAnyErrorText(hr), _T("Error"), MB_ICONERROR); }
	}

	LRESULT OnFilesDoubleClick(LPNMHDR pnmh)
	{
		int index;
		// Wow64Disable wow64Disabled;
		WTL::CWaitCursor wait;
		LPNMITEMACTIVATE pnmItem = (LPNMITEMACTIVATE) pnmh;
		index = this->lvFiles.HitTest(pnmItem->ptAction, 0);
		if (index >= 0)
		{
			this->DoubleClick(index);
		}
		return 0;
	}
	
	LRESULT OnFileNameArrowKey(LPNMHDR pnmh)
	{
		CUpDownNotify<WTL::CEdit>::KeyNotify *const p = (CUpDownNotify<WTL::CEdit>::KeyNotify *)pnmh;
		if (p->vkey == VK_UP || p->vkey == VK_DOWN)
		{
			this->cmbDrive.SendMessage(p->hdr.code == CUpDownNotify<WTL::CEdit>::CUN_KEYDOWN ? WM_KEYDOWN : WM_KEYUP, p->vkey, p->lParam);
		}
		else
		{
			if (p->hdr.code == CUpDownNotify<WTL::CEdit>::CUN_KEYDOWN && p->vkey == VK_DOWN && this->lvFiles.GetItemCount() > 0)
			{
				this->lvFiles.SetFocus();
			}
			this->lvFiles.SendMessage(p->hdr.code == CUpDownNotify<WTL::CEdit>::CUN_KEYDOWN ? WM_KEYDOWN : WM_KEYUP, p->vkey, p->lParam);
		}
		return 0;
	}

	LRESULT OnFilesKeyDown(LPNMHDR pnmh)
	{
		NMLVKEYDOWN *const p = (NMLVKEYDOWN *) pnmh;
		if (p->wVKey == VK_UP && this->lvFiles.GetNextItem(-1, LVNI_FOCUSED) == 0)
		{
			this->txtPattern.SetFocus();
		}
		return 0;
	}

	std::tstring GetSubItemText(Results::const_iterator const result, int const subitem, std::tstring &path) const
	{
		Results::value_type::first_type const &i = result->first;
		path = i->root_path();
		size_t const cch_root_path = path.size();
		i->get_path(result->second.first, path, false);
		std::tstring text(0x100, _T('\0'));
		switch (subitem)
		{
		case COLUMN_INDEX_NAME: if (path.size() == cch_root_path) { text = _T("."); } else { text = path; deldirsep(text); } text.erase(text.begin(), basename(text.begin(), text.end())); break;
		case COLUMN_INDEX_PATH: text = path; break;
		case COLUMN_INDEX_SIZE: text = nformat(result->second.second.first, this->loc); break;
		case COLUMN_INDEX_SIZE_ON_DISK: text = nformat(result->second.second.second, this->loc); break;
		case COLUMN_INDEX_CREATION_TIME: SystemTimeToString(i->get_stdinfo(result->second.first.first.first).first.second, &text[0], text.size()); text = std::tstring(text.c_str()); break;
		case COLUMN_INDEX_MODIFICATION_TIME: SystemTimeToString(i->get_stdinfo(result->second.first.first.first).second.first, &text[0], text.size()); text = std::tstring(text.c_str()); break;
		case COLUMN_INDEX_ACCESS_TIME: SystemTimeToString(i->get_stdinfo(result->second.first.first.first).second.second, &text[0], text.size()); text = std::tstring(text.c_str()); break;
		default: break;
		}
		return text;
	}

	LRESULT OnFilesGetDispInfo(LPNMHDR pnmh)
	{
		NMLVDISPINFO *const pLV = (NMLVDISPINFO *) pnmh;

		if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0 && (pLV->item.mask & LVIF_TEXT) != 0)
		{
			Results::const_iterator const result = this->results.begin() + static_cast<ptrdiff_t>(pLV->item.iItem);
			Results::value_type::first_type const &i = result->first;
			std::tstring path;
			std::tstring const text = this->GetSubItemText(result, pLV->item.iSubItem, path);
			if (!text.empty()) { _tcsncpy(pLV->item.pszText, text.c_str(), pLV->item.cchTextMax); }
			if (pLV->item.iSubItem == 0)
			{
				int iImage = this->CacheIcon(path, static_cast<int>(pLV->item.iItem), i->get_stdinfo(result->second.first.first.first).first.first, true);
				if (iImage >= 0) { pLV->item.iImage = iImage; }
			}
		}
		return 0;
	}

	void OnCancel(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (this->CheckAndCreateIcon(false))
		{
			this->ShowWindow(SW_HIDE);
		}
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		if (this->accel)
		{
			if (this->accel.TranslateAccelerator(this->m_hWnd, pMsg))
			{
				return TRUE;
			}
		}

		return this->CWindow::IsDialogMessage(pMsg);
	}
	
	LRESULT OnFilesListCustomDraw(LPNMHDR pnmh)
	{
		LRESULT result;
		COLORREF const deletedColor = RGB(0xFF, 0, 0);
		COLORREF encryptedColor = RGB(0, 0xFF, 0);
		COLORREF compressedColor = RGB(0, 0, 0xFF);
		WTL::CRegKeyEx key;
		if (key.Open(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")) == ERROR_SUCCESS)
		{
			key.QueryDWORDValue(_T("AltColor"), compressedColor);
			key.QueryDWORDValue(_T("AltEncryptedColor"), encryptedColor);
			key.Close();
		}
		COLORREF sparseColor = RGB(GetRValue(compressedColor), (GetGValue(compressedColor) + GetBValue(compressedColor)) / 2, (GetGValue(compressedColor) + GetBValue(compressedColor)) / 2);
		LPNMLVCUSTOMDRAW const pLV = (LPNMLVCUSTOMDRAW)pnmh;
		if (pLV->nmcd.dwItemSpec < this->results.size())
		{
			Results::const_iterator const item = this->results.begin() + static_cast<ptrdiff_t>(pLV->nmcd.dwItemSpec);
			Results::value_type::first_type const &i = item->first;
			unsigned long const attrs = i->get_stdinfo(item->second.first.first.first).first.first;
			switch (pLV->nmcd.dwDrawStage)
			{
			case CDDS_PREPAINT:
				result = CDRF_NOTIFYITEMDRAW;
				break;
			case CDDS_ITEMPREPAINT:
				result = CDRF_NOTIFYSUBITEMDRAW;
				break;
			case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
				if (pLV->iSubItem == 1) { result = 0x8 /*CDRF_DOERASE*/ | CDRF_NOTIFYPOSTPAINT; }
				else
				{
					if ((attrs & 0x40000000) != 0)
					{
						pLV->clrText = deletedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_ENCRYPTED) != 0)
					{
						pLV->clrText = encryptedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_COMPRESSED) != 0)
					{
						pLV->clrText = compressedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_SPARSE_FILE) != 0)
					{
						pLV->clrText = sparseColor;
					}
					result = CDRF_DODEFAULT;
				}
				break;
			case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
				result = CDRF_SKIPDEFAULT;
				{
					Results::const_iterator const row = this->results.begin() + static_cast<ptrdiff_t>(pLV->nmcd.dwItemSpec);
					std::tstring path;
					std::tstring itemText = this->GetSubItemText(row, pLV->iSubItem, path);
					WTL::CDCHandle dc(pLV->nmcd.hdc);
					RECT rcTwips = pLV->nmcd.rc;
					rcTwips.left = (int) ((rcTwips.left + 6) * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
					rcTwips.right = (int) (rcTwips.right * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
					rcTwips.top = (int) (rcTwips.top * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
					rcTwips.bottom = (int) (rcTwips.bottom * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
					int const savedDC = dc.SaveDC();
					{
						std::replace(itemText.begin(), itemText.end(), _T(' '), _T('\u00A0'));
						replace_all(itemText, _T("\\"), _T("\\\u200B"));
#ifdef _UNICODE
						this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, 1200);
#else
						this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, CP_ACP);
#endif
						CHARFORMAT format = { sizeof(format), CFM_COLOR, 0, 0, 0, 0 };
						if ((attrs & 0x40000000) != 0)
						{
							format.crTextColor = deletedColor;
						}
						else if ((attrs & FILE_ATTRIBUTE_ENCRYPTED) != 0)
						{
							format.crTextColor = encryptedColor;
						}
						else if ((attrs & FILE_ATTRIBUTE_COMPRESSED) != 0)
						{
							format.crTextColor = compressedColor;
						}
						else if ((attrs & FILE_ATTRIBUTE_SPARSE_FILE) != 0)
						{
							format.crTextColor = sparseColor;
						}
						else
						{
							bool const selected = (this->lvFiles.GetItemState(static_cast<int>(pLV->nmcd.dwItemSpec), LVIS_SELECTED) & LVIS_SELECTED) != 0;
							format.crTextColor = GetSysColor(selected && this->lvFiles.IsThemeNull() ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
						}
						this->richEdit.SetSel(0, -1);
						this->richEdit.SetSelectionCharFormat(format);
						if (false)
						{
							size_t last_sep = itemText.find_last_of(_T('\\'));
							if (~last_sep)
							{
								this->richEdit.SetSel(static_cast<long>(last_sep + 1), this->richEdit.GetTextLength());
								CHARFORMAT bold = { sizeof(bold), CFM_BOLD, CFE_BOLD, 0, 0, 0 };
								this->richEdit.SetSelectionCharFormat(bold);
							}
						}
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
				break;
			default:
				result = CDRF_DODEFAULT;
				break;
			}
		}
		else { result = CDRF_DODEFAULT; }
		return result;
	}

	void OnClose(UINT /*uNotifyCode*/ = 0, int nID = IDCANCEL, HWND /*hWnd*/ = NULL)
	{
		this->DestroyWindow();
		PostQuitMessage(nID);
		// this->EndDialog(nID);
	}
	
	LRESULT OnDeviceChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
	{
		switch (wParam)
		{
		case DBT_DEVICEQUERYREMOVEFAILED:
			{
			}
			break;
		case DBT_DEVICEQUERYREMOVE:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header);
				}
			}
			break;
		case DBT_DEVICEREMOVECOMPLETE:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_HANDLE)
				{
					reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header);
				}
			}
			break;
		case DBT_DEVICEARRIVAL:
			{
				DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
				if (header.dbch_devicetype == DBT_DEVTYP_VOLUME)
				{
				}
			}
			break;
		default: break;
		}
		return TRUE;
	}
	
	void OnShowWindow(BOOL bShow, UINT /*nStatus*/)
	{
		if (bShow)
		{
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
			this->DeleteNotifyIcon();
			this->txtPattern.SetFocus();
		}
		else
		{
			SetPriorityClass(GetCurrentProcess(), 0x100000 /*PROCESS_MODE_BACKGROUND_BEGIN*/);
			SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		}
	}

	void DeleteNotifyIcon()
	{
		NOTIFYICONDATA nid = { sizeof(nid), *this, 0 };
		Shell_NotifyIcon(NIM_DELETE, &nid);
		SetPriorityClass(GetCurrentProcess(), 0x200000 /*PROCESS_MODE_BACKGROUND_END*/);
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

	LRESULT OnDriveSetItemData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
	{
		this->cmbDrive.SetItemDataPtr(static_cast<int>(wParam), reinterpret_cast<NtfsIndex *>(lParam));
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
	
	void OnHelpAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		this->MessageBox(_T("© 2015 Mehrdad N.\r\nAll rights reserved."), _T("About"), MB_ICONINFORMATION);
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
		width = (std::max)(1, (int) (rect.right - rect.left) - GetSystemMetrics(SM_CXVSCROLL));
		WTL::CHeaderCtrl wndListViewHeader = wndListView.GetHeader();
		int oldTotalColumnsWidth;
		oldTotalColumnsWidth = 0;
		int columnCount;
		columnCount = wndListViewHeader.GetItemCount();
		for (int i = 0; i < columnCount; i++)
		{
			oldTotalColumnsWidth += wndListView.GetColumnWidth(i);
		}
		for (int i = 0; i < columnCount; i++)
		{
			int colWidth = wndListView.GetColumnWidth(i);
			int newWidth = MulDiv(colWidth, width, oldTotalColumnsWidth);
			newWidth = (std::max)(newWidth, 1);
			wndListView.SetColumnWidth(i, newWidth);
		}
	}

	void OnRefresh(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
	{
		int const selected = this->cmbDrive.GetCurSel();
		for (int ii = 0; ii < this->cmbDrive.GetCount(); ++ii)
		{
			if (selected == 0 || ii == selected)
			{
				if (boost::intrusive_ptr<NtfsIndex> const q = static_cast<NtfsIndex *>(this->cmbDrive.GetItemDataPtr(ii)))
				{
					std::tstring const path_name = q->root_path();
					q->cancel();
					this->cmbDrive.SetItemDataPtr(ii, NULL);
					intrusive_ptr_release(q.get());
					typedef OverlappedNtfsMftRead T;
					std::auto_ptr<T> p(new T(this->iocp, path_name, this->m_hWnd, this->closing_event));
					if (PostQueuedCompletionStatus(this->iocp, 0, static_cast<uintptr_t>(ii), &*p)) { p.release(); }
				}
			}
		}
	}

	BEGIN_MSG_MAP_EX(CMainDlg)
		CHAIN_MSG_MAP(CInvokeImpl<CMainDlg>)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_SHOWWINDOW(OnShowWindow)
		MSG_WM_CLOSE(OnClose)
		MESSAGE_HANDLER_EX(WM_DEVICECHANGE, OnDeviceChange)  // Don't use MSG_WM_DEVICECHANGE(); it's broken (uses DWORD)
		MESSAGE_HANDLER_EX(WM_NOTIFYICON, OnNotifyIcon)
		MESSAGE_HANDLER_EX(WM_DRIVESETITEMDATA, OnDriveSetItemData)
		MESSAGE_HANDLER_EX(WM_TASKBARCREATED, OnTaskbarCreated)
		MESSAGE_HANDLER_EX(WM_MOUSEWHEEL, OnMouseWheel)
		MESSAGE_HANDLER_EX(WM_CONTEXTMENU, OnContextMenu)
		COMMAND_ID_HANDLER_EX(ID_HELP_ABOUT, OnHelpAbout)
		COMMAND_ID_HANDLER_EX(ID_HELP_USINGREGULAREXPRESSIONS, OnHelpRegex)
		COMMAND_ID_HANDLER_EX(ID_FILE_FITCOLUMNSTOWINDOW, OnFileFitColumns)
		COMMAND_ID_HANDLER_EX(ID_FILE_EXIT, OnClose)
		COMMAND_ID_HANDLER_EX(ID_ACCELERATOR40006, OnRefresh)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_CUSTOMDRAW, OnFilesListCustomDraw)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_GETDISPINFO, OnFilesGetDispInfo)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_COLUMNCLICK, OnFilesListColumnClick)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, NM_DBLCLK, OnFilesDoubleClick)
		NOTIFY_HANDLER_EX(IDC_EDITFILENAME, CUpDownNotify<WTL::CEdit>::CUN_KEYDOWN, OnFileNameArrowKey)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_KEYDOWN, OnFilesKeyDown)
		END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_LISTFILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_EDITFILENAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDC_STATUS_BAR, DLSZ_SIZE_X | DLSZ_MOVE_Y)
		// DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()
	enum { IDD = IDD_DIALOG1 };
};
unsigned int const CMainDlg::WM_TASKBARCREATED = RegisterWindowMessage(_T("TaskbarCreated"));

WTL::CAppModule _Module;

int _tmain(int argc, TCHAR* argv[])
{
#if !defined(_WIN64) //&& defined(NDEBUG) && NDEBUG
	if (!IsDebuggerPresent())
	{
		HMODULE hKernel32 = NULL;
		GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCTSTR>(&GetSystemInfo), &hKernel32);
		typedef BOOL (WINAPI *PIsWow64Process)(IN HANDLE hProcess, OUT PBOOL Wow64Process);
		PIsWow64Process IsWow64Process = NULL;
		BOOL isWOW64 = FALSE;
		if (hKernel32 != NULL && (IsWow64Process = reinterpret_cast<PIsWow64Process>(GetProcAddress(hKernel32, "IsWow64Process"))) != NULL && IsWow64Process(GetCurrentProcess(), &isWOW64) && isWOW64)
		{
			HRSRC hRsrs = FindResource(NULL, _T("X64"), _T("BINARY"));
			LPVOID pBinary = LockResource(LoadResource(NULL, hRsrs));
			if (pBinary)
			{
				std::basic_string<TCHAR> tempDir(32 * 1024, _T('\0'));
				tempDir.resize(GetTempPath(static_cast<DWORD>(tempDir.size()), &tempDir[0]));
				if (!tempDir.empty())
				{
					std::basic_string<TCHAR> fileName = tempDir + _T("SwiftSearch64_{3CACE9B1-EF40-4a3b-B5E5-3447F6A1E703}.exe");
					struct Deleter
					{
						std::basic_string<TCHAR> file;
						~Deleter() { if (!this->file.empty()) { _tunlink(this->file.c_str()); } }
					} deleter;
					std::string fileNameChars;
					std::copy(fileName.begin(), fileName.end(), std::inserter(fileNameChars, fileNameChars.end()));
					std::ofstream file(fileNameChars.c_str(), std::ios::out | std::ios::trunc | std::ios::binary);
					if (!file.bad() && !file.fail())
					{
						deleter.file = fileName;
						file.write(static_cast<char const *>(pBinary), SizeofResource(NULL, hRsrs));
						file.flush();
						file.close();
						STARTUPINFO si = { sizeof(si) };
						GetStartupInfo(&si);
						PROCESS_INFORMATION pi;
						HANDLE hJob = CreateJobObject(NULL, NULL);
						JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobLimits = { { { 0 }, { 0 }, JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE } };
						if (hJob != NULL
							&& SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &jobLimits, sizeof(jobLimits))
							&& AssignProcessToJobObject(hJob, GetCurrentProcess()))
						{
							if (CreateProcess(fileName.c_str(), GetCommandLine(), NULL, NULL, FALSE, CREATE_PRESERVE_CODE_AUTHZ_LEVEL | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi))
							{
								if (ResumeThread(pi.hThread) != -1)
								{
									WaitForSingleObject(pi.hProcess, INFINITE);
									DWORD exitCode = 0;
									GetExitCodeProcess(pi.hProcess, &exitCode);
									return exitCode;
								}
								else { TerminateProcess(pi.hProcess, GetLastError()); }
							}
						}
					}
				}
			}
			/* continue running in x86 mode... */
		}
	}
#endif
	HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, _T("Local\\SwiftSearch.{CB77990E-A78F-44dc-B382-089B01207F02}"));
	if (hEvent != NULL && GetLastError() != ERROR_ALREADY_EXISTS)
	{
		typedef NTSTATUS(WINAPI *PNtSetTimerResolution)(unsigned long DesiredResolution, bool SetResolution, unsigned long *CurrentResolution);
		if (PNtSetTimerResolution const NtSetTimerResolution = reinterpret_cast<PNtSetTimerResolution>(GetProcAddress(GetModuleHandle(TEXT("ntdll.dll")), "NtSetTimerResolution")))
		{
			unsigned long prev; NtSetTimerResolution(1, true, &prev);
		}
		(void) argc;
		(void) argv;
		// pi();
		HINSTANCE const hInstance = GetModuleHandle(NULL);
		__if_exists(_Module) { _Module.Init(NULL, hInstance); }
		{
			WTL::CMessageLoop msgLoop;
			_Module.AddMessageLoop(&msgLoop);
			CMainDlg wnd;
			wnd.Create(reinterpret_cast<HWND>(NULL), NULL);
			wnd.ShowWindow(SW_SHOWDEFAULT);
			msgLoop.Run();
			_Module.RemoveMessageLoop();
		}
		__if_exists(_Module) { _Module.Term(); }
	}
	return 0;
}

int __stdcall _tWinMain(HINSTANCE const hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nShowCmd)
{
	(void) hInstance;
	(void) nShowCmd;
	return _tmain(__argc, __targv);
}

void pi()
{
	typedef long long Int;
	for (Int l = static_cast<Int>(1) << 23; !(l >> 28); l <<= 1)
	{
		Int num = 0, den = 0;
		clock_t const start = clock();
// #pragma omp parallel for schedule(static) reduction(+: num, den)
		for (Int x = 1; x <= l; x += 2)
		{
			Int const r = l * l - x * x;
			Int begin = 1, end = l + 1;
			while (begin != end)
			{
				Int const y = begin + (end - begin) / 2;
				if (y * y <= r) { begin = y + 1; }
				else { end = y + 0; }
			}
			num += begin / 2;
			den += (l + 1) / 2;
		}
		fprintf(
			stderr, "[%5u ms] %0.12f\n",
			static_cast<unsigned int>(clock() - start) * 1000 / CLOCKS_PER_SEC,
			(4.0 * num / den));
	}
}
