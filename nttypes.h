#pragma once

namespace winnt {
	struct IO_STATUS_BLOCK {
		union {
			long Status;
			void *Pointer;
		};
		uintptr_t Information;
	};

	struct UNICODE_STRING {
		unsigned short Length, MaximumLength;
		wchar_t *buffer_ptr;
	};

	struct OBJECT_ATTRIBUTES {
		unsigned long Length;
		HANDLE RootDirectory;
		UNICODE_STRING *ObjectName;
		unsigned long Attributes;
		void *SecurityDescriptor;
		void *SecurityQualityOfService;
	};

	typedef VOID NTAPI IO_APC_ROUTINE(IN PVOID ApcContext, IN IO_STATUS_BLOCK *IoStatusBlock,
		IN ULONG Reserved);

	enum IO_PRIORITY_HINT { IoPriorityVeryLow = 0, IoPriorityLow, IoPriorityNormal, IoPriorityHigh, IoPriorityCritical, MaxIoPriorityTypes };
	struct FILE_FS_SIZE_INFORMATION {
		long long TotalAllocationUnits, ActualAvailableAllocationUnits;
		unsigned long SectorsPerAllocationUnit, BytesPerSector;
	};
	struct FILE_FS_ATTRIBUTE_INFORMATION {
		unsigned long FileSystemAttributes;
		unsigned long MaximumComponentNameLength;
		unsigned long FileSystemNameLength;
		wchar_t FileSystemName[1];
	};
	union FILE_IO_PRIORITY_HINT_INFORMATION {
		IO_PRIORITY_HINT PriorityHint;
		unsigned long long _alignment;
	};

	template<class T> struct identity {
		typedef T type;
	};
	void init();
	typedef long NTSTATUS;
#define X(F, T) extern identity<T>::type *F;
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

class RaiseIoPriority {
	uintptr_t _volume;
	winnt::FILE_IO_PRIORITY_HINT_INFORMATION _old;
	RaiseIoPriority(RaiseIoPriority const &);
	RaiseIoPriority &operator =(RaiseIoPriority const &);
public:
	static winnt::FILE_IO_PRIORITY_HINT_INFORMATION set(uintptr_t const volume,
		winnt::IO_PRIORITY_HINT const value)
	{
		winnt::FILE_IO_PRIORITY_HINT_INFORMATION old = {};
		winnt::IO_STATUS_BLOCK iosb;
		winnt::NtQueryInformationFile(reinterpret_cast<HANDLE>(volume), &iosb, &old, sizeof(old),
			43);
		winnt::FILE_IO_PRIORITY_HINT_INFORMATION io_priority = { value };
		winnt::NtSetInformationFile(reinterpret_cast<HANDLE>(volume), &iosb, &io_priority,
			sizeof(io_priority), 43);
		return old;
	}
	uintptr_t volume() const
	{
		return this->_volume;
	}
	RaiseIoPriority() : _volume(), _old() { }
	explicit RaiseIoPriority(uintptr_t const volume) : _volume(volume), _old(set(volume,
		winnt::IoPriorityNormal)) { }
	~RaiseIoPriority()
	{
		if (this->_volume) {
			set(this->_volume, this->_old.PriorityHint);
		}
	}
	void swap(RaiseIoPriority &other)
	{
		using std::swap;
		swap(this->_volume, other._volume);
		swap(this->_old, other._old);
	}
};



