---
id: PKT-3013
title: "One menu button with app-level entries instead of a second copy of the right-click menu"
section: "B. Tracks & menus"
video_ref: "04:55–05:13"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/menus
branch: master-pkt-3013-single-app-menu-button
base_sha: ce4af2f
target_files:
  - modules/gui/qt/menus/qml_menu_wrapper.cpp
  - modules/gui/qt/menus/qml_menu_wrapper.hpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/menus/menus.hpp
  - modules/gui/qt/menus/app_menu_layout.hpp                              # new, pure
  - modules/gui/qt/menus/app_menu_layout.cpp                              # new, pure
  - modules/gui/qt/tests/test_app_menu_layout.cpp                         # new
  - modules/gui/qt/meson.build
estimate_h: 8.0
priority: P2
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qt: make the global menu button an application menu"
status: ready
---

# PKT-3013 — One menu button with app-level entries instead of a second copy of the right-click menu

## 1. Scope & intent
**What the video shows (04:55–05:13, sheet `s006` frames 5–7).** VLC 3: a classic menu bar
(Media · Playback · Audio · Video · Subtitle · Tools · View · Help) whose content repeats the right-click menu.
Redesign: no menu bar; one "⋯" button next to the cone at the top-left opens a flat **application menu**: Open File
(Ctrl+O), Open Multiple Files (Ctrl+Shift+O), Open Directory (Ctrl+F), Open Disc (Ctrl+D), Open Network Stream
(Ctrl+N), Open Capture Device (Ctrl+C), Open Location from Clipboard (Ctrl+V), Open Recent ›, Save Playlist to
File (Ctrl+Y), Convert / Save (Ctrl+R), Stream (Ctrl+S), Customize Player, Plugins & Extensions ›, Settings
(Ctrl+P); a later frame (sheet `s018`, frame 8) adds Keyboard Shortcuts and About VLC (Shift+F1). Playback,
Audio, Video and Subtitle no longer appear there: they live on the bar (PKT-3010/3011) and in the right-click
menu (PKT-3014).

**Master is already halfway (§2b).** The menu bar is off by default and a "⋯" button already exists — but it opens
`QmlGlobalMenu`, which is the whole menu bar folded into eight submenus, i.e. the duplication the designer
complains about, one level deeper. This packet changes **what that button shows**, not where it is.

**After.** The "⋯" button of the media-library top bar and of the player top bar opens the flat application menu
above. The classic menu bar (`qt-menubar`, F10) keeps its eight menus untouched, so nothing is lost for users who
turn it on.

One MR, two commits:
1. `qt: describe the application menu as a pure layout` — `app_menu_layout.*` + QtTest.
2. `qt: make the global menu button an application menu` — `QmlGlobalMenu::popup` builds from the layout.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- `modules/gui/qt/qt.cpp:261-262` (`QT_MENUBAR_TEXT` "Show the menu bar") and `:425` —
  `add_bool( "qt-menubar", false, … )`: the classic menu bar is **off by default** in master.
- `modules/gui/qt/maininterface/mainctx.cpp:444` loads it into `m_hasToolbarMenu`; `:607-617`
  (`MainCtx::setHasToolbarMenu`) persists it with `config_PutInt`; `:872-875` (`toggleToolbarMenu`), bound to F10
  in `modules/gui/qt/menus/qml/GlobalShortcuts.qml:48`.
- `modules/gui/qt/maininterface/qml/MainDisplay.qml:229-240` — `MenuTopbar` visible only when
  `MainCtx.hasToolbarMenu`.
- `modules/gui/qt/maininterface/qml/LocalTopbar.qml:198-219` (`menuBtn`) — `VLCIcons.more`, visible when
  `!MainCtx.hasToolbarMenu` (`:201`), opens `Menus.QmlGlobalMenu` (`:214-217`).
- `modules/gui/qt/player/qml/TopBar.qml:437-480` (`menuSelector`) — same button and same `QmlGlobalMenu` in the
  player, with auto-hide lock while open (`:466-470`).
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:244-294` (`QmlGlobalMenu::popup`) — builds Media (`FileMenu`),
  Playback (`NavigMenu`), Audio, Video, Subtitle, Tools, View, Help submenus: the menu bar, folded.
- `modules/gui/qt/menus/menus.cpp:174-238` (`VLCMenuBar::FileMenu`) — every entry of the design's first block, with
  the same shortcuts (Ctrl+O `:181`, Ctrl+Shift+O `:183`, Ctrl+F `:185`, … Ctrl+R `:213`, Ctrl+S `:215`), plus
  "Open Recent Media" only when `save-recentplay` and a media library exist (`:196-205`), "Quit at the end of play
  queue", "Close to systray" and Quit.
- `modules/gui/qt/menus/menus.cpp:241-276` (`VLCMenuBar::ToolsMenu`) — "Plu&gins and extensions" (`:266-267`),
  "Customi&ze Interface..." (`:270-272`), "&Preferences" Ctrl+P (`:274-275`).
- `modules/gui/qt/menus/menus.cpp:553-571` (`VLCMenuBar::HelpMenu`) — Help F1, Check for Updates, About Shift+F1.
- `modules/gui/qt/menus/qml_menu_wrapper.hpp:150-174` (`QmlGlobalMenu`: `ctx`, `shown`, `popup(QPoint)`, `close()`)
  — the QML-facing API stays identical, so `LocalTopbar.qml` and `TopBar.qml` need no change.
- `modules/gui/qt/player/qml/controlbarcontrols/MenuButton.qml:27-54` — a control-bar "⋯" opening the same
  `QmlGlobalMenu`; its id `MENU_BUTTON` is marked `// deprecated` (`modules/gui/qt/player/control_list_model.hpp:62`) and has no
  entry in `PlayerControlbarControls.qml`, so it is unreachable; left as is.
- `modules/gui/qt/tests/` has no menu test; `QmlGlobalMenu::popup` needs a live `qt_intf_t` and `MainCtx`
  (`:246-251`), which `VLCTestingEnv` does not provide ("no MainCtx", `tests/vlc_stub_modules.hpp`). Hence the
  pure layout helper.

Settings / variables involved: `qt-menubar` (`modules/gui/qt/qt.cpp:425`), `save-recentplay`
(`src/libvlc-module.c:1926`), `qt-autoload-extensions` (read in `modules/gui/qt/menus/menus.cpp:384`).
Mock boundary: none — `appMenuLayout(const AppMenuContext&)` is a pure function of booleans.

### Overlaps & contradictions
- **PKT-3014** edits `menus.cpp` (`PopupMenu`, `NavigMenu`) — different functions, same file; rebase carefully.
  The layout-helper pattern introduced here is reused by 3014 (`popup_menu_layout.*`).
- **PKT-3035** adds the "Keyboard Shortcuts" dialog and its menu entry; this packet reserves the slot (an
  `AppMenuEntry::KeyboardShortcuts` hidden while no dialog exists) and 3035 turns it on.
- **PKT-3034** splits the open-media dialog; the "Open …" entries call whatever `DialogsProvider` slots exist then.
- **PKT-3009** puts fullscreen/PiP buttons in the same top bar as the "⋯" button; no file shared.
- Upstream: the VLC 4 top bar already chose "⋯ = whole menu bar". Narrowing it is a VideoLAN design call.

## 2b. Upstream state
Already shipped in master: no menu bar by default (`modules/gui/qt/qt.cpp:425`), a single "⋯" button in both top bars, F10 to bring
the classic bar back. The video's complaint ("this top menu is the same as the right-click menu") describes VLC 3.
What remains is the content: master's "⋯" still mirrors the menu bar (`modules/gui/qt/menus/qml_menu_wrapper.cpp:244-294`). The packet is
narrowed to that content change.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `appMenuLayout` | default context (ML on, save-recentplay on) | build | ids in order: OpenFile, OpenMultiple, OpenDirectory, OpenDisc, OpenNetwork, OpenCapture, OpenClipboard, OpenRecent, Separator, SavePlaylist, ConvertSave, Stream, Separator, CustomizePlayer, Plugins, Settings, Separator, About | `test_app_menu_layout.cpp::testDefaultOrder` |
| 2 | `appMenuLayout` | no media library | build | no OpenRecent (as `modules/gui/qt/menus/menus.cpp:196`) | `::testNoRecentWithoutMediaLibrary` |
| 3 | `appMenuLayout` | any | build | no Playback/Audio/Video/Subtitle/View submenu id appears | `::testNoPlayerSubmenus` |
| 4 | `appMenuLayout` | any | build | shortcuts equal FileMenu's (Ctrl+O … Ctrl+S, Ctrl+P, Shift+F1) | `::testShortcutsMatchFileMenu` |
| 5 | `appMenuLayout` | `hasHotkeysDialog=false` / `true` | build | KeyboardShortcuts absent / present before CustomizePlayer | `::testKeyboardShortcutsSlot` |
| 6 | `appMenuLayout` | `isDialogProvider=true` | build | CustomizePlayer absent (as `modules/gui/qt/menus/menus.cpp:270`) | `::testDialogProviderHasNoCustomize` |
| 7 | "⋯" in library and player | — | click | the flat menu of row 1; each entry opens its dialog; Esc closes | MANUAL — screenshot `pkt3013-app-menu.png` |
| 8 | F10 | — | press F10 | classic menu bar still has its 8 menus | MANUAL — note in MR |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_thread_runner.cpp` and its meson block `modules/gui/qt/meson.build:1384-1401` (block opens at `:1384`)
  (QtTest with no libvlc; `qt6_dep` + `qtest_qt6_dep` only).
- Stubs: none; `AppMenuContext` is a plain struct (`hasMediaLibrary`, `saveRecentPlay`, `isDialogProvider`,
  `hasHotkeysDialog`, `hasUpdateCheck`).
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): the narrow "⋯" menu is accepted; Playback/Audio/Video/Subtitle/View stay in the right-click menu and the menu bar (F10). Narrowing "⋯" drops quick access to Playback/Audio/Video/Subtitle/View from the top bar. They stay
   in right-click and the menu bar (F10). accepted; the alternative was: keep them as a last "Player ›"
   submenu.
2. **RULED** RULED 2026-10-14 (R0): Quit stays last (Ctrl+Q); "Quit at the end of play queue" and "Close to systray" move to PKT-3025's playback tab / settings. Entries the design omits but master has in `FileMenu`: "Quit at the end of play queue", "Close to
   systray", "Quit". Proposed: keep Quit last (Ctrl+Q), move the other two to PKT-3025's playback tab / settings.
3. NOTE — "Settings" is the design's wording for "&Preferences" (`modules/gui/qt/menus/menus.cpp:274`); the label changes only in this
   menu, the classic Tools menu keeps "Preferences".
4. NOTE — `QmlGlobalMenu`'s QML API is unchanged, so no QML file is touched; `MenuButton.qml` (deprecated) follows
   automatically.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`.
- Strings added are marked for translation (`qtr`); new C++ files listed in `po/POTFILES.in`.
- Keyboard navigation works in the menu (QMenu default) and every action has its accelerator.

### Evidence
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3013-single-app-menu-button` from
      `ce4af2f` (or the merge of PKT-3003); §6 command → green.
- [ ] **Step 2: Dry-run** — re-read §2; list every `addDPStaticEntry` of `FileMenu`/`ToolsMenu`/`HelpMenu` with its
      shortcut into the test's expected table.
- [ ] **Step 3: Row 1 watched failing** — `menus/app_menu_layout.hpp` with `enum class AppMenuEntryId`,
      `struct AppMenuEntry { id; QString shortcut; }`, `QList<AppMenuEntry> appMenuLayout(const AppMenuContext&)`
      returning `{}`; `tests/test_app_menu_layout.cpp` row 1; register `test_qt_app_menu_layout` after
      `modules/gui/qt/meson.build:1401` → FAIL.
- [ ] **Step 4: Rows 2–6** — implement; one TDD cycle per row. Commit 1
      `qt: describe the application menu as a pure layout`.
- [ ] **Step 5: Wire it** — `QmlGlobalMenu::popup` (`modules/gui/qt/menus/qml_menu_wrapper.cpp:244-294`) iterates the layout and maps
      each id to the existing `addDPStaticEntry`/`RecentMenu`/`ExtensionsMenu` call (factor the single-entry
      helpers out of `FileMenu`/`ToolsMenu` in `menus.cpp` so the classic menus keep using them unchanged). Rows 7–8
      by hand. Commit 2 `qt: make the global menu button an application menu`.
- [ ] **Step 6: Whole gate** — §6 command; summary into §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`), §5.1–5.2 as recorded rulings; `status: review`.
