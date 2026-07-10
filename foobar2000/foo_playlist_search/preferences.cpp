#include "stdafx.h"

#include "resource.h"
#include "playlist_search.h"

#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>

// {CC68A2E9-A7C9-4559-8D40-98F1A01A2439}
static constexpr GUID guid_prefs_page = { 0xcc68a2e9, 0xa7c9, 0x4559, { 0x8d, 0x40, 0x98, 0xf1, 0xa0, 0x1a, 0x24, 0x39 } };

namespace {

// The info format is stored with \n line breaks; the multi-line edit wants \r\n.
static pfc::string8 toCtrl(const char* stored) {
	pfc::string8 out;
	for (const char* p = stored; *p; ++p) {
		if (*p == '\n' && (p == stored || p[-1] != '\r')) out += "\r\n";
		else out.add_byte(*p);
	}
	return out;
}
static pfc::string8 fromCtrl(const char* edited) {
	pfc::string8 out;
	for (const char* p = edited; *p; ++p) {
		if (*p != '\r') out.add_byte(*p);
	}
	return out;
}

class CPlaylistSearchPrefs : public CDialogImpl<CPlaylistSearchPrefs>, public preferences_page_instance {
public:
	CPlaylistSearchPrefs(preferences_page_callback::ptr callback) : m_callback(callback) {}

	enum { IDD = IDD_PREFS };

	t_uint32 get_state() override {
		t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
		if (HasChanged()) state |= preferences_state::changed;
		return state;
	}
	void apply() override {
		pfc::string8 text;
		uGetDlgItemText(*this, IDC_ROW_FORMAT, text);
		cfg_rowFormat = text;
		uGetDlgItemText(*this, IDC_INFO_FORMAT, text);
		cfg_infoFormat = fromCtrl(text);
		uGetDlgItemText(*this, IDC_TITLE_FORMAT, text);
		cfg_titleFormat = text;
		playlistSearchInvalidateIndex();
		OnChanged();
	}
	void reset() override {
		uSetDlgItemText(*this, IDC_ROW_FORMAT, default_rowFormat);
		uSetDlgItemText(*this, IDC_INFO_FORMAT, toCtrl(default_infoFormat));
		uSetDlgItemText(*this, IDC_TITLE_FORMAT, default_titleFormat);
		OnChanged();
	}

	BEGIN_MSG_MAP_EX(CPlaylistSearchPrefs)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_ROW_FORMAT, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_INFO_FORMAT, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM) {
		m_dark.AddDialogWithControls(*this);
		uSetDlgItemText(*this, IDC_ROW_FORMAT, cfg_rowFormat.get());
		uSetDlgItemText(*this, IDC_INFO_FORMAT, toCtrl(cfg_infoFormat.get()));
		uSetDlgItemText(*this, IDC_TITLE_FORMAT, cfg_titleFormat.get());
		return FALSE;
	}
	void OnEditChange(UINT, int, CWindow) {
		OnChanged();
	}
	bool HasChanged() {
		pfc::string8 text;
		uGetDlgItemText(*this, IDC_ROW_FORMAT, text);
		if (strcmp(text, cfg_rowFormat.get()) != 0) return true;
		uGetDlgItemText(*this, IDC_INFO_FORMAT, text);
		if (strcmp(fromCtrl(text), cfg_infoFormat.get()) != 0) return true;
		uGetDlgItemText(*this, IDC_TITLE_FORMAT, text);
		return strcmp(text, cfg_titleFormat.get()) != 0;
	}
	void OnChanged() {
		m_callback->on_state_changed();
	}

	const preferences_page_callback::ptr m_callback;
	fb2k::CDarkModeHooks m_dark;
};

class preferences_page_playlist_search : public preferences_page_impl<CPlaylistSearchPrefs> {
public:
	const char* get_name() override { return "Playlist Search"; }
	GUID get_guid() override { return guid_prefs_page; }
	GUID get_parent_guid() override { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_playlist_search> g_prefs_factory;

} // namespace
