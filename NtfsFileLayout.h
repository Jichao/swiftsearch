#pragma once

#define FSCTL_QUERY_FILE_LAYOUT             CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 157, METHOD_NEITHER, FILE_ANY_ACCESS)

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
#define QUERY_FILE_LAYOUT_INCLUDE_EXTRA_INFO                            (0x00000010)

// Include unallocated attributes in the enumeration, which in NTFS means one
// of two cases:
//      1. Resident attributes.
//      2. Compressed or sparse nonresident attributes with no physical
//         allocation (consisting only of a sparse hole).
//  This flag may only be used when no cluster ranges are specified (i. e.
//  on a whole-volume query).

#define QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED    (0x00000020)

typedef enum _QUERY_FILE_LAYOUT_FILTER_TYPE {
	QUERY_FILE_LAYOUT_FILTER_TYPE_NONE = 0,
	QUERY_FILE_LAYOUT_FILTER_TYPE_CLUSTERS = 1,
	QUERY_FILE_LAYOUT_FILTER_TYPE_FILEID = 2,
	QUERY_FILE_LAYOUT_NUM_FILTER_TYPES = 3,
} QUERY_FILE_LAYOUT_FILTER_TYPE;

typedef struct _CLUSTER_RANGE {

	// The starting cluster for this query region
	// (inclusive).
	LARGE_INTEGER       StartingCluster;

	// The length of the cluster region.
	LARGE_INTEGER       ClusterCount;

} CLUSTER_RANGE, *PCLUSTER_RANGE;

typedef struct _FILE_REFERENCE_RANGE {

	// The starting file reference number for this
	// query region (inclusive).
	ULONGLONG           StartingFileReferenceNumber;

	// The ending file reference number for this
	// query region (inclusive).
	ULONGLONG           EndingFileReferenceNumber;

} FILE_REFERENCE_RANGE, *PFILE_REFERENCE_RANGE;

typedef struct _QUERY_FILE_LAYOUT_INPUT {
	ULONG                         NumberOfPairs;
	ULONG                         Flags;
	QUERY_FILE_LAYOUT_FILTER_TYPE FilterType;
	ULONG                         Reserved;
	union {
		CLUSTER_RANGE        ClusterRanges[1];
		FILE_REFERENCE_RANGE FileReferenceRanges[1];
	} Filter;
} QUERY_FILE_LAYOUT_INPUT, *PQUERY_FILE_LAYOUT_INPUT;

// Indicates that the filesystem is returning stream extents in a
// single-instanced fashion.
#define QUERY_FILE_LAYOUT_SINGLE_INSTANCED                              (0x00000001)

typedef struct _QUERY_FILE_LAYOUT_OUTPUT {

	// Number of file entries following this header.
	// Includes only the number of file entries in
	// this iteration.
	ULONG               FileEntryCount;

	// Offset to the first file entry in this buffer,
	// expressed in bytes.
	ULONG               FirstFileOffset;

	// Flags indicating context that is applicable to the
	// entire output set.
	ULONG               Flags;

	// For alignment/later use.
	ULONG               Reserved;

} QUERY_FILE_LAYOUT_OUTPUT, *PQUERY_FILE_LAYOUT_OUTPUT;

typedef struct _FILE_LAYOUT_ENTRY {

	// Version number of this structure
	// (current version number is 1).
	ULONG         Version;

	// Offset to next file entry (in bytes)
	// or zero if this is the last entry.
	ULONG         NextFileOffset;

	// Flags containing context applicable to this
	// file.
	ULONG         Flags;

	// File attributes.
	ULONG         FileAttributes;

	// File ID for this file.
	ULONGLONG     FileReferenceNumber;

	// Offset to the first name entry
	// from the start of this record, or
	// zero if there are no link records.
	ULONG         FirstNameOffset;

	// Offset to the first stream entry
	// from the start of this record, or
	// zero if there are no stream records.
	ULONG         FirstStreamOffset;

	// Offset to additional per-file information,
	// contained in a FILE_LAYOUT_INFO_ENTRY
	// structure, or zero if this information was
	// not returned.
	ULONG         ExtraInfoOffset;

	// For alignment/future use.
	ULONG         Reserved;

	// The structure may be extended here to support
	// additional static fields (e.g. pointing to
	// a FILE_BASIC_INFORMATION structure, etc.). This
	// sort of change should coincide with a version
	// number increase.

} FILE_LAYOUT_ENTRY, *PFILE_LAYOUT_ENTRY;

// Each file name entry may be one, both, or neither of
// these.
#define FILE_LAYOUT_NAME_ENTRY_PRIMARY                                  (0x00000001)
#define FILE_LAYOUT_NAME_ENTRY_DOS                                      (0x00000002)

typedef struct _FILE_LAYOUT_NAME_ENTRY {

	// Offset to next name entry (in bytes)
	// or zero if this is the last entry.
	ULONG         NextNameOffset;

	// Flags for this file name entry.
	ULONG         Flags;

	// Parent FRN for this link.
	ULONGLONG     ParentFileReferenceNumber;

	// File name length (bytes).
	ULONG         FileNameLength;

	// For later use/alignment.
	ULONG         Reserved;

	// Starting point for the name itself
	// (NOT null-terminated).
	WCHAR         FileName[1];

} FILE_LAYOUT_NAME_ENTRY, *PFILE_LAYOUT_NAME_ENTRY;

typedef struct _FILE_LAYOUT_INFO_ENTRY {

	// Basic information for this file.
	struct {
		LARGE_INTEGER CreationTime;
		LARGE_INTEGER LastAccessTime;
		LARGE_INTEGER LastWriteTime;
		LARGE_INTEGER ChangeTime;
		ULONG FileAttributes;
	} BasicInformation;

	// Owner ID for this file.
	ULONG                       OwnerId;

	// Security ID for this file.
	ULONG                       SecurityId;

	// Update sequence number for this file.
	USN                         Usn;

} FILE_LAYOUT_INFO_ENTRY, *PFILE_LAYOUT_INFO_ENTRY;

// This attribute/stream is known to the filesystem to be immovable.
#define STREAM_LAYOUT_ENTRY_IMMOVABLE                                   (0x00000001)

// This attribute/stream is currently pinned by another application.
// It is unmovable for the duration of the pin.
#define STREAM_LAYOUT_ENTRY_PINNED                                      (0x00000002)

// This attribute is resident.
#define STREAM_LAYOUT_ENTRY_RESIDENT                                    (0x00000004)

// This attribute has no clusters allocated to it.
#define STREAM_LAYOUT_ENTRY_NO_CLUSTERS_ALLOCATED                       (0x00000008)

typedef struct _STREAM_LAYOUT_ENTRY {

	// Version of this struct. Current version is 1.
	ULONG         Version;

	// Offset to the next stream entry (bytes).
	ULONG         NextStreamOffset;

	// FSCTL-specific flags.
	ULONG         Flags;

	// Offset to the extent information buffer
	// for this stream, or zero if none exists.
	// This is relative to the start of this
	// stream record.
	ULONG         ExtentInformationOffset;

	// Total allocated size of this stream,
	// in bytes.
	LARGE_INTEGER AllocationSize;

	// End of file location as a byte offset.
	LARGE_INTEGER EndOfFile;

	// For alignment purposes. Can be used
	// for future expansion.
	ULONGLONG     Reserved;

	// Stream attribute flags.
	ULONG         AttributeFlags;

	// Length of the stream identifier, in bytes.
	ULONG         StreamIdentifierLength;

	// Starting point for the stream identifier
	// buffer.
	WCHAR         StreamIdentifier[1];

} STREAM_LAYOUT_ENTRY, *PSTREAM_LAYOUT_ENTRY;

// Flag noting that the extent information may be interpreted as
// a RETRIEVAL_POINTERS_BUFFER structure
#define STREAM_EXTENT_ENTRY_AS_RETRIEVAL_POINTERS                       (0x00000001)

// Flag noting that all of the stream's extents are returned in
// this structure, even if only some of them fall within the caller's
// specified interest region(s).
#define STREAM_EXTENT_ENTRY_ALL_EXTENTS                                 (0x00000002)


typedef struct _STREAM_EXTENT_ENTRY {

	// Extent-level flags for this entry.
	ULONG        Flags;

	union {

		// All that's defined for now is a retrieval
		// pointers buffer, since this is what NTFS
		// will use.
		RETRIEVAL_POINTERS_BUFFER        RetrievalPointers;

	} ExtentInformation;

} STREAM_EXTENT_ENTRY, *PSTREAM_EXTENT_ENTRY;
