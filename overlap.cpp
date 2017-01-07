#include "stdafx.h"
#include "utils.h"
#include "resource.h"
#include "nformat.hpp"
#include "path.hpp"
#include "BackgroundWorker.hpp"
#include "QueryFileLayout.h"
#include "ShellItemIDList.hpp"
#include "CModifiedDialogImpl.hpp"
#include "ntfsindex.h"
#include "ProgressDialog.h"
#include "overlap.h"

class OverlappedNtfsMftReadPayload::Operation : public Overlapped {
    boost::intrusive_ptr<OverlappedNtfsMftReadPayload volatile> q;
    unsigned long long _voffset;
    bool _is_file_layout;
    static void *operator new(size_t n)
    {
        return ::operator new(n);
    }
public:
    static void *operator new(size_t n, size_t m)
    {
        return operator new(n + m);
    }
    static void operator delete(void *p)
    {
        return ::operator delete(p);
    }
    static void operator delete(void *p, size_t /*m*/)
    {
        return operator delete(p);
    }
    explicit Operation(boost::intrusive_ptr<OverlappedNtfsMftReadPayload volatile> const &q) :
        Overlapped(), q(q), _voffset(), _is_file_layout() { }
    unsigned long long voffset()
    {
        return this->_voffset;
    }
    void voffset(unsigned long long const value)
    {
        this->_voffset = value;
    }
    bool is_file_layout() const
    {
        return this->_is_file_layout;
    }
    void is_file_layout(bool const value)
    {
        this->_is_file_layout = value;
    }
    int operator()(size_t const size, uintptr_t const /*key*/)
    {
        OverlappedNtfsMftReadPayload const *const q =
            const_cast<OverlappedNtfsMftReadPayload const *>(this->q.get());
        if (!q->p->cancelled()) {
            this->q->queue_next();
            q->p->load(this->voffset(), this + 1, size, this->is_file_layout());
        }
        return -1;
    }
};
bool OverlappedNtfsMftReadPayload::queue_next() volatile
{
    OverlappedNtfsMftReadPayload const *const me =
        const_cast<OverlappedNtfsMftReadPayload const *>(this);
    bool any = false;
    size_t const j = this->j.fetch_add(1, boost::memory_order_acq_rel);
    if (j < me->ret_ptrs.size()) {
        unsigned int const cb = static_cast<unsigned int>(me->ret_ptrs[j].first.second *
                                me->cluster_size);
        boost::intrusive_ptr<Operation> p(new(cb) Operation(this));
        p->offset(me->ret_ptrs[j].second * static_cast<long long>(me->cluster_size));
        p->voffset(me->ret_ptrs[j].first.first * me->cluster_size);
        QUERY_FILE_LAYOUT_INPUT input = { 1, QUERY_FILE_LAYOUT_INCLUDE_EXTENTS | QUERY_FILE_LAYOUT_INCLUDE_EXTRA_INFO | QUERY_FILE_LAYOUT_INCLUDE_NAMES | QUERY_FILE_LAYOUT_INCLUDE_STREAMS | QUERY_FILE_LAYOUT_RESTART | QUERY_FILE_LAYOUT_INCLUDE_STREAMS_WITH_NO_CLUSTERS_ALLOCATED, QUERY_FILE_LAYOUT_FILTER_TYPE_FILEID };
        input.Filter.FileReferenceRanges->StartingFileReferenceNumber =
            (me->ret_ptrs[j].first.first * me->cluster_size) / me->p->mft_record_size;
        input.Filter.FileReferenceRanges->EndingFileReferenceNumber = (me->ret_ptrs[j].first.first
                * me->cluster_size + cb) / me->p->mft_record_size;
        p->is_file_layout(false);
        if (p->is_file_layout()
                && DeviceIoControl(me->p->volume(), FSCTL_QUERY_FILE_LAYOUT, &input, sizeof(input),
                                   p.get() + 1, cb, NULL, p.get())) {
            any = true, p.detach();
        } else if (p->is_file_layout() && GetLastError() == ERROR_HANDLE_EOF) {
            if (PostQueuedCompletionStatus(me->iocp, 0, 0, p.get())) {
                p.detach();
            }
        } else {
            p->is_file_layout(false);
            if (ReadFile(me->p->volume(), p.get() + 1, cb, NULL, p.get())) {
                if (PostQueuedCompletionStatus(me->iocp, cb, 0, p.get())) {
                    any = true, p.detach();
                } else {
                    CheckAndThrow(false);
                }
            } else if (GetLastError() == ERROR_IO_PENDING) {
                any = true, p.detach();
            } else {
                CheckAndThrow(false);
            }
        }
    }
    return any;
}
int OverlappedNtfsMftReadPayload::operator()(size_t const /*size*/, uintptr_t const key)
{
    int result = -1;
    boost::intrusive_ptr<NtfsIndex> p(new NtfsIndex(this->path_name));
    this->p = p;
    if (void *const volume = p->volume()) {
        if (::PostMessage(this->m_hWnd, WM_DRIVESETITEMDATA, static_cast<WPARAM>(key),
                          reinterpret_cast<LPARAM>(&*p))) {
            intrusive_ptr_add_ref(p.get());
        }
        DEV_BROADCAST_HANDLE dev = { sizeof(dev), DBT_DEVTYP_HANDLE, 0, volume, reinterpret_cast<HDEVNOTIFY>(this->m_hWnd) };
        dev.dbch_hdevnotify = RegisterDeviceNotification(this->m_hWnd, &dev,
                              DEVICE_NOTIFY_WINDOW_HANDLE);
        unsigned long br;
        NTFS_VOLUME_DATA_BUFFER info;
        CheckAndThrow(DeviceIoControl(volume, FSCTL_GET_NTFS_VOLUME_DATA, NULL, 0, &info,
                                      sizeof(info), &br, NULL));
        p->mft_record_size = info.BytesPerFileRecordSegment;
        p->total_records = static_cast<unsigned int>(info.MftValidDataLength.QuadPart /
                           p->mft_record_size);
        CheckAndThrow(!!CreateIoCompletionPort(volume, this->iocp,
                                               reinterpret_cast<uintptr_t>(&*p), 0));
        long long llsize = 0;
        typedef std::vector<std::pair<unsigned long long, long long> > RP;
        RP const ret_ptrs = get_mft_retrieval_pointers(volume, _T("$MFT::$DATA"), &llsize,
                            info.MftStartLcn.QuadPart, p->mft_record_size);
        this->cluster_size = static_cast<unsigned int>(info.BytesPerCluster);
        unsigned long long prev_vcn = 0;
        for (RP::const_iterator i = ret_ptrs.begin(); i != ret_ptrs.end(); ++i) {
            long long const clusters_left = static_cast<long long>(std::max(i->first,
                                            prev_vcn) - prev_vcn);
            unsigned long long n;
            for (long long m = 0; m < clusters_left; m += static_cast<long long>(n)) {
                n = std::min(i->first - prev_vcn, 1 + ((2ULL << 20) - 1) / this->cluster_size);
                this->ret_ptrs.push_back(RetPtrs::value_type(RetPtrs::value_type::first_type(prev_vcn, n),
                                         i->second + m));
                prev_vcn += n;
            }
        }
        p->reserve(p->total_records);
        bool any = false;
        for (int concurrency = 0; concurrency < 2; ++concurrency) {
            any |= this->queue_next();
        }
        if (any) {
            result = 0;
        }
    } else {
        ::PostMessage(this->m_hWnd, WM_DRIVESETITEMDATA, static_cast<WPARAM>(key), NULL);
    }
    return result;
}

