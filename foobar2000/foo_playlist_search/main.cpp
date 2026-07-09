#include "stdafx.h"

DECLARE_COMPONENT_VERSION("Playlist Search", "0.1",
	"Hotkey-summoned search window for the active playlist.\n"
	"Bind 'View / Quick playlist search' to a keyboard shortcut in Preferences > Keyboard Shortcuts.");

VALIDATE_COMPONENT_FILENAME("foo_playlist_search.dll");

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
