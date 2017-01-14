#include "stdafx.h"
#include "ntfsindex.h"
#include "QueryFileLayout.h"

void NtfsIndex::load(uint64_t const virtual_offset, void *const buffer, size_t const size,
	bool const is_file_layout)
{
	if (is_file_layout) {
		if (size) {
			QUERY_FILE_LAYOUT_OUTPUT const *const output =
				static_cast<QUERY_FILE_LAYOUT_OUTPUT const *>(buffer);
			for (FILE_LAYOUT_ENTRY const *i = output->FirstFile(); i;
				i = i->NextFile(), this->_records_so_far.fetch_add(1, boost::memory_order_acq_rel)) {
				unsigned int const frs_base = static_cast<unsigned char>(i->FileReferenceNumber);
				Records::iterator base_record = this->at(frs_base);
				if (FILE_LAYOUT_INFO_ENTRY const *const fn = i->ExtraInfo()) {
					base_record->stdinfo.created = fn->BasicInformation.CreationTime.QuadPart;
					base_record->stdinfo.written = fn->BasicInformation.LastWriteTime.QuadPart;
					base_record->stdinfo.accessed = fn->BasicInformation.LastAccessTime.QuadPart;
					base_record->stdinfo.attributes = fn->BasicInformation.FileAttributes;
				}
				for (FILE_LAYOUT_NAME_ENTRY const *fn = i->FirstName(); fn; fn = fn->NextName()) {
					unsigned int const frs_parent = static_cast<unsigned int>(fn->ParentFileReferenceNumber);
					if (fn->Flags != FILE_LAYOUT_NAME_ENTRY_DOS) {
						LinkInfo info = LinkInfo();
						info.name.offset = static_cast<unsigned int>(this->names.size());
						info.name.length = static_cast<unsigned char>(fn->FileNameLength / sizeof(*fn->FileName));
						info.parent = frs_parent;
						append(this->names, fn->FileName, fn->FileNameLength / sizeof(*fn->FileName));
						size_t const link_index = this->nameinfos.size();
						this->nameinfos.push_back(LinkInfos::value_type(info, base_record->first_name));
						base_record->first_name = static_cast<small_t<size_t>::type>(link_index);
						Records::iterator const parent = this->at(frs_parent, &base_record);
						if (~parent->first_child.first.first) {
							size_t const ichild = this->childinfos.size();
							this->childinfos.push_back(parent->first_child);
							parent->first_child.second = static_cast<small_t<size_t>::type>(ichild);
						}
						parent->first_child.first.first = frs_base;
						parent->first_child.first.second = base_record->name_count;
						this->_total_items += base_record->stream_count;
						++base_record->name_count;
					}
				}
				for (STREAM_LAYOUT_ENTRY const *fn = i->FirstStream(); fn; fn = fn->NextStream()) {
					bool const is_nonresident = true;
					// if (!is_nonresident || !ah->NonResident.LowestVCN)
					{
						bool const isI30 = fn->StreamIdentifierLength == 4
							&& memcmp(fn->StreamIdentifier, _T("$I30"), sizeof(*fn->StreamIdentifier) * 4) == 0;
						// if (!isI30)
						{
							StreamInfo info = StreamInfo();
							if (isI30) {
								// Suppress name
							}
							else {
								info.name.offset = static_cast<unsigned int>(this->names.size());
								info.name.length = static_cast<unsigned char>(fn->StreamIdentifierLength / sizeof(
									*fn->StreamIdentifier));
								append(this->names, fn->StreamIdentifier,
									fn->StreamIdentifierLength / sizeof(*fn->StreamIdentifier));
							}
							info.type_name_id = static_cast<unsigned char>(isI30 ? ntfs::AttributeIndexRoot : 0);
							info.length = static_cast<uint64_t>(fn->EndOfFile.QuadPart);
							info.allocated = is_nonresident ? static_cast<uint64_t>
								(fn->AllocationSize.QuadPart) : 0;
							info.bulkiness = info.allocated;
							if (StreamInfos::value_type *const si = this->streaminfo(base_record)) {
								size_t const stream_index = this->streaminfos.size();
								this->streaminfos.push_back(*si);
								si->second = static_cast<small_t<size_t>::type>(stream_index);
							}
							base_record->first_stream.first = info;
							this->_total_items += base_record->name_count;
							++base_record->stream_count;
						}
					}
				}
			}
		}
		else {
			throw std::logic_error("we know this is the last request, but are all previous requests already finished being processed?");
		}
	}
	else {
		if (size % this->mft_record_size) {
			throw std::runtime_error("cluster size is smaller than MFT record size; split MFT records not supported");
		}
		for (size_t i = virtual_offset % this->mft_record_size ? this->mft_record_size -
			virtual_offset % this->mft_record_size : 0; i + this->mft_record_size <= size;
			i += this->mft_record_size, this->_records_so_far.fetch_add(1,
			boost::memory_order_acq_rel)) {
			unsigned int const frs = static_cast<unsigned int>((virtual_offset + i) /
				this->mft_record_size);
			ntfs::FILE_RECORD_SEGMENT_HEADER *const frsh =
				reinterpret_cast<ntfs::FILE_RECORD_SEGMENT_HEADER *>(&static_cast<unsigned char *>
				(buffer)[i]);
			if (frsh->MultiSectorHeader.Magic == 'ELIF'
				&& frsh->MultiSectorHeader.unfixup(this->mft_record_size)
				&& !!(frsh->Flags & ntfs::FRH_IN_USE)) {
				unsigned int const frs_base = frsh->BaseFileRecordSegment ? static_cast<unsigned int>
					(frsh->BaseFileRecordSegment) : frs;
				Records::iterator base_record = this->at(frs_base);
				for (ntfs::ATTRIBUTE_RECORD_HEADER const
					*ah = frsh->begin(); ah < frsh->end(this->mft_record_size)
					&& ah->Type != ntfs::AttributeTypeCode()
					&& ah->Type != ntfs::AttributeEnd; ah = ah->next()) {
					switch (ah->Type) {
					case ntfs::AttributeStandardInformation:
						if (ntfs::STANDARD_INFORMATION const *const fn =
							static_cast<ntfs::STANDARD_INFORMATION const *>(ah->Resident.GetValue())) {
							base_record->stdinfo.created = fn->CreationTime;
							base_record->stdinfo.written = fn->LastModificationTime;
							base_record->stdinfo.accessed = fn->LastAccessTime;
							base_record->stdinfo.attributes = fn->FileAttributes | ((frsh->Flags &
								ntfs::FRH_DIRECTORY) ? FILE_ATTRIBUTE_DIRECTORY : 0);
						}
						break;
					case ntfs::AttributeFileName:
						if (ntfs::FILENAME_INFORMATION const *const fn =
							static_cast<ntfs::FILENAME_INFORMATION const *>(ah->Resident.GetValue())) {
							unsigned int const frs_parent = static_cast<unsigned int>(fn->ParentDirectory);
							if (fn->Flags != 0x02 /* FILE_NAME_DOS */) {
								LinkInfo info = LinkInfo();
								info.name.offset = static_cast<unsigned int>(this->names.size());
								info.name.length = static_cast<unsigned char>(fn->FileNameLength);
								info.parent = frs_parent;
								append(this->names, fn->FileName, fn->FileNameLength);
								size_t const link_index = this->nameinfos.size();
								this->nameinfos.push_back(LinkInfos::value_type(info, base_record->first_name));
								base_record->first_name = static_cast<small_t<size_t>::type>(link_index);
								Records::iterator const parent = this->at(frs_parent, &base_record);
								if (~parent->first_child.first.first) {
									size_t const ichild = this->childinfos.size();
									this->childinfos.push_back(parent->first_child);
									parent->first_child.second = static_cast<small_t<size_t>::type>(ichild);
								}
								parent->first_child.first.first = frs_base;
								parent->first_child.first.second = base_record->name_count;
								this->_total_items += base_record->stream_count;
								++base_record->name_count;
							}
						}
						break;
						// case ntfs::AttributeAttributeList:
						// case ntfs::AttributeLoggedUtilityStream:
					case ntfs::AttributeBitmap:
					case ntfs::AttributeIndexAllocation:
					case ntfs::AttributeIndexRoot:
					case ntfs::AttributeData:
						if (!ah->IsNonResident || !ah->NonResident.LowestVCN) {
							bool const isI30 = ah->NameLength == 4
								&& memcmp(ah->name(), _T("$I30"), sizeof(*ah->name()) * 4) == 0;
							if (ah->Type == (isI30 ? ntfs::AttributeIndexAllocation : ntfs::AttributeIndexRoot)) {
								// Skip this -- for $I30, index header will take care of index allocation; for others, no point showing index root anyway
							}
							else if (!(isI30 && ah->Type == ntfs::AttributeBitmap)) {
								StreamInfo info = StreamInfo();
								if ((ah->Type == ntfs::AttributeIndexRoot || ah->Type == ntfs::AttributeIndexAllocation)
									&& isI30) {
									// Suppress name
								}
								else {
									info.name.offset = static_cast<unsigned int>(this->names.size());
									info.name.length = static_cast<unsigned char>(ah->NameLength);
									append(this->names, ah->name(), ah->NameLength);
								}
								info.type_name_id = static_cast<unsigned char>((ah->Type == ntfs::AttributeIndexRoot
									|| ah->Type == ntfs::AttributeIndexAllocation) && isI30 ? 0 : ah->Type >> (CHAR_BIT / 2));
								info.length = ah->IsNonResident ? static_cast<uint64_t>
									(frs_base == 0x000000000008 /* $BadClus */ ?
									ah->NonResident.InitializedSize /* actually this is still wrong... */ :
									ah->NonResident.DataSize) : ah->Resident.ValueLength;
								info.allocated = ah->IsNonResident ? ah->NonResident.CompressionUnit ?
									static_cast<uint64_t>(ah->NonResident.CompressedSize) :
									static_cast<uint64_t>(frs_base == 0x000000000008 /* $BadClus */ ?
									ah->NonResident.InitializedSize /* actually this is still wrong... should be looking at VCNs */ :
									ah->NonResident.AllocatedSize) : 0;
								info.bulkiness = info.allocated;
								if (StreamInfos::value_type *const si = this->streaminfo(base_record)) {
									size_t const stream_index = this->streaminfos.size();
									this->streaminfos.push_back(*si);
									si->second = static_cast<small_t<size_t>::type>(stream_index);
								}
								base_record->first_stream.first = info;
								this->_total_items += base_record->name_count;
								++base_record->stream_count;
							}
						}
						break;
					}
				}
				// fprintf(stderr, "%llx\n", frsh->BaseFileRecordSegment);
			}
		}
	}
	this->check_finished();
}

