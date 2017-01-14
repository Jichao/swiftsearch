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
#include "maindlg.h"

unsigned int const CMainDlg::WM_TASKBARCREATED = RegisterWindowMessage(
	_T("TaskbarCreated"));

unsigned int CALLBACK CMainDlg::iocp_worker(void *iocp)
{
	ULONG_PTR key;
	OVERLAPPED *overlapped_ptr;
	Overlapped *p;
	for (unsigned long nr;
		GetQueuedCompletionStatus(iocp, &nr, &key, &overlapped_ptr, INFINITE);) {
		p = static_cast<Overlapped *>(overlapped_ptr);
		boost::intrusive_ptr<Overlapped> overlapped(p, false);
		if (overlapped.get()) {
			int r = (*overlapped)(static_cast<size_t>(nr), key);
			if (r > 0) {
				r = PostQueuedCompletionStatus(iocp, nr, key, overlapped_ptr) ? 0 : -1;
			}
			if (r >= 0) {
				overlapped.detach();
			}
		}
		else if (!key) {
			break;
		}
	}
	return 0;
}

BOOL CMainDlg::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
	_Module.GetMessageLoop()->AddMessageFilter(this);
	setupUI();

	for (size_t i = 0; i != this->num_threads; ++i) {
		unsigned int id;
		this->threads.push_back(_beginthreadex(NULL, 0, iocp_worker, this->iocp, 0, &id));
	}
	RegisterWaitForSingleObject(&this->hWait, hEvent, &WaitCallback, this->m_hWnd, INFINITE,
		WT_EXECUTEINUITHREAD);
	return TRUE;
}

LRESULT CMainDlg::OnFilesListColumnClick(LPNMHDR pnmh)
{
	WTL::CWaitCursor wait;
	LPNM_LISTVIEW pLV = (LPNM_LISTVIEW)pnmh;
	WTL::CHeaderCtrl header = this->lvFiles.GetHeader();
	HDITEM hditem = { HDI_LPARAM };
	header.GetItem(pLV->iSubItem, &hditem);
	bool const ctrl_pressed = GetKeyState(VK_CONTROL) < 0;
	bool const shift_pressed = GetKeyState(VK_SHIFT) < 0;
	bool const alt_pressed = GetKeyState(VK_MENU) < 0;
	bool cancelled = false;
	if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0) {
		try {
			std::vector<Results::value_type::first_type::element_type *> indices;
			for (Results::iterator i = this->results.begin(); i != this->results.end(); ++i) {
				if (std::find(indices.begin(), indices.end(), i->index.get()) == indices.end()) {
					indices.push_back(i->index.get());
				}
			}
			std::vector<lock_guard<mutex> > indices_locks(indices.size());
			for (size_t i = 0; i != indices.size(); ++i) {
				lock_guard<mutex>(indices[i]->get_mutex()).swap(indices_locks[i]);
			}
			std::vector<std::pair<std::tstring, std::tstring> > vnames(
#ifdef _OPENMP
				omp_get_max_threads()
#else
				1
#endif
				);
			int const subitem = pLV->iSubItem;
			bool const reversed = !!hditem.lParam;
			inplace_mergesort(this->results.begin(), this->results.end(), [this, subitem, &vnames,
				reversed, shift_pressed, alt_pressed, ctrl_pressed](Results::value_type const &_a,
				Results::value_type const &_b) {
				Results::value_type const &a = reversed ? _b : _a, &b = reversed ? _a : _b;
				std::pair<std::tstring, std::tstring> &names = *(vnames.begin()
#ifdef _OPENMP
					+ omp_get_thread_num()
#endif
					);
				if (GetAsyncKeyState(VK_ESCAPE) < 0) {
					throw CStructured_Exception(ERROR_CANCELLED, NULL);
				}
				boost::remove_cv<Results::value_type::first_type::element_type>::type const
					*index1 = a.index->unvolatile(),
					*index2 = b.index->unvolatile();
				NtfsIndex::size_info a_size_info, b_size_info;
				bool less = false;
				bool further_test = true;
				if (shift_pressed) {
					size_t const
						a_depth = ctrl_pressed ? a.depth : a.depth / 2,
						b_depth = ctrl_pressed ? b.depth : b.depth / 2;
					if (a_depth < b_depth) {
						less = true;
						further_test = false;
					}
					else if (b_depth < a_depth) {
						less = false;
						further_test = false;
					}
				}
				if (!less && further_test) {
					switch (subitem) {
					case COLUMN_INDEX_NAME:
						names.first = index1->root_path();
						index1->get_path(a.key, names.first, true);
						names.second = index2->root_path();
						index2->get_path(b.key, names.second, true);
						less = names.first < names.second;
						break;
					case COLUMN_INDEX_PATH:
						names.first = index1->root_path();
						index1->get_path(a.key, names.first, false);
						names.second = index2->root_path();
						index2->get_path(b.key, names.second, false);
						less = names.first < names.second;
						break;
					case COLUMN_INDEX_SIZE:
						less = index1->get_sizes(a.key).length > index2->get_sizes(b.key).length;
						break;
					case COLUMN_INDEX_SIZE_ON_DISK:
						a_size_info = index1->get_sizes(a.key), b_size_info = index2->get_sizes(b.key);
						less = (alt_pressed ? a_size_info.bulkiness : a_size_info.allocated) >
							(alt_pressed ? b_size_info.bulkiness : b_size_info.allocated);
						break;
					case COLUMN_INDEX_CREATION_TIME:
						less = index1->get_stdinfo(a.key.first.first).created > index2->get_stdinfo(
							b.key.first.first).created;
						break;
					case COLUMN_INDEX_MODIFICATION_TIME:
						less = index1->get_stdinfo(a.key.first.first).written > index2->get_stdinfo(
							b.key.first.first).written;
						break;
					case COLUMN_INDEX_ACCESS_TIME:
						less = index1->get_stdinfo(a.key.first.first).accessed > index2->get_stdinfo(
							b.key.first.first).accessed;
						break;
					default:
						less = false;
						break;
					}
				}
				return less;
			}, false /* parallelism BREAKS exception handling, and therefore user-cancellation */);
		}
		catch (CStructured_Exception &ex) {
			cancelled = true;
			if (ex.GetSENumber() != ERROR_CANCELLED) {
				throw;
			}
		}
		this->lvFiles.SetItemCount(this->lvFiles.GetItemCount());
	}
	if (!cancelled) {
		hditem.lParam = !hditem.lParam;
		header.SetItem(pLV->iSubItem, &hditem);
	}
	return TRUE;
}

void CMainDlg::Search()
{
	WTL::CWaitCursor wait;
	bool const ctrl_pressed = GetKeyState(VK_CONTROL) < 0;
	bool const shift_pressed = GetKeyState(VK_SHIFT) < 0;
	int const selected = this->cmbDrive.GetCurSel();
	if (selected != 0 && !this->cmbDrive.GetItemDataPtr(selected)) {
		this->MessageBox(_T("This does not appear to be a valid NTFS volume."), _T("Error"),
			MB_OK | MB_ICONERROR);
		return;
	}
	CProgressDialog dlg(*this);
	dlg.SetProgressTitle(_T("Searching..."));
	if (dlg.HasUserCancelled()) {
		return;
	}
	this->lastRequestedIcon.resize(0);
	this->lvFiles.SetItemCount(0);
	this->results.clear();
	std::tstring pattern;
	{
		ATL::CComBSTR bstr;
		if (this->txtPattern.GetWindowText(bstr.m_str)) {
			pattern.assign(bstr, bstr.Length());
		}
	}
	bool const is_regex = !pattern.empty() && *pattern.begin() == _T('>');
	if (is_regex) {
		pattern.erase(pattern.begin());
	}
	bool const is_path_pattern = is_regex || ~pattern.find(_T('\\'));
	bool const requires_root_path_match = is_path_pattern && !pattern.empty() && (is_regex
		? *pattern.begin() != _T('.') && *pattern.begin() != _T('(')
		&& *pattern.begin() != _T('[') && *pattern.begin() != _T('.')
		: *pattern.begin() != _T('*') && *pattern.begin() != _T('?'));
	typedef std::tstring::const_iterator It;
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
	typedef boost::xpressive::basic_regex<It> RE;
	boost::xpressive::match_results<RE::iterator_type> mr;
	RE re;
	if (is_regex) {
		try {
			re = RE::compile(pattern.begin(), pattern.end(),
				boost::xpressive::regex_constants::nosubs | boost::xpressive::regex_constants::optimize |
				boost::xpressive::regex_constants::single_line | boost::xpressive::regex_constants::icase
				| boost::xpressive::regex_constants::collate);
		}
		catch (boost::xpressive::regex_error const &ex) {
			this->MessageBox(static_cast<WTL::CString>(ex.what()), _T("Regex Error"), MB_ICONERROR);
			return;
		}
	}
#else
	if (is_regex) {
		this->MessageBox(_T("Regex support not included."), _T("Regex Error"), MB_ICONERROR);
		return;
	}
#endif
	if (!is_path_pattern && !~pattern.find(_T('*')) && !~pattern.find(_T('?'))) {
		pattern.insert(pattern.begin(), _T('*'));
		pattern.insert(pattern.end(), _T('*'));
	}
	clock_t const start = clock();
	std::vector<uintptr_t> wait_handles;
	std::vector<Results::value_type::first_type> wait_indices;
	// TODO: What if they exceed maximum wait objects?
	bool any_io_pending = false;
	size_t overall_progress_numerator = 0, overall_progress_denominator = 0;
	for (int ii = 0; ii < this->cmbDrive.GetCount(); ++ii) {
		boost::intrusive_ptr<NtfsIndex> const p = static_cast<NtfsIndex *>
			(this->cmbDrive.GetItemDataPtr(ii));
		if (p && (selected == ii || selected == 0)) {
			std::tstring const root_path = p->root_path();
			if (!requires_root_path_match || pattern.size() >= root_path.size()
				&& std::equal(root_path.begin(), root_path.end(), pattern.begin())) {
				wait_handles.push_back(p->finished_event());
				wait_indices.push_back(p);
				size_t const records_so_far = p->records_so_far();
				any_io_pending |= records_so_far < p->total_records;
				overall_progress_denominator += p->total_records * 2;
			}
		}
	}
	if (!any_io_pending) {
		overall_progress_denominator /= 2;
	}
	if (any_io_pending) {
		dlg.ForceShow();
	}
	RaiseIoPriority set_priority;
	while (!dlg.HasUserCancelled() && !wait_handles.empty()) {
		if (uintptr_t const volume = reinterpret_cast<uintptr_t>(wait_indices.at(0)->volume())) {
			if (set_priority.volume() != volume) {
				RaiseIoPriority(volume).swap(set_priority);
			}
		}
		unsigned long const wait_result = dlg.WaitMessageLoop(wait_handles.empty() ? NULL :
			&*wait_handles.begin(), wait_handles.size());
		if (wait_result == WAIT_TIMEOUT) {
			if (dlg.ShouldUpdate()) {
				std::basic_ostringstream<TCHAR> ss;
				ss << _T("Reading file tables...");
				bool any = false;
				size_t temp_overall_progress_numerator = overall_progress_numerator;
				for (size_t i = 0; i != wait_indices.size(); ++i) {
					Results::value_type::first_type const j = wait_indices[i];
					size_t const records_so_far = j->records_so_far();
					temp_overall_progress_numerator += records_so_far;
					if (records_so_far != j->total_records) {
						if (any) {
							ss << _T(", ");
						}
						else {
							ss << _T(" ");
						}
						ss << j->root_path() << _T(" ") << _T("(") << nformat(records_so_far, this->loc,
							true) << _T(" of ") << nformat(j->total_records, this->loc, true) << _T(")");
						any = true;
					}
				}
				std::tstring const text = ss.str();
				dlg.SetProgressText(boost::iterator_range<TCHAR const *>(text.data(),
					text.data() + text.size()));
				dlg.SetProgress(static_cast<int64_t>(temp_overall_progress_numerator),
					static_cast<int64_t>(overall_progress_denominator));
				dlg.Flush();
			}
		}
		else {
			if (wait_result < wait_handles.size()) {
				std::vector<Results> results_at_depths;
				results_at_depths.reserve(std::numeric_limits<unsigned short>::max() + 1);
				Results::value_type::first_type const i = wait_indices[wait_result];
				size_t const old_size = this->results.size();
				size_t current_progress_numerator = 0;
				size_t const current_progress_denominator = i->total_items();
				std::tstring const root_path = i->root_path();
				std::tstring current_path = root_path;
				while (!current_path.empty() && *(current_path.end() - 1) == _T('\\')) {
					current_path.erase(current_path.end() - 1);
				}
				try {
					i->matches([&dlg, is_path_pattern, &results_at_depths, &root_path, &pattern, is_regex,
						shift_pressed, ctrl_pressed, this, i, &wait_indices, any_io_pending,
						&current_progress_numerator, current_progress_denominator,
						overall_progress_numerator, overall_progress_denominator
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
						, &re
						, &mr
#endif
					](std::pair<It, It> const path, NtfsIndex::key_type const key, size_t const depth) {
						if (dlg.ShouldUpdate()
							|| current_progress_denominator - current_progress_numerator <= 1) {
							if (dlg.HasUserCancelled()) {
								throw CStructured_Exception(ERROR_CANCELLED, NULL);
							}
							// this->lvFiles.SetItemCountEx(static_cast<int>(this->results.size()), 0), this->lvFiles.UpdateWindow();
							size_t temp_overall_progress_numerator = overall_progress_numerator;
							if (any_io_pending) {
								for (size_t k = 0; k != wait_indices.size(); ++k) {
									Results::value_type::first_type const j = wait_indices[k];
									size_t const records_so_far = j->records_so_far();
									temp_overall_progress_numerator += records_so_far;
								}
							}
							std::tstring text(0x100 + root_path.size() + static_cast<ptrdiff_t>
								(path.second - path.first), _T('\0'));
							text.resize(static_cast<size_t>(_stprintf(&*text.begin(),
								_T("Searching %.*s (%s of %s)...\r\n%.*s"),
								static_cast<int>(root_path.size()), root_path.c_str(),
								nformat(current_progress_numerator, this->loc, true).c_str(),
								nformat(current_progress_denominator, this->loc, true).c_str(),
								static_cast<int>(path.second - path.first),
								path.first == path.second ? NULL : &*path.first)));
							dlg.SetProgressText(boost::iterator_range<TCHAR const *>(text.data(),
								text.data() + text.size()));
							dlg.SetProgress(temp_overall_progress_numerator + static_cast<uint64_t>
								(i->total_records) * static_cast<uint64_t>(current_progress_numerator) /
								static_cast<uint64_t>(current_progress_denominator),
								static_cast<int64_t>(overall_progress_denominator));
							dlg.Flush();
						}
						++current_progress_numerator;
						if (current_progress_numerator > current_progress_denominator) {
							throw std::logic_error("current_progress_numerator > current_progress_denominator");
						}
						std::pair<It, It> needle = path;
						if (!is_path_pattern) {
							while (needle.first != needle.second && *(needle.second - 1) == _T('\\')) {
								--needle.second;
							}
							needle.first = basename(needle.first, needle.second);
						}
						bool const match =
#ifdef BOOST_XPRESSIVE_DYNAMIC_HPP_EAN
							is_regex ? boost::xpressive::regex_match(needle.first, needle.second, mr, re) :
#endif
							wildcard(pattern.begin(), pattern.end(), needle.first, needle.second, tchar_ci_traits())
							;
						if (match) {
							size_t depth2 = depth *
								4 /* dividing by 2 later should not mess up the actual depths; it should only affect files vs. directory sub-depths */;
							if (ctrl_pressed
								&& !(i->get_stdinfo(key.first.first).attributes & FILE_ATTRIBUTE_DIRECTORY)) {
								++depth2;
							}
							if (!shift_pressed) {
								if (depth2 >= results_at_depths.size()) {
									results_at_depths.resize(depth2 + 1);
								}
								results_at_depths[depth2].push_back(Results::value_type(i, key, depth2));
							}
							else {
								this->results.push_back(Results::value_type(i, key, depth2));
							}
						}
					}, current_path);
				}
				catch (CStructured_Exception &ex) {
					if (ex.GetSENumber() != ERROR_CANCELLED) {
						throw;
					}
				}
				if (any_io_pending) {
					overall_progress_numerator += i->total_records;
				}
				if (current_progress_denominator) {
					overall_progress_numerator += static_cast<size_t>(static_cast<uint64_t>
						(i->total_records) * static_cast<uint64_t>(current_progress_numerator) /
						static_cast<uint64_t>(current_progress_denominator));
				}
				while (!results_at_depths.empty()) {
					Results &r = results_at_depths.back();
					this->results.insert(this->results.end(), r.begin(), r.end());
					results_at_depths.pop_back();
				}
				std::reverse(this->results.begin() + static_cast<ptrdiff_t>(old_size),
					this->results.end());
				this->lvFiles.SetItemCountEx(static_cast<int>(this->results.size()), 0);
			}
			wait_indices.erase(wait_indices.begin() + static_cast<ptrdiff_t>(wait_result));
			wait_handles.erase(wait_handles.begin() + static_cast<ptrdiff_t>(wait_result));
		}
	}
	clock_t const end = clock();
	TCHAR buf[0x100];
	_stprintf(buf, _T("%s results in %.2lf seconds"), nformat(this->results.size(), this->loc,
		true).c_str(), (end - start) * 1.0 / CLOCKS_PER_SEC);
	this->statusbar.SetText(0, buf);
}

void CMainDlg::OnBrowse(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
{
	TCHAR path[MAX_PATH];
	BROWSEINFO info = { this->m_hWnd, NULL, path, _T("If you would like to filter the results such that they include only the subfolders and files of a specific folder, specify that folder here:"), BIF_NONEWFOLDERBUTTON | BIF_USENEWUI | BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN };
	if (LPITEMIDLIST const pidl = SHBrowseForFolder(&info)) {
		bool const success = !!SHGetPathFromIDList(pidl, path);
		ILFree(pidl);
		if (success) {
			this->txtPattern.SetWindowText((std::tstring(path) + _T("\\*")).c_str());
			this->GotoDlgCtrl(this->txtPattern);
			this->txtPattern.SetSel(this->txtPattern.GetWindowTextLength(),
				this->txtPattern.GetWindowTextLength());
		}
	}
}

void CMainDlg::OnOK(UINT /*uNotifyCode*/, int /*nID*/, HWND /*hWnd*/)
{
	if (GetFocus() == this->lvFiles) {
		int const index = this->lvFiles.GetNextItem(-1, LVNI_FOCUSED);
		if (index >= 0 && (this->lvFiles.GetItemState(index, LVNI_SELECTED) & LVNI_SELECTED)) {
			this->DoubleClick(index);
		}
		else {
			this->Search();
			if (index >= 0) {
				this->lvFiles.EnsureVisible(index, FALSE);
				this->lvFiles.SetItemState(index, LVNI_FOCUSED, LVNI_FOCUSED);
			}
		}
	}
	else if (GetFocus() == this->txtPattern || GetFocus() == this->btnOK) {
		this->Search();
	}
}

LRESULT CMainDlg::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	(void)uMsg;
	LRESULT result = 0;
	POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	if ((HWND)wParam == this->lvFiles) {
		std::vector<int> indices;
		int index;
		if (point.x == -1 && point.y == -1) {
			index = this->lvFiles.GetSelectedIndex();
			RECT bounds = {};
			this->lvFiles.GetItemRect(index, &bounds, LVIR_SELECTBOUNDS);
			this->lvFiles.ClientToScreen(&bounds);
			point.x = bounds.left;
			point.y = bounds.top;
			indices.push_back(index);
		}
		else {
			POINT clientPoint = point;
			this->lvFiles.ScreenToClient(&clientPoint);
			index = this->lvFiles.HitTest(clientPoint, 0);
			if (index >= 0) {
				int i = -1;
				for (;;) {
					i = this->lvFiles.GetNextItem(i, LVNI_SELECTED);
					if (i < 0) {
						break;
					}
					indices.push_back(i);
				}
			}
		}
		int const focused = this->lvFiles.GetNextItem(-1, LVNI_FOCUSED);
		if (!indices.empty()) {
			this->RightClick(indices, point, focused);
		}
	}
	return result;
}

void CMainDlg::RightClick(std::vector<int> const &indices, POINT const &point, int const focused)
{
	std::vector<Results::const_iterator> results;
	for (size_t i = 0; i < indices.size(); ++i) {
		if (indices[i] < 0) {
			continue;
		}
		results.push_back(this->results.begin() + indices[i]);
	}
	HRESULT volatile hr = S_OK;
	UINT const minID = 1000;
	WTL::CMenu menu;
	menu.CreatePopupMenu();
	ATL::CComPtr<IContextMenu> contextMenu;
	std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > >
		p(
		new std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> >());
	p->second.reserve(
		results.size());  // REQUIRED, to avoid copying CShellItemIDList objects (they're not copyable!)
	SFGAOF sfgao = 0;
	std::tstring common_ancestor_path;
	for (size_t i = 0; i < results.size(); ++i) {
		Results::const_iterator const row = results[i];
		Results::value_type::first_type const &index = row->index;
		std::tstring path = index->root_path();
		if (index->get_path(row->key, path, false)) {
			remove_path_stream_and_trailing_sep(path);
		}
		if (i == 0) {
			common_ancestor_path = path;
		}
		CShellItemIDList itemIdList;
		if (SHParseDisplayName(path.c_str(), NULL, &itemIdList, sfgao, &sfgao) == S_OK) {
			p->second.push_back(CShellItemIDList());
			p->second.back().Attach(itemIdList.Detach());
			if (i != 0) {
				common_ancestor_path = path;
				size_t j;
				for (j = 0;
					j < (path.size() < common_ancestor_path.size() ? path.size() :
					common_ancestor_path.size()); j++) {
					if (path[j] != common_ancestor_path[j]) {
						break;
					}
				}
				common_ancestor_path.erase(common_ancestor_path.begin() + static_cast<ptrdiff_t>(j),
					common_ancestor_path.end());
			}
		}
	}
	common_ancestor_path.erase(dirname(common_ancestor_path.begin(),
		common_ancestor_path.end()), common_ancestor_path.end());
	if (hr == S_OK) {
		hr = SHParseDisplayName(common_ancestor_path.c_str(), NULL, &p->first.first, sfgao,
			&sfgao);
	}
	if (hr == S_OK) {
		ATL::CComPtr<IShellFolder> desktop;
		hr = SHGetDesktopFolder(&desktop);
		if (hr == S_OK) {
			if (p->first.first.m_pidl->mkid.cb) {
				hr = desktop->BindToObject(p->first.first, NULL, IID_IShellFolder,
					reinterpret_cast<void **>(&p->first.second));
			}
			else {
				hr = desktop.QueryInterface(&p->first.second);
			}
		}
	}

	if (hr == S_OK) {
		std::vector<LPCITEMIDLIST> relative_item_ids(p->second.size());
		for (size_t i = 0; i < p->second.size(); ++i) {
			relative_item_ids[i] = ILFindChild(p->first.first, p->second[i]);
		}
		hr = p->first.second->GetUIObjectOf(
			*this,
			static_cast<UINT>(relative_item_ids.size()),
			relative_item_ids.empty() ? NULL : &relative_item_ids[0],
			IID_IContextMenu,
			NULL,
			&reinterpret_cast<void *&>(contextMenu.p));
	}
	if (hr == S_OK) {
		hr = contextMenu->QueryContextMenu(menu, 0, minID, UINT_MAX, 0x80 /*CMF_ITEMMENU*/);
	}

	unsigned int ninserted = 0;
	UINT const openContainingFolderId = minID - 1;

	if (results.size() == 1) {
		MENUITEMINFO mii2 = { sizeof(mii2), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_ENABLED, openContainingFolderId, NULL, NULL, NULL, NULL, _T("Open &Containing Folder") };
		menu.InsertMenuItem(ninserted++, TRUE, &mii2);

		if (false) {
			menu.SetMenuDefaultItem(openContainingFolderId, FALSE);
		}
	}
	if (0 <= focused && static_cast<size_t>(focused) < this->results.size()) {
		std::basic_stringstream<TCHAR> ssName;
		ssName.imbue(this->loc);
		ssName << _T("File #") << (this->results.begin() + focused)->key.first.first;
		std::tstring name = ssName.str();
		if (!name.empty()) {
			MENUITEMINFO mii1 = { sizeof(mii1), MIIM_ID | MIIM_STRING | MIIM_STATE, MFT_STRING, MFS_DISABLED, minID - 2, NULL, NULL, NULL, NULL, (name.c_str(), &name[0]) };
			menu.InsertMenuItem(ninserted++, TRUE, &mii1);
		}
	}
	if (contextMenu && ninserted) {
		MENUITEMINFO mii = { sizeof(mii), 0, MFT_MENUBREAK };
		menu.InsertMenuItem(ninserted, TRUE, &mii);
	}
	UINT id = menu.TrackPopupMenu(
		TPM_RETURNCMD | TPM_NONOTIFY | (GetKeyState(VK_SHIFT) < 0 ? CMF_EXTENDEDVERBS : 0) |
		(GetSystemMetrics(SM_MENUDROPALIGNMENT) ? TPM_RIGHTALIGN | TPM_HORNEGANIMATION :
		TPM_LEFTALIGN | TPM_HORPOSANIMATION),
		point.x, point.y, *this);
	if (!id) {
		// User cancelled
	}
	else if (id == openContainingFolderId) {
		if (QueueUserWorkItem(&SHOpenFolderAndSelectItemsThread, p.get(), WT_EXECUTEINUITHREAD)) {
			p.release();
		}
	}
	else if (id >= minID) {
		CMINVOKECOMMANDINFO cmd = { sizeof(cmd), CMIC_MASK_ASYNCOK, *this, reinterpret_cast<LPCSTR>(id - minID), NULL, NULL, SW_SHOW };
		hr = contextMenu ? contextMenu->InvokeCommand(&cmd) : S_FALSE;
		if (hr == S_OK) {
		}
		else {
			this->MessageBox(GetAnyErrorText(hr), _T("Error"), MB_OK | MB_ICONERROR);
		}
	}
}

void CMainDlg::DoubleClick(int index)
{
	Results::const_iterator const result = this->results.begin() + static_cast<ptrdiff_t>
		(index);
	Results::value_type::first_type const &i = result->index;
	std::tstring path;
	path = i->root_path(), i->get_path(result->key, path, false);
	remove_path_stream_and_trailing_sep(path);
	std::auto_ptr<std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> > >
		p(
		new std::pair<std::pair<CShellItemIDList, ATL::CComPtr<IShellFolder> >, std::vector<CShellItemIDList> >());
	SFGAOF sfgao = 0;
	std::tstring const path_directory(path.begin(), dirname(path.begin(), path.end()));
	HRESULT hr = SHParseDisplayName(path_directory.c_str(), NULL, &p->first.first, 0, &sfgao);
	if (hr == S_OK) {
		ATL::CComPtr<IShellFolder> desktop;
		hr = SHGetDesktopFolder(&desktop);
		if (hr == S_OK) {
			if (p->first.first.m_pidl->mkid.cb) {
				hr = desktop->BindToObject(p->first.first, NULL, IID_IShellFolder,
					reinterpret_cast<void **>(&p->first.second));
			}
			else {
				hr = desktop.QueryInterface(&p->first.second);
			}
		}
	}
	if (hr == S_OK && basename(path.begin(), path.end()) != path.end()) {
		p->second.resize(1);
		hr = SHParseDisplayName((path.c_str(), path.empty() ? NULL : &path[0]), NULL,
			&p->second.back().m_pidl, sfgao, &sfgao);
	}
	SHELLEXECUTEINFO shei = { sizeof(shei), SEE_MASK_INVOKEIDLIST | SEE_MASK_UNICODE, *this, NULL, NULL, p->second.empty() ? path_directory.c_str() : NULL, path_directory.c_str(), SW_SHOWDEFAULT, 0, p->second.empty() ? NULL : p->second.back().m_pidl };
	ShellExecuteEx(&shei);
}

LRESULT CMainDlg::OnFilesDoubleClick(LPNMHDR pnmh)
{
	// Wow64Disable wow64Disabled;
	WTL::CWaitCursor wait;
	LPNMITEMACTIVATE pnmItem = (LPNMITEMACTIVATE)pnmh;
	if (this->lvFiles.GetSelectedCount() == 1) {
		this->DoubleClick(this->lvFiles.GetNextItem(-1, LVNI_SELECTED));
	}
	return 0;
}

LRESULT CMainDlg::OnFileNameArrowKey(LPNMHDR pnmh)
{
	CSearchPattern::KeyNotify *const p = (CSearchPattern::KeyNotify *)pnmh;
	if (p->vkey == VK_UP || p->vkey == VK_DOWN) {
		this->cmbDrive.SendMessage(p->hdr.code == CSearchPattern::CUN_KEYDOWN ? WM_KEYDOWN :
			WM_KEYUP, p->vkey, p->lParam);
	}
	else {
		if (p->hdr.code == CSearchPattern::CUN_KEYDOWN && p->vkey == VK_DOWN
			&& this->lvFiles.GetItemCount() > 0) {
			this->lvFiles.SetFocus();
		}
		this->lvFiles.SendMessage(p->hdr.code == CSearchPattern::CUN_KEYDOWN ? WM_KEYDOWN :
			WM_KEYUP, p->vkey, p->lParam);
	}
	return 0;
}

LRESULT CMainDlg::OnFilesKeyDown(LPNMHDR pnmh)
	{
		NMLVKEYDOWN *const p = (NMLVKEYDOWN *)pnmh;
		if (p->wVKey == VK_UP && this->lvFiles.GetNextItem(-1, LVNI_FOCUSED) == 0) {
			this->txtPattern.SetFocus();
		}
		return 0;
	}

std::tstring CMainDlg::GetSubItemText(Results::const_iterator const result, int const subitem, std::tstring &path) const
	{
		Results::value_type::first_type const &i = result->index;
		path = i->root_path();
		i->get_path(result->key, path, false);
		std::tstring text(0x100, _T('\0'));
		switch (subitem) {
		case COLUMN_INDEX_NAME:
			text = path;
			deldirsep(text);
			text.erase(text.begin(), basename(text.begin(), text.end()));
			break;
		case COLUMN_INDEX_PATH:
			text = path;
			break;
		case COLUMN_INDEX_SIZE:
			text = nformat(i->get_sizes(result->key).length, this->loc, true);
			break;
		case COLUMN_INDEX_SIZE_ON_DISK:
			text = nformat(i->get_sizes(result->key).allocated, this->loc, true);
			break;
		case COLUMN_INDEX_CREATION_TIME:
			SystemTimeToString(i->get_stdinfo(result->key.first.first).created, &text[0],
				text.size());
			text = std::tstring(text.c_str());
			break;
		case COLUMN_INDEX_MODIFICATION_TIME:
			SystemTimeToString(i->get_stdinfo(result->key.first.first).written, &text[0],
				text.size());
			text = std::tstring(text.c_str());
			break;
		case COLUMN_INDEX_ACCESS_TIME:
			SystemTimeToString(i->get_stdinfo(result->key.first.first).accessed, &text[0],
				text.size());
			text = std::tstring(text.c_str());
			break;
		default:
			break;
		}
		return text;
	}

LRESULT CMainDlg::OnFilesGetDispInfo(LPNMHDR pnmh)
	{
		NMLVDISPINFO *const pLV = (NMLVDISPINFO *)pnmh;

		if ((this->lvFiles.GetStyle() & LVS_OWNERDATA) != 0
			&& (pLV->item.mask & LVIF_TEXT) != 0) {
			Results::const_iterator const result = this->results.begin() + static_cast<ptrdiff_t>
				(pLV->item.iItem);
			Results::value_type::first_type const &i = result->index;
			std::tstring path;
			std::tstring const text = this->GetSubItemText(result, pLV->item.iSubItem, path);
			if (!text.empty()) {
				_tcsncpy(pLV->item.pszText, text.c_str(), pLV->item.cchTextMax);
			}
			if (pLV->item.iSubItem == 0) {
				int iImage = this->CacheIcon(path, static_cast<int>(pLV->item.iItem),
					i->get_stdinfo(result->key.first.first).attributes, true);
				if (iImage >= 0) {
					pLV->item.iImage = iImage;
				}
			}
		}
		return 0;
	}

LRESULT CMainDlg::OnFilesListCustomDraw(LPNMHDR pnmh)
{
	LRESULT result;
	COLORREF const deletedColor = RGB(0xFF, 0, 0);
	COLORREF encryptedColor = RGB(0, 0xFF, 0);
	COLORREF compressedColor = RGB(0, 0, 0xFF);
	WTL::CRegKeyEx key;
	if (key.Open(HKEY_CURRENT_USER,
		_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer")) == ERROR_SUCCESS) {
		key.QueryDWORDValue(_T("AltColor"), compressedColor);
		key.QueryDWORDValue(_T("AltEncryptedColor"), encryptedColor);
		key.Close();
	}
	COLORREF sparseColor = RGB(GetRValue(compressedColor),
		(GetGValue(compressedColor) + GetBValue(compressedColor)) / 2,
		(GetGValue(compressedColor) + GetBValue(compressedColor)) / 2);
	LPNMLVCUSTOMDRAW const pLV = (LPNMLVCUSTOMDRAW)pnmh;
	if (pLV->nmcd.dwItemSpec < this->results.size()) {
		Results::const_iterator const item = this->results.begin() + static_cast<ptrdiff_t>
			(pLV->nmcd.dwItemSpec);
		Results::value_type::first_type const &i = item->index;
		unsigned long const attrs = i->get_stdinfo(item->key.first.first).attributes;
		switch (pLV->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			result = CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			result = CDRF_NOTIFYSUBITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
			if ((this->lvFiles.GetImageList(LVSIL_SMALL) == this->imgListLarge
				|| this->lvFiles.GetImageList(LVSIL_SMALL) == this->imgListExtraLarge)
				&& pLV->iSubItem == 1) {
				result = 0x8 /*CDRF_DOERASE*/ | CDRF_NOTIFYPOSTPAINT;
			}
			else {
				if ((attrs & 0x40000000) != 0) {
					pLV->clrText = deletedColor;
				}
				else if ((attrs & FILE_ATTRIBUTE_ENCRYPTED) != 0) {
					pLV->clrText = encryptedColor;
				}
				else if ((attrs & FILE_ATTRIBUTE_COMPRESSED) != 0) {
					pLV->clrText = compressedColor;
				}
				else if ((attrs & FILE_ATTRIBUTE_SPARSE_FILE) != 0) {
					pLV->clrText = sparseColor;
				}
				result = CDRF_DODEFAULT;
			}
			break;
		case CDDS_ITEMPOSTPAINT | CDDS_SUBITEM:
			result = CDRF_SKIPDEFAULT;
			{
				Results::const_iterator const row = this->results.begin() + static_cast<ptrdiff_t>
					(pLV->nmcd.dwItemSpec);
				std::tstring path;
				std::tstring itemText = this->GetSubItemText(row, pLV->iSubItem, path);
				WTL::CDCHandle dc(pLV->nmcd.hdc);
				RECT rcTwips = pLV->nmcd.rc;
				rcTwips.left = (int)((rcTwips.left + 6) * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
				rcTwips.right = (int)(rcTwips.right * 1440 / dc.GetDeviceCaps(LOGPIXELSX));
				rcTwips.top = (int)(rcTwips.top * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
				rcTwips.bottom = (int)(rcTwips.bottom * 1440 / dc.GetDeviceCaps(LOGPIXELSY));
				int const savedDC = dc.SaveDC();
				{
					std::replace(itemText.begin(), itemText.end(), _T(' '), _T('\u00A0'));
					replace_all(itemText, _T("\\"), _T("\\\u200B"));
#ifdef _UNICODE
					this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, 1200);
#else
					this->richEdit.SetTextEx(itemText.c_str(), ST_DEFAULT, CP_ACP);
#endif
					CHARFORMAT format = { sizeof(format), CFM_COLOR, 0, 0, 0, 0 };
					if ((attrs & 0x40000000) != 0) {
						format.crTextColor = deletedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_ENCRYPTED) != 0) {
						format.crTextColor = encryptedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_COMPRESSED) != 0) {
						format.crTextColor = compressedColor;
					}
					else if ((attrs & FILE_ATTRIBUTE_SPARSE_FILE) != 0) {
						format.crTextColor = sparseColor;
					}
					else {
						bool const selected = (this->lvFiles.GetItemState(static_cast<int>(pLV->nmcd.dwItemSpec),
							LVIS_SELECTED) & LVIS_SELECTED) != 0;
						format.crTextColor = GetSysColor(selected
							&& this->lvFiles.IsThemeNull() ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
					}
					this->richEdit.SetSel(0, -1);
					this->richEdit.SetSelectionCharFormat(format);
					if (false) {
						size_t last_sep = itemText.find_last_of(_T('\\'));
						if (~last_sep) {
							this->richEdit.SetSel(static_cast<long>(last_sep + 1), this->richEdit.GetTextLength());
							CHARFORMAT bold = { sizeof(bold), CFM_BOLD, CFE_BOLD, 0, 0, 0 };
							this->richEdit.SetSelectionCharFormat(bold);
						}
					}
					FORMATRANGE formatRange = { dc, dc, rcTwips, rcTwips, { 0, -1 } };
					this->richEdit.FormatRange(formatRange, FALSE);
					LONG height = formatRange.rc.bottom - formatRange.rc.top;
					formatRange.rc = formatRange.rcPage;
					formatRange.rc.top += (formatRange.rc.bottom - formatRange.rc.top - height) / 2;
					this->richEdit.FormatRange(formatRange, TRUE);

					this->richEdit.FormatRange(NULL);
				}
				dc.RestoreDC(savedDC);
			}
			break;
		default:
			result = CDRF_DODEFAULT;
			break;
		}
	}
	else {
		result = CDRF_DODEFAULT;
	}
	return result;
}

void CMainDlg::OnClose(UINT /*uNotifyCode*/ /*= 0*/, int nID /*= IDCANCEL*/, HWND /*hWnd*/ /*= NULL*/)
{
	this->DestroyWindow();
	PostQuitMessage(nID);
	// this->EndDialog(nID);
}

LRESULT CMainDlg::OnDeviceChange(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case DBT_DEVICEQUERYREMOVEFAILED: {
	}
									  break;
	case DBT_DEVICEQUERYREMOVE: {
		DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
		if (header.dbch_devicetype == DBT_DEVTYP_HANDLE) {
			reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header);
		}
	}
								break;
	case DBT_DEVICEREMOVECOMPLETE: {
		DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
		if (header.dbch_devicetype == DBT_DEVTYP_HANDLE) {
			reinterpret_cast<DEV_BROADCAST_HANDLE const &>(header);
		}
	}
								   break;
	case DBT_DEVICEARRIVAL: {
		DEV_BROADCAST_HDR const &header = *reinterpret_cast<DEV_BROADCAST_HDR *>(lParam);
		if (header.dbch_devicetype == DBT_DEVTYP_VOLUME) {
		}
	}
							break;
	default:
		break;
	}
	return TRUE;
}

void CMainDlg::OnWindowPosChanged(LPWINDOWPOS lpWndPos)
{
	if (lpWndPos->flags & SWP_SHOWWINDOW) {
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		this->DeleteNotifyIcon();
	}
	else if (lpWndPos->flags & SWP_HIDEWINDOW) {
		SetPriorityClass(GetCurrentProcess(), 0x100000 /*PROCESS_MODE_BACKGROUND_BEGIN*/);
		SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	}
	this->SetMsgHandled(FALSE);
}

BOOL CMainDlg::CheckAndCreateIcon(bool checkVisible)
{
	NOTIFYICONDATA nid = { sizeof(nid), *this, 0, NIF_MESSAGE | NIF_ICON | NIF_TIP, WM_NOTIFYICON, this->GetIcon(FALSE), _T("SwiftSearch") };
	return (!checkVisible || !this->IsWindowVisible()) && Shell_NotifyIcon(NIM_ADD, &nid);
}

LRESULT CMainDlg::OnDriveSetItemData(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam)
{
	if (this->indices_created >= this->cmbDrive.GetCount()) {
		// throw std::logic_error("indices_created is broken!! avoid deleting entries in the list...");
	}
	this->cmbDrive.SetItemDataPtr(static_cast<int>(wParam),
		reinterpret_cast<NtfsIndex *>(lParam));
	++this->indices_created;
	if (this->indices_created == this->cmbDrive.GetCount() - 1) {
		for (int i = 1; i < this->cmbDrive.GetCount(); i++) {
			if (!this->cmbDrive.GetItemDataPtr(i)) {
				this->cmbDrive.DeleteString(i--);
			}
		}
	}
	return 0;
}

LRESULT CMainDlg::OnTaskbarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	this->CheckAndCreateIcon(true);
	return 0;
}

LRESULT CMainDlg::OnNotifyIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
	if (lParam == WM_LBUTTONUP || lParam == WM_KEYUP) {
		this->ShowWindow(SW_SHOW);
	}
	return 0;
}

void CMainDlg::OnHelpRegex(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	this->MessageBox(
		_T("To find a file, select the drive you want to search, enter part of the file name or path, and click Search.\r\n\r\n")
		_T("You can either use wildcards, which are the default, or regular expressions, which require starting the pattern with a '>' character.\r\n\r\n")
		_T("Wildcards work the same as in Windows; regular expressions are implemented using the Boost.Xpressive library.\r\n\r\n")
		_T("Some common regular expressions:\r\n")
		_T(".\t= A single character\r\n")
		_T("\\+\t= A plus symbol (backslash is the escape character)\r\n")
		_T("[a-cG-K]\t= A single character from a to c or from G to K\r\n")
		_T("(abc|def)\t= Either \"abc\" or \"def\"\r\n\r\n")
		_T("\"Quantifiers\" can follow any expression:\r\n")
		_T("*\t= Zero or more occurrences\r\n")
		_T("+\t= One or more occurrences\r\n")
		_T("{m,n}\t= Between m and n occurrences (n is optional)\r\n\r\n")
		_T("Examples of regular expressions:\r\n")
		_T("Hi{2,}.*Bye= At least two occurrences of \"Hi\", followed by any number of characters, followed by \"Bye\"\r\n")
		_T(".*\t= At least zero characters\r\n")
		_T("Hi.+\\+Bye\t= At least one character between \"Hi\" and \"+Bye\"\r\n")
		, _T("Regular expressions"), MB_ICONINFORMATION);
}

void CMainDlg::OnViewGridlines(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	this->lvFiles.SetExtendedListViewStyle(this->lvFiles.GetExtendedListViewStyle() ^
		LVS_EX_GRIDLINES);
	this->lvFiles.RedrawWindow();
	this->menu.CheckMenuItem(ID_VIEW_GRIDLINES,
		(this->lvFiles.GetExtendedListViewStyle() & LVS_EX_GRIDLINES) ? MF_CHECKED :
		MF_UNCHECKED);
}

void CMainDlg::OnRefresh(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	if (this->indices_created < this->cmbDrive.GetCount()) {
		// Ignore the request due to potential race condition... at least wait until all the threads have started!
		// Otherwise, if the user presses F5 we will delete some entries, which will invalidate threads' copies of their index in the combobox
		return;
	}
	int const selected = this->cmbDrive.GetCurSel();
	for (int ii = 0; ii < this->cmbDrive.GetCount(); ++ii) {
		if (selected == 0 || ii == selected) {
			if (boost::intrusive_ptr<NtfsIndex> const q = static_cast<NtfsIndex *>
				(this->cmbDrive.GetItemDataPtr(ii))) {
				std::tstring const path_name = q->root_path();
				q->cancel();
				this->cmbDrive.SetItemDataPtr(ii, NULL);
				intrusive_ptr_release(q.get());
				typedef OverlappedNtfsMftReadPayload T;
				boost::intrusive_ptr<T> p(new T(this->iocp, path_name, this->m_hWnd, WM_DRIVESETITEMDATA,
					this->closing_event));
				if (PostQueuedCompletionStatus(this->iocp, 0, static_cast<uintptr_t>(ii), &*p)) {
					p.detach();
				}
			}
		}
	}
}


void CMainDlg::OnHelpAbout(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	this->MessageBox(_T("? 2015 Mehrdad N.\r\nAll rights reserved."), _T("About"),
		MB_ICONINFORMATION);
}

void CMainDlg::OnViewLargeIcons(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	bool const large = this->lvFiles.GetImageList(LVSIL_SMALL) != this->imgListLarge;
	this->lvFiles.SetImageList(large ? this->imgListLarge : this->imgListSmall, LVSIL_SMALL);
	this->lvFiles.RedrawWindow();
	this->menu.CheckMenuItem(ID_VIEW_LARGEICONS, large ? MF_CHECKED : MF_UNCHECKED);
}

void CMainDlg::OnViewFitColumns(UINT /*uNotifyCode*/, int /*nID*/, CWindow /*wndCtl*/)
{
	WTL::CListViewCtrl &wndListView = this->lvFiles;

	RECT rect;
	wndListView.GetClientRect(&rect);
	DWORD width;
	width = (std::max)(1, (int)(rect.right - rect.left) - GetSystemMetrics(SM_CXVSCROLL));
	WTL::CHeaderCtrl wndListViewHeader = wndListView.GetHeader();
	int oldTotalColumnsWidth;
	oldTotalColumnsWidth = 0;
	int columnCount;
	columnCount = wndListViewHeader.GetItemCount();
	for (int i = 0; i < columnCount; i++) {
		oldTotalColumnsWidth += wndListView.GetColumnWidth(i);
	}
	for (int i = 0; i < columnCount; i++) {
		int colWidth = wndListView.GetColumnWidth(i);
		int newWidth = MulDiv(colWidth, width, oldTotalColumnsWidth);
		newWidth = (std::max)(newWidth, 1);
		wndListView.SetColumnWidth(i, newWidth);
	}
}

void CMainDlg::setupUI()
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	this->SetIcon((HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0), FALSE);
	this->SetIcon((HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON,
		GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0), TRUE);

	this->accel.LoadAccelerators(IDR_ACCELERATOR1);
	this->menu.Attach(this->GetMenu());
	this->btnOK.Attach(this->GetDlgItem(IDOK));

	this->txtPattern.SubclassWindow(this->GetDlgItem(IDC_EDITFILENAME));
	this->txtPattern.EnsureTrackingMouseHover();
	this->txtPattern.SetCueBannerText(_T("Search by name or path (hover for help)"), true);
	SHAutoComplete(this->txtPattern, SHACF_FILESYS_ONLY | SHACF_USETAB);

	setupFileList();
	setupStatusbar();
	setupDriverComboList();

	WTL::CRect rcStatusPane1;
	this->statusbar.GetRect(1, &rcStatusPane1);
	WTL::CRect clientRect;
	if (this->lvFiles.GetWindowRect(&clientRect)) {
		this->ScreenToClient(&clientRect);
		this->lvFiles.SetWindowPos(NULL, 0, 0, clientRect.Width(),
			clientRect.Height() - rcStatusPane1.Height(), SWP_NOMOVE | SWP_NOZORDER);
	}
	this->DlgResize_Init(false, false);
}

void CMainDlg::setupDriverComboList()
{
	this->cmbDrive.Attach(this->GetDlgItem(IDC_LISTVOLUMES));
	this->cmbDrive.SetCueBannerText(_T("Search where?"));

	std::vector<std::tstring> path_names;
	{
		std::tstring buf;
		size_t prev;
		do {
			prev = buf.size();
			buf.resize(std::max(static_cast<size_t>(GetLogicalDriveStrings(static_cast<unsigned long>
				(buf.size()), buf.empty() ? NULL : &*buf.begin())), buf.size()));
		} while (prev < buf.size());
		for (size_t i = 0, n; n = std::char_traits<TCHAR>::length(&buf[i]), i < buf.size()
			&& buf[i]; i += n + 1) {
			path_names.push_back(std::tstring(&buf[i], n));
		}
	}

	this->cmbDrive.SetCurSel(this->cmbDrive.AddString(_T("(All drives)")));
	for (size_t j = 0; j != path_names.size(); ++j) {
		// Do NOT capture 'this'!! It is volatile!!
		int const index = this->cmbDrive.AddString(path_names[j].c_str());
		typedef OverlappedNtfsMftReadPayload T;
		boost::intrusive_ptr<T> q(new T(this->iocp, path_names[j], this->m_hWnd,
			WM_DRIVESETITEMDATA, this->closing_event));
		if (PostQueuedCompletionStatus(this->iocp, 0, static_cast<uintptr_t>(index), &*q)) {
			q.detach();
		}
	}
}

void CMainDlg::setupFileList()
{
	this->lvFiles.Attach(this->GetDlgItem(IDC_LISTFILES));
	this->richEdit.Create(this->lvFiles, NULL, 0, ES_MULTILINE, WS_EX_TRANSPARENT);
	this->richEdit.SetFont(this->lvFiles.GetFont());
	WTL::CHeaderCtrl hdr = this->lvFiles.GetHeader();
	// COLUMN_INDEX_NAME, COLUMN_INDEX_PATH, COLUMN_INDEX_SIZE, COLUMN_INDEX_SIZE_ON_DISK, COLUMN_INDEX_CREATION_TIME, COLUMN_INDEX_MODIFICATION_TIME, COLUMN_INDEX_ACCESS_TIME
	{
		int const icol = COLUMN_INDEX_NAME;
		LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 200, _T("Name") };
		this->lvFiles.InsertColumn(icol, &column);
		HDITEM hditem = { HDI_LPARAM };
		hditem.lParam = 0;
		hdr.SetItem(icol, &hditem);
	}
		{
			int const icol = COLUMN_INDEX_PATH;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 340, _T("Path") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}
		{
			int const icol = COLUMN_INDEX_SIZE;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_RIGHT, 100, _T("Size") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}
		{
			int const icol = COLUMN_INDEX_SIZE_ON_DISK;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_RIGHT, 100, _T("Size on Disk") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}
		{
			int const icol = COLUMN_INDEX_CREATION_TIME;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Created") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}
		{
			int const icol = COLUMN_INDEX_MODIFICATION_TIME;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Written") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}
		{
			int const icol = COLUMN_INDEX_ACCESS_TIME;
			LVCOLUMN column = { LVCF_FMT | LVCF_WIDTH | LVCF_TEXT, LVCFMT_LEFT, 80, _T("Accessed") };
			this->lvFiles.InsertColumn(icol, &column);
			HDITEM hditem = { HDI_LPARAM };
			hditem.lParam = 0;
			hdr.SetItem(icol, &hditem);
		}

		{
			const int IMAGE_LIST_INCREMENT = 0x100;
			this->imgListSmall.Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CXSMICON),
				ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
			this->imgListLarge.Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CXICON),
				ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
			this->imgListExtraLarge.Create(48, 48, ILC_COLOR32, 0, IMAGE_LIST_INCREMENT);
		}

		this->lvFiles.OpenThemeData(VSCLASS_LISTVIEW);
		SetWindowTheme(this->lvFiles, _T("Explorer"), NULL);
		if (false) {
			WTL::CFontHandle font = this->txtPattern.GetFont();
			LOGFONT logFont;
			if (font.GetLogFont(logFont)) {
				logFont.lfHeight = logFont.lfHeight * 100 / 85;
				this->txtPattern.SetFont(WTL::CFontHandle().CreateFontIndirect(&logFont));
			}
		}
		this->lvFiles.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT |
			LVS_EX_LABELTIP | LVS_EX_GRIDLINES | 0x80000000 /*LVS_EX_COLUMNOVERFLOW*/);
		{
			this->lvFiles.SetImageList(this->imgListLarge, LVSIL_SMALL);
			this->lvFiles.SetImageList(this->imgListLarge, LVSIL_NORMAL);
			this->lvFiles.SetImageList(this->imgListExtraLarge, LVSIL_NORMAL);
		}
}

void CMainDlg::setupStatusbar()
{
	this->statusbar = CreateStatusWindow(WS_CHILD | SBT_TOOLTIPS, NULL, *this,
		IDC_STATUS_BAR);
	int const rcStatusPaneWidths[] = { 360, -1 };
	if ((this->statusbar.GetStyle() & WS_VISIBLE) != 0) {
		RECT rect;
		this->lvFiles.GetWindowRect(&rect);
		this->ScreenToClient(&rect);
		{
			RECT sbRect;
			this->statusbar.GetWindowRect(&sbRect);
			rect.bottom -= (sbRect.bottom - sbRect.top);
		}
		this->lvFiles.MoveWindow(&rect);
	}
	this->statusbar.SetParts(sizeof(rcStatusPaneWidths) / sizeof(*rcStatusPaneWidths),
		const_cast<int *>(rcStatusPaneWidths));
	this->statusbar.SetText(0, _T("Type in a file name and press Enter."));
	//this->statusbarProgress.Create(this->statusbar, rcStatusPane1, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, 0);
	//this->statusbarProgress.SetRange(0, INT_MAX);
	//this->statusbarProgress.SetPos(INT_MAX / 2);
	this->statusbar.ShowWindow(SW_SHOW);
}

void CMainDlg::OnDestroy()
{
	UnregisterWait(this->hWait);
	this->DeleteNotifyIcon();
	this->iconLoader->clear();
	for (size_t i = 0; i != this->threads.size(); ++i) {
		PostQueuedCompletionStatus(this->iocp, 0, 0, NULL);
	}
}

int CMainDlg::CacheIcon(std::tstring path, int const iItem, ULONG fileAttributes, bool lifo)
	{
		remove_path_stream_and_trailing_sep(path);
		if (this->cache.find(path) == this->cache.end()) {
			this->cache[path] = CacheInfo();
		}

		CacheInfo const &entry = this->cache[path];

		if (!entry.valid && this->lastRequestedIcon != path) {
			SIZE iconSmallSize;
			this->imgListSmall.GetIconSize(iconSmallSize);
			SIZE iconSmallLarge;
			this->imgListLarge.GetIconSize(iconSmallLarge);

			IconLoaderCallback callback = { this, path, iconSmallSize, iconSmallLarge, fileAttributes, iItem };
			this->iconLoader->add(callback, lifo);
			this->lastRequestedIcon = path;
		}
		return entry.iIconSmall;
	}

