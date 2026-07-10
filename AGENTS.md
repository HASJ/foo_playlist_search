# Repository Guidelines

## Project Structure & Module Organization

The component implementation lives in `foobar2000/foo_playlist_search/`. Keep UI and search behavior in `playlist_search.cpp`/`.h`, preference-page logic in `preferences.cpp`, component metadata in `main.cpp`, and Windows resources in `foo_playlist_search.rc` and `resource.h`. The solution also references the foobar2000 SDK under `foobar2000/`, plus `pfc/`, `libPPUI/`, and `wtl/`; these are external dependencies, not primary project code. Generated `Release*/`, `Debug*/`, `x64/`, `.vs/`, and `*.fb2k-component` files are ignored and should not be committed.

## Build, Test, and Development Commands

Run builds from a Visual Studio Developer Command Prompt with the v142 C++ toolset and Windows 10 SDK installed:

```powershell
msbuild foobar2000/foo_playlist_search/foo_playlist_search.sln /m /p:Configuration=Debug /p:Platform=x64
msbuild foobar2000/foo_playlist_search/foo_playlist_search.sln /m /p:Configuration=Release /p:Platform=x86
msbuild foobar2000/foo_playlist_search/foo_playlist_search.sln /m /p:Configuration=Release /p:Platform=x64
```

Use Debug x64 for routine development. Build both Release architectures before publishing a component. Follow `README.md` to place the foobar2000 SDK and WTL headers correctly.

## Coding Style & Naming Conventions

Match the existing C++17 style: tabs for indentation, braces on the same line, and compact guard clauses. Use `PascalCase` for classes, `camelCase` for functions, `m_` for members, `cfg_` for persisted settings, and `guid_` for GUID constants. Keep SDK/WTL message-map patterns consistent with neighboring code. No repository formatter is configured; avoid unrelated formatting changes.

## Testing Guidelines

There is no automated test suite or coverage target. Treat a warning-free Debug and Release build as the baseline check. Manually install the resulting DLL/component in matching 32-bit or 64-bit foobar2000, then verify popup opening, live filtering, keyboard navigation, playback, preferences persistence, resizing, and dark mode. Describe the tested architecture and foobar2000 version in the pull request.

## Commit & Pull Request Guidelines

Existing history uses short, imperative subjects such as `Add playlist search component source`; follow that style and keep each commit focused. Pull requests should explain user-visible behavior, list build and manual-test results, and link relevant issues. Include screenshots for UI or preference-dialog changes, and call out SDK, resource, or compatibility changes explicitly.
