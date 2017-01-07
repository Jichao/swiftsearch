#pragma once

class CProgressDialog : private CModifiedDialogImpl<CProgressDialog>,
    private WTL::CDialogResize<CProgressDialog> {
    typedef CProgressDialog This;
    typedef CModifiedDialogImpl<CProgressDialog> Base;
    friend CDialogResize<This>;
    friend CDialogImpl<This>;
    friend CModifiedDialogImpl<This>;
    enum { IDD = IDD_DIALOGPROGRESS, BACKGROUND_COLOR = COLOR_WINDOW };
    class CUnselectableWindow : public ATL::CWindowImpl<CUnselectableWindow> {
        BEGIN_MSG_MAP(CUnselectableWindow)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        END_MSG_MAP()
        LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
        {
            LRESULT result = this->DefWindowProc(uMsg, wParam, lParam);
            return result == HTCLIENT ? HTTRANSPARENT : result;
        }
    };

    CUnselectableWindow progressText;
    WTL::CProgressBarCtrl progressBar;
    bool canceled;
    bool invalidated;
    DWORD creationTime;
    DWORD lastUpdateTime;
    HWND parent;
    std::basic_string<TCHAR> lastProgressText, lastProgressTitle;
    bool windowCreated;
    int lastProgress, lastProgressTotal;

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
    {
        (this->progressText.SubclassWindow)(this->GetDlgItem(IDC_RICHEDITPROGRESS));
        // SetClassLongPtr(this->m_hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
        this->progressBar.Attach(this->GetDlgItem(IDC_PROGRESS1));
        this->DlgResize_Init(false, false, 0);
        ATL::CComBSTR bstr;
        this->progressText.GetWindowText(&bstr);
        this->lastProgressText = bstr;

        return TRUE;
    }

    void OnPause(UINT uNotifyCode, int nID, CWindow wndCtl)
    {
        UNREFERENCED_PARAMETER((uNotifyCode, nID, wndCtl));
        __debugbreak();
    }

    void OnCancel(UINT uNotifyCode, int nID, CWindow wndCtl)
    {
        UNREFERENCED_PARAMETER((uNotifyCode, nID, wndCtl));
        PostQuitMessage(ERROR_CANCELLED);
    }

    BOOL OnEraseBkgnd(WTL::CDCHandle dc)
    {
        RECT rc = {};
        this->GetClientRect(&rc);
        dc.FillRect(&rc, BACKGROUND_COLOR);
        return TRUE;
    }

    HBRUSH OnCtlColorStatic(WTL::CDCHandle dc, WTL::CStatic /*wndStatic*/)
    {
        return GetSysColorBrush(BACKGROUND_COLOR);
    }

    BEGIN_MSG_MAP_EX(This)
    CHAIN_MSG_MAP(CDialogResize<This>)
    MSG_WM_INITDIALOG(OnInitDialog)
    // MSG_WM_ERASEBKGND(OnEraseBkgnd)
    // MSG_WM_CTLCOLORSTATIC(OnCtlColorStatic)
    COMMAND_HANDLER_EX(IDRETRY, BN_CLICKED, OnPause)
    COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnCancel)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(This)
    DLGRESIZE_CONTROL(IDC_PROGRESS1, DLSZ_MOVE_Y | DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDC_RICHEDITPROGRESS, DLSZ_SIZE_Y | DLSZ_SIZE_X)
    DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    static BOOL EnableWindowRecursive(HWND hWnd, BOOL enable, BOOL includeSelf = true)
    {
        struct Callback {
            static BOOL CALLBACK EnableWindowRecursiveEnumProc(HWND hWnd, LPARAM lParam)
            {
                EnableWindowRecursive(hWnd, static_cast<BOOL>(lParam), TRUE);
                return TRUE;
            }
        };
        if (enable) {
            EnumChildWindows(hWnd, &Callback::EnableWindowRecursiveEnumProc, enable);
            return includeSelf && ::EnableWindow(hWnd, enable);
        } else {
            BOOL result = includeSelf && ::EnableWindow(hWnd, enable);
            EnumChildWindows(hWnd, &Callback::EnableWindowRecursiveEnumProc, enable);
            return result;
        }
    }

    unsigned long WaitMessageLoop(uintptr_t const handles[], size_t const nhandles)
    {
        for (;;) {
            unsigned long result = MsgWaitForMultipleObjectsEx(static_cast<unsigned int>(nhandles),
                                   reinterpret_cast<HANDLE const *>(handles), UPDATE_INTERVAL, QS_ALLINPUT,
                                   MWMO_INPUTAVAILABLE);
            if (WAIT_OBJECT_0 <= result && result < WAIT_OBJECT_0 + nhandles
                    || result == WAIT_TIMEOUT) {
                return result;
            } else if (result == WAIT_OBJECT_0 + static_cast<unsigned int>(nhandles)) {
                this->ProcessMessages();
            } else {
                RaiseException(GetLastError(), 0, 0, NULL);
            }
        }
    }

    DWORD GetMinDelay() const
    {
        return IsDebuggerPresent() ? 0 : 500;
    }

public:
    enum { UPDATE_INTERVAL = 25 };
    CProgressDialog(ATL::CWindow parent)
        : Base(true), parent(parent), lastUpdateTime(0), creationTime(GetTickCount()),
          lastProgress(0), lastProgressTotal(1), invalidated(false), canceled(false),
          windowCreated(false)
    {
    }

    ~CProgressDialog()
    {
        if (this->windowCreated) {
            ::EnableWindow(parent, TRUE);
            this->DestroyWindow();
        }
    }

    unsigned long Elapsed() const
    {
        return GetTickCount() - this->lastUpdateTime;
    }

    bool ShouldUpdate() const
    {
        return this->Elapsed() >= UPDATE_INTERVAL;
    }

    void ProcessMessages()
    {
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (!this->windowCreated || !this->IsDialogMessage(&msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            if (msg.message == WM_QUIT) {
                this->canceled = true;
            }
        }
    }

    void ForceShow()
    {
        if (!this->windowCreated) {
            this->windowCreated = !!this->Create(parent);
            ::EnableWindow(parent, FALSE);
            this->Flush();
        }
    }

    bool HasUserCancelled()
    {
        bool justCreated = false;
        unsigned long const now = GetTickCount();
        if (abs(static_cast<int>(now - this->creationTime)) >= static_cast<int>
                (this->GetMinDelay())) {
            this->ForceShow();
        }
        if (this->windowCreated && (justCreated || this->ShouldUpdate())) {
            this->ProcessMessages();
        }
        return this->canceled;
    }

    void Flush()
    {
        if (this->invalidated) {
            if (this->windowCreated) {
                this->invalidated = false;
                this->SetWindowText(this->lastProgressTitle.c_str());
                this->progressBar.SetRange32(0, this->lastProgressTotal);
                this->progressBar.SetPos(this->lastProgress);
                this->progressText.SetWindowText(this->lastProgressText.c_str());
                this->progressBar.UpdateWindow();
                this->progressText.UpdateWindow();
                this->UpdateWindow();
            }
            this->lastUpdateTime = GetTickCount();
        }
    }

    void SetProgress(long long current, long long total)
    {
        if (total > INT_MAX) {
            current = static_cast<long long>((static_cast<double>(current) / total) * INT_MAX);
            total = INT_MAX;
        }
        this->invalidated |= this->lastProgress != current || this->lastProgressTotal != total;
        this->lastProgressTotal = static_cast<int>(total);
        this->lastProgress = static_cast<int>(current);
    }

    void SetProgressTitle(LPCTSTR title)
    {
        this->invalidated |= this->lastProgressTitle != title;
        this->lastProgressTitle = title;
    }

    void SetProgressText(boost::iterator_range<TCHAR const *> const text)
    {
        this->invalidated |= !boost::equal(this->lastProgressText, text);
        this->lastProgressText.assign(text.begin(), text.end());
    }
};

