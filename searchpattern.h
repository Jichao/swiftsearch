#pragma once

class CSearchPattern : public ATL::CWindowImpl<CSearchPattern, WTL::CEdit> {
	BEGIN_MSG_MAP_EX(CCustomDialogCode)
		MSG_WM_MOUSEMOVE(OnMouseMove)
		MSG_WM_MOUSELEAVE(OnMouseLeave)
		MSG_WM_MOUSEHOVER(OnMouseHover)
		MESSAGE_HANDLER_EX(EM_REPLACESEL, OnReplaceSel)
		MESSAGE_RANGE_HANDLER_EX(WM_KEYDOWN, WM_KEYUP, OnKey)
		END_MSG_MAP()
		bool tracking;
public:
	CSearchPattern() : tracking() { }
	struct KeyNotify {
		NMHDR hdr;
		WPARAM vkey;
		LPARAM lParam;
	};
	enum { CUN_KEYDOWN, CUN_KEYUP };

	LRESULT OnReplaceSel(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
	{
		int start = 0, end = 0;
		this->GetSel(start, end);
		TCHAR const *const sz = reinterpret_cast<TCHAR const *>(lParam);
		if ((!sz || !*sz) && start == 0 && end == this->GetWindowTextLength()) {
			this->PostMessage(EM_SETSEL, start, end);
		}
		else {
			this->SetMsgHandled(FALSE);
		}
		return 0;
	}

	LRESULT OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if ((wParam == VK_UP || wParam == VK_DOWN) || (wParam == VK_PRIOR || wParam == VK_NEXT)) {
			int id = this->GetWindowLong(GWL_ID);
			KeyNotify msg = { { *this, id, uMsg == WM_KEYUP ? CUN_KEYUP : CUN_KEYDOWN }, wParam, lParam };
			HWND hWndParent = this->GetParent();
			return hWndParent == NULL
				|| this->SendMessage(hWndParent, WM_NOTIFY, id, (LPARAM)&msg) ? this->DefWindowProc(uMsg,
				wParam, lParam) : 0;
		}
		else {
			return this->DefWindowProc(uMsg, wParam, lParam);
		}
	}

	void EnsureTrackingMouseHover()
	{
		if (!this->tracking) {
			TRACKMOUSEEVENT tme = { sizeof(tme), TME_HOVER | TME_LEAVE, this->m_hWnd, 0 };
			this->tracking = !!TrackMouseEvent(&tme);
		}
	}

	void OnMouseMove(UINT /*nFlags*/, WTL::CPoint /*point*/)
	{
		this->SetMsgHandled(FALSE);
		this->EnsureTrackingMouseHover();
	}

	void OnMouseLeave()
	{
		this->SetMsgHandled(FALSE);
		this->tracking = false;
		this->HideBalloonTip();
	}

	void OnMouseHover(WPARAM /*wParam*/, WTL::CPoint /*ptPos*/)
	{
		this->SetMsgHandled(FALSE);
		EDITBALLOONTIP tip = { sizeof(tip), _T("Search Pattern"), _T("Entern pattern to match against the file's name or path, such as:\r\nC:\\Windows\\*.exe\r\nPicture*.jpg"), TTI_INFO };
		this->ShowBalloonTip(&tip);
	}
};

