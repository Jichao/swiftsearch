#pragma once

#include <assert.h>
#include <windows.h>
#include <winioctl.h>


namespace NTFS
{
	const LONGLONG FILE_SEGMENT_NUMBER_MASK = 0x0000FFFFFFFFFFFFLL;
	const UINT FILE_RECORD_SIZE = 1024;
	struct NtfsFileRecordOutputBuffer
	{
		NTFS_FILE_RECORD_OUTPUT_BUFFER Output;
		BYTE Buffer[FILE_RECORD_SIZE + FIELD_OFFSET(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer) - sizeof(NTFS_FILE_RECORD_OUTPUT_BUFFER)];
	};
	const BYTE NTFS_FILE_NAME_WIN32 = 1;
	struct MULTI_SECTOR_HEADER
	{
		ULONG Magic;
		USHORT USAOffset;
		USHORT USACount;

		USHORT* GetUSA() { return (USHORT*)((BYTE*)this + this->USAOffset); }

		bool TryUnFixup(size_t cbMax)
		{
			bool result = true;
			USHORT* pUSA = this->GetUSA();
			for (USHORT i = 1; i < this->USACount; i++)
			{
				const size_t offset = i * 512 - sizeof(USHORT);
				USHORT *pCheck = (USHORT*)((BYTE*)this + offset);
				if (offset < cbMax)
				{
					if (pUSA[0] != *pCheck)
					{
						result = false;
					}
					*pCheck = pUSA[i];
				}
				else { break; }
			}
			return result;
		}

		void UnFixup(size_t cbMax = ULONG_MAX)
		{
			if (!this->TryUnFixup(cbMax))
			{
				RaiseException(ERROR_FILE_CORRUPT, 0, 0, NULL);
			}
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
		End = -1,
	};
	struct ATTRIBUTE_RECORD_HEADER
	{
		AttributeTypeCode Type;
		ULONG Length;
		UCHAR IsNonResident;
		UCHAR NameLength;
		USHORT NameOffset;
		USHORT Flags;
		USHORT Instance;
		union
		{
			struct RESIDENT
			{
				ULONG ValueLength;
				USHORT ValueOffset;
				USHORT Flags;
				inline void *GetValue() { return reinterpret_cast<void *>(reinterpret_cast<char *>(CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset); }
				inline void const *GetValue() const { return reinterpret_cast<const void *>(reinterpret_cast<const char *>(CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, Resident)) + this->ValueOffset); }
			} Resident;
			struct NONRESIDENT
			{
				ULONGLONG LowestVCN;
				ULONGLONG HighestVCN;
				USHORT MappingPairsOffset;
				UCHAR CompressionUnit;
				UCHAR Reserved[5];
				LONGLONG AllocatedSize;
				LONGLONG DataSize;
				LONGLONG InitializedSize;
				LONGLONG CompressedSize;
				inline BYTE *GetMappingPairs() { return (BYTE *)CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, NonResident) + this->MappingPairsOffset; }
				inline BYTE const *GetMappingPairs() const { return (BYTE const *)CONTAINING_RECORD(this, ATTRIBUTE_RECORD_HEADER, NonResident) + this->MappingPairsOffset; }
			} NonResident;
		};
		inline ATTRIBUTE_RECORD_HEADER *TryGetNextAttributeHeader(){ ATTRIBUTE_RECORD_HEADER *pNext = reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<char *>(this) + this->Length); return pNext->Type == End || pNext->Type == 0 ? NULL : pNext; }
		inline ATTRIBUTE_RECORD_HEADER const *TryGetNextAttributeHeader() const { const ATTRIBUTE_RECORD_HEADER *pNext = reinterpret_cast<const ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<const char *>(this) + this->Length); return pNext->Type == End || pNext->Type == 0 ? NULL : pNext; }
		inline LPWSTR GetName() { return this->NameLength > 0 ? (LPWSTR)((char*)this + this->NameOffset) : NULL; }
		inline LPCWSTR GetName() const { return this->NameLength > 0 ? (LPCWSTR)((char*)this + this->NameOffset) : NULL; }
	};
	#define FRH_IN_USE    0x0001    /* Record is in use */
	#define FRH_DIRECTORY 0x0002    /* Record is a directory */
	struct FILE_RECORD_SEGMENT_HEADER
	{
		MULTI_SECTOR_HEADER MultiSectorHeader;
		ULONGLONG LogFileSequenceNumber;
		USHORT SequenceNumber;
		USHORT LinkCount;
		USHORT FirstAttributeOffset;
		USHORT Flags;
		ULONG BytesInUse;
		ULONG BytesAllocated;
		ULONGLONG BaseFileRecordSegment;
		USHORT NextAttributeNumber;
		//http://blogs.technet.com/b/joscon/archive/2011/01/06/how-hard-links-work.aspx
		USHORT SegmentNumberUpper_or_USA_or_UnknownReserved;  // WARNING: This does NOT seem to be the actual "upper" segment number of anything! I found it to be 0x26e on one of my drives... and checkdisk didn't say anything about it
		ULONG SegmentNumberLower;

		inline ULONGLONG GetSegmentNumber() const { return (ULONGLONG)this->SegmentNumberLower /*+ ((ULONGLONG)this->SegmentNumberUpper << (8 * sizeof(this->SegmentNumberLower)))*/; }

		inline ATTRIBUTE_RECORD_HEADER *TryGetAttributeHeader(USHORT attributeNumber = 0)
		{
			// KEEP IN SYNC WITH BELOW
			int n = attributeNumber;
			ATTRIBUTE_RECORD_HEADER * pCurrent = reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<char *>(this) + this->FirstAttributeOffset);
			while (n > 0 && pCurrent->Type != End)
			{ pCurrent = reinterpret_cast<ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<char *>(pCurrent) + pCurrent->Length); n--; }
			if (pCurrent->Type == End) { pCurrent = NULL; }
			return pCurrent;
		}

		inline ATTRIBUTE_RECORD_HEADER const *TryGetAttributeHeader(USHORT attributeNumber = 0) const
		{
			// KEEP IN SYNC WITH ABOVE
			int n = attributeNumber;
			const ATTRIBUTE_RECORD_HEADER * pCurrent = reinterpret_cast<const ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<const char *>(this) + this->FirstAttributeOffset);
			while (n > 0 && pCurrent->Type != End)
			{ pCurrent = reinterpret_cast<const ATTRIBUTE_RECORD_HEADER *>(reinterpret_cast<const char *>(pCurrent) + pCurrent->Length); n--; }
			if (pCurrent->Type == End) { pCurrent = NULL; }
			return pCurrent;
		}
	};
	struct FILENAME_INFORMATION
	{
		ULONGLONG ParentDirectory;
		LONGLONG CreationTime;
		LONGLONG LastModificationTime;
		LONGLONG LastChangeTime;
		LONGLONG LastAccessTime;
		LONGLONG AllocatedLength;
		LONGLONG FileSize;
		ULONG FileAttributes;
		USHORT PackedEaSize;
		USHORT Reserved;
		UCHAR FileNameLength;
		UCHAR Flags;
		WCHAR FileName[1];
	};
	struct STANDARD_INFORMATION
	{
		LONGLONG CreationTime;
		LONGLONG LastModificationTime;
		LONGLONG LastChangeTime;
		LONGLONG LastAccessTime;
		ULONG FileAttributes;
		// There's more, but only in newer versions
	};
	struct INDEX_HEADER
	{
		ULONG FirstIndexEntry;
		ULONG FirstFreeByte;
		ULONG BytesAvailable;
		UCHAR Flags;  // '1' == has INDEX_ALLOCATION
		UCHAR Reserved[3];
	};
	struct INDEX_ROOT
	{
		AttributeTypeCode Type;
		ULONG CollationRule;
		ULONG BytesPerIndexBlock;
		BYTE ClustersPerIndexBlock;
		INDEX_HEADER Header;
	};
	struct ATTRIBUTE_LIST
	{
		AttributeTypeCode AttributeType;
		USHORT Length;
		UCHAR NameLength;
		UCHAR NameOffset;
		ULONGLONG StartVcn; // LowVcn
		ULONGLONG FileReferenceNumber;
		USHORT AttributeNumber;
		USHORT AlignmentOrReserved[3];
		LPTSTR GetName() { return (LPTSTR)((PBYTE)this + this->NameOffset); }
	};
	typedef DWORD (*ReadFileRecordProc)(LONGLONG fileReferenceNumber, NTFS_FILE_RECORD_OUTPUT_BUFFER* buffer, PVOID state);
	static LONGLONG MapVirtualToLogical(LONGLONG virtualOffset, const RETRIEVAL_POINTERS_BUFFER *pClusterRetrievalPointers, ULONG clusterSize, LONGLONG* pRangeLength)
	{
		LONGLONG requestedVBN = virtualOffset;
		LONGLONG currentVBN = pClusterRetrievalPointers->StartingVcn.QuadPart * clusterSize;
		for (DWORD i = 0; i < pClusterRetrievalPointers->ExtentCount; i++)
		{
			LONGLONG nextVBN = pClusterRetrievalPointers->Extents[i].NextVcn.QuadPart * clusterSize;
			if (currentVBN <= requestedVBN && requestedVBN < nextVBN)
			{
				*pRangeLength = nextVBN - requestedVBN;
				return pClusterRetrievalPointers->Extents[i].Lcn.QuadPart * clusterSize + (requestedVBN - currentVBN);
			}
			currentVBN = nextVBN;
		}
		return -1;
	}
	static DWORD ProcessFileVirtual(bool write, HANDLE hVolume, const RETRIEVAL_POINTERS_BUFFER *pClusterRetrievalPointers, ULONG clusterSize, const LONGLONG virtualOffset, PVOID buffer, DWORD length)
	{
		DWORD lengthProcessed = 0;

		while (lengthProcessed < length)
		{
			LONGLONG rangeLength = 0;
			LONGLONG logical = MapVirtualToLogical(virtualOffset + lengthProcessed, pClusterRetrievalPointers, clusterSize, &rangeLength);
			if (logical >= 0)
			{
				int processed;
				DWORD bytesProcessed;
				OVERLAPPED overlapped = {0};
				LARGE_INTEGER logicalLI;
				logicalLI.QuadPart = logical;
				overlapped.Offset = logicalLI.LowPart;
				overlapped.OffsetHigh = logicalLI.HighPart;
				DWORD toProcess = (DWORD)(min(length - lengthProcessed, rangeLength));
				if (write)
				{
					RaiseException(ERROR_NOT_SUPPORTED, 0, 0, NULL);
					/*if (!WriteFile(hVolume, (PBYTE)buffer + lengthProcessed, toProcess, &bytesProcessed, &overlapped)) { RaiseException(GetLastError(), NULL, NULL, 0); }*/
					processed = toProcess;
				}
				else { if (!ReadFile(hVolume, (PBYTE)buffer + lengthProcessed, toProcess, &bytesProcessed, &overlapped)) { RaiseException(GetLastError(), NULL, NULL, 0); } }
				lengthProcessed += toProcess;
			}
			else
			{
				//__debugbreak();
				//memset((PBYTE)buffer + lengthProcessed, 0, length - lengthProcessed);
				break;
			}
		}

		return lengthProcessed;
	}
	static LPCTSTR GetAttributeName(AttributeTypeCode typeCode)
	{
		switch (typeCode)
		{
			case AttributeFileName:
				return TEXT("$FILE_NAME");
			case AttributeStandardInformation:
				return TEXT("$STANDARD_INFORMATION");
			case AttributeAttributeList:
				return TEXT("$ATTRIBUTE_LIST");
			case AttributeObjectId:
				return TEXT("$OBJECT_ID");
			case AttributeSecurityDescriptor:
				return TEXT("$SECURITY_DESCRIPTOR");
			case AttributeVolumeName:
				return TEXT("$VOLUME_NAME");
			case AttributeVolumeInformation:
				return TEXT("$VOLUME_INFORMATION");
			case AttributeData:
				return TEXT("$DATA");
			case AttributeIndexRoot:
				return TEXT("$INDEX_ROOT");
			case AttributeIndexAllocation:
				return TEXT("$INDEX_ALLOCATION");
			case AttributeBitmap:
				return TEXT("$BITMAP");
			case AttributeReparsePoint:
				return TEXT("$REPARSE_POINT");
			case AttributeEAInformation:
				return TEXT("$EA_INFORMATION");
			case AttributeEA:
				return TEXT("$EA");
			case AttributePropertySet:
				return TEXT("$PROPERTY_SET");
			case AttributeLoggedUtilityStream:
				return TEXT("$LOGGED_UTILITY_STREAM");
			default:
				RaiseException(ERROR_INVALID_PARAMETER, 0, 0, NULL);
				break;
		}
		return NULL;
	}
	inline ULONG RunLength(unsigned char const *run) { return(*run & 0x0f) + ((*run >> 4) & 0x0f) + 1; }
	inline LONGLONG RunDeltaLCN(unsigned char const *run)
	{
		UCHAR n1 = *run & 0x0f;
		UCHAR n2 = (*run >> 4) & 0x0f;
		if (n1 > sizeof(LONGLONG)) { __debugbreak(); }
		if (n2 > sizeof(LONGLONG)) { __debugbreak(); }
		LONGLONG lcn = (n2 == 0) ? 0 : (CHAR)(run[n1 + n2]);
		LONG i = 0;
		for (i = n1 + n2 - 1; i > n1; i--) { lcn = (lcn * (1 << CHAR_BIT)) + run[i]; }
		return lcn;
	}
	inline ULONGLONG RunCount(unsigned char const *run)
	{
		UCHAR n =  *run & 0xf;
		ULONGLONG count = 0;
		ULONG i = 0;

		for (i = n; i > 0; i--) { count = (count * (1 << CHAR_BIT)) + run[i]; }
		return count;
	}
	inline bool FindRun(ATTRIBUTE_RECORD_HEADER* NresAttr, ULONGLONG vcn, PULONGLONG lcn, PULONGLONG count)
	{
		PBYTE run;
		ULONGLONG base = NresAttr->NonResident.LowestVCN;
		if (vcn < NresAttr->NonResident.LowestVCN || vcn > NresAttr->NonResident.HighestVCN) { return false; }
		*lcn = 0;
		for (run = (PBYTE)NresAttr->NonResident.GetMappingPairs(); *run != 0; run += RunLength(run))
		{
			*lcn += RunDeltaLCN(run);
			*count = RunCount(run);

			if (base <= vcn && vcn < base + *count)
			{
				*lcn = (RunDeltaLCN(run) == 0) ? 0 : *lcn + vcn - base;
				*count -= (ULONG)(vcn - base);
				return true;
			}
			else { base += *count; }
		}
		return false;
	}

	static PRETRIEVAL_POINTERS_BUFFER AllocAndConvertRetrievalPointers(ATTRIBUTE_RECORD_HEADER* pNRA)
	{
		DWORD counter = 0;

		assert(pNRA->IsNonResident);

		{
			PBYTE pRun = pNRA->NonResident.GetMappingPairs();
			ULONGLONG vcn = pNRA->NonResident.LowestVCN;
			while (*pRun != 0)
			{
				vcn += RunCount(pRun);
				pRun += RunLength(pRun);

				counter++;
			}
		}

		PRETRIEVAL_POINTERS_BUFFER pRetPtrs = (PRETRIEVAL_POINTERS_BUFFER)malloc(FIELD_OFFSET(RETRIEVAL_POINTERS_BUFFER, Extents) + counter * 2 * sizeof(LONGLONG) * 4);
		pRetPtrs->ExtentCount = counter;
		counter = 0;
		{
			LONGLONG runLCN = 0;
			LONGLONG runVCN = pRetPtrs->StartingVcn.QuadPart = pNRA->NonResident.LowestVCN;
			BYTE* pRuns = pNRA->NonResident.GetMappingPairs();
			for (PBYTE pRun = pRuns; *pRun != 0; pRun += RunLength(pRun))
			{
				LONGLONG deltaLCN = RunDeltaLCN(pRun);
				runLCN += deltaLCN;
				ULONGLONG runCount = RunCount(pRun);
				runVCN += runCount;

				pRetPtrs->Extents[counter].Lcn.QuadPart = deltaLCN == 0 ? -1 : runLCN;
				pRetPtrs->Extents[counter].NextVcn.QuadPart = runVCN;

				counter++;
			}
		}
		assert(pRetPtrs->ExtentCount == 0 || pRetPtrs->Extents[pRetPtrs->ExtentCount - 1].NextVcn.QuadPart == (LONGLONG)pNRA->NonResident.HighestVCN + 1);
		return pRetPtrs;
	}
	static ATTRIBUTE_RECORD_HEADER* FindAttribute(HANDLE hVolume, PRETRIEVAL_POINTERS_BUFFER pMFT, ULONG clusterSize, FILE_RECORD_SEGMENT_HEADER* pFileRecord, USHORT instance, AttributeTypeCode type, LPCTSTR name, ReadFileRecordProc readProc, PVOID readProcState)
	{
		ATTRIBUTE_RECORD_HEADER* pListFound = NULL;
		ATTRIBUTE_RECORD_HEADER* pBackup = NULL;
		for (ATTRIBUTE_RECORD_HEADER* pAttribHeader = pFileRecord->TryGetAttributeHeader(); pAttribHeader != NULL; pAttribHeader = pAttribHeader->TryGetNextAttributeHeader())
		{
			if ((type == End || pAttribHeader->Type == type)
				&& (instance == (USHORT)(-1) || pAttribHeader->Instance == instance)
				&& (name == NULL || _tcsnicmp(name, pAttribHeader->GetName(), pAttribHeader->NameLength) == 0))
			{
				if (type == AttributeFileName && instance == (USHORT)(-1))
				{
					FILENAME_INFORMATION* pFNIA = (FILENAME_INFORMATION*)pAttribHeader->Resident.GetValue();
					if (pFNIA->Flags == 2) { pBackup = pAttribHeader; }
					else { return pAttribHeader; }
				}
				else { return pAttribHeader; }
			}
			else if (pAttribHeader->Type == AttributeAttributeList)
			{
				pListFound = pAttribHeader;
			}
		}
		if (pListFound != NULL)
		{
			std::vector<BYTE> attributeListHeapBuffer;
			ATTRIBUTE_LIST* pAttributeList;
			LONGLONG attrLength;
			if (pListFound->IsNonResident)
			{
				attrLength = pListFound->NonResident.DataSize;
				//BUG: If not all mapping pairs are here, we won't find them...
				LONGLONG alignedLength = (1 + (attrLength - 1) / clusterSize) * clusterSize;
				assert(alignedLength <= MAXDWORD);
				attributeListHeapBuffer.resize(static_cast<size_t>(alignedLength));
				pAttributeList = static_cast<ATTRIBUTE_LIST *>(static_cast<PVOID>(attributeListHeapBuffer.empty() ? NULL : &attributeListHeapBuffer[0]));
				if (pListFound->NonResident.LowestVCN != 0) { __debugbreak(); }
				PRETRIEVAL_POINTERS_BUFFER pRetPtrs = AllocAndConvertRetrievalPointers(pListFound);
				ProcessFileVirtual(false, hVolume, pRetPtrs, clusterSize, pListFound->NonResident.LowestVCN * clusterSize, pAttributeList, (DWORD)alignedLength);
				free(pRetPtrs);
				//BUG: Fixups needed??
			}
			else
			{
				attrLength = pListFound->Resident.ValueLength;
				pAttributeList = static_cast<ATTRIBUTE_LIST *>(pListFound->Resident.GetValue());
			}
			for (ULONG offset = 0; offset < attrLength; offset += ((ATTRIBUTE_LIST*)((PBYTE)pAttributeList + offset))->Length)
			{
				ATTRIBUTE_LIST* pCurrent = (ATTRIBUTE_LIST*)((PBYTE)pAttributeList + offset);
				if ((type == End || pCurrent->AttributeType == type)
					&& (instance == (USHORT)(-1) || pCurrent->AttributeNumber == instance)
					&& (name == NULL || _tcsnicmp(name, pCurrent->GetName(), pCurrent->NameLength) == 0))
				{
					if ((pCurrent->FileReferenceNumber & FILE_SEGMENT_NUMBER_MASK) != pFileRecord->GetSegmentNumber())
					{
						union { FILE_RECORD_SEGMENT_HEADER Header; BYTE Buffer[1024]; } record;
						ProcessFileVirtual(false, hVolume, pMFT, clusterSize, (pCurrent->FileReferenceNumber & FILE_SEGMENT_NUMBER_MASK) * pFileRecord->BytesAllocated, &record, sizeof(record));
						record.Header.MultiSectorHeader.UnFixup();
						return FindAttribute(hVolume, pMFT, clusterSize, &record.Header, pCurrent->AttributeNumber, pCurrent->AttributeType, pCurrent->NameLength > 0 ? pCurrent->GetName() : NULL, readProc, readProcState);
					}
				}
			}
		}
		if (pBackup == NULL && pFileRecord->BaseFileRecordSegment != 0)
		{
			union { BYTE Buffer[FILE_RECORD_SIZE]; FILE_RECORD_SEGMENT_HEADER Header; } baseFileRecord;
			ProcessFileVirtual(false, hVolume, pMFT, clusterSize, (pFileRecord->BaseFileRecordSegment & FILE_SEGMENT_NUMBER_MASK) * FILE_RECORD_SIZE, &baseFileRecord, sizeof(baseFileRecord));
			baseFileRecord.Header.MultiSectorHeader.UnFixup();
			pBackup = FindAttribute(hVolume, pMFT, clusterSize, &baseFileRecord.Header, instance, type, name, readProc, readProcState);
		}
		return pBackup;
	}
	inline LPTSTR TryMakePath(HANDLE hVolume, PRETRIEVAL_POINTERS_BUFFER pMFT, DWORD bytesPerCluster, FILE_RECORD_SEGMENT_HEADER* pFileRecord, ATTRIBUTE_RECORD_HEADER* pAttribute, ReadFileRecordProc readProc, PVOID readProcState)
	{
		static TCHAR filePath[32 * 1024];
		filePath[0] = TEXT('\0');
		TCHAR temp[FILE_RECORD_SIZE];
		NtfsFileRecordOutputBuffer fileRecord;


		TCHAR attrName[MAXBYTE + 1];
		_tcsncpy(attrName, pAttribute->GetName(), pAttribute->NameLength);
		attrName[pAttribute->NameLength] = TEXT('\0');

		if (((pFileRecord->Flags & FRH_DIRECTORY) == 0 && (pAttribute->Type != AttributeData || pAttribute->NameLength != 0))
			|| ((pFileRecord->Flags & FRH_DIRECTORY) != 0 && pAttribute->Type != AttributeIndexAllocation && (pAttribute->Type != AttributeIndexRoot || _tcscmp(TEXT("$I30"), attrName) != 0)))
		{
			_tcscpy(temp, GetAttributeName(pAttribute->Type));
			_tcsrev(temp);
			_tcscat(filePath, temp);
			_tcscat(filePath, TEXT(":"));

			_tcscpy(temp, attrName);
			temp[pAttribute->NameLength] = TEXT('\0');
			_tcsrev(temp);
			_tcscat(filePath, temp);
			_tcscat(filePath, TEXT(":"));
		}
		else
		{
			if ((pFileRecord->Flags & FRH_DIRECTORY) != 0) { _tcscat(filePath, TEXT("\\")); }
		}

		ATTRIBUTE_RECORD_HEADER* pAttrFileName = FindAttribute(hVolume, pMFT, bytesPerCluster, pFileRecord, (USHORT)(-1), AttributeFileName, NULL, readProc, readProcState);

		if (pAttrFileName == NULL) { __debugbreak(); }

		assert(pAttrFileName != NULL);
		FILENAME_INFORMATION* pFileNameInfoCurrent = (FILENAME_INFORMATION*)pAttrFileName->Resident.GetValue();
		for (; ; )
		{
			if (pFileRecord->GetSegmentNumber() == 5) { pFileNameInfoCurrent->FileName[0] = TEXT('\0'); pFileNameInfoCurrent->FileNameLength = 0; }
			_tcsncpy(temp, pFileNameInfoCurrent->FileName, pFileNameInfoCurrent->FileNameLength);
			temp[pFileNameInfoCurrent->FileNameLength] = TEXT('\0');
			_tcsrev(temp);
			_tcscat(filePath, temp);
			_tcscat(filePath, TEXT("\\"));

			if (pFileNameInfoCurrent->ParentDirectory == 0x0005000000000005LL) { break; }

			fileRecord.Output.FileRecordLength = sizeof(fileRecord) - FIELD_OFFSET(NTFS_FILE_RECORD_OUTPUT_BUFFER, FileRecordBuffer);
			(*readProc)(pFileNameInfoCurrent->ParentDirectory, &fileRecord.Output, readProcState);
			/*
			{
			LONGLONG frn = pFileNameInfoCurrent->ParentDirectory & FILE_SEGMENT_NUMBER_MASK;
			FILE_RECORD_SEGMENT_HEADER* pHeader = GetNtfsFileRecordRoundDownOrNull(hVolume, &frn);
			memcpy(&fileRecord, pHeader, pHeader->BytesAllocated);
			}
			//*/

			ATTRIBUTE_RECORD_HEADER* pNameAttr = FindAttribute(hVolume, pMFT, bytesPerCluster, (FILE_RECORD_SEGMENT_HEADER*)fileRecord.Output.FileRecordBuffer, (USHORT)(-1), AttributeFileName, NULL, readProc, readProcState);
			if (pNameAttr != NULL)
			{
				FILENAME_INFORMATION* pName = (FILENAME_INFORMATION*)pNameAttr->Resident.GetValue();

				pFileNameInfoCurrent = pName;
			}
			else { return NULL; }
		}

		_tcsrev(filePath);

		return filePath;
	}

	inline DWORD CountFragments(PRETRIEVAL_POINTERS_BUFFER pRetPtrs, LONGLONG* lcnAfterLastContigCluster, LONGLONG* vcnAfterLastContigCluster)
	{
		*lcnAfterLastContigCluster = 0;
		*vcnAfterLastContigCluster = pRetPtrs->StartingVcn.QuadPart;
		LONGLONG lastLengthAllocated = 0;
		DWORD iLastAllocated = (DWORD)(-1);
		DWORD fragCount = 1;
		for (DWORD i = 0; i < pRetPtrs->ExtentCount; i++)
		{
			if (pRetPtrs->Extents[i].Lcn.QuadPart != -1)
			{
				if (pRetPtrs->Extents[i].Lcn.QuadPart != *lcnAfterLastContigCluster && iLastAllocated != (DWORD)(-1))
				{
					fragCount++;
				}
				lastLengthAllocated = pRetPtrs->Extents[i].NextVcn.QuadPart - *vcnAfterLastContigCluster;
				*lcnAfterLastContigCluster = pRetPtrs->Extents[i].Lcn.QuadPart + lastLengthAllocated;
				iLastAllocated = i;
			}
			*vcnAfterLastContigCluster = pRetPtrs->Extents[i].NextVcn.QuadPart;
		}
		*lcnAfterLastContigCluster = -1;
		return fragCount;
	}
}