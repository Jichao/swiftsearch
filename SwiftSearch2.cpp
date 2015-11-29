#include <process.h>
#include <stdio.h>
#include <tchar.h>

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

namespace winnt
{
	template<class F>
	inline F get_ntdll_proc(char const name []) { return reinterpret_cast<F>(GetProcAddress(GetModuleHandle(_T("NTDLL.dll")), name)); }

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
	typedef NTSTATUS NTAPI PNtOpenFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN OBJECT_ATTRIBUTES *ObjectAttributes, OUT IO_STATUS_BLOCK *IoStatusBlock, IN ULONG ShareAccess, IN ULONG OpenOptions);
	typedef NTSTATUS NTAPI PNtReadFile(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN IO_APC_ROUTINE *ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL);
	typedef VOID NTAPI PRtlInitUnicodeString(UNICODE_STRING * DestinationString, PCWSTR SourceString);
	typedef unsigned long NTAPI PRtlNtStatusToDosError(IN NTSTATUS NtStatus);
	static PNtOpenFile &NtOpenFile = *get_ntdll_proc<PNtOpenFile *>("NtOpenFile");
	static PNtReadFile &NtReadFile = *get_ntdll_proc<PNtReadFile *>("NtReadFile");
	static PRtlInitUnicodeString &RtlInitUnicodeString = *get_ntdll_proc<PRtlInitUnicodeString *>("RtlInitUnicodeString");
	static PRtlNtStatusToDosError &RtlNtStatusToDosError = *get_ntdll_proc<PRtlNtStatusToDosError *>("RtlNtStatusToDosError");
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
		size_t file_record_size() const { return this->ClustersPerFileRecordSegment >= 0 ? this->ClustersPerFileRecordSegment * this->SectorsPerCluster * this->BytesPerSector : 1U << static_cast<int>(-this->ClustersPerFileRecordSegment); }
		size_t cluster_size() const { return this->SectorsPerCluster * this->BytesPerSector; }
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

template<class T>
class buffered_ptr
{
	T *p;
	buffered_ptr(buffered_ptr &);
	buffered_ptr &operator =(buffered_ptr &);
public:
	explicit buffered_ptr(T *const released) : p(released) { }
	explicit buffered_ptr(size_t const n) : p(new(operator new(sizeof(T) + n)) T(sizeof(T))) { }
	~buffered_ptr() { if (p) { p->~T(); operator delete(p); } }
	T *get() const { return this->p; }
	T &operator *() const { return *this->get(); }
	T *operator->() const { return &**this; }
	T *release() { T *const p = this->p; this->p = NULL; return p; }
	void swap(buffered_ptr &other) { using std::swap; swap(this->p, other.p); }
};

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
		CheckAndThrow(ReadFile(file, buffer, size, &nr, &overlapped) || (GetLastError() == ERROR_IO_PENDING && GetOverlappedResult(file, &overlapped, &nr, TRUE)));
	}
}

ntfs::NTFS_BOOT_SECTOR read_boot_sector(void *const volume)
{
	ntfs::NTFS_BOOT_SECTOR boot_sector;
	read(volume, 0, &boot_sector, sizeof(boot_sector));
	return boot_sector;
}

unsigned int get_cluster_size(void *const volume)
{
	struct IO_STATUS_BLOCK { union { long Status; void *Pointer; }; uintptr_t Information; };
	typedef NTSTATUS NTAPI PNtQueryVolumeInformationFile(HANDLE FileHandle, IO_STATUS_BLOCK *IoStatusBlock, PVOID FsInformation, unsigned long Length, unsigned long FsInformationClass);
	PNtQueryVolumeInformationFile *const NtQueryVolumeInformationFile = reinterpret_cast<PNtQueryVolumeInformationFile *>(GetProcAddress(GetModuleHandle(TEXT("NTDLL.dll")), _CRT_STRINGIZE(NtQueryVolumeInformationFile)));
	struct FILE_FS_SIZE_INFORMATION { long long TotalAllocationUnits, ActualAvailableAllocationUnits; unsigned long SectorsPerAllocationUnit, BytesPerSector; };
	IO_STATUS_BLOCK iosb;
	FILE_FS_SIZE_INFORMATION info = {};
	if (!NtQueryVolumeInformationFile || NtQueryVolumeInformationFile(volume, &iosb, &info, sizeof(info), 3))
	{ SetLastError(ERROR_INVALID_FUNCTION), CheckAndThrow(false); }
	return info.BytesPerSector * info.SectorsPerAllocationUnit;
}

std::vector<std::pair<unsigned long long, long long> > get_mft_retrieval_pointers(void *const volume, TCHAR const path[], long long *const size = NULL)
{
	typedef std::vector<std::pair<unsigned long long, long long> > Result;
	Result result;
	ntfs::NTFS_BOOT_SECTOR const boot_sector = read_boot_sector(volume);
	if (false)
	{
		std::vector<unsigned char> file_record(boot_sector.file_record_size());
		read(volume, static_cast<unsigned long long>(boot_sector.MftStartLcn) * boot_sector.cluster_size(), file_record.empty() ? NULL : &*file_record.begin(), file_record.size() * sizeof(*file_record.begin()));
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
		for (unsigned long nr; !(success = DeviceIoControl(handle, FSCTL_GET_RETRIEVAL_POINTERS, &input, sizeof(input), &*result.begin(), result.size() * sizeof(*result.begin()), &nr, NULL), success) && GetLastError() == ERROR_MORE_DATA;)
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
			i->first *= boot_sector.cluster_size();
			i->second *= static_cast<long long>(boot_sector.cluster_size());
		}
	}
	return result;
}

unsigned int get_ntfs_cluster_size(void *const volume)
{
	ntfs::NTFS_BOOT_SECTOR boot_sector;
	read(volume, 0, &boot_sector, sizeof(boot_sector));
	return boot_sector.ClustersPerFileRecordSegment >= 0 ? boot_sector.ClustersPerFileRecordSegment * boot_sector.SectorsPerCluster * boot_sector.BytesPerSector : 1U << static_cast<int>(-boot_sector.ClustersPerFileRecordSegment);
}

class Overlapped : public OVERLAPPED
{
	void *p;
public:
	explicit Overlapped(size_t const this_size) : OVERLAPPED(), p(), semaphore(), virtual_offset() { this->p = reinterpret_cast<unsigned char *>(this) + this_size; }
	void *semaphore;
	unsigned long long virtual_offset;
	void *data() { return this->p; }
	void const *data() const { return this->p; }
	virtual void operator()(size_t const size, uintptr_t const key) = 0;
};

class NtfsIndex
{
public:
	std::vector<bool> mft_bitmap;
	unsigned int mft_record_size;
	explicit NtfsIndex(unsigned int const mft_record_size) : mft_record_size(mft_record_size) { }
	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size)
	{
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
					for (ntfs::ATTRIBUTE_RECORD_HEADER const
						*ah = frsh->begin(); ah < frsh->end(this->mft_record_size) && ah->Type != ntfs::AttributeTypeCode() && ah->Type != ntfs::AttributeEnd; ah = ah->next())
					{
						if (ah->Type == ntfs::AttributeFileName)
						{
							if (ntfs::FILENAME_INFORMATION const *const fn = static_cast<ntfs::FILENAME_INFORMATION const *>(ah->Resident.GetValue()))
							{
								_ftprintf(stderr, _T("%.*s\n"), fn->FileNameLength, fn->FileName);
							}
						}
					}
					// fprintf(stderr, "%llx\n", frsh->BaseFileRecordSegment);
				}
			}
		}
	}
};

class OverlappedNtfsMftRead : public Overlapped
{
public:
	explicit OverlappedNtfsMftRead(size_t const this_size) : Overlapped(this_size) { }
	virtual void operator()(size_t const size, uintptr_t const key)
	{
		if (NtfsIndex *const index = reinterpret_cast<NtfsIndex *>(key))
		{
			index->load(this->virtual_offset, this->data(), size);
		}
	}
};

unsigned int CALLBACK worker(void *iocp)
{
	ULONG_PTR key;
	OVERLAPPED *overlapped_ptr;
	for (unsigned long nr; GetQueuedCompletionStatus(iocp, &nr, &key, &overlapped_ptr, INFINITE);)
	{
		buffered_ptr<Overlapped> const overlapped(static_cast<Overlapped *>(overlapped_ptr));
		long prev;
		if (overlapped->semaphore)
		{ CheckAndThrow(ReleaseSemaphore(overlapped->semaphore, 1, &prev)); }
		(*overlapped)(static_cast<size_t>(nr), key);
	}
	return 0;
}

class CMainDlg : public CModifiedDialogImpl<CMainDlg>, public WTL::CDialogResize<CMainDlg>, public CInvokeImpl<CMainDlg>, private WTL::CMessageFilter
{
	struct CThemedListViewCtrl : public WTL::CListViewCtrl, public WTL::CThemeImpl<CThemedListViewCtrl> { using WTL::CListViewCtrl::Attach; };
	CThemedListViewCtrl lvFiles;
public:
	void OnDestroy()
	{
	}

	BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
	{
		_Module.GetMessageLoop()->AddMessageFilter(this);
		this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
		this->lvFiles.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP | 0x80000000 /*LVS_EX_COLUMNOVERFLOW*/ /*| 0x10000000 LVS_EX_AUTOSIZECOLUMNS*/);
		return TRUE;
	}

	void OnOK(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
	{

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
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP(CMainDlg)
		DLGRESIZE_CONTROL(IDC_LISTFILES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		DLGRESIZE_CONTROL(IDC_EDITFILENAME, DLSZ_SIZE_X)
		DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X)
	END_DLGRESIZE_MAP()
	enum { IDD = IDD_DIALOG1 };
};

WTL::CAppModule _Module;

VOID NTAPI IoApcRoutine(IN PVOID ApcContext, IN winnt::IO_STATUS_BLOCK *IoStatusBlock, IN ULONG Reserved)
{

}

#include <stddef.h>
#include <stdio.h>
#include <time.h>

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

int _tmain(int argc, TCHAR* argv[])
{
	pi();
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

	(void) argc;
	(void) argv;
	unsigned long num_threads;
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		num_threads = sysinfo.dwNumberOfProcessors;
	}
	Handle iocp(CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 0));
	std::tstring const volume_path = _T("C:\\");
	Handle volume(CreateFile((volume_path + _T("$Volume")).c_str(), FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL));
	NtfsIndex index(read_boot_sector(volume).file_record_size());
	unsigned int const cluster_size = get_ntfs_cluster_size(volume);
	CheckAndThrow(!!CreateIoCompletionPort(volume, iocp, reinterpret_cast<uintptr_t>(&index), 0));
	Handle semaphore(CreateSemaphore(NULL, num_threads, num_threads, NULL));
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
				WaitForMultipleObjects(n, reinterpret_cast<void *const *>(&*this->begin() + this->size() - n), TRUE, INFINITE);
				while (n) { this->pop_back(); --n; }
			}
		}
	} threads(num_threads);
	for (unsigned int i = 0; i != threads.size(); ++i)
	{
		unsigned int id;
		threads[i] = _beginthreadex(NULL, 0, worker, iocp, 0, &id);
	}
	for (int pass = 0; pass < 2; ++pass)
	{
		Handle io_event(CreateEvent(NULL, TRUE, FALSE, NULL));
		long long size = 0;
		std::vector<std::pair<unsigned long long, long long> > const ret_ptrs = get_mft_retrieval_pointers(volume, pass ? _T("$MFT::$DATA") : _T("$MFT::$BITMAP"), &size);
		std::vector<char> mft_bitmap_c(static_cast<size_t>(size));
		for (size_t i = 0; i != ret_ptrs.size(); ++i)
		{
			unsigned long nb;
			long long lbn = ret_ptrs[i].second;
			for (unsigned long long vbn = i ? ret_ptrs[i - 1].first : 0; vbn < ret_ptrs[i].first; vbn += nb)
			{
				nb = static_cast<unsigned long>(std::min(std::max(1ULL * cluster_size, 0x100000ULL), ret_ptrs[i].first - vbn));
				buffered_ptr<OverlappedNtfsMftRead> overlapped(nb);
				overlapped->semaphore = semaphore;
				overlapped->Offset = static_cast<unsigned long>(lbn);
				overlapped->OffsetHigh = static_cast<unsigned long>(lbn >> (CHAR_BIT * sizeof(overlapped->Offset)));
				overlapped->virtual_offset = vbn;
				if (pass) { CheckAndThrow(WaitForSingleObject(semaphore, INFINITE) == WAIT_OBJECT_0); }
				else { overlapped->hEvent = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(static_cast<void *>(io_event)) | 1); }
				if (ReadFile(volume, overlapped->data(), nb, NULL, overlapped.get()))
				{
					// Completed synchronously... call the method
					if (pass)
					{
						CheckAndThrow(PostQueuedCompletionStatus(iocp, overlapped->InternalHigh, reinterpret_cast<uintptr_t>(static_cast<void *>(volume)), overlapped.get()));
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
					CheckAndThrow(GetOverlappedResult(volume, overlapped.get(), &nr, TRUE));
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
		if (!pass) { vector_bool(mft_bitmap_c).swap(index.mft_bitmap); }
	}
	return 0;
}
