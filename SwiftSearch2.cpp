#include <process.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
namespace WTL { using std::min; using std::max; }

#include <Windows.h>

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

#include "vector_bool.hpp"

#include "CModifiedDialogImpl.hpp"

#include "resource.h"

namespace std { typedef basic_string<TCHAR> tstring; }

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
	explicit lock_guard(mutex_type &mutex, bool const already_locked = false) : p(&mutex) { if (!already_locked) { p->lock(); } }
	lock_guard(lock_guard const &other) : p(other.p) { if (p) { p->lock(); } }
	lock_guard &operator =(lock_guard other) { return this->swap(other), *this; }
	void swap(lock_guard &other) { using std::swap; swap(this->p, other.p); }
	friend void swap(lock_guard &a, lock_guard &b) { return a.swap(b); }
};

template<class F>
void async(F f)
{
	struct Runner { static void async(void *const f) { SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL); (*std::auto_ptr<F>(static_cast<F *>(f)))(); } };
	std::auto_ptr<F> p(new F(f));
	if (~_beginthread(static_cast<void(*)(void *)>(&Runner::async), 0, static_cast<void *>(p.get()))) { p.release(); }
	else { throw std::runtime_error("error spawning thread"); }
}

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

loopStart:
	for (s = strBegin, p = patBegin; s != strEnd && p != patEnd; ++s, ++p)
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
	return p == patEnd && s == strEnd;

starCheck:
	if (!star) { return false; }
	strBegin++;
	goto loopStart;
}

namespace winnt
{
	template<class T> struct identity { typedef T type; };

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

	struct FILE_FS_SIZE_INFORMATION { long long TotalAllocationUnits, ActualAvailableAllocationUnits; unsigned long SectorsPerAllocationUnit, BytesPerSector; };
	struct FILE_FS_ATTRIBUTE_INFORMATION { unsigned long FileSystemAttributes; unsigned long MaximumComponentNameLength; unsigned long FileSystemNameLength; wchar_t FileSystemName[1]; };

#define X(F, T) identity<T>::type &F = *reinterpret_cast<identity<T>::type *>(GetProcAddress(GetModuleHandle(_T("NTDLL.dll")), #F))
	X(NtOpenFile, NTSTATUS NTAPI(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, OUT IO_STATUS_BLOCK *IoStatusBlock, IN ULONG ShareAccess, IN ULONG OpenOptions));
	X(NtReadFile, NTSTATUS NTAPI(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN IO_APC_ROUTINE *ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL));
	X(NtQueryVolumeInformationFile, NTSTATUS NTAPI(HANDLE FileHandle, IO_STATUS_BLOCK *IoStatusBlock, PVOID FsInformation, unsigned long Length, unsigned long FsInformationClass));
	X(RtlInitUnicodeString, VOID NTAPI(UNICODE_STRING * DestinationString, PCWSTR SourceString));
	X(RtlNtStatusToDosError, unsigned long NTAPI(IN NTSTATUS NtStatus));
#undef  X
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
}

void CheckAndThrow(int const success) { if (!success) { RaiseException(GetLastError(), 0, 0, NULL); } }

class Handle
{
	static bool valid(void *const value) { return value && value != reinterpret_cast<void *>(-1); }
public:
	void *value;
	Handle() : value() { }
	explicit Handle(void *const value) : value(value) { if (!valid(value)) { throw std::invalid_argument("invalid handle"); } }
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
	void *p;
	Overlapped(Overlapped const &);
	Overlapped &operator =(Overlapped const &);
public:
	~Overlapped()
	{
		if (this->hEvent) { if (CloseHandle(reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(this->hEvent) & ~static_cast<uintptr_t>(1)))) { this->hEvent = NULL; } }
		if (this->semaphore) { long prev; CheckAndThrow(ReleaseSemaphore(this->semaphore, 1, &prev)); }
	}
	explicit Overlapped(size_t const this_size, Handle const &semaphore) : OVERLAPPED(), p(), virtual_offset(), semaphore(semaphore)
	{
		this->p = reinterpret_cast<unsigned char *>(this) + this_size;
		if (semaphore) { CheckAndThrow(WaitForSingleObject(semaphore, INFINITE) == WAIT_OBJECT_0); }
	}
	static void *operator new(size_t const this_size, size_t const buffer_size = 0) { return ::operator new(this_size + buffer_size); }
	static void operator delete(void *const p) { return ::operator delete(p); }
	unsigned long long virtual_offset;
	Handle semaphore;
	void *data() { return this->p; }
	void const *data() const { return this->p; }
	virtual void operator()(size_t const size, uintptr_t const key) = 0;
};

class NtfsIndex
{
	typedef NtfsIndex this_type;
	mutable mutex _mutex;
	std::tstring _root_path;
	Handle volume;
	typedef std::codecvt<std::tstring::value_type, char, std::mbstate_t> CodeCvt;
	NtfsIndex *unvolatile() volatile { return const_cast<NtfsIndex *>(this); }
	NtfsIndex const *unvolatile() const volatile { return const_cast<NtfsIndex *>(this); }
	std::tstring names;
#pragma pack(push)
	// #pragma pack(1)
	struct StandardInfo
	{
		long long created, written, accessed;
		unsigned long attributes;
	};
	struct NameInfo
	{
		unsigned int offset;
		unsigned char length;
	};
	struct LinkInfo
	{
		unsigned int parent;
		NameInfo name;
	};
	struct StreamInfo
	{
		NameInfo name;
		long long length, allocated;
	};
#pragma pack(pop)
	typedef std::vector<std::pair<LinkInfo, size_t /* next */> > LinkInfos;
	typedef std::vector<std::pair<StreamInfo, size_t /* next */> > StreamInfos;
	struct Record;
	typedef std::vector<Record> Records;
	typedef std::vector<std::pair<Records::size_type, size_t /* next */> > ChildInfos;
	struct Record
	{
		StandardInfo stdinfo;
		LinkInfos::size_type inameinfo;
		StreamInfos::size_type istreaminfo;
		ChildInfos::size_type children;
	};
	Records records;
	LinkInfos nameinfos;
	StreamInfos streaminfos;
	ChildInfos childinfos;
	Handle _finished_event;
	size_t loads_so_far;
	size_t total_loads;
public:
	std::vector<bool> mft_bitmap;
	unsigned int mft_record_size;
	NtfsIndex() : _finished_event(CreateEvent(NULL, TRUE, FALSE, NULL)), loads_so_far(), total_loads(), mft_record_size() { this->check_finished(); }
	typedef std::pair<unsigned int, std::pair<unsigned short, unsigned short> > key_type;
	uintptr_t open(std::tstring value)
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
			Handle(CreateFile(path_name.c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL)).swap(this->volume);
			success = true;
		}
		catch (std::invalid_argument &) {}
		if (success) { using std::swap; swap(this->_root_path, value); }
		return success ? reinterpret_cast<uintptr_t>(this->volume.value) : NULL;
	}
	std::tstring const &root_path() const { return this->_root_path; }
	std::tstring const &root_path() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		return me->root_path();
	}
	bool is_finished() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		return me->is_finished();
	}
	bool is_finished() const { return this->loads_so_far == this->total_loads; }
	uintptr_t finished_event() const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		return me->finished_event();
	}
	uintptr_t finished_event() const
	{
		return reinterpret_cast<uintptr_t>(this->_finished_event.value);
	}
	void check_finished() volatile
	{
		this_type *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		return me->check_finished();
	}
	void check_finished()
	{
		this->is_finished() ? SetEvent(this->_finished_event) : ResetEvent(this->_finished_event);
		if (this->is_finished() && !this->_root_path.empty()) { _ftprintf(stderr, _T("Finished: %s\n"), this->_root_path.c_str()); }
	}
	void set_total_loads(size_t const n) volatile
	{
		this_type *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		me->set_total_loads(n);
	}
	void set_total_loads(size_t const n)
	{
		this->total_loads = n;
	}
	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size) volatile
	{
		this_type *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		me->load(virtual_offset, buffer, size);
	}
	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size)
	{
		Record empty_record = Record(/* TODO: Should be -1 */);
		empty_record.istreaminfo = empty_record.inameinfo = empty_record.children = ~size_t();


		if (size % this->mft_record_size)
		{ throw std::runtime_error("cluster size is smaller than MFT record size; split MFT records not supported"); }
		for (size_t i = virtual_offset % this->mft_record_size ? this->mft_record_size - virtual_offset % this->mft_record_size : 0; i + this->mft_record_size <= size; i += this->mft_record_size)
		{
			unsigned int const frs = static_cast<unsigned int>((virtual_offset + i) / this->mft_record_size);
			if (this->mft_bitmap[frs])
			{
				ntfs::FILE_RECORD_SEGMENT_HEADER *const frsh = reinterpret_cast<ntfs::FILE_RECORD_SEGMENT_HEADER *>(&static_cast<unsigned char *>(buffer)[i]);
				if (frsh->MultiSectorHeader.Magic == 'ELIF' && frsh->MultiSectorHeader.unfixup(this->mft_record_size) && !!(frsh->Flags & ntfs::FRH_IN_USE))
				{
					unsigned int const frs_base = frsh->BaseFileRecordSegment ? static_cast<unsigned int>(frsh->BaseFileRecordSegment) : frs;
					if (frs_base >= this->records.size()) { this->records.resize(frs_base + 1, empty_record); }
					Records::iterator record = this->records.begin() + static_cast<ptrdiff_t>(frs_base);
					for (ntfs::ATTRIBUTE_RECORD_HEADER const
						*ah = frsh->begin(); ah < frsh->end(this->mft_record_size) && ah->Type != ntfs::AttributeTypeCode() && ah->Type != ntfs::AttributeEnd; ah = ah->next())
					{
						switch (ah->Type)
						{
						case ntfs::AttributeStandardInformation:
							if (ntfs::STANDARD_INFORMATION const *const fn = static_cast<ntfs::STANDARD_INFORMATION const *>(ah->Resident.GetValue()))
							{
								record->stdinfo.created = fn->CreationTime;
								record->stdinfo.written = fn->LastModificationTime;
								record->stdinfo.accessed = fn->LastAccessTime;
								record->stdinfo.attributes = fn->FileAttributes;
							}
							break;
						case ntfs::AttributeFileName:
							if (ntfs::FILENAME_INFORMATION const *const fn = static_cast<ntfs::FILENAME_INFORMATION const *>(ah->Resident.GetValue()))
							{
								unsigned int const frs_parent = static_cast<unsigned int>(fn->ParentDirectory);
								if (frs_base != frs_parent && fn->Flags != 0x02 /* FILE_NAME_DOS */)
								{
									LinkInfo const info = { frs_parent, static_cast<unsigned int>(this->names.size()), fn->FileNameLength };
									this->names.append(fn->FileName, fn->FileName + fn->FileNameLength);
									{
										size_t const link_index = this->nameinfos.size();
										this->nameinfos.push_back(LinkInfos::value_type(info, record->inameinfo));
										record->inameinfo = link_index;
									}
									if (frs_parent >= this->records.size())
									{
										this->records.resize(frs_parent + 1, empty_record);
										record = this->records.begin() + static_cast<ptrdiff_t>(frs_base);
									}
									size_t const ichild = this->childinfos.size();
									this->childinfos.push_back(ChildInfos::value_type(frs_base, this->records[frs_parent].children));
									this->records[frs_parent].children = ichild;
								}
							}
							break;
						case ntfs::AttributeData:
							{
								StreamInfo const info = { { static_cast<unsigned int>(this->names.size()), ah->NameLength }, ah->IsNonResident ? ah->Resident.ValueLength : ah->NonResident.DataSize, ah->IsNonResident ? ah->Length : ah->NonResident.CompressionUnit ? ah->NonResident.CompressedSize : ah->NonResident.AllocatedSize };
								this->names.append(ah->name(), ah->name() + ah->NameLength);
								{
									size_t const stream_index = this->streaminfos.size();
									this->streaminfos.push_back(StreamInfos::value_type(info, record->istreaminfo));
									record->istreaminfo = stream_index;
								}
							}
							break;
						default:
							break;
						}
					}
					// fprintf(stderr, "%llx\n", frsh->BaseFileRecordSegment);
				}
			}
		}
		++this->loads_so_far;
	}

	size_t get_path(key_type key, std::tstring &result) const volatile
	{
		this_type const *const me = this->unvolatile();
		lock_guard<mutex> const lock(me->_mutex);
		return me->get_path(key, result);
	}

	size_t get_path(key_type key, std::tstring &result) const
	{
		size_t actual_length = 0;
		while (~key.first && key.first != 5)
		{
			if (actual_length)
			{
				result.append(1, _T('\\'));
				++actual_length;
			}
			bool found = false;
			unsigned short ji = 0;
			for (size_t j = this->records[key.first].inameinfo; ~j; j = this->nameinfos[j].second, ++ji)
			{
				if (ji == key.second.first)
				{
					found = true;
					std::tstring::const_iterator const
						sb = this->names.begin() + static_cast<ptrdiff_t>(this->nameinfos[j].first.name.offset),
						se = sb + static_cast<ptrdiff_t>(this->nameinfos[j].first.name.length);
					result.append(sb, se);
					std::reverse(result.end() - (se - sb), result.end());
					actual_length += static_cast<size_t>(se - sb);
					key = key_type();
					key.first = this->nameinfos[j].first.parent /* ... | 0 | 0 (since we want the first name of all ancestors)*/;
					break;
				}
			}
			if (!found)
			{
				throw std::logic_error("could not find a file attribute");
				break;
			}
		}
		std::reverse(result.end() - static_cast<ptrdiff_t>(actual_length), result.end());
		return actual_length;
	}

	template<class F>
	void matches(F func, std::tstring const &pattern, std::tstring &path) const
	{
		std::vector<size_t> scratch;
		return lock_guard<mutex>(this->_mutex), this->matches<F>(func, pattern, path, 5, scratch);
	}

	template<class F>
	void matches(F func, std::tstring const &pattern, std::tstring &path, size_t const frs, std::vector<size_t> &scratch) const
	{
		bool const is_root = frs == 5;
		for (size_t i = frs < this->records.size() ? this->records[frs].children : ~size_t(); ~i; i = this->childinfos[i].second)
		{
			unsigned int const frs_child = static_cast<unsigned int>(this->childinfos[i].first);
			size_t n0 = scratch.size(), m = 0;
			for (size_t j = this->records[frs_child].inameinfo; ~j; j = this->nameinfos[j].second, ++m)
			{
				scratch.push_back(j);
			}
			for (unsigned short ji = 0; ji != m; ++ji)
			{
				LinkInfos::const_iterator const jj = this->nameinfos.begin() + static_cast<ptrdiff_t>(scratch[n0 + ji]);
				std::tstring::const_iterator const
					sb = this->names.begin() + static_cast<ptrdiff_t>(jj->first.name.offset),
					se = sb + static_cast<ptrdiff_t>(jj->first.name.length);
				path.append(!is_root, _T('\\'));
				path.append(sb, se);
				this->matches<F>(func, pattern, path, frs_child, scratch);
				if (wildcard(pattern.begin(), pattern.end(), path.begin(), path.end(), std::char_traits<std::tstring::value_type>()))
				{
					func(key_type(static_cast<key_type::first_type>(frs_child), key_type::second_type(ji, key_type::second_type::second_type())));
				}
				path.erase(path.end() - (se - sb), path.end());
				path.erase(path.end() - !is_root, path.end());
			}
		}
	}
};

class CMainDlg : public CModifiedDialogImpl<CMainDlg>, public WTL::CDialogResize<CMainDlg>, public CInvokeImpl<CMainDlg>, private WTL::CMessageFilter
{
	struct CThemedListViewCtrl : public WTL::CListViewCtrl, public WTL::CThemeImpl<CThemedListViewCtrl> { using WTL::CListViewCtrl::Attach; };
	typedef std::vector<NtfsIndex> Indices;
	class Threads : public std::vector<uintptr_t>
	{
		Threads(Threads const &) { }
		Threads &operator =(Threads const &) { return *this; }
	public:
		Threads(size_t const n) : std::vector<uintptr_t>(n) { }
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
	struct LimitedIocp
	{
		Handle iocp;
		Handle semaphore;
		explicit LimitedIocp(size_t const threads) : iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0)), semaphore(CreateSemaphore(NULL, static_cast<unsigned long>(threads), static_cast<unsigned long>(threads), NULL)) { }
	};
	class OverlappedNtfsMftRead : public Overlapped
	{
	public:
		explicit OverlappedNtfsMftRead(size_t const this_size, Handle const &semaphore) : Overlapped(this_size, semaphore) { }
		void operator()(size_t const size, uintptr_t const key)
		{
			NtfsIndex volatile *const p = reinterpret_cast<NtfsIndex volatile *>(key);
			p->load(this->virtual_offset, this->data(), size);
			p->check_finished();
		}
	};
	class OverlappedCompletion : public Overlapped
	{
		size_t n;
	public:
		explicit OverlappedCompletion(size_t const this_size, Handle const &semaphore, size_t const n) : Overlapped(this_size, semaphore), n(n) { }
		void operator()(size_t const size, uintptr_t const key)
		{
			(void) size;
			NtfsIndex volatile *const p = reinterpret_cast<NtfsIndex volatile *>(key);
			p->set_total_loads(this->n);
			p->check_finished();
		}
	};
	static unsigned long get_num_threads()
	{
		unsigned long num_threads;
		{
			SYSTEM_INFO sysinfo;
			GetSystemInfo(&sysinfo);
			num_threads = sysinfo.dwNumberOfProcessors;
		}
		return num_threads;
	}
	static unsigned int CALLBACK iocp_worker(void *iocp)
	{
		ULONG_PTR key;
		OVERLAPPED *overlapped_ptr;
		for (unsigned long nr; GetQueuedCompletionStatus(iocp, &nr, &key, &overlapped_ptr, INFINITE);)
		{
			std::auto_ptr<Overlapped> const overlapped(static_cast<Overlapped *>(overlapped_ptr));
			if (overlapped.get()) { (*overlapped)(static_cast<size_t>(nr), key); }
			else if (!key) { break; }
		}
		return 0;
	}
	typedef std::vector<std::pair<NtfsIndex volatile *, NtfsIndex::key_type> > Results;

	size_t num_threads;
	WTL::CEdit txtPattern;
	Results results;
	CThemedListViewCtrl lvFiles;
	Indices indices;
	Handle closing_event;
	LimitedIocp queue;
	Threads threads;
public:
	CMainDlg() : num_threads(static_cast<size_t>(get_num_threads())), closing_event(CreateEvent(NULL, TRUE, FALSE, NULL)), queue(num_threads * 100), threads(num_threads)
	{
		for (unsigned int i = 0; i != this->threads.size(); ++i)
		{
			unsigned int id;
			this->threads[i] = _beginthreadex(NULL, 0, iocp_worker, this->queue.iocp, 0, &id);
		}
	}
	void OnDestroy()
	{
		for (size_t i = 0; i != this->threads.size(); ++i)
		{
			PostQueuedCompletionStatus(this->queue.iocp, 0, 0, NULL);
		}
	}

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		_Module.GetMessageLoop()->AddMessageFilter(this);
		this->DlgResize_Init(false, false);
		this->txtPattern.Attach(this->GetDlgItem(IDC_EDITFILENAME));
		this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
		this->txtPattern.SetWindowText(_T("*"));
		{ LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 800, _T("Name") }; this->lvFiles.InsertColumn(0, &column); }
		SetWindowTheme(this->lvFiles, _T("Explorer"), NULL);
		this->lvFiles.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | 0x80000000 /*LVS_EX_COLUMNOVERFLOW*/ /*| 0x10000000 LVS_EX_AUTOSIZECOLUMNS*/);
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
		this->indices.reserve(path_names.size());
		for (size_t j = 0; j != path_names.size(); ++j)
		{
			Indices::iterator const i = this->indices.insert(this->indices.end(), NtfsIndex());
			std::tstring const path_name = path_names[j];
			async([this, i, path_name]() mutable
			/* TODO: Make this a separate struct */
			{
				_sleep(static_cast<unsigned long>(static_cast<size_t>(i - this->indices.begin()) + 1) * 25);
				if (uintptr_t const volume_handle = i->open(path_name))
				{
					void *const volume = reinterpret_cast<void *>(volume_handle);
					unsigned int cluster_size;
					long long mft_start_lcn;
					if (true)
					{
						unsigned long br;
						NTFS_VOLUME_DATA_BUFFER volume_data;
						CheckAndThrow(DeviceIoControl(volume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &volume_data, sizeof(volume_data), &br, NULL));
						cluster_size = static_cast<unsigned int>(volume_data.BytesPerCluster);
						mft_start_lcn = volume_data.MftStartLcn.QuadPart;
						i->mft_record_size = volume_data.BytesPerFileRecordSegment;
					}
					else
					{
						ntfs::NTFS_BOOT_SECTOR boot_sector;
						read(volume, 0, &boot_sector, sizeof(boot_sector));
						cluster_size = boot_sector.cluster_size();
						mft_start_lcn = boot_sector.MftStartLcn;
						i->mft_record_size = boot_sector.file_record_size();
					}
					CheckAndThrow(!!CreateIoCompletionPort(volume, this->queue.iocp, reinterpret_cast<uintptr_t>(&*i), 0));
					for (int pass = 0; pass < 2; ++pass)
					{
						unsigned int num_reads = 0;
						long long size = 0;
						typedef std::vector<std::pair<unsigned long long, long long> > RetPtrs;
						RetPtrs const ret_ptrs = get_mft_retrieval_pointers(volume, pass ? _T("$MFT::$DATA") : _T("$MFT::$BITMAP"), &size, mft_start_lcn, cluster_size, i->mft_record_size);
						std::vector<char> mft_bitmap_c;
						if (!pass) { mft_bitmap_c.resize(static_cast<size_t>(size)); }
						for (RetPtrs::const_iterator j = ret_ptrs.begin(); j != ret_ptrs.end(); ++j)
						{
							unsigned long nb;
							long long lbn = j->second;
							for (unsigned long long vbn = j != ret_ptrs.begin() ? (j - 1)->first : 0; vbn < j->first; vbn += nb)
							{
								if (WaitForSingleObject(this->closing_event, 0) == WAIT_OBJECT_0) { break; }
								nb = static_cast<unsigned long>(std::min(std::max(1ULL * cluster_size, 0x100000ULL), j->first - vbn));
								typedef OverlappedNtfsMftRead T;
								// TODO: There is an error here when the semaphore is closed by the main thread...
								std::auto_ptr<T> overlapped(new(nb) T(sizeof(T), this->queue.semaphore));
								overlapped->Offset = static_cast<unsigned long>(lbn);
								overlapped->OffsetHigh = static_cast<unsigned long>(lbn >> (CHAR_BIT * sizeof(overlapped->Offset)));
								overlapped->virtual_offset = vbn;
								if (!pass) { overlapped->hEvent = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(CreateEvent(NULL, TRUE, FALSE, NULL)) | 1); }
								++num_reads;
								if (ReadFile(volume, overlapped->data(), nb, NULL, static_cast<Overlapped *>(overlapped.get())))
								{
									// Completed synchronously... call the method
									if (pass)
									{
										CheckAndThrow(PostQueuedCompletionStatus(this->queue.iocp, static_cast<unsigned long>(overlapped->InternalHigh), reinterpret_cast<uintptr_t>(&*i), static_cast<Overlapped *>(overlapped.get())));
										overlapped.release();
									}
								}
								else if (GetLastError() != ERROR_IO_PENDING)
								{ CheckAndThrow(false); }
								else if (pass)
								{ overlapped.release(); }
								else
								{
									unsigned long nr;
									CheckAndThrow(GetOverlappedResult(volume, static_cast<Overlapped *>(overlapped.get()), &nr, TRUE));
									// TODO: $MFT::$BITMAP has been read; use it to avoid reading invalid portions on the second pass
									nr = std::min(nr, std::max(static_cast<unsigned long>(mft_bitmap_c.size()), static_cast<unsigned long>(overlapped->virtual_offset)) - static_cast<unsigned long>(overlapped->virtual_offset));
									if (nr /* otherwise the iterator might go out of bounds */)
									{
										std::copy(
											static_cast<char *>(overlapped->data()),
											static_cast<char *>(overlapped->data()) + static_cast<ptrdiff_t>(nr),
											mft_bitmap_c.begin() + static_cast<ptrdiff_t>(overlapped->virtual_offset));
									}
								}
								lbn += nb;
							}
						}
						if (pass)
						{
							typedef OverlappedCompletion T;
							CheckAndThrow(PostQueuedCompletionStatus(this->queue.iocp, num_reads, reinterpret_cast<uintptr_t>(&*i), static_cast<Overlapped *>(new(0) T(sizeof(T), this->queue.semaphore, num_reads))));
						}
						if (!pass) { vector_bool(mft_bitmap_c).swap(i->mft_bitmap); }
					}
				}
			});
		}
		return TRUE;
	}

	void OnOK(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{
		if (GetFocus() == this->txtPattern)
		{
			WTL::CWaitCursor wait;
			this->lvFiles.SetItemCount(0);
			this->results.clear();
			std::tstring pattern;
			{
				ATL::CComBSTR bstr;
				if (this->txtPattern.GetWindowText(bstr.m_str))
				{ pattern.assign(bstr, bstr.Length()); }
			}
			clock_t const start = clock();
			for (Indices::iterator i = this->indices.begin(); i != this->indices.end(); ++i)
			{
				uintptr_t const finished_event = i->finished_event();
				// if (WaitForSingleObject(reinterpret_cast<HANDLE>(finished_event), INFINITE) == WAIT_OBJECT_0)
				{
					std::tstring path;
					i->matches([this, i](NtfsIndex::key_type const key)
					{
						this->results.push_back(Results::value_type(&*i, key));
					}, pattern, path);
					this->lvFiles.SetItemCountEx(static_cast<int>(this->results.size()), LVSICF_NOINVALIDATEALL);
				}
			}
			clock_t const end = clock();
			_ftprintf(stderr, _T("%u ms\n"), (end - start) * 1000 / CLOCKS_PER_SEC);
		}
	}

	LRESULT OnFilesGetDispInfo(LPNMHDR pnmh)
	{
		NMLVDISPINFO *const pLV = (NMLVDISPINFO *) pnmh;

		if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0 && (pLV->item.mask & LVIF_TEXT) != 0)
		{
			Results::value_type const &result = this->results.at(static_cast<ptrdiff_t>(pLV->item.iItem));
			std::tstring path = result.first->root_path();
			switch (pLV->item.iSubItem)
			{
			case 0:
				// _ultot(result.second, pLV->item.pszText, 10);
				result.first->get_path(result.second, path);
				_tcsncpy(pLV->item.pszText, path.c_str(), pLV->item.cchTextMax);
				break;
			default:
				break;
			}
		}
		return 0;
	}

	void OnCancel(UINT /*uNotifyCode*/, int nID, HWND /*hWnd*/)
	{
		this->DestroyWindow();
		PostQuitMessage(nID);
		// this->EndDialog(nID);
	}

	BOOL PreTranslateMessage(MSG* pMsg)
	{
		return this->CWindow::IsDialogMessage(pMsg);
	}

	BEGIN_MSG_MAP_EX(CMainDlg)
		CHAIN_MSG_MAP(CInvokeImpl<CMainDlg>)
		CHAIN_MSG_MAP(CDialogResize<CMainDlg>)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnOK)
		NOTIFY_HANDLER_EX(IDC_LISTFILES, LVN_GETDISPINFO, OnFilesGetDispInfo)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_LISTFILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_EDITFILENAME, DLSZ_SIZE_X)
		// DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()
	enum { IDD = IDD_DIALOG1 };
};

WTL::CAppModule _Module;

int _tmain(int argc, TCHAR* argv[])
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

	return 0;
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
