#pragma once
#include "utils.h"
#include "resource.h"
#include "globals.h"

class NtfsIndex : public RefCounted<NtfsIndex> {
    typedef NtfsIndex this_type;
    template<class = void> struct small_t {
        typedef unsigned int type;
    };
#pragma pack(push)
#pragma pack(2)
    struct StandardInfo {
        long long created, written, accessed;
        unsigned long attributes;
    };
    struct SizeInfo {
        unsigned long long length, allocated, bulkiness;
    };
    friend struct std::is_scalar<StandardInfo>;
    struct NameInfo {
        small_t<size_t>::type offset;
        unsigned char length;
    };
    friend struct std::is_scalar<NameInfo>;
    struct LinkInfo {
        NameInfo name;
        unsigned int parent;
    };
    friend struct std::is_scalar<LinkInfo>;
    struct StreamInfo : SizeInfo {
        StreamInfo() : SizeInfo(), name(), type_name_id() { }
        NameInfo name;
        unsigned char
        type_name_id /* zero if and only if $I30:$INDEX_ROOT or $I30:$INDEX_ALLOCATION */;
    };
    friend struct std::is_scalar<StreamInfo>;
    typedef std::codecvt<std::tstring::value_type, char, int /*std::mbstate_t*/> CodeCvt;
    typedef std::vector<std::pair<LinkInfo, small_t<size_t>::type /* next */> > LinkInfos;
    typedef std::vector<std::pair<StreamInfo, small_t<size_t>::type /* next */> > StreamInfos;
    struct Record;
    typedef std::vector<Record> Records;
    typedef std::vector<unsigned int> RecordsLookup;
    typedef std::vector<std::pair<std::pair<small_t<Records::size_type>::type, small_t<LinkInfos::size_type>::type>, small_t<size_t>::type /* next */> >
    ChildInfos;
    struct Record {
        StandardInfo stdinfo;
        unsigned short name_count, stream_count;
        ChildInfos::value_type::first_type::second_type first_name;
        StreamInfos::value_type first_stream;
        ChildInfos::value_type first_child;
    };
#pragma pack(pop)
    friend struct std::is_scalar<Record>;
    mutable mutex _mutex;
    std::tstring _root_path;
    Handle _volume;
    std::tstring names;
    Records records_data;
    RecordsLookup records_lookup;
    LinkInfos nameinfos;
    StreamInfos streaminfos;
    ChildInfos childinfos;
    Handle _finished_event;
    size_t _total_items;
    boost::atomic<bool> _cancelled;
    boost::atomic<unsigned int> _records_so_far;
    typedef std::pair<std::pair<unsigned int, unsigned short>, std::pair<unsigned short, unsigned long long /* direct address */> >
    key_type_internal;

    Records::iterator at(size_t const frs,
                         Records::iterator *const existing_to_revalidate = NULL)
    {
        if (frs >= this->records_lookup.size()) {
            this->records_lookup.resize(frs + 1, ~RecordsLookup::value_type());
        }
        RecordsLookup::iterator const k = this->records_lookup.begin() + static_cast<ptrdiff_t>
                                          (frs);
        if (!~*k) {
            ptrdiff_t const j = (existing_to_revalidate ? *existing_to_revalidate :
                                 this->records_data.end()) - this->records_data.begin();
            *k = static_cast<unsigned int>(this->records_data.size());
            Record empty_record = Record();
            empty_record.name_count = 0;
            empty_record.stream_count = 0;
            empty_record.stdinfo.attributes = negative_one;
            empty_record.first_name = negative_one;
            empty_record.first_child.second = negative_one;
            empty_record.first_child.first.first = negative_one;
            empty_record.first_stream.first.name.offset = negative_one;
            empty_record.first_stream.first.name.length = 0;
            empty_record.first_stream.second = negative_one;
            this->records_data.push_back(empty_record);
            if (existing_to_revalidate) {
                *existing_to_revalidate = this->records_data.begin() + j;
            }
        }
        return this->records_data.begin() + static_cast<ptrdiff_t>(*k);
    }

    FORCEINLINE LinkInfos::value_type *nameinfo(size_t const i)
    {
        return i < this->nameinfos.size() ? &this->nameinfos[i] : NULL;
    }
    FORCEINLINE LinkInfos::value_type const *nameinfo(size_t const i) const
    {
        return i < this->nameinfos.size() ? &this->nameinfos[i] : NULL;
    }
    FORCEINLINE LinkInfos::value_type *nameinfo(Records::iterator const i)
    {
        return this->nameinfo(i->first_name);
    }
    FORCEINLINE LinkInfos::value_type const *nameinfo(Records::const_iterator const i) const
    {
        return this->nameinfo(i->first_name);
    }
    FORCEINLINE StreamInfos::value_type *streaminfo(Records::iterator const i) const
    {
        assert(~i->first_stream.first.name.offset || (!i->first_stream.first.name.length
                && !i->first_stream.first.length));
        return ~i->first_stream.first.name.offset ? &i->first_stream : NULL;
    }
    FORCEINLINE StreamInfos::value_type const *streaminfo(Records::const_iterator const i)
    const
    {
        assert(~i->first_stream.first.name.offset || (!i->first_stream.first.name.length
                && !i->first_stream.first.length));
        return ~i->first_stream.first.name.offset ? &i->first_stream : NULL;
    }
public:
    typedef key_type_internal key_type;
    typedef StandardInfo standard_info;
    typedef SizeInfo size_info;
    unsigned int mft_record_size;
    unsigned int total_records;
    NtfsIndex(std::tstring value) : _finished_event(CreateEvent(NULL, TRUE, FALSE, NULL)),
        _total_items(), _records_so_far(0), _cancelled(false), mft_record_size(), total_records()
    {
        bool success = false;
        std::tstring dirsep;
        dirsep.append(1, _T('\\'));
        dirsep.append(1, _T('/'));
        try {
            std::tstring path_name = value;
            path_name.erase(path_name.begin() + static_cast<ptrdiff_t>(path_name.size() - std::min(
                                path_name.find_last_not_of(dirsep), path_name.size())), path_name.end());
            if (!path_name.empty() && *path_name.begin() != _T('\\')
                    && *path_name.begin() != _T('/')) {
                path_name.insert(0, _T("\\\\.\\"));
            }
            Handle volume(CreateFile(path_name.c_str(),
                                     FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                                     FILE_FLAG_OVERLAPPED, NULL));
            winnt::IO_STATUS_BLOCK iosb;
            struct : winnt::FILE_FS_ATTRIBUTE_INFORMATION {
                unsigned char buf[MAX_PATH];
            } info = {};
            if (winnt::NtQueryVolumeInformationFile(volume.value, &iosb, &info, sizeof(info), 5) ||
                    info.FileSystemNameLength != 4 * sizeof(*info.FileSystemName)
                    || std::char_traits<TCHAR>::compare(info.FileSystemName, _T("NTFS"), 4)) {
                throw std::invalid_argument("invalid volume");
            }
            winnt::FILE_IO_PRIORITY_HINT_INFORMATION io_priority = { winnt::IoPriorityLow };
            winnt::NtSetInformationFile(volume, &iosb, &io_priority, sizeof(io_priority), 43);
            volume.swap(this->_volume);
            success = true;
        } catch (std::invalid_argument &) {}
        if (success) {
            using std::swap;
            swap(this->_root_path, value);
        }
        if (!success) {
            SetEvent(this->_finished_event);
        }
    }
    ~NtfsIndex()
    {
    }
    NtfsIndex *unvolatile() volatile
    {
        return const_cast<NtfsIndex *>(this);
    }
    NtfsIndex const *unvolatile() const volatile
    {
        return const_cast<NtfsIndex *>(this);
    }
    size_t total_items() const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->total_items();
    }
    size_t total_items() const
    {
        return this->_total_items;
    }
    size_t records_so_far() const volatile
    {
        return this->_records_so_far.load(boost::memory_order_acquire);
    }
    size_t records_so_far() const
    {
        return this->_records_so_far.load(boost::memory_order_relaxed);
    }
    void *volume() const volatile
    {
        return this->_volume.value;
    }
    mutex &get_mutex() const volatile
    {
        return this->unvolatile()->_mutex;
    }
    std::tstring const &root_path() const
    {
        return this->_root_path;
    }
    std::tstring const &root_path() const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->root_path();
    }
    bool cancelled() const volatile
    {
        this_type const *const me = this->unvolatile();
        return me->_cancelled.load(boost::memory_order_acquire);
    }
    void cancel() volatile
    {
        this_type *const me = this->unvolatile();
        me->_cancelled.store(true, boost::memory_order_release);
    }
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
    bool check_finished()
    {
        unsigned int const records_so_far = this->_records_so_far.load(
                                                boost::memory_order_acquire);
        bool const finished = records_so_far >= this->total_records;
        bool const b = finished && !this->_root_path.empty();
        if (b) {
            size_t volatile arr[] = {
                this->names.size() * sizeof(*this->names.begin()),
                this->records_data.size() * sizeof(*this->records_data.begin()),
                this->records_lookup.size() * sizeof(*this->records_lookup.begin()),
                this->nameinfos.size() * sizeof(*this->nameinfos.begin()),
                this->streaminfos.size() * sizeof(*this->streaminfos.begin()),
                this->childinfos.size() * sizeof(*this->childinfos.begin()),
            };
            for (size_t i = 0; i < sizeof(arr) / sizeof(*arr); i++) {
                arr[i] = arr[i];
            }
            typedef std::tstring::const_iterator It;
            std::vector<unsigned long long> scratch;
            this->preprocess(0x000000000005, scratch);
            Handle().swap(this->_volume);
            _ftprintf(stderr, _T("Finished: %s (%u ms)\n"), this->_root_path.c_str(),
                      (clock() - begin_time) * 1000U / CLOCKS_PER_SEC);
        }
        finished ? SetEvent(this->_finished_event) : ResetEvent(this->_finished_event);
        return b;
    }
    void reserve(unsigned int const records)
    {
        if (this->records_lookup.size() < records) {
            this->nameinfos.reserve(records * 2);
            this->streaminfos.reserve(records);
            this->childinfos.reserve(records + records / 2);
            this->names.reserve(records * 32);
            this->records_lookup.resize(records, ~RecordsLookup::value_type());
        }
    }
    void load(unsigned long long const virtual_offset, void *const buffer, size_t const size,
              bool const is_file_layout) volatile
    {
        this_type *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        me->load(virtual_offset, buffer, size, is_file_layout);
    }

	void load(unsigned long long const virtual_offset, void *const buffer, size_t const size,
		bool const is_file_layout);
    
    size_t get_path(key_type key, std::tstring &result, bool const name_only) const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->get_path(key, result, name_only);
    }

    size_t get_path(key_type key, std::tstring &result, bool const name_only) const
    {
        size_t const old_size = result.size();
        bool leaf = true;
        while (~key.first.first) {
            Records::const_iterator const fr = this->records_data.begin() + static_cast<ptrdiff_t>
                                               (this->records_lookup[key.first.first]);
            bool found = false;
            unsigned short ji = 0;
            for (LinkInfos::value_type const *j = this->nameinfo(fr); !found
                    && j; j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji) {
                if (key.first.second == (std::numeric_limits<unsigned short>::max)()
                        || ji == key.first.second) {
                    unsigned short ki = 0;
                    for (StreamInfos::value_type const *k = this->streaminfo(fr); !found
                            && k; k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki) {
                        if (k->first.name.offset + k->first.name.length > this->names.size()) {
                            throw std::logic_error("invalid entry");
                        }
                        if (key.second.first == (std::numeric_limits<unsigned short>::max)() ?
                                !k->first.type_name_id : ki == key.second.first) {
                            found = true;
                            size_t const old_size = result.size();
                            append(result, &this->names[j->first.name.offset], j->first.name.length);
                            if (leaf) {
                                bool const is_alternate_stream = k->first.type_name_id
                                                                 && (k->first.type_name_id << (CHAR_BIT / 2)) != ntfs::AttributeData;
                                if (k->first.name.length || is_alternate_stream) {
                                    result += _T(':');
                                }
                                append(result, k->first.name.length ? &this->names[k->first.name.offset] : NULL,
                                       k->first.name.length);
                                if (is_alternate_stream
                                        && k->first.type_name_id < sizeof(ntfs::attribute_names) / sizeof(
                                            *ntfs::attribute_names)) {
                                    result += _T(':');
                                    append(result, ntfs::attribute_names[k->first.type_name_id]);
                                }
                            }
                            if (key.first.first != 0x000000000005) {
                                if (!k->first.type_name_id) {
                                    result += _T('\\');
                                }
                            }
                            std::reverse(result.begin() + static_cast<ptrdiff_t>(old_size), result.end());
                            key = key_type(key_type::first_type(
                                               j->first.parent /* ... | 0 | 0 (since we want the first name of all ancestors)*/,
                                               ~key_type::first_type::second_type()),
                                           key_type::second_type(~key_type::second_type::first_type(),
                                                                 static_cast<key_type::second_type::second_type>(std::numeric_limits<long long>::max())));
                        }
                    }
                }
            }
            if (!found) {
                throw std::logic_error("could not find a file attribute");
                break;
            }
            leaf = false;
            if (name_only || key.first.first == 0x000000000005) {
                break;
            }
        }
        std::reverse(result.begin() + static_cast<ptrdiff_t>(old_size), result.end());
        return result.size() - old_size;
    }

    size_info get_sizes(key_type const key) const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->get_sizes(key);
    }

    size_info const &get_sizes(key_type const key) const
    {
        return (~key.second.second < key.second.second ? this->records_data[static_cast<size_t>
                (~key.second.second)].first_stream : this->streaminfos[static_cast<size_t>
                        (key.second.second)]).first;
    }

    standard_info get_stdinfo(unsigned int const frn) const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->get_stdinfo(frn);
    }

    standard_info const &get_stdinfo(unsigned int const frn) const
    {
        return this->records_data[this->records_lookup[frn]].stdinfo;
    }

    std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long>
    preprocess(key_type::first_type::first_type const frs,
               std::vector<unsigned long long> &scratch)
    {
        std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long> result;
        if (frs < this->records_lookup.size()) {
            Records::const_iterator const i = this->records_data.begin() + static_cast<ptrdiff_t>
                                              (this->records_lookup[frs]);
            unsigned short const jn = i->name_count;
            unsigned short ji = 0;
            for (LinkInfos::value_type const *j = this->nameinfo(i); j;
                    j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji) {
                std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long> const
                subresult = this->preprocess(key_type::first_type(frs, ji), jn, scratch);
                result.first.first += subresult.first.first;
                result.first.second += subresult.first.second;
                result.second += subresult.second;
                ++ji;
            }
        }
        return result;
    }

    std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long>
    preprocess(key_type::first_type const key_first, unsigned short const total_names,
               std::vector<unsigned long long> &scratch)
    {
        size_t const old_scratch_size = scratch.size();
        std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long> result;
        if (key_first.first < this->records_lookup.size()) {
            Records::iterator const fr = this->records_data.begin() + static_cast<ptrdiff_t>
                                         (this->records_lookup[key_first.first]);
            std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long>
            children_size;
            unsigned short ii = 0;
            for (ChildInfos::value_type *i = &fr->first_child; i
                    && ~i->first.first; i = ~i->second ? &this->childinfos[i->second] : NULL, ++ii) {
                Records::const_iterator const fr2 = this->records_data.begin() + static_cast<ptrdiff_t>
                                                    (this->records_lookup[i->first.first]);
                unsigned short const jn = fr2->name_count;
                unsigned short ji = 0;
                for (LinkInfos::value_type const *j = this->nameinfo(fr2); j;
                        j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji) {
                    if (j->first.parent == key_first.first
                            && i->first.second == jn - static_cast<size_t>(1) - ji &&
                            (static_cast<unsigned int>(i->first.first) != key_first.first
                             || ji != key_first.second)) {
                        std::pair<std::pair<unsigned long long, unsigned long long>, unsigned long long> const
                        subresult = this->preprocess(key_type::first_type(static_cast<unsigned int>
                                                     (i->first.first), ji), jn, scratch);
                        scratch.push_back(subresult.second);
                        children_size.first.first += subresult.first.first;
                        children_size.first.second += subresult.first.second;
                        children_size.second += subresult.second;
                    }
                }
            }
            std::sort(scratch.begin() + static_cast<ptrdiff_t>(old_scratch_size), scratch.end());
            unsigned long long const threshold = children_size.first.second / 100;
            while (scratch.size() > old_scratch_size && scratch.back() >= threshold) {
                children_size.second -= scratch.back();
                scratch.pop_back();
            }
            result = children_size;
            unsigned short ki = 0;
            for (StreamInfos::value_type *k = this->streaminfo(fr); k;
                    k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki) {
                result.first.first += k->first.length * (key_first.second + 1) / total_names -
                                      k->first.length * key_first.second / total_names;
                result.first.second += k->first.allocated * (key_first.second + 1) / total_names -
                                       k->first.allocated * key_first.second / total_names;
                result.second += k->first.bulkiness * (key_first.second + 1) / total_names -
                                 k->first.bulkiness * key_first.second / total_names;
                if (!k->first.type_name_id) {
                    k->first.length += children_size.first.first;
                    k->first.allocated += children_size.first.second;
                    k->first.bulkiness += children_size.second;
                }
            }
        }
        scratch.erase(scratch.begin() + static_cast<ptrdiff_t>(old_scratch_size), scratch.end());
        return result;
    }

    template<class F>
    void matches(F func, std::tstring &path) const volatile
    {
        this_type const *const me = this->unvolatile();
        lock_guard<mutex> const lock(me->_mutex);
        return me->matches<F &>(func, path);
    }

    template<class F>
    void matches(F func, std::tstring &path) const
    {
        return this->matches<F &>(func, path, 0x000000000005);
    }

private:
    template<class F>
    void matches(F func, std::tstring &path, key_type::first_type::first_type const frs) const
    {
        if (frs < this->records_lookup.size()) {
            Records::const_iterator const i = this->records_data.begin() + static_cast<ptrdiff_t>
                                              (this->records_lookup[frs]);
            unsigned short ji = 0;
            for (LinkInfos::value_type const *j = this->nameinfo(i); j;
                    j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji) {
                this->matches<F>(func, path, 0, key_type::first_type(frs, ji),
                                 &this->names[j->first.name.offset], j->first.name.length);
                ++ji;
            }
        }
    }

    template<class F>
    void matches(F func, std::tstring &path, size_t const depth,
                 key_type::first_type const key_first, TCHAR const stream_prefix [] = NULL,
                 size_t const stream_prefix_size = 0) const
    {
        std::tstring const empty_string;
        if (key_first.first < this->records_lookup.size()) {
            size_t const islot = this->records_lookup[key_first.first];
            Records::const_iterator const fr = this->records_data.begin() + static_cast<ptrdiff_t>
                                               (islot);
            unsigned short ii = 0;
            for (ChildInfos::value_type const *i = &fr->first_child; i
                    && ~i->first.first; i = ~i->second ? &this->childinfos[i->second] : NULL, ++ii) {
                Records::const_iterator const fr2 = this->records_data.begin() + static_cast<ptrdiff_t>
                                                    (this->records_lookup[i->first.first]);
                unsigned short const jn = fr2->name_count;
                unsigned short ji = 0;
                for (LinkInfos::value_type const *j = this->nameinfo(fr2); j;
                        j = ~j->second ? &this->nameinfos[j->second] : NULL, ++ji) {
                    if (j->first.parent == key_first.first
                            && i->first.second == jn - static_cast<size_t>(1) - ji) {
                        size_t const old_size = path.size();
                        path += _T('\\'), append(path, &this->names[j->first.name.offset], j->first.name.length);
                        key_type::first_type const subkey_first(static_cast<unsigned int>(i->first.first), ji);
                        if (subkey_first != key_first) {
                            this->matches<F>(func, path, depth + 1, subkey_first);
                        }
                        path.resize(old_size);
                    }
                }
            }
            unsigned short ki = 0;
            for (StreamInfos::value_type const *k = this->streaminfo(fr); k;
                    k = ~k->second ? &this->streaminfos[k->second] : NULL, ++ki) {
                if (k->first.name.offset > this->names.size()) {
                    throw std::logic_error("invalid entry");
                }
                size_t const old_size = path.size();
                append(path, stream_prefix, stream_prefix_size);
                if (fr->stdinfo.attributes & FILE_ATTRIBUTE_DIRECTORY && key_first.first != 0x00000005) {
                    path += _T('\\');
                }
                if (k->first.name.length) {
                    path += _T(':');
                    append(path, k->first.name.length ? &this->names[k->first.name.offset] : NULL,
                           k->first.name.length);
                }
                bool const is_alternate_stream = k->first.type_name_id
                                                 && (k->first.type_name_id << (CHAR_BIT / 2)) != ntfs::AttributeData;
                if (is_alternate_stream) {
                    if (!k->first.name.length) {
                        path += _T(':');
                    }
                    path += _T(":"), append(path, ntfs::attribute_names[k->first.type_name_id]);
                }
                func(
                    std::pair<std::tstring::const_iterator, std::tstring::const_iterator>(path.begin(),
                            path.end()),
                    key_type(key_first, key_type::second_type(ki,
                             k == this->streaminfo(fr) ? ~static_cast<key_type::second_type::second_type>
                             (islot) : static_cast<key_type::second_type::second_type>(k -
                                     &*this->streaminfos.begin()))),
                    depth);
                path.erase(old_size);
            }
        }
    }
};

namespace std {
#ifdef _XMEMORY_
#define X(...) template<> struct is_scalar<__VA_ARGS__> : is_pod<__VA_ARGS__>{}
	X(NtfsIndex::StandardInfo);
	X(NtfsIndex::NameInfo);
	X(NtfsIndex::StreamInfo);
	X(NtfsIndex::LinkInfo);
	X(NtfsIndex::Record);
#undef X
#endif
}


