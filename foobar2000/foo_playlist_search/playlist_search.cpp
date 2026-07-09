#include "stdafx.h"

// Playlist search popup: hotkey-bindable mainmenu command opening a modeless
// dialog with a live-filtered view of the active playlist.

#include "resource.h"
#include "playlist_search.h"

#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
#include <helpers/CDialogResizeHelper.h>
#include <libPPUI/CListControlOwnerData.h>

#include <string>
#include <vector>

// {875F43F2-0B68-4BE2-819C-B9D79832B828}
static constexpr GUID guid_cfg_rowFormat = { 0x875f43f2, 0x0b68, 0x4be2, { 0x81, 0x9c, 0xb9, 0xd7, 0x98, 0x32, 0xb8, 0x28 } };
// {F920ED44-1A17-4DDE-82A3-01E09C37D3ED}
static constexpr GUID guid_cfg_infoFormat = { 0xf920ed44, 0x1a17, 0x4dde, { 0x82, 0xa3, 0x01, 0xe0, 0x9c, 0x37, 0xd3, 0xed } };
// {E064CA33-53DF-4DF1-B7E5-F476449827BA}
static constexpr GUID guid_cfg_popupPosition = { 0xe064ca33, 0x53df, 0x4df1, { 0xb7, 0xe5, 0xf4, 0x76, 0x44, 0x98, 0x27, 0xba } };
// {7F66295E-5D85-4BE4-89C4-891F858DD69F}
static constexpr GUID guid_menu_command = { 0x7f66295e, 0x5d85, 0x4be4, { 0x89, 0xc4, 0x89, 0x1f, 0x85, 0x8d, 0xd6, 0x9f } };

const char default_rowFormat[] = "[%album artist% - ]['['%date%']' ]%album%|[[%discnumber%.][%tracknumber%. ]][%track artist% - ]%title%";
const char default_infoFormat[] = "Title: %title%$crlf()Artist: %artist%$crlf()Album: %album%$crlf()Time: [%length%][ | %codec%][ | %bitrate% kbps]$crlf()Path: %path%";

cfg_string cfg_rowFormat(guid_cfg_rowFormat, default_rowFormat);
cfg_string cfg_infoFormat(guid_cfg_infoFormat, default_infoFormat);
static cfgDialogPosition cfg_popupPosition(guid_cfg_popupPosition);

namespace {

// Case- and accent-insensitive fold: decompose accented chars, drop combining
// marks, lowercase. Applied once per row at index time, once per query word.
static std::wstring fold(const char* utf8) {
	pfc::stringcvt::string_wide_from_utf8 w(utf8);
	int n = FoldStringW(MAP_COMPOSITE, w, -1, nullptr, 0);
	if (n <= 0) return std::wstring();
	std::wstring buf((size_t)n, L'\0');
	FoldStringW(MAP_COMPOSITE, w, -1, buf.data(), n);
	std::vector<WORD> types(buf.size());
	GetStringTypeW(CT_CTYPE3, buf.data(), (int)buf.size(), types.data());
	std::wstring out; out.reserve(buf.size());
	for (size_t i = 0; i < buf.size(); ++i) {
		if (buf[i] != 0 && !(types[i] & C3_NONSPACING)) out += buf[i];
	}
	if (!out.empty()) CharLowerBuffW(out.data(), (DWORD)out.size());
	return out;
}

static std::vector<std::wstring> splitQuery(const char* utf8) {
	std::vector<std::wstring> words;
	std::wstring folded = fold(utf8), word;
	for (wchar_t c : folded) {
		if (iswspace(c)) {
			if (!word.empty()) { words.push_back(std::move(word)); word.clear(); }
		} else word += c;
	}
	if (!word.empty()) words.push_back(std::move(word));
	return words;
}

static bool matchesAllWords(const std::wstring& folded, const std::vector<std::wstring>& words) {
	for (auto& w : words) {
		if (folded.find(w) == std::wstring::npos) return false;
	}
	return true;
}

// ================================================================
// Cached index of the active playlist, rebuilt only when marked dirty
// ================================================================
struct row_t {
	pfc::string8 display;
	std::wstring folded;
};

static struct {
	std::vector<row_t> rows;
	bool dirty = true;

	void ensureFresh() {
		if (!dirty) return;
		rows.clear();
		auto api = playlist_manager::get();
		titleformat_object::ptr script;
		titleformat_compiler::get()->compile_safe(script, cfg_rowFormat.get());
		const t_size count = api->activeplaylist_get_item_count();
		rows.reserve(count);
		pfc::string8 temp;
		for (t_size i = 0; i < count; ++i) {
			api->activeplaylist_item_format_title(i, nullptr, temp, script, nullptr, play_control::display_level_all);
			row_t r;
			r.display = temp;
			r.folded = fold(temp);
			rows.push_back(std::move(r));
		}
		dirty = false;
	}
} g_index;

// ================================================================
// The popup dialog
// ================================================================
class CSearchDlg : public CDialogImpl<CSearchDlg>, private IListControlOwnerDataSource {
public:
	enum { IDD = IDD_PLAYLIST_SEARCH };

	CSearchDlg() : m_list(this), m_search(this, 1), m_resizer(resizeTable, CRect(230, 170, 0, 0), cfg_popupPosition) {}

	BEGIN_MSG_MAP_EX(CSearchDlg)
		CHAIN_MSG_MAP_MEMBER(m_resizer)
		MSG_WM_INITDIALOG(OnInitDialog)
		MSG_WM_DESTROY(OnDestroy)
		MSG_WM_CLOSE(OnClose)
		COMMAND_HANDLER_EX(IDC_SEARCH, EN_CHANGE, OnSearchChange)
		COMMAND_ID_HANDLER_EX(IDOK, OnOK)
		COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
	ALT_MSG_MAP(1) // subclassed search edit box
		MSG_WM_KEYDOWN(OnSearchKeyDown)
	END_MSG_MAP()

	void RefreshIndex() {
		g_index.ensureFresh();
		titleformat_compiler::get()->compile_safe(m_scriptInfo, cfg_infoFormat.get());
		ApplyFilter();
	}

	void FocusSearchBox() {
		m_search.SetFocus();
		m_search.SetSel(0, -1);
	}

	static CSearchDlg* g_dlg;

private:
	BOOL OnInitDialog(CWindow, LPARAM) {
		g_dlg = this;
		// reuse foobar2000's own window icon
		CWindow main = core_api::get_main_window();
		HICON iconSmall = (HICON)main.SendMessage(WM_GETICON, ICON_SMALL, 0);
		if (iconSmall == NULL) iconSmall = (HICON)::GetClassLongPtr(main, GCLP_HICONSM);
		if (iconSmall != NULL) SetIcon(iconSmall, FALSE);
		HICON iconBig = (HICON)main.SendMessage(WM_GETICON, ICON_BIG, 0);
		if (iconBig == NULL) iconBig = (HICON)::GetClassLongPtr(main, GCLP_HICON);
		if (iconBig != NULL) SetIcon(iconBig, TRUE);
		m_search.SubclassWindow(GetDlgItem(IDC_SEARCH));
		m_list.CreateInDialog(*this, IDC_LIST);
		// AFTER CreateInDialog, so dark hooks bind the CListControl, not the placeholder
		m_dark.AddDialogWithControls(*this);
		m_list.SetSelectionModeSingle();
		// no AddColumn call: headerless single-column mode, full item width
		RefreshIndex();
		ShowWindow(SW_SHOW);
		return TRUE; // system sets focus to first tabstop = search box
	}
	void OnDestroy() {
		g_dlg = nullptr;
	}
	void OnClose() {
		DestroyWindow();
	}
	void OnSearchChange(UINT, int, CWindow) {
		ApplyFilter();
	}
	void OnOK(UINT, int, CWindow) {
		PlayFocused();
	}
	void OnCancel(UINT, int, CWindow) {
		DestroyWindow();
	}
	void OnSearchKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) {
		switch (nChar) {
		case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
			// navigate the list while the caret stays in the search box
			m_list.SendMessage(WM_KEYDOWN, nChar, MAKELPARAM(nRepCnt, nFlags));
			return;
		case 'A':
			if (GetKeyState(VK_CONTROL) < 0) { m_search.SetSel(0, -1); return; }
			break;
		}
		SetMsgHandled(FALSE);
	}

	void ApplyFilter() {
		pfc::string8 text;
		uGetDlgItemText(*this, IDC_SEARCH, text);
		const auto words = splitQuery(text);
		m_view.clear();
		m_view.reserve(g_index.rows.size());
		for (size_t i = 0; i < g_index.rows.size(); ++i) {
			if (matchesAllWords(g_index.rows[i].folded, words)) m_view.push_back(i);
		}
		m_list.ReloadData();
		if (!m_view.empty()) {
			m_list.SetFocusItem(0);
			m_list.SetSelection(pfc::bit_array_true(), pfc::bit_array_one(0));
			m_list.EnsureItemVisible(0);
		}
		UpdateInfoPane();
	}

	void UpdateInfoPane() {
		pfc::string8 out;
		const size_t focus = m_list.GetFocusItem();
		if (focus < m_view.size() && m_scriptInfo.is_valid()) {
			playlist_manager::get()->activeplaylist_item_format_title(m_view[focus], nullptr, out, m_scriptInfo, nullptr, play_control::display_level_all);
		}
		// edit controls need \r\n; normalize lone \n from titleformat
		pfc::string8 crlf;
		crlf.prealloc(out.length() + 16);
		for (size_t i = 0; i < out.length(); ++i) {
			if (out[i] == '\n' && (i == 0 || out[i - 1] != '\r')) crlf += "\r\n";
			else crlf.add_byte(out[i]);
		}
		uSetDlgItemText(*this, IDC_INFO, crlf);
	}

	void PlayFocused() {
		const size_t focus = m_list.GetFocusItem();
		if (focus >= m_view.size()) return;
		playlist_manager::get()->activeplaylist_execute_default_action(m_view[focus]);
		DestroyWindow();
	}

	// IListControlOwnerDataSource
	size_t listGetItemCount(ctx_t) override {
		return m_view.size();
	}
	pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override {
		if (subItem == 0 && item < m_view.size()) return g_index.rows[m_view[item]].display;
		return "";
	}
	void listItemAction(ctx_t, size_t item) override { // double-click / Enter in list
		if (item < m_view.size()) {
			playlist_manager::get()->activeplaylist_execute_default_action(m_view[item]);
			// deferred close: destroying the dialog here would free the list control mid-callback
			PostMessage(WM_CLOSE);
		}
	}
	void listFocusChanged(ctx_t) override {
		UpdateInfoPane();
	}
	void listSelChanged(ctx_t) override {
		UpdateInfoPane();
	}

	static const CDialogResizeHelper::Param resizeTable[5];

	std::vector<size_t> m_view; // filtered view: positions in g_index.rows == playlist item indices
	titleformat_object::ptr m_scriptInfo;
	CListControlOwnerData m_list;
	CContainedWindowT<CEdit> m_search;
	CDialogResizeHelperPT m_resizer;
	fb2k::CDarkModeHooks m_dark;
};

const CDialogResizeHelper::Param CSearchDlg::resizeTable[5] = {
	{ IDC_SEARCH, 0, 0, 1, 0 },
	{ IDC_LIST,   0, 0, 1, 1 },
	{ IDC_INFO,   0, 1, 1, 1 },
	{ IDOK,       1, 1, 1, 1 },
	{ IDCANCEL,   1, 1, 1, 1 },
};

CSearchDlg* CSearchDlg::g_dlg = nullptr;

void SpawnOrFocus() {
	auto dlg = CSearchDlg::g_dlg;
	if (dlg != nullptr) {
		::SetForegroundWindow(*dlg);
		dlg->FocusSearchBox();
	} else {
		fb2k::newDialog<CSearchDlg>();
	}
}

static void markDirty() {
	g_index.dirty = true;
	if (CSearchDlg::g_dlg != nullptr) CSearchDlg::g_dlg->RefreshIndex();
}

// ================================================================
// Playlist change notifications - keep the index lazily up to date
// ================================================================
class playlist_search_callback : public playlist_callback_static {
public:
	unsigned get_flags() override {
		return flag_on_items_added | flag_on_items_reordered | flag_on_items_removed
			| flag_on_items_modified | flag_on_items_replaced
			| flag_on_playlist_activate | flag_on_playlists_removed;
	}

	void on_items_added(t_size, t_size, const pfc::list_base_const_t<metadb_handle_ptr>&, const bit_array&) override { onChange(); }
	void on_items_reordered(t_size, const t_size*, t_size) override { onChange(); }
	void on_items_removed(t_size, const bit_array&, t_size, t_size) override { onChange(); }
	void on_items_modified(t_size, const bit_array&) override { onChange(); }
	void on_items_replaced(t_size, const bit_array&, const pfc::list_base_const_t<t_on_items_replaced_entry>&) override { onChange(); }
	void on_playlist_activate(t_size, t_size) override { onChange(); }
	void on_playlists_removed(const bit_array&, t_size, t_size) override { onChange(); }

	// not subscribed
	void on_items_removing(t_size, const bit_array&, t_size, t_size) override {}
	void on_items_selection_change(t_size, const bit_array&, const bit_array&) override {}
	void on_item_focus_change(t_size, t_size, t_size) override {}
	void on_items_modified_fromplayback(t_size, const bit_array&, play_control::t_display_level) override {}
	void on_item_ensure_visible(t_size, t_size) override {}
	void on_playlist_created(t_size, const char*, t_size) override {}
	void on_playlists_reorder(const t_size*, t_size) override {}
	void on_playlists_removing(const bit_array&, t_size, t_size) override {}
	void on_playlist_renamed(t_size, const char*, t_size) override {}
	void on_default_format_changed() override {}
	void on_playback_order_changed(t_size) override {}
	void on_playlist_locked(t_size, bool) override {}

private:
	void onChange() { markDirty(); }
};

FB2K_SERVICE_FACTORY(playlist_search_callback);

// ================================================================
// Mainmenu command - hidden by default, bindable to a keyboard shortcut
// ================================================================
class mainmenu_playlist_search : public mainmenu_commands {
public:
	t_uint32 get_command_count() override { return 1; }
	GUID get_command(t_uint32) override { return guid_menu_command; }
	void get_name(t_uint32, pfc::string_base& out) override { out = "Quick playlist search"; }
	bool get_description(t_uint32, pfc::string_base& out) override {
		out = "Opens the playlist search window for the active playlist.";
		return true;
	}
	GUID get_parent() override { return mainmenu_groups::view; }
	bool get_display(t_uint32 index, pfc::string_base& text, t_uint32& flags) override {
		get_name(index, text);
		flags = flag_defaulthidden;
		return true;
	}
	void execute(t_uint32, service_ptr_t<service_base>) override {
		SpawnOrFocus();
	}
};

static mainmenu_commands_factory_t<mainmenu_playlist_search> g_mainmenu_factory;

} // namespace

void playlistSearchInvalidateIndex() {
	markDirty();
}
