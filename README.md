# foo_playlist_search

A [foobar2000](https://www.foobar2000.org/) component that adds a hotkey-summoned quick search window for the active playlist.

<img width="1047" height="614" alt="image" src="https://github.com/user-attachments/assets/53858de1-6082-4866-a6ae-4b7e5d33d086" />

## Features

- Modeless popup dialog, bound to a keyboard shortcut of your choice
- Live filtering as you type — case- and accent-insensitive, all words must match
- Arrow keys / PgUp / PgDn navigate the result list while the caret stays in the search box
- Enter or double-click plays the focused track and closes the window
- Info pane showing details of the focused track
- Row and info pane formats are configurable with title formatting (Preferences > Tools > Playlist Search)
- Dark mode support, remembers window size and position
- Does not summon foobar2000 into focus when playing a song

## Installation

1. Grab `foo_playlist_search.fb2k-component` from the [Releases](../../releases) page (contains both 32-bit and 64-bit binaries).
2. In foobar2000, go to _Preferences > Components > Install..._, pick the file and restart.
3. Bind a shortcut to _View / Quick playlist search_ in _Preferences > Keyboard Shortcuts_ (the command is hidden from the menu by default).

## Building

1. Download the [foobar2000 SDK](https://www.foobar2000.org/SDK) and extract it into the repository root (it provides `foobar2000/`, `pfc/`, `libPPUI/`).
2. Place [WTL](https://sourceforge.net/projects/wtl/) in `wtl/` and add its `Include` directory to your MSBuild user property sheets (or the project's include paths).
3. Open `foobar2000/foo_playlist_search/foo_playlist_search.sln` in Visual Studio and build Release for Win32 and/or x64.

## License

[MIT](LICENSE) — by [HASJ](https://github.com/HASJ).
