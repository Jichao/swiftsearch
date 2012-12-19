#pragma once

#ifndef NTOBJECT_HPP
#define NTOBJECT_HPP

#include <tchar.h>

// Do _NOT_ use <ntdkk.h>! use <wdm.h> instead, it's newer according to OSR.
//#include <wdm.h>

#include <Windows.h>
#include <WinIoCtl.h>
#include <NTSecAPI.h>

#include <ProvExce.h>

#if defined(_WDMDDK_)
#elif defined(_WINDOWS_)
#	if !defined(_WINIOCTL_)
#		error If you  #include <Windows.h>  then you should also  #include <WinIoCtl.h>
#	endif
#	if !defined(_NTSECAPI_)
#		error If you  #include <Windows.h>  then you should also  #include <NtSecAPI.h>
#	endif
#else
#	error You must either  #include <Windows.h>  or  #include <wdm.h>  (but do NOT  #include <ntddk.h>)
#endif

#pragma warning(push)
#	pragma warning(disable: 4005)  // macro redefinition
#	include <ntstatus.h>
#pragma warning(pop)

#include <stddef.h>
#include <stdlib.h>

#include <algorithm>
#include <iterator>
#include <string>
#include <vector>
#include <utility>  // pair

#ifndef NtDllProc
#define NtDllProc(fn) winnt::fn
#endif

#ifdef _NTDDK_
typedef int FileOptions;
typedef int FileCreationDisposition;
typedef int KernelObjectAttributes;
#else
enum FileOptions
{
	FILE_DIRECTORY_FILE = 0x00000001, FILE_SEQUENTIAL_ONLY = 0x00000004, FILE_NO_INTERMEDIATE_BUFFERING = 0x00000008,
	FILE_NON_DIRECTORY_FILE = 0x00000040, FILE_SYNCHRONOUS_IO_ALERT = 0x00000010, FILE_SYNCHRONOUS_IO_NONALERT = 0x00000020,
	FILE_OPEN_BY_FILE_ID = 0x00002000, FILE_OPEN_FOR_BACKUP_INTENT = 0x00004000, FILE_OPEN_REPARSE_POINT = 0x00200000,
	FILE_DELETE_ON_CLOSE = 0x00001000
};
enum FileCreationDisposition
{
	FILE_SUPERSEDED = 0x00000000, FILE_OPENED = 0x00000001, FILE_CREATED = 0x00000002,
	FILE_OVERWRITTEN = 0x00000003, FILE_EXISTS = 0x00000004, FILE_DOES_NOT_EXIST = 0x00000005
};
enum KernelObjectAttributes
{
	OBJ_INHERIT = 0x00000002, OBJ_PERMANENT = 0x00000010, OBJ_EXCLUSIVE = 0x00000020, OBJ_CASE_INSENSITIVE = 0x00000040,
	OBJ_OPENIF = 0x00000080, OBJ_OPENLINK = 0x00000100, OBJ_KERNEL_HANDLE = 0x00000200, OBJ_FORCE_ACCESS_CHECK = 0x00000400
};
#endif
enum TokenPrivilege
{
	SeCreateTokenPrivilege = 2, SeAssignPrimaryTokenPrivilege = 3, SeLockMemoryPrivilege = 4,
	SeIncreaseQuotaPrivilege = 5, SeMachineAccountPrivilege = 6, SeUnsolicitedInputPrivilege = 6,
	SeTCBPrivilege = 7, SeSecurityPrivilege = 8, SeTakeOwnershipPrivilege = 9, SeLoadDriverPrivilege = 10,
	SeSystemProfilePrivilege = 11, SeSystemTimePrivilege = 12, SeProfileSingleProcessPrivilege = 13,
	SeIncreaseBasePriorityPrivilege = 14, SeCreatePagefilePrivilege = 15, SeCreatePermanentPrivilege = 16,
	SeBackupPrivilege = 17, SeRestorePrivilege = 18, SeShutdownPrivilege = 19, SeDebugPrivilege = 20,
	SeAuditPrivilege = 21, SeSystemEnvironmentPrivilege = 22, SeChangeNotifyPrivilege = 23,
	SeRemoteShutdownPrivilege = 24, SeUndockPrivilege = 25, SeSyncAgentPrivilege = 26, SeEnableDelegationPrivilege = 27,
	SeManageVolumePrivilege = 28, SeImpersonatePrivilege = 29, SeCreateGlobalPrivilege = 30
};

#ifndef MOUNTMGR_DEVICE_NAME
#	define MOUNTMGR_DEVICE_NAME L"\\Device\\MountPointManager"
#endif
#ifndef MOUNTMGR_DOS_DEVICE_NAME
#	define MOUNTMGR_DOS_DEVICE_NAME L"\\\\.\\MountPointManager"
#endif

enum
{
#ifndef IOCTL_VOLUME_BASE
	IOCTL_VOLUME_BASE = 'V',
#endif
#ifndef IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
	IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS = CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
};

enum
{
#ifndef IOCTL_STORAGE_BASE
	IOCTL_STORAGE_BASE = '-',
#endif
#ifndef IOCTL_STORAGE_GET_DEVICE_NUMBER
	IOCTL_STORAGE_GET_DEVICE_NUMBER = CTL_CODE(IOCTL_STORAGE_BASE, 0x0420, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
};

enum
{
#ifndef MOUNTMGRCONTROLTYPE
	MOUNTMGRCONTROLTYPE = 'm',
#endif
#ifndef MOUNTDEVCONTROLTYPE
	MOUNTDEVCONTROLTYPE = 'M',
#endif
#ifndef IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH
	IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH = CTL_CODE(MOUNTMGRCONTROLTYPE, 12, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
#ifndef IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS
	IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS = CTL_CODE(MOUNTMGRCONTROLTYPE, 13, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
};

enum
{
#ifndef FSCTL_GET_NTFS_VOLUME_DATA
	FSCTL_GET_NTFS_VOLUME_DATA = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 25, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
#ifndef FSCTL_GET_NTFS_FILE_RECORD
	FSCTL_GET_NTFS_FILE_RECORD = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 26, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
#ifndef FSCTL_GET_RETRIEVAL_POINTERS
	FSCTL_GET_RETRIEVAL_POINTERS = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 28,  METHOD_NEITHER, FILE_ANY_ACCESS),
#endif
#ifndef FSCTL_IS_VOLUME_MOUNTED
	FSCTL_IS_VOLUME_MOUNTED = CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 10, METHOD_BUFFERED, FILE_ANY_ACCESS),
#endif
};

typedef LONG NTSTATUS;
typedef VOID (NTAPI *PIO_APC_ROUTINE)(IN PVOID ApcContext, IN struct _IO_STATUS_BLOCK * IoStatusBlock, IN ULONG Reserved);
#if !defined(_NTDDK_)
typedef struct _IO_STATUS_BLOCK { union { NTSTATUS Status; PVOID Pointer; }; ULONG_PTR Information; } IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
#endif
#if !defined(_NTDDK_) || !defined(NTDDI_VISTA)
typedef enum _IO_PRIORITY_HINT { IoPriorityVeryLow = 0, IoPriorityLow, IoPriorityNormal, IoPriorityHigh, IoPriorityCritical, MaxIoPriorityTypes } IO_PRIORITY_HINT;
#endif
typedef struct _CURDIR { UNICODE_STRING DosPath; HANDLE Handle; } CURDIR, *PCURDIR;
typedef struct _FILE_END_OF_FILE_INFORMATION { LARGE_INTEGER EndOfFile; } FILE_END_OF_FILE_INFORMATION, *PFILE_END_OF_FILE_INFORMATION;
#if !defined(_NTDDK_) || !defined(NTDDI_VISTA)
# pragma warning(push)
# pragma warning(disable: 4324)
typedef struct DECLSPEC_ALIGN(8) _FILE_IO_PRIORITY_HINT_INFORMATION { IO_PRIORITY_HINT PriorityHint; } FILE_IO_PRIORITY_HINT_INFORMATION, *PFILE_IO_PRIORITY_HINT_INFORMATION;
# pragma warning(pop)
#endif
typedef struct _FILE_MODE_INFORMATION { ULONG FileMode; } FILE_MODE_INFORMATION, *PFILE_MODE_INFORMATION;
typedef struct _FILE_NAME_INFORMATION { ULONG FileNameLength; WCHAR FileName[1]; } FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;
#if !defined(_NTDDK_)
typedef struct _FILE_POSITION_INFORMATION { LARGE_INTEGER CurrentByteOffset; } FILE_POSITION_INFORMATION, *PFILE_POSITION_INFORMATION;
#endif
#if !defined(_NTDDK_)
typedef struct _FILE_STANDARD_INFORMATION { LARGE_INTEGER AllocationSize; LARGE_INTEGER EndOfFile; ULONG NumberOfLinks; BOOLEAN DeletePending; BOOLEAN Directory; } FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;
#endif
#if !defined(_NTDDK_)
typedef struct _FILE_FS_DEVICE_INFORMATION { ULONG DeviceType; ULONG Characteristics; } FILE_FS_DEVICE_INFORMATION, *PFILE_FS_DEVICE_INFORMATION;
#endif
typedef struct _FILE_FS_SIZE_INFORMATION { LARGE_INTEGER TotalAllocationUnits; LARGE_INTEGER AvailableAllocationUnits; ULONG SectorsPerAllocationUnit; ULONG BytesPerSector; } FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;
typedef struct _MOUNTMGR_TARGET_NAME { USHORT DeviceNameLength; WCHAR DeviceName[1]; } MOUNTMGR_TARGET_NAME, *PMOUNTMGR_TARGET_NAME;
typedef struct _MOUNTMGR_VOLUME_PATHS { ULONG MultiSzLength; WCHAR MultiSz[1]; } MOUNTMGR_VOLUME_PATHS, *PMOUNTMGR_VOLUME_PATHS;
#if defined(_NTDDK_)
typedef struct _NTFS_FILE_RECORD_OUTPUT_BUFFER { LARGE_INTEGER FileReferenceNumber; ULONG FileRecordLength; UCHAR FileRecordBuffer[1]; } NTFS_FILE_RECORD_OUTPUT_BUFFER, *PNTFS_FILE_RECORD_OUTPUT_BUFFER;
typedef struct _NTFS_VOLUME_DATA_BUFFER { LARGE_INTEGER VolumeSerialNumber; LARGE_INTEGER NumberSectors; LARGE_INTEGER TotalClusters; LARGE_INTEGER FreeClusters; LARGE_INTEGER TotalReserved; ULONG BytesPerSector; ULONG BytesPerCluster; ULONG BytesPerFileRecordSegment; ULONG ClustersPerFileRecordSegment; LARGE_INTEGER MftValidDataLength; LARGE_INTEGER MftStartLcn; LARGE_INTEGER Mft2StartLcn; LARGE_INTEGER MftZoneStart; LARGE_INTEGER MftZoneEnd; } NTFS_VOLUME_DATA_BUFFER, *PNTFS_VOLUME_DATA_BUFFER;
#endif
#if !defined(_NTDDK_)
typedef struct _OBJECT_ATTRIBUTES { ULONG Length; HANDLE RootDirectory; PUNICODE_STRING ObjectName; ULONG Attributes; struct _SECURITY_DESCRIPTOR * SecurityDescriptor; struct _SECURITY_QUALITY_OF_SERVICE * SecurityQualityOfService; } OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
#endif
typedef struct _OBJECT_BASIC_INFORMATION { ULONG Attributes; ACCESS_MASK DesiredAccess; ULONG HandleCount; ULONG ReferenceCount; ULONG PagedPoolQuota; ULONG NonPagedPoolQuota; UCHAR Unknown2[32]; } OBJECT_BASIC_INFORMATION, *POBJECT_BASIC_INFORMATION;
#if defined(_NTDDK_)
typedef struct _RETRIEVAL_POINTERS_BUFFER { ULONG ExtentCount; LARGE_INTEGER StartingVcn; struct { LARGE_INTEGER NextVcn; LARGE_INTEGER Lcn; } Extents[1]; } RETRIEVAL_POINTERS_BUFFER, *PRETRIEVAL_POINTERS_BUFFER;
typedef struct _STARTING_VCN_INPUT_BUFFER { LARGE_INTEGER StartingVcn; } STARTING_VCN_INPUT_BUFFER, *PSTARTING_VCN_INPUT_BUFFER;
#endif
#if !defined(_NTDDSTOR_H_)
typedef struct _STORAGE_DEVICE_NUMBER { DEVICE_TYPE DeviceType; ULONG DeviceNumber; ULONG PartitionNumber; } STORAGE_DEVICE_NUMBER, *PSTORAGE_DEVICE_NUMBER;
#endif
typedef struct _THREAD_IO_PRIORITY_INFORMATION { ULONG IoPriority; } THREAD_IO_PRIORITY_INFORMATION, *PTHREAD_IO_PRIORITY_INFORMATION;

template<typename To, typename From>
inline To int_cast(const From value) //throw(std::out_of_range)
{	
	To result = To(value);
	const From reverted = From(result);
	if (reverted != value || (result >= To()) != (value >= From()))
	{
		throw std::out_of_range("numeric_cast: value out of range");
	}
	return result;
}

namespace winnt
{
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_END_OF_FILE_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(20); }
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_IO_PRIORITY_HINT_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(43); }
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_MODE_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(16); }
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_NAME_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(9); }
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_POSITION_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(14); }
	inline enum _FILE_INFORMATION_CLASS GetInfoClass(_FILE_STANDARD_INFORMATION const *) { return static_cast<enum _FILE_INFORMATION_CLASS>(5); }
	inline enum _FS_INFORMATION_CLASS GetInfoClass(_FILE_FS_DEVICE_INFORMATION const *) { return static_cast<enum _FS_INFORMATION_CLASS>(4); }
	inline enum _FS_INFORMATION_CLASS GetInfoClass(_FILE_FS_SIZE_INFORMATION const *) { return static_cast<enum _FS_INFORMATION_CLASS>(3); }
	inline enum _OBJECT_INFORMATION_CLASS GetInfoClass(_OBJECT_BASIC_INFORMATION const *) { return static_cast<enum _OBJECT_INFORMATION_CLASS>(0); }
	struct ObjectAttributes
	{
		enum { DEFAULT_ATTRIBUTES = 0x00000040L /*OBJ_CASE_INSENSITIVE*/, };
		struct _OBJECT_ATTRIBUTES value;
		UNICODE_STRING name;
		ULONGLONG frn;
		bool isUsingFrn;

	public:
		ObjectAttributes(
			LPCTSTR const &name = NULL, HANDLE rootDirectory = NULL,
			ULONG attributes = DEFAULT_ATTRIBUTES,
			struct _SECURITY_DESCRIPTOR * pSecurityDescriptor = NULL)
		{
			this->isUsingFrn = false;
			size_t cchName = 0;
			while (name != NULL && name[cchName] != TEXT('\0')) { cchName++; }
			this->initialize(
				const_cast<LPTSTR>(name), int_cast<USHORT>(cchName), rootDirectory,
				attributes, pSecurityDescriptor);
		}

		ObjectAttributes(
			ULONGLONG fileReferenceNumber, HANDLE rootDirectory,
			ULONG attributes = DEFAULT_ATTRIBUTES,
			struct _SECURITY_DESCRIPTOR * pSecurityDescriptor = NULL)
		{
			this->isUsingFrn = true;
			this->frn = fileReferenceNumber;
			this->initialize(
				reinterpret_cast<LPTSTR>(&this->frn), sizeof(this->frn) / sizeof(*this->name.Buffer),
				rootDirectory, attributes, pSecurityDescriptor);
		}

		ObjectAttributes(
			LPCTSTR const &name, size_t cchName, HANDLE rootDirectory = NULL,
			ULONG attributes = DEFAULT_ATTRIBUTES,
			struct _SECURITY_DESCRIPTOR * pSecurityDescriptor = NULL)
		{
			this->isUsingFrn = false;
			this->initialize(
				const_cast<LPTSTR>(name), int_cast<USHORT>(cchName), rootDirectory,
				attributes, pSecurityDescriptor);
		}

		ObjectAttributes(
			std::basic_string<TCHAR> &name, HANDLE rootDirectory = NULL,
			ULONG attributes = DEFAULT_ATTRIBUTES,
			struct _SECURITY_DESCRIPTOR * pSecurityDescriptor = NULL)
		{
			this->isUsingFrn = false;
			this->initialize(
				name.size() > 0 ? &name[0] : NULL, int_cast<USHORT>(name.size()),
				rootDirectory, attributes, pSecurityDescriptor);
		}

		void initialize(
			LPCTSTR name, size_t cchName, HANDLE rootDirectory = NULL,
			ULONG attributes = DEFAULT_ATTRIBUTES,
			struct _SECURITY_DESCRIPTOR * pSecurityDescriptor = NULL)
		{
			this->name.Buffer = const_cast<LPTSTR>(name);
			this->name.MaximumLength = this->name.Length = int_cast<USHORT>(cchName * sizeof(*name));
			this->value.Length = sizeof(struct _OBJECT_ATTRIBUTES);
			this->value.RootDirectory = rootDirectory;
			this->value.SecurityDescriptor = pSecurityDescriptor;
			this->value.SecurityQualityOfService = NULL;
			this->value.Attributes = attributes;
			this->value.ObjectName = &this->name;
		}

		operator struct _OBJECT_ATTRIBUTES &()
		{
			this->value.ObjectName = this->name.Buffer != NULL ? &this->name : NULL;
			if (this->isUsingFrn) { this->name.Buffer = reinterpret_cast<LPTSTR>(&this->frn); }
			return this->value;
		}

		operator struct _OBJECT_ATTRIBUTES *()
		{
			this->value.ObjectName = this->name.Buffer != NULL ? &this->name : NULL;
			if (this->isUsingFrn) { this->name.Buffer = reinterpret_cast<LPTSTR>(&this->frn); }
			return &this->value;
		}
	};

	// Functions ================================================================

	namespace
	{
#if defined(_SECURE_SCL) && _SECURE_SCL && _CPPLIB_VER >= 520
		template<class It> stdext::unchecked_array_iterator<It> unchecked_iterator(It const &it)
		{ return stdext::unchecked_array_iterator<It>(it); }

		stdext::checked_array_iterator<PWSTR> checked_iterator(PUNICODE_STRING p)
		{ return stdext::checked_array_iterator<PWSTR>(p->Buffer, p->Length / sizeof(*p->Buffer)); }
		template<class It>
		stdext::checked_array_iterator<It> checked_iterator(It const &it, size_t size, size_t index = 0)
		{ return stdext::checked_array_iterator<It>(it, size, index); }
		template<class T, size_t N>
		stdext::checked_array_iterator<T *> checked_iterator(T (&arr)[N])
		{ return stdext::checked_array_iterator<T *>(arr, N); }
#else
		template<class It> It unchecked_iterator(It it) { return it; }

		PWSTR checked_iterator(PUNICODE_STRING p) { return p->Buffer; }
		template<class It> It checked_iterator(It it) { return it; }
		template<class T, size_t N> T *checked_iterator(T (&arr)[N]) { return arr; }
#endif
	}

	extern "C"
	{
		NTSYSAPI NTSTATUS NTAPI RtlAdjustPrivilege(IN ULONG Privilege, IN BOOLEAN Enable, IN BOOLEAN Client, OUT PBOOLEAN WasEnabled);
		NTSYSAPI BOOLEAN NTAPI RtlDosPathNameToNtPathName_U(IN PCWSTR DosPathName, OUT UNICODE_STRING * NtPathName, OUT PCWSTR *NtFileNamePart, OUT struct _CURDIR *DirectoryInfo);
		NTSYSAPI VOID NTAPI RtlFreeUnicodeString(IN OUT UNICODE_STRING * UnicodeString);
		NTSYSAPI ULONG NTAPI RtlGetLastWin32Error(VOID);
		NTSYSAPI NTSTATUS NTAPI RtlLocalTimeToSystemTime(IN LARGE_INTEGER const *LocalTime, OUT PLARGE_INTEGER SystemTime);
		NTSYSAPI ULONG NTAPI RtlNtStatusToDosError(IN NTSTATUS Status);
		NTSYSAPI NTSTATUS NTAPI RtlNtPathNameToDosPathName(IN ULONG Flags, IN OUT /*PRTL_UNICODE_STRING_BUFFER*/ UNICODE_STRING * Path, OUT PULONG Disposition OPTIONAL, IN OUT PWSTR *FilePart OPTIONAL);
		NTSYSAPI VOID NTAPI RtlRaiseStatus(NTSTATUS Status);
		NTSYSAPI VOID NTAPI RtlSetLastWin32Error(ULONG ErrorCode);
		NTSYSAPI VOID NTAPI RtlSetLastWin32ErrorAndNtStatusFromNtStatus(NTSTATUS Status);
		NTSYSAPI VOID NTAPI RtlSecondsSince1970ToTime(IN ULONG ElapsedSeconds, OUT PLARGE_INTEGER Time);
		NTSYSAPI VOID NTAPI RtlSecondsSince1980ToTime(IN ULONG ElapsedSeconds, OUT PLARGE_INTEGER Time);
		NTSYSAPI BOOLEAN NTAPI RtlTimeToSecondsSince1970(IN PLARGE_INTEGER Time, OUT PULONG ElapsedSeconds);
		NTSYSAPI BOOLEAN NTAPI RtlTimeToSecondsSince1980(IN PLARGE_INTEGER Time, OUT PULONG ElapsedSeconds);

		NTSYSAPI NTSTATUS NTAPI NtAssignProcessToJobObject(HANDLE JobHandle, HANDLE ProcessHandle );
		NTSYSAPI NTSTATUS NTAPI NtClose(IN HANDLE Handle);
		NTSYSAPI NTSTATUS NTAPI NtCreateFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN struct _OBJECT_ATTRIBUTES * ObjectAttributes, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes, IN ULONG ShareAccess, IN ULONG CreateDisposition, IN ULONG CreateOptions, IN PVOID EaBuffer, IN ULONG EaLength);
		NTSYSAPI NTSTATUS NTAPI NtCreateJobObject(OUT PHANDLE JobHandle, IN ACCESS_MASK DesiredAccess, IN struct _OBJECT_ATTRIBUTES * ObjectAttributes);
		NTSYSAPI NTSTATUS NTAPI NtDeleteFile(IN struct _OBJECT_ATTRIBUTES * ObjectAttributes);
		NTSYSAPI NTSTATUS NTAPI NtDeviceIoControlFile(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, IN ULONG IoControlCode, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength);
		NTSYSAPI NTSTATUS NTAPI NtDuplicateObject(IN HANDLE SourceProcessHandle, IN HANDLE SourceHandle, IN HANDLE TargetProcessHandle OPTIONAL, OUT PHANDLE TargetHandle OPTIONAL, IN ACCESS_MASK DesiredAccess, IN ULONG HandleAttributes, IN ULONG Options);
		NTSYSAPI NTSTATUS NTAPI NtFsControlFile(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, IN ULONG FsControlCode, IN PVOID InputBuffer OPTIONAL, IN ULONG InputBufferLength, OUT PVOID OutputBuffer OPTIONAL, IN ULONG OutputBufferLength);
		NTSYSAPI NTSTATUS NTAPI NtMakePermanentObject(IN HANDLE Handle);
		NTSYSAPI NTSTATUS NTAPI NtOpenDirectoryObject(OUT PHANDLE DirectoryHandle, IN ACCESS_MASK DesiredAccess, IN struct _OBJECT_ATTRIBUTES * ObjectAttributes);
		NTSYSAPI NTSTATUS NTAPI NtOpenFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN struct _OBJECT_ATTRIBUTES * ObjectAttributes, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, IN ULONG ShareAccess, IN ULONG OpenOptions);
		NTSYSAPI NTSTATUS NTAPI NtQueryDirectoryFile(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN enum _FILE_INFORMATION_CLASS FileInformationClass, IN BOOLEAN ReturnSingleEntry, IN UNICODE_STRING * FileName OPTIONAL, IN BOOLEAN RestartScan);
		NTSYSAPI NTSTATUS NTAPI NtQueryInformationFile(IN HANDLE FileHandle, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length, IN enum _FILE_INFORMATION_CLASS FileInformationClass);
		NTSYSAPI NTSTATUS NTAPI NtQueryObject(IN HANDLE ObjectHandle, IN enum _OBJECT_INFORMATION_CLASS ObjectInformationClass, OUT PVOID ObjectInformation, IN ULONG Length, OUT PULONG ResultLength);
		NTSYSAPI NTSTATUS NTAPI NtQueryVolumeInformationFile(IN HANDLE FileHandle, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, OUT PVOID FsInformation, IN ULONG Length, IN enum _FS_INFORMATION_CLASS FsInformationClass);
		NTSYSAPI NTSTATUS NTAPI NtReadFile(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL, IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, OUT PVOID Buffer, IN ULONG Length, IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL);
		NTSYSAPI NTSTATUS NTAPI NtSetInformationFile(IN HANDLE FileHandle, OUT struct _IO_STATUS_BLOCK * IoStatusBlock, IN PVOID FileInformation, IN ULONG Length, IN enum _FILE_INFORMATION_CLASS FileInformationClass);
		NTSYSAPI NTSTATUS NTAPI NtWaitForSingleObject(IN HANDLE Handle, IN BOOLEAN Alertable, IN PLARGE_INTEGER Timeout OPTIONAL);
		NTSYSAPI NTSTATUS NTAPI NtWaitForMultipleObjects(IN ULONG HandleCount, IN PHANDLE Handles, IN enum _WAIT_TYPE WaitType, IN BOOLEAN Alertable, IN PLARGE_INTEGER Timeout OPTIONAL);
	}

	struct NtStatus
	{
		NTSTATUS value;
		NtStatus(NTSTATUS const &value = 0) : value(value) { }
		operator NTSTATUS &() { return this->value; }
		operator NTSTATUS const &() const { return this->value; }
		UCHAR GetSeverity() const { return static_cast<UCHAR>((this->value >> (sizeof(this->value) * CHAR_BIT - 2)) & 3); }
		bool NtStatusIsBadBufferSize() const
		{
			switch (this->value)
			{
			case STATUS_BUFFER_TOO_SMALL:
			case STATUS_BUFFER_OVERFLOW:
			case STATUS_INFO_LENGTH_MISMATCH:
				return true;
			default:
				return false;
			}
		}

		FORCEINLINE void CheckAndThrow() const
		{
			if (this->value != 0)
			{
				ThrowNtStatus(this->value);
			}
		}

#if defined(_MSC_VER)
		__declspec(noinline)
#endif
		static void ThrowWin32(unsigned long exCode)
		{
			NtDllProc(RtlSetLastWin32Error)(exCode);
#ifdef _PROVIDER_EXCEPT_H
			struct CppExceptionThrower
			{
				static void throw_(unsigned long exCode, struct _EXCEPTION_POINTERS *pExPtrs)
				{ throw CStructured_Exception(static_cast<UINT>(exCode), pExPtrs); }
				static bool assign(struct _EXCEPTION_POINTERS **to, struct _EXCEPTION_POINTERS *from)
				{ *to = from; return true; }
			};
			struct _EXCEPTION_POINTERS *pExPtrs = NULL;
			bool thrown = false;
			__try { RaiseException(exCode, 0, 0, NULL); }
			__except (
				(CppExceptionThrower::assign(&pExPtrs, GetExceptionInformation()))
				? EXCEPTION_EXECUTE_HANDLER
				: EXCEPTION_CONTINUE_SEARCH)
			{ exCode = GetExceptionCode(); thrown = true; }
			if (thrown) { CppExceptionThrower::throw_(exCode, pExPtrs); }
#else
			RaiseException(exCode, 0, 0, NULL);
#endif
		}

#if defined(_MSC_VER)
		__declspec(noinline)
#endif
		static void ThrowNtStatus(NTSTATUS exCode)
		{
			NtDllProc(RtlSetLastWin32ErrorAndNtStatusFromNtStatus)(exCode);
#ifdef _PROVIDER_EXCEPT_H
			struct CppExceptionThrower
			{
				static void throw_(NTSTATUS exCode, struct _EXCEPTION_POINTERS *pExPtrs)
				{ throw CStructured_Exception(static_cast<UINT>(exCode), pExPtrs); }
				static bool assign(struct _EXCEPTION_POINTERS **to, struct _EXCEPTION_POINTERS *from)
				{ *to = from; return true; }
			};
			struct _EXCEPTION_POINTERS *pExPtrs = NULL;
			bool thrown = false;
			__try { NtDllProc(RtlRaiseStatus)(exCode); }
			__except (
				(CppExceptionThrower::assign(&pExPtrs, GetExceptionInformation()))
				? EXCEPTION_EXECUTE_HANDLER
				: EXCEPTION_CONTINUE_SEARCH)
			{ exCode = static_cast<NTSTATUS>(GetExceptionCode()); thrown = true; }
			if (thrown) { CppExceptionThrower::throw_(exCode, pExPtrs); }
#else
			NtDllProc(RtlRaiseStatus)(exCode);
#endif
		}
	};

	namespace
	{
		UNICODE_STRING ToString(wchar_t const *text, size_t cchText)
		{
			UNICODE_STRING result =
			{
				int_cast<USHORT>(cchText * sizeof(*text)),
				int_cast<USHORT>(cchText * sizeof(*text)),
				const_cast<LPTSTR>(text)
			};
			return result;
		}

		UNICODE_STRING ToString(wchar_t const *text)
		{
			return ToString(text, wcslen(text));
		}

		UNICODE_STRING ToString(std::basic_string<wchar_t> const *pS)
		{
			return ToString(pS->data(), pS->size());
		}

		STRING ToString(char const *const text, size_t cchText)
		{
			STRING result =
			{
				int_cast<USHORT>(cchText * sizeof(*text)),
				int_cast<USHORT>(cchText * sizeof(*text)),
				const_cast<char *>(text)
			};
			return result;
		}

		STRING ToString(char const *const text)
		{
			return ToString(text, strlen(text));
		}

		STRING ToString(std::basic_string<char> const *pS)
		{
			return ToString(pS->data(), pS->size());
		}

		FORCEINLINE ptrdiff_t bytediff(void const *pA, void const *pB)
		{ return static_cast<char const *>(pA) - static_cast<char const *>(pB); }

		template<typename T> struct size_of { static size_t const value = sizeof(T); };
		template<> struct size_of<void> { static size_t const value = 0; };
		template<typename T> T &dereference(T &v) { return v; }
		template<typename T> T const &dereference(T const &v) { return v; }
	}

	struct Access
	{
		enum Mask
		{
			QueryAttributes = FILE_READ_ATTRIBUTES,
			Read = FILE_READ_DATA | FILE_LIST_DIRECTORY | KEY_QUERY_VALUE,  // Also: DIRECTORY_QUERY
			SetAttributes = FILE_WRITE_ATTRIBUTES,
			Write = FILE_WRITE_DATA | FILE_ADD_FILE | KEY_SET_VALUE,
			Append = FILE_APPEND_DATA | FILE_ADD_SUBDIRECTORY | FILE_CREATE_PIPE_INSTANCE | KEY_CREATE_SUB_KEY,
			EnumerateSubKeys = KEY_ENUMERATE_SUB_KEYS | FILE_READ_EA,
			Execute = FILE_EXECUTE | FILE_TRAVERSE,
			DeleteChild = FILE_DELETE_CHILD,
			Synchronize = SYNCHRONIZE,
			Delete = DELETE,
			ReadControl = READ_CONTROL,
			WriteDAC = WRITE_DAC,
			WriteOwner = WRITE_OWNER,
			GenericRead = GENERIC_READ,
			GenericWrite = GENERIC_WRITE,
			GenericExecute = GENERIC_EXECUTE,
			GenericAll = GENERIC_ALL,
			MaximumAllowed = MAXIMUM_ALLOWED,
		};
		Mask Value;
		Access() : Value() { }
		Access(Mask const value) : Value(value) { }
		Access(int const value) : Value(static_cast<Mask>(value)) { }
		Access(ACCESS_MASK const value) : Value(static_cast<Mask>(value)) { }
		operator Mask &() { return this->Value; }
		operator Mask const &() const { return this->Value; }
		operator ACCESS_MASK() const { return static_cast<ACCESS_MASK>(this->Value); }
		Mask *operator &() { return &this->Value; }
		Mask const *operator &() const { return &this->Value; }
		bool operator ==(Mask const other) const { return this->Value == other; }
		bool operator !=(Mask const other) const { return this->Value != other; }
		Access operator &(Access const other) const { return this->Value & other.Value; }
		Access operator &(Mask const other) const { return this->Value & other; }
		Access operator |(Access const other) const { return this->Value | other.Value; }
		Access operator |(Mask const other) const { return this->Value | other; }
		Access operator ^(Access const other) const { return this->Value ^ other.Value; }
		Access operator ^(Mask const other) const { return this->Value ^ other; }
		Access &operator &=(Access const other) { *this = *this & other.Value; return *this; }
		Access &operator &=(Mask const other) { *this = *this & other; return *this; }
		Access &operator |=(Access const other) { *this = *this | other.Value; return *this; }
		Access &operator |=(Mask const other) { *this = *this | other; return *this; }
		Access &operator ^=(Access const other) { *this = *this ^ other.Value; return *this; }
		Access &operator ^=(Mask const other) { *this = *this ^ other; return *this; }
		bool Get(Mask const mask) const { return (this->Value & mask) != 0; }
		void Clear(Mask const mask) { *this &= static_cast<Mask>(~mask); }
		void Set(Mask const mask) { *this |= mask; }
	};

	class NtSystem
	{
	public:
		static ULONG RtlTimeToSecondsSince1970(LONGLONG time)
		{
			LARGE_INTEGER time2;
			time2.QuadPart = time;
			ULONG result = 0;
			if (!NtDllProc(RtlTimeToSecondsSince1970)(&time2, &result) && time != 0)
			{ NtStatus(STATUS_UNSUCCESSFUL).CheckAndThrow(); }
			return result;
		}

		static ULONG RtlTimeToSecondsSince1980(LONGLONG time)
		{
			LARGE_INTEGER time2;
			time2.QuadPart = time;
			ULONG result = 0;
			if (!NtDllProc(RtlTimeToSecondsSince1980)(&time2, &result) && time != 0)
			{ NtStatus(STATUS_UNSUCCESSFUL).CheckAndThrow(); }
			return result;
		}

		static LONGLONG RtlSecondsSince1970ToTime(ULONG time)
		{
			LARGE_INTEGER time2 = {0};
			NtDllProc(RtlSecondsSince1970ToTime)(time, &time2);
			return time2.QuadPart;
		}

		static LONGLONG RtlSecondsSince1980ToTime(ULONG time)
		{
			LARGE_INTEGER time2 = {0};
			NtDllProc(RtlSecondsSince1980ToTime)(time, &time2);
			return time2.QuadPart;
		}
	};

	class NtObject
	{
		typedef NtObject This;
		typedef void (This::*bool_type)() const;
		void this_type_does_not_support_comparisons() const { }
	protected:
		template<typename T> static FORCEINLINE T *align_up_cast(void *const pUnaligned)
		{
			// assume size and alignment are the same thing
			static size_t const alignMask = sizeof(T) - 1;
			return reinterpret_cast<T *>((reinterpret_cast<size_t>(pUnaligned) + alignMask) & ~alignMask);
		}

		static HANDLE NtDuplicateHandle(HANDLE handle)
		{
			if (handle != NULL && handle != reinterpret_cast<HANDLE>(-1))
			{
				HANDLE const hProcess = reinterpret_cast<HANDLE>(-1);
				NtStatus(NtDllProc(NtDuplicateObject)(hProcess, handle, hProcess, &handle, 0, 0, DUPLICATE_SAME_ACCESS | 4 /*DUPLICATE_SAME_ATTRIBUTES*/)).CheckAndThrow();
			}
			return handle;
		}

	public:
		// If you add any fields, make sure to swap() as appropriate
		HANDLE _handle;

		NtObject(HANDLE const &handle = NULL) : _handle(handle) { }
		NtObject(This const &other) : _handle(NtDuplicateHandle(other.get())) { }
		~NtObject()
		{
			if (this->owned() && this->get() && this->get() != reinterpret_cast<HANDLE>(-1))
			{ NtStatus(NtDllProc(NtClose)(this->get())).CheckAndThrow(); }
		}
		This &operator =(This const &other) { if (this != &other) { This(other).swap(*this); } return *this; }
		operator bool_type() const { return this->get() ? &This::this_type_does_not_support_comparisons : NULL; }
		This &swap(This &other) { using std::swap; swap(this->_handle, other._handle); return *this; }
		friend void swap(This &a, This &b) { a.swap(b); }
		bool disown() { if (this->owned()) { this->_handle = (HANDLE)((uintptr_t)this->_handle | 2); return true; } else { return false; } }
		bool owned() const { return (intptr_t)this->_handle > 0 && ((uintptr_t)this->_handle & 2) == 0; }
		HANDLE detach() { HANDLE result = this->get(); this->_handle = NULL; return result; }
		HANDLE *receive() { return this->get() == NULL || this->get() == reinterpret_cast<HANDLE>(-1) ? &this->_handle : NULL; }
		HANDLE get() const { return (intptr_t)this->_handle < 0 ? this->_handle /* special handles are as-is */ : (HANDLE)((uintptr_t)this->_handle & (~(uintptr_t)0 << 2)); }

		template<typename It>
		It GetObjectFullName(It const &it) const
		{
			UCHAR buffer[sizeof(UNICODE_STRING) + 32 * 1024];
			ULONG cb;
			PUNICODE_STRING pUS = reinterpret_cast<PUNICODE_STRING>(&buffer[0]);
			NtStatus(NtDllProc(NtQueryObject)(this->get(), static_cast<enum _OBJECT_INFORMATION_CLASS>(1) /*ObjectNameInformation*/, pUS, int_cast<ULONG>(sizeof(buffer)), &cb)).CheckAndThrow();
			return std::copy(
				checked_iterator(pUS),
				checked_iterator(pUS) + pUS->Length / sizeof(*pUS->Buffer),
				it);
		}

		std::basic_string<TCHAR> GetObjectFullName() const
		{
			std::basic_string<TCHAR> result;
			this->GetObjectFullName(std::inserter(result, result.end()));
			return result;
		}

		template<typename TInfo> TInfo NtQueryObject() const
		{
			TInfo info;
			ULONG cb;
			/*Deprecated();*/
			NtStatus(NtDllProc(NtQueryObject)(this->get(), GetInfoClass(static_cast<TInfo *>(NULL)), &info, sizeof(info), &cb)).CheckAndThrow();
			return info;
		}

		void NtMakePermanentObject() { NtStatus(NtDllProc(NtMakePermanentObject)(this->get())).CheckAndThrow(); }

		NtStatus NtWaitForSingleObject(bool alertable, PLONGLONG pTimeout = NULL) const
		{
			LARGE_INTEGER timeout2;
			if (pTimeout != NULL) { timeout2.QuadPart = *pTimeout; }
			NtStatus status = NtDllProc(NtWaitForSingleObject)(this->get(), alertable, pTimeout != NULL ? &timeout2 : NULL);
			if (status.GetSeverity() >= STATUS_SEVERITY_WARNING) { status.CheckAndThrow(); }
			return status;
		}

		NtStatus NtWaitForSingleObject(bool alertable, LONGLONG timeout) const
		{
			return this->NtWaitForSingleObject(alertable, &timeout);
		}

		bool NtWaitForSingleObject(PLONGLONG pTimeout) const
		{
			return this->NtWaitForSingleObject(false, pTimeout) == 0;
		}

		bool NtWaitForSingleObjectNoWait() const
		{
			LONGLONG zero = 0;
			return this->NtWaitForSingleObject(&zero);
		}

		void NtWaitForSingleObject() const
		{
			PLONGLONG pTimeout = NULL;
			this->NtWaitForSingleObject(false, pTimeout);
		}

		static NtStatus NtWaitForMultipleObjects(
			ULONG cObjects, This const objects[],
			bool waitAny = false, bool alertable = false, LONGLONG const *pTimeout = NULL)
		{
			HANDLE handles[MAXIMUM_WAIT_OBJECTS];
			for (ULONG i = 0; i < cObjects; i++) { handles[i] = objects[i].get(); }
			LARGE_INTEGER timeout2;
			if (pTimeout != NULL) { timeout2.QuadPart = *pTimeout; }
			NtStatus status = NtDllProc(NtWaitForMultipleObjects)(cObjects, handles, static_cast<enum _WAIT_TYPE>(waitAny), alertable, pTimeout != NULL ? &timeout2 : NULL);
			if (status.GetSeverity() >= STATUS_SEVERITY_WARNING) { status.CheckAndThrow(); }
			return status;
		}

		template<typename TNtObject>
		static TNtObject NtDuplicateObject(
			TNtObject object, ULONG options = DUPLICATE_SAME_ACCESS | 4 /*DUPLICATE_SAME_ATTRIBUTES*/,
			ULONG attributes = 0, Access desiredAccess = MAXIMUM_ALLOWED, HANDLE hSourceProcess = NULL,
			HANDLE hTargetProcess = NULL)
		{
			if (hSourceProcess == NULL) { hSourceProcess = GetCurrentProcess(); }
			if (hTargetProcess == NULL) { hTargetProcess = currentProcess.get(); }
			TNtObject dup;
			NtStatus(NtDllProc(NtDuplicateObject)(hSourceProcess, object.get(), hTargetProcess, dup.receive(), desiredAccess, attributes, options)).CheckAndThrow();
			return dup;
		}
	};

	class NtEvent : public NtObject
	{
	public:

		NtEvent(HANDLE const value = NULL) : NtObject(value) { }
		NtEvent(NtEvent const &other) : NtObject(other) { }

	};

	class NtFile : public NtObject
	{
		static NtFile const mountPointManager;
		static void *NullCallback(void *, size_t /*cbInput*/, void * /*context_*/) { return NULL; }
		struct VectorAppenderCallback { std::vector<std::basic_string<TCHAR> > *pNames; bool operator()(LPCTSTR name) { pNames->push_back(name); return true; } };
		struct MOUNTMGR_TARGET_NAME_WITH_BUFFER { MOUNTMGR_TARGET_NAME Value; TCHAR PathBuffer[32 * 1024]; };
		template<typename It>
		struct MountMgrQueryDosVolumePathCallback
		{
			It *pIT;
			PUCHAR pItBuffer;
			MountMgrQueryDosVolumePathCallback(It *pIT, PUCHAR pItBuffer) : pIT(pIT), pItBuffer(pItBuffer) { }
			bool operator()(LPCTSTR name)
			{
				new(pItBuffer) It(std::copy(&name[0], &name[std::char_traits<TCHAR>::length(name)], *pIT));
				return true;
			}
		};
		class scoped_ptr
		{
			typedef std::vector<unsigned char> element_type;
			element_type *p;
		public:
			scoped_ptr() : p() { }
			scoped_ptr(element_type *p) : p(p) { }
			scoped_ptr(scoped_ptr const &other) : p(new element_type(*other.p)) { }
			~scoped_ptr() { delete this->p; }
			element_type *const &get() { return this->p; }
			element_type *const &get() const { return this->p; }
			element_type &operator *() { return *this->p; }
			element_type &operator *() const { return *this->p; }
			element_type *operator ->() { return this->get(); }
			element_type *operator ->() const { return this->get(); }
			element_type *const *operator &() { return &this->p; }
			element_type *const *operator &() const { return &this->p; }
			void reset(element_type *p) { delete this->p; this->p = p; }
			void swap(scoped_ptr &other) { using std::swap; swap(this->p, other.p); }
			friend void swap(scoped_ptr &a, scoped_ptr &b) { a.swap(b); }
		};

		template<typename TInfo> struct FileInfo { static LONGLONG GetFileId(TInfo const &info) { return 0; } };
		template<bool value> struct boolean; template<> struct boolean<true> { static bool const value = true; };

		template<typename TOutput, typename TInput>
		TOutput NtDeviceIoControlFileConstUnsafe(ULONG controlCode, TInput const &input, ULONG cbInput = ULONG_MAX, size_t const initOutputSize = sizeof(TOutput), size_t const maxOutputSize = ULONG_MAX, TOutput ctor(void *, size_t, void *context) = NULL, void *context = NULL) const
		{
			UCHAR stackBuffer[1024];
			PUCHAR pOutputBuffer = stackBuffer;
			size_t cbOutputBuffer = min(sizeof(stackBuffer), maxOutputSize == ULONG_MAX ? initOutputSize : maxOutputSize);
			std::vector<UCHAR> output;
			IO_STATUS_BLOCK iosb;
			NtStatus status;
			bool looped = false;
			do
			{
				bool exitAnyway = false;
				if (looped)
				{
					if (cbOutputBuffer < maxOutputSize)
					{
						output.resize(maxOutputSize == ULONG_MAX ? max(sizeof(TOutput), cbOutputBuffer * 2) : maxOutputSize);
						pOutputBuffer = &output[0];
						cbOutputBuffer = output.size();
					}
					else { exitAnyway = true; }
				}
				status = NtDllProc(NtDeviceIoControlFile)(this->get(), NULL, NULL, NULL, &iosb, controlCode, const_cast<TInput *>(&input), cbInput == ULONG_MAX ? sizeof(input) : cbInput, pOutputBuffer, int_cast<ULONG>(cbOutputBuffer));
				if (exitAnyway) { break; }
				looped = true;
			} while (status.NtStatusIsBadBufferSize());
			if (status == STATUS_BUFFER_OVERFLOW) {  /*We had a buffer overflow, and the size exceeded our maximum limit. So ignore it; there's already some data..*/  }
			else { status.CheckAndThrow(); }
			return ctor == NULL ? *reinterpret_cast<TOutput *>(pOutputBuffer) : ctor(pOutputBuffer, iosb.Information, context);
		}

		template<typename TOutput, typename TInput>
		TOutput NtFsControlFileConstUnsafe(ULONG controlCode, TInput const &input, ULONG cbInput = ULONG_MAX, size_t const initOutputSize = size_of<TOutput>::value, size_t const maxOutputSize = ULONG_MAX, TOutput ctor(void *, size_t, void *context) = NULL, void *context = NULL) const
		{
			UCHAR stackBuffer[1024];
			PUCHAR pOutputBuffer = stackBuffer;
			size_t cbOutputBuffer = min(sizeof(stackBuffer), maxOutputSize == ULONG_MAX ? initOutputSize : maxOutputSize);
			scoped_ptr /*<std::vector<UCHAR> >*/ pOutputHeap;
			if (cbOutputBuffer < initOutputSize)
			{
				pOutputHeap.reset(new std::vector<UCHAR>(maxOutputSize == ULONG_MAX ? max(size_of<TOutput>::value, initOutputSize) : maxOutputSize));
				pOutputBuffer = &(*pOutputHeap)[0];
				cbOutputBuffer = pOutputHeap->size();
			}
			IO_STATUS_BLOCK iosb;
			NtStatus status;
			bool looped = false;
			do
			{
				bool exitAnyway = false;
				if (looped)
				{
					if (cbOutputBuffer < maxOutputSize)
					{
						if (pOutputHeap.get() == NULL)
						{ pOutputHeap.reset(new std::vector<UCHAR>()); }
						pOutputHeap->resize(maxOutputSize == ULONG_MAX ? max(size_of<TOutput>::value, cbOutputBuffer * 2) : maxOutputSize);
						pOutputBuffer = &(*pOutputHeap)[0];
						cbOutputBuffer = pOutputHeap->size();
					}
					else { exitAnyway = true; }
				}
				status = NtDllProc(NtFsControlFile)(this->get(), NULL, NULL, NULL, &iosb, controlCode, const_cast<TInput *>(&input), cbInput == ULONG_MAX ? sizeof(input) : cbInput, pOutputBuffer, int_cast<ULONG>(cbOutputBuffer));
				if (exitAnyway) { break; }
				looped = true;
			} while (status.NtStatusIsBadBufferSize());
			if (status == STATUS_BUFFER_OVERFLOW) {  /*We had a buffer overflow, and the size exceeded our maximum limit. So ignore it; there's already some data..*/  }
			else { status.CheckAndThrow(); }
			return ctor == NULL ? *reinterpret_cast<TOutput *>(pOutputBuffer) : ctor(pOutputBuffer, iosb.Information, context);
		}
	public:

		NtFile(HANDLE const value = NULL)
			: NtObject(value)
		{ }

		NtFile(NtFile const &other)
			: NtObject(other)
		{ }

		typedef std::vector<std::pair<long long /*LCN*/, long long /*VirtualClusterCount (negated, if LCN is negative)*/> > RetrievalPointers;

		static NtFile NtOpenFile(ObjectAttributes const &oa, Access desiredAccess = Access::GenericRead | Access::Synchronize, ULONG shareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE, ULONG openOptions = FILE_SYNCHRONOUS_IO_NONALERT, OUT FileCreationDisposition *pDisposition = NULL, bool throwIfNotFound = true)
		{
			if ((openOptions & (FILE_SYNCHRONOUS_IO_ALERT | FILE_SYNCHRONOUS_IO_NONALERT)) != 0 && (desiredAccess & Access::Synchronize) == 0) { __debugbreak(); }
			NtFile file;
			IO_STATUS_BLOCK iosb;
			ObjectAttributes oa2 = oa;
			NtStatus status = NtDllProc(NtOpenFile)(file.receive(), desiredAccess, oa2, &iosb, shareAccess, openOptions);
			if (throwIfNotFound) { status.CheckAndThrow(); }
			if (pDisposition != NULL) { *pDisposition = static_cast<FileCreationDisposition>(iosb.Information); }
			return file;
		}

		template<typename TInfo>
		TInfo NtQueryInformationFile() const
		{
			IO_STATUS_BLOCK iosb;
			TInfo info;
			/*Deprecated();*/
			NtStatus(NtDllProc(NtQueryInformationFile)(this->get(), &iosb, &info, sizeof(info), GetInfoClass(static_cast<TInfo *>(NULL)))).CheckAndThrow();
			return info;
		}

		template<typename TInfo>
		TInfo NtQueryVolumeInformationFile() const
		{
			IO_STATUS_BLOCK iosb;
			TInfo info;
			/*Deprecated();*/
			NtStatus(NtDllProc(NtQueryVolumeInformationFile)(this->get(), &iosb, &info, sizeof(info), GetInfoClass(static_cast<TInfo *>(NULL)))).CheckAndThrow();
			return info;
		}

		std::basic_string<TCHAR> MountMgrQueryDosVolumePath(LPCTSTR target, bool *pThrowIfNotFound = NULL) const
		{
			std::basic_string<TCHAR> result;
			this->MountMgrQueryDosVolumePath(target, std::inserter(result, result.end()), pThrowIfNotFound);
			return result;
		}

		// It is OKAY for target and name to be the same string!
		template<typename It>
		It MountMgrQueryDosVolumePath(LPCTSTR target, It it, bool *pThrowIfNotFound = NULL /*If pointing to 'false', the target will receive 'true' if there was an ERROR, or 'false' otherwise.*/) const
		{
			UCHAR itBuffer[sizeof(it)];

			this->MountMgrQueryDosVolumePaths(target, false, MountMgrQueryDosVolumePathCallback<It>(&it, &itBuffer[0]), pThrowIfNotFound);
			if (pThrowIfNotFound == NULL || *pThrowIfNotFound == false /*no error*/)
			{
				It itCopy(*reinterpret_cast<It *>(&itBuffer[0]));
				reinterpret_cast<It *>(&itBuffer[0])->~It();
				return itCopy;
			}
			else { return it; }
		}

		std::vector<std::basic_string<TCHAR> > MountMgrQueryDosVolumePaths(LPCTSTR target, bool *pThrowIfNotFound = NULL) const
		{
			std::vector<std::basic_string<TCHAR> > result;
			this->MountMgrQueryDosVolumePaths(target, result, pThrowIfNotFound);
			return result;
		}

		void MountMgrQueryDosVolumePaths(LPCTSTR target, std::vector<std::basic_string<TCHAR> > &names, bool *pThrowIfNotFound = NULL) const
		{
			VectorAppenderCallback callback = { &names };
			this->MountMgrQueryDosVolumePaths(target, true, callback, pThrowIfNotFound);
		}

		template<typename TCallback /*bool(LPCTSTR)*/>
		static bool __cdecl MountMgrQueryDosVolumePathsCallback(void *pOutput, size_t /*cbOutput*/, void * context_)
		{
			TCallback &callback = *reinterpret_cast<TCallback *>(context_);
			MOUNTMGR_VOLUME_PATHS &paths = *static_cast<PMOUNTMGR_VOLUME_PATHS>(pOutput);
			for (size_t i = 0; i < paths.MultiSzLength / sizeof(*paths.MultiSz) && paths.MultiSz[i] != TEXT('\0'); i += std::char_traits<TCHAR>::length(&paths.MultiSz[i]) + 1)
			{
				if (!dereference(callback)(&paths.MultiSz[i]))
				{
					return false;
				}
			}
			return true;
		}

		template<typename TCallback /*bool(LPCTSTR)*/>
		bool MountMgrQueryDosVolumePaths(LPCTSTR target, bool queryMultiplePaths, TCallback callback, bool *pThrowIfNotFound = NULL) const
		{
			MOUNTMGR_TARGET_NAME_WITH_BUFFER input = {};
			input.Value.DeviceNameLength = int_cast<USHORT>(sizeof(*target) * std::char_traits<TCHAR>::length(target));
			std::copy(&target[0], &target[std::char_traits<TCHAR>::length(target)], unchecked_iterator(&input.Value.DeviceName[0]));
			TCallback *pCallback = &callback; // DO NOT REMOVE THIS LINE!  It serves to give an error if the address-of operator is overloaded
			try
			{
				if (pThrowIfNotFound != NULL) { *pThrowIfNotFound = false; }
				return this->NtDeviceIoControlFileConstUnsafe<bool>(queryMultiplePaths ? IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS : IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH, input, sizeof(input), sizeof(MOUNTMGR_VOLUME_PATHS), ULONG_MAX, &MountMgrQueryDosVolumePathsCallback<TCallback>, pCallback);
			}
			catch (CStructured_Exception &ex)
			{
				if (pThrowIfNotFound != NULL && !*pThrowIfNotFound && ex.GetSENumber() == STATUS_OBJECT_NAME_NOT_FOUND)
				{
					if (pThrowIfNotFound != NULL) { *pThrowIfNotFound = true; }
					return false;
				}
				else { throw; }
			}
		}

		std::basic_string<TCHAR> GetPathOfVolume() const
		{
			std::basic_string<TCHAR> s;
			this->GetPathOfVolume(std::inserter(s, s.end()));
			return s;
		}

		template<typename It>
		It GetPathOfVolume(It const &it, bool *pThrowOnError = NULL) const
		{
			TCHAR buffer[32 * 1024], myPath[32 * 1024];
			size_t const cchVolPath = int_cast<size_t>(this->GetPath(checked_iterator(myPath), false) - checked_iterator(myPath));
			size_t const cchFullPath = int_cast<size_t>(this->GetObjectFullName(checked_iterator(buffer)) - checked_iterator(buffer));
			if (cchFullPath < cchVolPath) { __debugbreak(); }
			buffer[cchFullPath - cchVolPath] = TEXT('\0');
			It it2 = it;
			return GetMountPointManager().MountMgrQueryDosVolumePath(buffer, it, pThrowOnError);
		}

		template<typename It>
		It GetWin32PathByMountMgr(It const &it, bool *pThrowOnError = NULL) const
		{
			TCHAR buffer[32 * 1024], myPath[32 * 1024];
			size_t const cchVolPath = int_cast<size_t>(this->GetPath(checked_iterator(myPath), false) - checked_iterator(myPath));
			size_t const cchFullPath = int_cast<size_t>(this->GetObjectFullName(checked_iterator(buffer)) - checked_iterator(buffer));
			if (cchFullPath < cchVolPath) { __debugbreak(); }
			buffer[cchFullPath - cchVolPath] = TEXT('\0');
			It it2 = it;
			return std::copy(&myPath[0], &myPath[cchVolPath], GetMountPointManager().MountMgrQueryDosVolumePath(buffer, it2, pThrowOnError));
		}

		template<typename It>
		It GetWin32Path(It const &it) const
		{
			bool throwIfNotFound = false;
			return this->GetWin32PathByMountMgr(it, &throwIfNotFound);
		}

		std::basic_string<TCHAR> GetWin32Path() const
		{
			std::basic_string<TCHAR> result;
			this->GetWin32Path(std::inserter(result, result.end()));
			return result;
		}

		IO_PRIORITY_HINT GetIoPriorityHint() const
		{
			FILE_IO_PRIORITY_HINT_INFORMATION info = { IoPriorityNormal };
			IO_STATUS_BLOCK iosb;
			NtStatus status = NtDllProc(NtQueryInformationFile)(this->get(), &iosb, &info, sizeof(info), GetInfoClass(&info));
			if (status != STATUS_NOT_IMPLEMENTED && status != STATUS_INVALID_INFO_CLASS)
			{ status.CheckAndThrow(); }
			return info.PriorityHint;
		}

		void SetIoPriorityHint(IO_PRIORITY_HINT const priority) const
		{
			FILE_IO_PRIORITY_HINT_INFORMATION info = { priority };
			IO_STATUS_BLOCK iosb;
			NtStatus status = NtDllProc(NtSetInformationFile)(this->get(), &iosb, &info, sizeof(info), GetInfoClass(&info));
			if (status != STATUS_NOT_IMPLEMENTED && status != STATUS_INVALID_INFO_CLASS)
			{ status.CheckAndThrow(); }
		}

		ULONG GetMode() const
		{
			return this->NtQueryInformationFile<FILE_MODE_INFORMATION>().FileMode;
		}

		ULONG GetSectorSize() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_SIZE_INFORMATION>().BytesPerSector;
		}

		LONGLONG GetAvailableAllocationUnits() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_SIZE_INFORMATION>().AvailableAllocationUnits.QuadPart;
		}

		LONGLONG GetTotalAllocationUnits() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_SIZE_INFORMATION>().TotalAllocationUnits.QuadPart;
		}

		ULONG GetCharacteristics() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_DEVICE_INFORMATION>().Characteristics;
		}

		ULONG GetDeviceType() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_DEVICE_INFORMATION>().DeviceType;
		}

		ULONG GetClusterSize() const
		{
			FILE_FS_SIZE_INFORMATION info = this->NtQueryVolumeInformationFile<FILE_FS_SIZE_INFORMATION>();
			return info.BytesPerSector * info.SectorsPerAllocationUnit;
		}

		ULONG GetSectorsPerCluster() const
		{
			return this->NtQueryVolumeInformationFile<FILE_FS_SIZE_INFORMATION>().SectorsPerAllocationUnit;
		}

		LONGLONG GetPosition() const
		{
			return this->NtQueryInformationFile<FILE_POSITION_INFORMATION>().CurrentByteOffset.QuadPart;
		}

		LONGLONG GetEndOfFile() const
		{
			return this->NtQueryInformationFile<FILE_STANDARD_INFORMATION>().EndOfFile.QuadPart;
		}

		LONGLONG GetAllocationSize() const
		{
			return this->NtQueryInformationFile<FILE_STANDARD_INFORMATION>().AllocationSize.QuadPart;
		}

		template<typename It>
		It GetPath(It const &it, bool backslashTerminateIfDirectory) const
		{
			UCHAR buffer[sizeof(FILE_NAME_INFORMATION) + 32 * 1024];
			IO_STATUS_BLOCK iosb;
			PFILE_NAME_INFORMATION pInfo = reinterpret_cast<PFILE_NAME_INFORMATION>((reinterpret_cast<uintptr_t>(&buffer[0]) + (sizeof(FILE_NAME_INFORMATION) - 1)) & ~(sizeof(FILE_NAME_INFORMATION) - 1));
			/*Deprecated();*/
			NtStatus status = NtDllProc(NtQueryInformationFile)(this->get(), &iosb, pInfo, int_cast<ULONG>(sizeof(buffer)), GetInfoClass(static_cast<FILE_NAME_INFORMATION *>(NULL)));
			if ((status == STATUS_INVALID_PARAMETER || status == STATUS_INVALID_PARAMETER) && this->GetDeviceType() == FILE_DEVICE_DISK)
			{
				// It's the volume handle, so the path is empty
				return it;
			}
			else
			{
				status.CheckAndThrow();
				if (backslashTerminateIfDirectory && pInfo->FileName[pInfo->FileNameLength / sizeof(*pInfo->FileName) - 1] != TEXT('\\') && this->IsDirectory())
				{
					pInfo->FileName[pInfo->FileNameLength / sizeof(*pInfo->FileName)] = TEXT('\\');
					pInfo->FileNameLength += sizeof(*pInfo->FileName);
				}
				return std::copy(&pInfo->FileName[0], &pInfo->FileName[pInfo->FileNameLength / sizeof(*pInfo->FileName)], it);
			}
		}

		std::basic_string<TCHAR> GetPath(bool backslashTerminateIfDirectory = true) const
		{
			std::basic_string<TCHAR> result;
			this->GetPath(std::inserter(result, result.end()), backslashTerminateIfDirectory);
			return result;
		}

		bool IsDirectory() const
		{
			return this->NtQueryInformationFile<FILE_STANDARD_INFORMATION>().Directory != false;
		}

		std::vector<UCHAR> NtReadFile(unsigned long length = ULONG_MAX /*Read to EOF*/, long long offset = -2, unsigned long *pKey = NULL) const
		{
			if (length == ULONG_MAX)
			{
				LONGLONG length2 = this->GetEndOfFile() - this->GetPosition();
				if (length2 > ULONG_MAX)
				{
					NtStatus(STATUS_INTEGER_OVERFLOW).CheckAndThrow();
				}
				length = int_cast<ULONG>(length2);
			}

			std::vector<UCHAR> buffer(length);
			buffer.resize(this->NtReadFile(&buffer[0], int_cast<ULONG>(buffer.size()), offset, pKey));
			return buffer;
		}

		unsigned long NtReadFile(void *buffer, unsigned long length, long long offset = -2, unsigned long *pKey = NULL) const
		{
			IO_STATUS_BLOCK iosb;
			LARGE_INTEGER offset2;
			offset2.QuadPart = offset;
			NtStatus status = NtDllProc(NtReadFile)(this->get(), NULL, NULL, NULL, &iosb, buffer, length, &offset2, pKey);
			if (status == STATUS_PENDING)
			{
				/*
				DO _NOT_ use the error-checking version of NtWaitForSingleObject()!
				If the user doesn't have SYNCHRONIZE access, we don't want to throw that; we just say we're pending instead.
				*/
				if (NtDllProc(NtWaitForSingleObject)(this->get(), FALSE, NULL) == 0) { status = iosb.Status; }
			}
			status.CheckAndThrow();
			return int_cast<ULONG>(iosb.Information);
		}

		NtStatus NtDeviceIoControlFileConstUnsafe(ULONG controlCode, void const *pInput, ULONG cbInput, void *pOutput, ULONG cbOutput) const
		{
			IO_STATUS_BLOCK iosb;
			return NtDllProc(NtDeviceIoControlFile)(this->get(), NULL, NULL, NULL, &iosb, controlCode, const_cast<PVOID>(pInput), cbInput, pOutput, cbOutput);
		}

		template<typename TOutput, typename TInput>
		TOutput NtDeviceIoControlFile(ULONG controlCode, TInput const &input, ULONG cbInput = 0, size_t const initOutputSize = sizeof(TOutput), size_t const maxOutputSize = ULONG_MAX, TOutput ctor(void *, size_t, void *context) = NULL, void *context = NULL)
		{ return this->NtDeviceIoControlFileConstUnsafe<TOutput, TInput>(controlCode, input, cbInput, initOutputSize, maxOutputSize, ctor, context); }

		template<typename TOutput, typename TInput>
		TOutput NtFsControlFile(ULONG controlCode, TInput const &input, ULONG cbInput = 0, size_t const initOutputSize = sizeof(TOutput), size_t const maxOutputSize = ULONG_MAX, TOutput ctor(void *, size_t, void *context) = NULL, void *context = NULL)
		{ return this->NtFsControlFileConstUnsafe<TOutput, TInput>(controlCode, input, cbInput, initOutputSize, maxOutputSize, ctor, context); }

		unsigned long NtReadFileVirtual(
			std::pair<unsigned long long, std::pair<size_t, std::pair<unsigned long long, long long> const *> > const clusterRetrievalPointers,
			unsigned long clusterSize /*Can be ZERO*/, long long const virtualOffset, void *buffer, unsigned long length) const
		{
			if (clusterSize == 0) { clusterSize = this->GetClusterSize(); }
			ULONG lengthProcessed = 0;

			while (lengthProcessed < length)
			{
				long long rangeLength = 0;
				long long logical = -1;
				{
					// Map virtual to logical
					long long const requestedVBN = virtualOffset + lengthProcessed;
					long long currentVBN = clusterRetrievalPointers.first * clusterSize;
					for (ULONG i = 0; i < clusterRetrievalPointers.second.first; i++)
					{
						long long nextVBN = clusterRetrievalPointers.second.second[i].first * clusterSize;
						if (currentVBN <= requestedVBN && requestedVBN < nextVBN)
						{
							rangeLength = nextVBN - requestedVBN;
							logical = clusterRetrievalPointers.second.second[i].second * clusterSize + (requestedVBN - currentVBN);
							break;
						}
						currentVBN = nextVBN;
					}
				}
				if (logical >= 0)
				{
					ULONG toProcess = min(length - lengthProcessed, int_cast<ULONG>(rangeLength));
					this->NtReadFile((PUCHAR)buffer + lengthProcessed, toProcess, logical);
					lengthProcessed += toProcess;
				}
				else
				{
					//__debugbreak();
					//memset((PUCHAR)buffer + lengthProcessed, 0, length - lengthProcessed);
					break;
				}
			}

			return lengthProcessed;
		}

		LONGLONG FsctlGetRetrievalPointer(LONGLONG *pStartingVcn /*Irrelevant to the result itself -- the result is correct either way.*/) const
		{
			STARTING_VCN_INPUT_BUFFER input = {};
			input.StartingVcn.QuadPart = pStartingVcn != NULL ? *pStartingVcn : 0;
			struct Callback { static RETRIEVAL_POINTERS_BUFFER callback(void *pBuffer, size_t /*cbInput*/, void * /*context_*/) { return *static_cast<PRETRIEVAL_POINTERS_BUFFER>(pBuffer); } };
			RETRIEVAL_POINTERS_BUFFER result = this->NtFsControlFileConstUnsafe(FSCTL_GET_RETRIEVAL_POINTERS, input, sizeof(input), sizeof(RETRIEVAL_POINTERS_BUFFER), sizeof(RETRIEVAL_POINTERS_BUFFER), &Callback::callback);
			if (pStartingVcn != NULL) { *pStartingVcn = result.StartingVcn.QuadPart; }
			for (ULONG i = 0; i < result.ExtentCount; i++)
			{
				LONGLONG const currentVcn = i == 0 ? result.StartingVcn.QuadPart : result.Extents[i - 1].NextVcn.QuadPart;
				if (currentVcn <= input.StartingVcn.QuadPart && input.StartingVcn.QuadPart < result.Extents[i].NextVcn.QuadPart)
				{
					return result.Extents[i].Lcn.QuadPart < 0 ? result.Extents[i].Lcn.QuadPart : result.Extents[i].Lcn.QuadPart + input.StartingVcn.QuadPart - currentVcn;
				}
			}
			return -1;
		}


		LONGLONG FsctlGetRetrievalPointer(LONGLONG startingVcn = 0) const { return this->FsctlGetRetrievalPointer(&startingVcn); }

		// Returns true if all the pointers were received, or false if there are more to query for
		bool FsctlGetRetrievalPointers(PRETRIEVAL_POINTERS_BUFFER pOutput, size_t &cbOutput, long long startingVCN = 0) const
		{
			STARTING_VCN_INPUT_BUFFER input = {0};
			input.StartingVcn.QuadPart = startingVCN;
			IO_STATUS_BLOCK iosb;
			NtStatus status = NtDllProc(NtFsControlFile)(this->get(), NULL, NULL, NULL, &iosb, FSCTL_GET_RETRIEVAL_POINTERS, &input, sizeof(input), pOutput, int_cast<ULONG>(cbOutput));
			if (status != STATUS_BUFFER_OVERFLOW)
			{
				if (status == STATUS_END_OF_FILE) { iosb.Information = offsetof(RETRIEVAL_POINTERS_BUFFER, Extents); }
				else { status.CheckAndThrow(); }
				cbOutput = iosb.Information;
				return true;
			}
			else { cbOutput = iosb.Information; return false; }
		}

		size_t FsctlGetNtfsFileRecordFloor(ULONGLONG &fileReferenceNumber, UCHAR buffer[], size_t cbBuffer) const
		{
			ULONGLONG input = fileReferenceNumber;
			struct Callback
			{
				void *buffer;
				size_t cbBuffer;
				ULONGLONG *pFileRefNumber;
				static size_t callback(void *pBuffer, size_t /*cbInput*/, void *context_)
				{
					Callback &context = *static_cast<Callback *>(context_);
					NTFS_FILE_RECORD_OUTPUT_BUFFER &output = *static_cast<PNTFS_FILE_RECORD_OUTPUT_BUFFER>(pBuffer);
					size_t cb = min(context.cbBuffer, static_cast<size_t>(output.FileRecordLength));
					memcpy(context.buffer, output.FileRecordBuffer, cb);
					if (context.pFileRefNumber != NULL) { *context.pFileRefNumber = output.FileReferenceNumber.QuadPart; }
					return cb;
				}
			} callback = { buffer, cbBuffer, &fileReferenceNumber };
			return this->NtFsControlFileConstUnsafe(FSCTL_GET_NTFS_FILE_RECORD, input, sizeof(input), cbBuffer, cbBuffer, &Callback::callback, &callback);
		}

		NTFS_VOLUME_DATA_BUFFER FsctlGetVolumeData() const
		{
			return this->NtFsControlFileConstUnsafe<NTFS_VOLUME_DATA_BUFFER>(FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0);
		}

		void FsctlGetNtfsFileRecordFloor(ULONGLONG fileReferenceNumber, NTFS_FILE_RECORD_OUTPUT_BUFFER *pOutput, size_t cbOutput) const
		{
			IO_STATUS_BLOCK iosb;
			NtStatus(NtDllProc(NtFsControlFile)(this->get(), NULL, NULL, NULL, &iosb, FSCTL_GET_NTFS_FILE_RECORD, &fileReferenceNumber, sizeof(fileReferenceNumber), pOutput, int_cast<ULONG>(cbOutput))).CheckAndThrow();
		}

		ULONGLONG GetNtfsFileReferenceNumber(ULONGLONG fileSegmentNumber, OUT ULONGLONG *pBaseFileSegmentNumber = NULL, bool floorIfEmpty = false) const
		{
			ULONGLONG input = fileSegmentNumber;
			struct { NTFS_FILE_RECORD_OUTPUT_BUFFER Buffer; UCHAR Padding[0x2A + sizeof(USHORT)]; } buffer;
			IO_STATUS_BLOCK iosb;
			NTSTATUS status = NtDllProc(NtFsControlFile)(this->get(), NULL, NULL, NULL, &iosb, FSCTL_GET_NTFS_FILE_RECORD, &input, sizeof(input), &buffer.Buffer, sizeof(buffer));
			if (NtStatus(status).NtStatusIsBadBufferSize()) { }
			else if (status == STATUS_INVALID_PARAMETER) { return ~0ULL; }
			else { NtStatus(status).CheckAndThrow(); }
			if (pBaseFileSegmentNumber != NULL) { *pBaseFileSegmentNumber = *reinterpret_cast<ULONGLONG const *>(&buffer.Buffer.FileRecordBuffer[0x20]); }
			return static_cast<LONGLONG>(fileSegmentNumber) == buffer.Buffer.FileReferenceNumber.QuadPart || floorIfEmpty
				? buffer.Buffer.FileReferenceNumber.QuadPart | (static_cast<ULONGLONG>(*reinterpret_cast<USHORT const *>(&buffer.Buffer.FileRecordBuffer[0x10])) * (1LL << 48))
				: -1;
		}

		ULONGLONG FindHighestNtfsFileRecord(bool returnUsed = false) const //Uses binary search to find the highest file record
		{
			ULONGLONG start = 0, end = (ULONGLONG)(((ULONGLONG)1 << ((sizeof(ULONGLONG) * 8) - 1)) - 1);
			do
			{
				ULONGLONG const segNum = (start + end) / 2;
				if (this->GetNtfsFileReferenceNumber(segNum, NULL, true) != -1) { start = segNum; }
				else { end = segNum; }
			} while (start < end - 1);
			if (returnUsed)
			{
				LONGLONG const result = this->GetNtfsFileReferenceNumber(start, NULL, true) & 0x0000FFFFFFFFFFFFULL;
				if (result != -1) { start = result; }
			}
			return start;
		}

		std::pair<unsigned long, std::pair<unsigned long, unsigned long> > IoctlStorageGetDeviceNumber() const
		{
			STORAGE_DEVICE_NUMBER output;
			this->NtDeviceIoControlFileConstUnsafe(IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &output, sizeof(output)).CheckAndThrow();
			return std::make_pair(output.DeviceType, std::make_pair(output.DeviceNumber, output.PartitionNumber));
		}

		std::vector<DISK_EXTENT> IoctlVolumeGetVolumeDiskExtents() const
		{
			struct Callback
			{
				static std::vector<DISK_EXTENT> callback(void *pOutput, size_t /*cbOutput*/, void * /*context_*/)
				{
					PVOLUME_DISK_EXTENTS pExtents = static_cast<PVOLUME_DISK_EXTENTS>(pOutput);
					std::vector<DISK_EXTENT> result(pExtents->NumberOfDiskExtents);
					for (ULONG i = 0; i < pExtents->NumberOfDiskExtents; i++)
					{
						result[i] = pExtents->Extents[i];
					}
					return result;
				}
			};
			return this->NtDeviceIoControlFileConstUnsafe(IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, sizeof(VOLUME_DISK_EXTENTS), ULONG_MAX, &Callback::callback);
		}

		static NtFile GetMountPointManager() { return mountPointManager; }

		static std::basic_string<TCHAR> NTAPI RtlDosPathNameToNtPathName(LPCTSTR dosPathName, std::basic_string<TCHAR> *pFileNamePart = NULL)
		{
			(void)(&RtlDosPathNameToNtPathName);
			LPCTSTR fileNamePart;
			UNICODE_STRING ntPathName;
			if (!NtDllProc(RtlDosPathNameToNtPathName_U)(dosPathName, &ntPathName, &fileNamePart, NULL)) { NtStatus::ThrowWin32(161 /*ERROR_BAD_PATHNAME*/); }
			std::basic_string<TCHAR> result(ntPathName.Buffer, ntPathName.Length / sizeof(*ntPathName.Buffer));
			if (pFileNamePart != NULL) { pFileNamePart->assign(fileNamePart); }
			NtDllProc(RtlFreeUnicodeString)(&ntPathName);
			return result;
		}
	};
	DECLSPEC_SELECTANY NtFile const NtFile::mountPointManager(NtOpenFile(MOUNTMGR_DEVICE_NAME, FILE_READ_ATTRIBUTES | SYNCHRONIZE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT));

	class NtThread : public NtObject
	{
	public:
		NtThread(HANDLE const value = NULL) : NtObject(value) { }

	};

	class NtProcess : public NtObject
	{
	public:
		NtProcess(HANDLE const value = NULL) : NtObject(value) { }

		static bool RtlAdjustPrivilege(TokenPrivilege privilege, bool enable = true, bool currentThreadInsteadOfProcess = false)
		{ BOOLEAN prev; NtStatus(NtDllProc(RtlAdjustPrivilege)(privilege, enable, currentThreadInsteadOfProcess, &prev)).CheckAndThrow(); return !!prev; }
	};
}

#endif