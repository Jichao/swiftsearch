#ifndef FSCTL_QUERY_FILE_LAYOUT
#define FSCTL_QUERY_FILE_LAYOUT             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 157, METHOD_NEITHER, FILE_ANY_ACCESS)

enum QUERY_FILE_LAYOUT_FILTER_TYPE
{
	QUERY_FILE_LAYOUT_FILTER_TYPE_NONE = 0,
	QUERY_FILE_LAYOUT_FILTER_TYPE_CLUSTERS = 1,
	QUERY_FILE_LAYOUT_FILTER_TYPE_FILEID = 2,
	QUERY_FILE_LAYOUT_NUM_FILTER_TYPES = 3,
};

struct CLUSTER_RANGE
{
	// The starting cluster for this query region (inclusive).
	LARGE_INTEGER       StartingCluster;

	// The length of the cluster region.
	LARGE_INTEGER       ClusterCount;
};

struct FILE_REFERENCE_RANGE
{
	// The starting file reference number for this query region (inclusive).
	DWORDLONG           StartingFileReferenceNumber;

	// The ending file reference number for this query region (inclusive).
	DWORDLONG           EndingFileReferenceNumber;
};

struct QUERY_FILE_LAYOUT_INPUT
{
	// Number of filter range pairs in the following array. The input buffer must be large enough to contain this number or the operation will fail.
	DWORD               NumberOfPairs;

	// Flags for the operation.
	DWORD               Flags;

	// The type of filter being applied for this operation.

	QUERY_FILE_LAYOUT_FILTER_TYPE   FilterType;

	// Reserved for future use. Should be set to zero.

	DWORD               Reserved;

	// A pointer to the filter-type-specific information.  This is  the caller's actual set of cluster ranges or filter ranges.
	union
	{

		// The following  is used when the caller wishes to filter on a set of cluster ranges.

		CLUSTER_RANGE ClusterRanges[1];

		// The following is used when the caller wishes to filter on a set of file reference ranges.

		FILE_REFERENCE_RANGE FileReferenceRanges[1];

	} Filter;
};

// Clear the state of the internal cursor.
#define QUERY_FILE_LAYOUT_RESTART                                       (0x00000001)

// Request that the API call retrieve name information for the
// objects on the volume.
#define QUERY_FILE_LAYOUT_INCLUDE_NAMES                                 (0x00000002)

// Request that the API call include streams of the file.
#define QUERY_FILE_LAYOUT_INCLUDE_STREAMS                               (0x00000004)

// Include extent information with the attribute entries, where applicable.
// Use of this flag requires the _INCLUDE_STREAMS flag.
#define QUERY_FILE_LAYOUT_INCLUDE_EXTENTS                               (0x00000008)

// Include extra information, such as modification times and security
// IDs, with each returned file layout entry.
#define QUERY_FILE_LAYOUT_INCLUDE_EXTRA_INFO							(0x00000010)

// Include unallocated attributes in the enumeration, which in NTFS means one
// of two cases:
//      1. Resident attributes.
//      2. Compressed or sparse nonresident attributes with no physical
//         allocation (consisting only of a sparse hole).
//  This flag may only be used when no cluster ranges are specified (i. e.
//  on a whole-volume query).

#define QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED	(0x00000020)

// Indicates that the filesystem is returning stream extents in a
// single-instanced fashion.
#define QUERY_FILE_LAYOUT_SINGLE_INSTANCED                              (0x00000001)

struct QUERY_FILE_LAYOUT_OUTPUT
{
	// Number of file entries following this header. Includes only the number of file entries in this iteration.
	DWORD               FileEntryCount;

	// Offset to the first file entry in this buffer, expressed in bytes.
	DWORD               FirstFileOffset;

	// Flags indicating context that is applicable to the entire output set.
	DWORD               Flags;

	// For alignment/later use.
	DWORD               Reserved;
	struct FILE_LAYOUT_ENTRY *FirstFile() { return this->FirstFileOffset ? reinterpret_cast<FILE_LAYOUT_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->FirstFileOffset) : NULL; }
	struct FILE_LAYOUT_ENTRY const *FirstFile() const { return this->FirstFileOffset ? reinterpret_cast<FILE_LAYOUT_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->FirstFileOffset) : NULL; }
};

struct FILE_LAYOUT_ENTRY
{
	// Version number of this structure (current version number is 1).
	DWORD         Version;

	// Offset to next file entry (in bytes) or zero if this is the last entry.
	DWORD         NextFileOffset;

	// Flags containing context applicable to this file.
	DWORD         Flags;

	// File attributes.
	DWORD         FileAttributes;

	// File ID for this file.
	DWORDLONG     FileReferenceNumber;

	// Offset to the first name entry from the start of this record, or zero if there are no link records.
	DWORD         FirstNameOffset;

	// Offset to the first stream entry from the start of this record, or zero if there are no stream records.
	DWORD         FirstStreamOffset;

	// Offset to additional per-file information, contained in a FILE_LAYOUT_INFO_ENTRY structure, or zero if this information was not returned.
	DWORD         ExtraInfoOffset;

	// For alignment/future use.
	DWORD         Reserved;

	// The structure may be extended here to support additional static fields (e.g. pointing to a FILE_BASIC_INFORMATION structure, etc.). This sort of change should coincide with a version number increase.

	FILE_LAYOUT_ENTRY *NextFile() { return this->NextFileOffset ? reinterpret_cast<FILE_LAYOUT_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->NextFileOffset) : NULL; }
	FILE_LAYOUT_ENTRY const *NextFile() const { return this->NextFileOffset ? reinterpret_cast<FILE_LAYOUT_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->NextFileOffset) : NULL; }
	struct FILE_LAYOUT_NAME_ENTRY *FirstName() { return this->FirstNameOffset ? reinterpret_cast<FILE_LAYOUT_NAME_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->FirstNameOffset) : NULL; }
	struct FILE_LAYOUT_NAME_ENTRY const *FirstName() const { return this->FirstNameOffset ? reinterpret_cast<FILE_LAYOUT_NAME_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->FirstNameOffset) : NULL; }
	struct STREAM_LAYOUT_ENTRY *FirstStream() { return this->FirstStreamOffset ? reinterpret_cast<STREAM_LAYOUT_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->FirstStreamOffset) : NULL; }
	struct STREAM_LAYOUT_ENTRY const *FirstStream() const { return this->FirstStreamOffset ? reinterpret_cast<STREAM_LAYOUT_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->FirstStreamOffset) : NULL; }
	struct FILE_LAYOUT_INFO_ENTRY *ExtraInfo() { return this->ExtraInfoOffset ? reinterpret_cast<FILE_LAYOUT_INFO_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->ExtraInfoOffset) : NULL; }
	struct FILE_LAYOUT_INFO_ENTRY const *ExtraInfo() const { return this->ExtraInfoOffset ? reinterpret_cast<FILE_LAYOUT_INFO_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->ExtraInfoOffset) : NULL; }
};

// Each file name entry may be one, both, or neither of
// these.
#define FILE_LAYOUT_NAME_ENTRY_PRIMARY                                  (0x00000001)
#define FILE_LAYOUT_NAME_ENTRY_DOS                                      (0x00000002)

struct FILE_LAYOUT_NAME_ENTRY
{
	// Offset to next name entry (in bytes) or zero if this is the last entry.
	DWORD         NextNameOffset;

	// Flags for this file name entry.
	DWORD         Flags;

	// Parent FRN for this link.
	DWORDLONG     ParentFileReferenceNumber;

	// File name length (bytes).
	DWORD         FileNameLength;

	// For later use/alignment.
	DWORD         Reserved;

	// Starting point for the name itself (NOT null-terminated).
	WCHAR         FileName[1];

	FILE_LAYOUT_NAME_ENTRY *NextName() { return this->NextNameOffset ? reinterpret_cast<FILE_LAYOUT_NAME_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->NextNameOffset) : NULL; }
	FILE_LAYOUT_NAME_ENTRY const *NextName() const { return this->NextNameOffset ? reinterpret_cast<FILE_LAYOUT_NAME_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->NextNameOffset) : NULL; }
};

struct FILE_LAYOUT_INFO_ENTRY
{
	// Basic information for this file.
	struct
	{
		LARGE_INTEGER CreationTime;
		LARGE_INTEGER LastAccessTime;
		LARGE_INTEGER LastWriteTime;
		LARGE_INTEGER ChangeTime;
		DWORD FileAttributes;
	} BasicInformation;

	// Owner ID for this file.
	DWORD                       OwnerId;

	// Security ID for this file.
	DWORD                       SecurityId;

	// Update sequence number for this file.
	USN                         Usn;
};

// This attribute/stream is known to the filesystem to be immovable.
#define STREAM_LAYOUT_ENTRY_IMMOVABLE                                   (0x00000001)

// This attribute/stream is currently pinned by another application.
// It is unmovable for the duration of the pin.
#define STREAM_LAYOUT_ENTRY_PINNED                                      (0x00000002)

// This attribute is resident.
#define STREAM_LAYOUT_ENTRY_RESIDENT                                    (0x00000004)

// This attribute has no clusters allocated to it.
#define STREAM_LAYOUT_ENTRY_NO_CLUSTERS_ALLOCATED                       (0x00000008)

struct STREAM_LAYOUT_ENTRY
{
	// Version of this struct. Current version is 1.
	DWORD         Version;

	// Offset to the next stream entry (bytes).
	DWORD         NextStreamOffset;

	// FSCTL-specific flags.
	DWORD         Flags;

	// Offset to the extent information buffer for this stream, or zero if none exists.
	// This is relative to the start of this stream record.
	DWORD         ExtentInformationOffset;

	// Total allocated size of this stream, in bytes.
	LARGE_INTEGER AllocationSize;

	// End of file location as a byte offset.
	LARGE_INTEGER EndOfFile;

	// For alignment purposes. Can be used for future expansion.
	DWORDLONG     Reserved;

	// Stream attribute flags.
	DWORD         AttributeFlags;

	// Length of the stream identifier, in bytes.
	DWORD         StreamIdentifierLength;

	// Starting point for the stream identifier buffer.
	WCHAR         StreamIdentifier[1];

	STREAM_LAYOUT_ENTRY *NextStream() { return this->NextStreamOffset ? reinterpret_cast<STREAM_LAYOUT_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->NextStreamOffset) : NULL; }
	STREAM_LAYOUT_ENTRY const *NextStream() const { return this->NextStreamOffset ? reinterpret_cast<STREAM_LAYOUT_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->NextStreamOffset) : NULL; }
	struct STREAM_EXTENT_ENTRY *ExtentInformation() { return this->ExtentInformationOffset ? reinterpret_cast<STREAM_EXTENT_ENTRY *>(reinterpret_cast<unsigned char *>(this) + this->NextStreamOffset) : NULL; }
	struct STREAM_EXTENT_ENTRY const *ExtentInformation() const { return this->ExtentInformationOffset ? reinterpret_cast<STREAM_EXTENT_ENTRY const *>(reinterpret_cast<unsigned char const *>(this) + this->NextStreamOffset) : NULL; }
};

// Flag noting that the extent information may be interpreted as a RETRIEVAL_POINTERS_BUFFER structure
#define STREAM_EXTENT_ENTRY_AS_RETRIEVAL_POINTERS                       (0x00000001)

// Flag noting that all of the stream's extents are returned in this structure, even if only some of them fall within the caller's specified interest region(s).
#define STREAM_EXTENT_ENTRY_ALL_EXTENTS                                 (0x00000002)

struct STREAM_EXTENT_ENTRY
{
	// Extent-level flags for this entry.
	DWORD        Flags;

	union
	{
		// All that's defined for now is a retrieval pointers buffer, since this is what NTFS will use.
		RETRIEVAL_POINTERS_BUFFER        RetrievalPointers;
	} ExtentInformation;
};
#endif
