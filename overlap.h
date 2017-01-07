#pragma once

class OverlappedNtfsMftReadPayload : public Overlapped {
    typedef std::vector<std::pair<std::pair<unsigned long long, unsigned long long>, long long> >
    RetPtrs;
    std::tstring path_name;
    Handle iocp;
    HWND m_hWnd;
    int WM_DRIVESETITEMDATA;
    Handle closing_event;
    RetPtrs ret_ptrs;
    unsigned int cluster_size;
    boost::atomic<RetPtrs::size_type> j;
    boost::atomic<unsigned int> records_so_far;
    boost::intrusive_ptr<NtfsIndex volatile> p;
public:
    class Operation;
    ~OverlappedNtfsMftReadPayload()
    {
    }
    OverlappedNtfsMftReadPayload(Handle const &iocp, std::tstring const &path_name,
                                 HWND const m_hWnd, int const WM_DRIVESETITEMDATA, Handle const &closing_event)
        : Overlapped(), path_name(path_name), iocp(iocp), m_hWnd(m_hWnd),
          WM_DRIVESETITEMDATA(WM_DRIVESETITEMDATA), closing_event(closing_event), records_so_far(0),
          ret_ptrs(), cluster_size(), j(0)
    { }
    bool queue_next() volatile;
    int operator()(size_t const /*size*/, uintptr_t const /*key*/);
};

