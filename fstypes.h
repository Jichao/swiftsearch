#pragma once

namespace ntfs {
#pragma pack(push, 1)
	struct NTFS_BOOT_SECTOR {
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
		int64_t TotalSectors;
		int64_t MftStartLcn;
		int64_t Mft2StartLcn;
		signed char ClustersPerFileRecordSegment;
		unsigned char Padding3[3];
		unsigned long ClustersPerIndexBlock;
		int64_t VolumeSerialNumber;
		unsigned long Checksum;

		unsigned char BootStrap[0x200 - 0x54];
		unsigned int file_record_size() const
		{
			return this->ClustersPerFileRecordSegment >= 0 ? this->ClustersPerFileRecordSegment *
				this->SectorsPerCluster * this->BytesPerSector : 1U << static_cast<int>
				(-this->ClustersPerFileRecordSegment);
		}
		unsigned int cluster_size() const
		{
			return this->SectorsPerCluster * this->BytesPerSector;
		}
	};
#pragma pack(pop)
	struct MULTI_SECTOR_HEADER {
		unsigned long Magic;
		unsigned short USAOffset;
		unsigned short USACount;

		bool unfixup(size_t max_size)
		{
			unsigned short *usa = reinterpret_cast<unsigned short *>
				(&reinterpret_cast<unsigned char *>(this)[this->USAOffset]);
			bool result = true;
			for (unsigned short i = 1; i < this->USACount; i++) {
				const size_t offset = i * 512 - sizeof(unsigned short);
				unsigned short *const check = (unsigned short *)((unsigned char*)this + offset);
				if (offset < max_size) {
					if (usa[0] != *check) {
						result = false;
					}
					*check = usa[i];
				}
				else {
					break;
				}
			}
			return result;
		}
	};
	enum AttributeTypeCode {
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
	struct ATTRIBUTE_RECORD_HEADER {
		AttributeTypeCode Type;
		unsigned long Length;
		unsigned char IsNonResident;
		unsigned char NameLength;
		unsigned short NameOffset;
		unsigned short Flags;
		unsigned short Instance;
		union {
			struct RESIDENT {
				unsigned long ValueLength;
				unsigned short ValueOffset;
				unsigned short Flags;
				inline void *GetValue()
				{
					return reinterpret_cast<void *>(reinterpret_cast<char *>(CONTAINING_RECORD(this,
						ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset);
				}
				inline void const *GetValue() const
				{
					return reinterpret_cast<const void *>(reinterpret_cast<const char *>(CONTAINING_RECORD(
						this, ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset);
				}
			} Resident;
			struct NONRESIDENT {
				uint64_t LowestVCN;
				uint64_t HighestVCN;
				unsigned short MappingPairsOffset;
				unsigned char CompressionUnit;
				unsigned char Reserved[5];
				int64_t AllocatedSize;
				int64_t DataSize;
				int64_t InitializedSize;
				int64_t CompressedSize;
			} NonResident;
		};
		ATTRIBUTE_RECORD_HEADER *next()
		{
			return reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<unsigned char *>
				(this) + this->Length);
		}
		ATTRIBUTE_RECORD_HEADER const *next() const
		{
			return reinterpret_cast<ATTRIBUTE_RECORD_HEADER const *>
				(reinterpret_cast<unsigned char const *>(this) + this->Length);
		}
		wchar_t *name()
		{
			return reinterpret_cast<wchar_t *>(reinterpret_cast<unsigned char *>
				(this) + this->NameOffset);
		}
		wchar_t const *name() const
		{
			return reinterpret_cast<wchar_t const *>(reinterpret_cast<unsigned char const *>
				(this) + this->NameOffset);
		}
	};
	enum FILE_RECORD_HEADER_FLAGS {
		FRH_IN_USE = 0x0001,    /* Record is in use */
		FRH_DIRECTORY = 0x0002,    /* Record is a directory */
	};
	struct FILE_RECORD_SEGMENT_HEADER {
		MULTI_SECTOR_HEADER MultiSectorHeader;
		uint64_t LogFileSequenceNumber;
		unsigned short SequenceNumber;
		unsigned short LinkCount;
		unsigned short FirstAttributeOffset;
		unsigned short Flags /* FILE_RECORD_HEADER_FLAGS */;
		unsigned long BytesInUse;
		unsigned long BytesAllocated;
		uint64_t BaseFileRecordSegment;
		unsigned short NextAttributeNumber;
		//http://blogs.technet.com/b/joscon/archive/2011/01/06/how-hard-links-work.aspx
		unsigned short
			SegmentNumberUpper_or_USA_or_UnknownReserved;  // WARNING: This does NOT seem to be the actual "upper" segment number of anything! I found it to be 0x26e on one of my drives... and checkdisk didn't say anything about it
		unsigned long SegmentNumberLower;
		ATTRIBUTE_RECORD_HEADER *begin()
		{
			return reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<unsigned char *>
				(this) + this->FirstAttributeOffset);
		}
		ATTRIBUTE_RECORD_HEADER const *begin() const
		{
			return reinterpret_cast<ATTRIBUTE_RECORD_HEADER const *>
				(reinterpret_cast<unsigned char const *>(this) + this->FirstAttributeOffset);
		}
		void *end(size_t const max_buffer_size = ~size_t())
		{
			return reinterpret_cast<unsigned char *>(this) + (max_buffer_size < this->BytesInUse ?
			max_buffer_size : this->BytesInUse);
		}
		void const *end(size_t const max_buffer_size = ~size_t()) const
		{
			return reinterpret_cast<unsigned char const *>(this) + (max_buffer_size < this->BytesInUse
				? max_buffer_size : this->BytesInUse);
		}
	};
	struct FILENAME_INFORMATION {
		uint64_t ParentDirectory;
		int64_t CreationTime;
		int64_t LastModificationTime;
		int64_t LastChangeTime;
		int64_t LastAccessTime;
		int64_t AllocatedLength;
		int64_t FileSize;
		unsigned long FileAttributes;
		unsigned short PackedEaSize;
		unsigned short Reserved;
		unsigned char FileNameLength;
		unsigned char Flags;
		WCHAR FileName[1];
	};
	struct STANDARD_INFORMATION {
		int64_t CreationTime;
		int64_t LastModificationTime;
		int64_t LastChangeTime;
		int64_t LastAccessTime;
		unsigned long FileAttributes;
		// There's more, but only in newer versions
	};
	struct INDEX_HEADER {
		unsigned long FirstIndexEntry;
		unsigned long FirstFreeByte;
		unsigned long BytesAvailable;
		unsigned char Flags;  // '1' == has INDEX_ALLOCATION
		unsigned char Reserved[3];
	};
	struct INDEX_ROOT {
		AttributeTypeCode Type;
		unsigned long CollationRule;
		unsigned long BytesPerIndexBlock;
		unsigned char ClustersPerIndexBlock;
		INDEX_HEADER Header;
	};
	struct ATTRIBUTE_LIST {
		AttributeTypeCode AttributeType;
		unsigned short Length;
		unsigned char NameLength;
		unsigned char NameOffset;
		uint64_t StartVcn; // LowVcn
		uint64_t FileReferenceNumber;
		unsigned short AttributeNumber;
		unsigned short AlignmentOrReserved[3];
	};
	static TCHAR const *attribute_names[] = {
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

