#include "stdafx.h"
#include "nttypes.h"

namespace winnt
{
#define X(F, T) identity<T>::type *F;
		X(NtOpenFile, NTSTATUS NTAPI(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess,
			IN OBJECT_ATTRIBUTES *ObjectAttributes, OUT IO_STATUS_BLOCK *IoStatusBlock,
			IN ULONG ShareAccess, IN ULONG OpenOptions));
		X(NtReadFile, NTSTATUS NTAPI(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL,
			IN IO_APC_ROUTINE *ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL,
			OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID buffer_ptr, IN ULONG Length,
			IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL));
		X(NtQueryVolumeInformationFile, NTSTATUS NTAPI(HANDLE FileHandle,
			IO_STATUS_BLOCK *IoStatusBlock, PVOID FsInformation, unsigned long Length,
			unsigned long FsInformationClass));
		X(NtQueryInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle,
			OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length,
			IN unsigned long FileInformationClass));
		X(NtSetInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle,
			OUT IO_STATUS_BLOCK *IoStatusBlock, IN PVOID FileInformation, IN ULONG Length,
			IN unsigned long FileInformationClass));
		X(RtlInitUnicodeString, VOID NTAPI(UNICODE_STRING * DestinationString,
			PCWSTR SourceString));
		X(RtlNtStatusToDosError, unsigned long NTAPI(IN NTSTATUS NtStatus));
		X(RtlSystemTimeToLocalTime, NTSTATUS NTAPI(IN LARGE_INTEGER const *SystemTime,
			OUT PLARGE_INTEGER LocalTime));
#undef  X

	void init()
	{
#define X(F, T) winnt::F = *reinterpret_cast<identity<T>::type *>(GetProcAddress(GetModuleHandle(_T("NTDLL.dll")), #F))
		X(NtOpenFile, NTSTATUS NTAPI(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess,
			IN OBJECT_ATTRIBUTES *ObjectAttributes, OUT IO_STATUS_BLOCK *IoStatusBlock,
			IN ULONG ShareAccess, IN ULONG OpenOptions));
		X(NtReadFile, NTSTATUS NTAPI(IN HANDLE FileHandle, IN HANDLE Event OPTIONAL,
			IN IO_APC_ROUTINE *ApcRoutine OPTIONAL, IN PVOID ApcContext OPTIONAL,
			OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID buffer_ptr, IN ULONG Length,
			IN PLARGE_INTEGER ByteOffset OPTIONAL, IN PULONG Key OPTIONAL));
		X(NtQueryVolumeInformationFile, NTSTATUS NTAPI(HANDLE FileHandle,
			IO_STATUS_BLOCK *IoStatusBlock, PVOID FsInformation, unsigned long Length,
			unsigned long FsInformationClass));
		X(NtQueryInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle,
			OUT IO_STATUS_BLOCK *IoStatusBlock, OUT PVOID FileInformation, IN ULONG Length,
			IN unsigned long FileInformationClass));
		X(NtSetInformationFile, NTSTATUS NTAPI(IN HANDLE FileHandle,
			OUT IO_STATUS_BLOCK *IoStatusBlock, IN PVOID FileInformation, IN ULONG Length,
			IN unsigned long FileInformationClass));
		X(RtlInitUnicodeString, VOID NTAPI(UNICODE_STRING * DestinationString,
			PCWSTR SourceString));
		X(RtlNtStatusToDosError, unsigned long NTAPI(IN NTSTATUS NtStatus));
		X(RtlSystemTimeToLocalTime, NTSTATUS NTAPI(IN LARGE_INTEGER const *SystemTime,
			OUT PLARGE_INTEGER LocalTime));
#undef  X
	}
}
