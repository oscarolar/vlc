---
id: PKT-3014
title: "A shorter right-click menu with shortcuts shown and a top-level Hide interface entry"
section: "B. Tracks & menus"
video_ref: "05:13–05:30"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/menus
branch: master-pkt-3014-context-menu-cleanup-hide-ui
base_sha: ce4af2f
target_files:
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/menus/menus.hpp
  - modules/gui/qt/menus/popup_menu_layout.hpp                            # new, pure
  - modules/gui/qt/menus/popup_menu_layout.cpp                            # new, pure
  - modules/gui/qt/tests/test_popup_menu_layout.cpp                       # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 10.0
priority: P2
depends_on: [PKT-3013]
upstream_ruling: needed
target: fork
mr_title: "qt: tidy the video context menu and add a hide interface entry"
status: ready
---

# PKT-3014 — A shorter right-click menu with shortcuts shown and a top-level Hide interface entry

## 1. Scope & intent
**Before.** Right-clicking the video opens `VLCMenuBar::PopupMenu`: Play/Stop/Previous/Next/Record (no shortcuts
shown), Fullscreen, Audio ›, Video ›, Subtitle ›, Playback ›, Tools ›, View ›, Open Media ›, Quit. The
Playback submenu ends with **another** Speed submenu, Jump Forward/Backward, Jump to Time, Previous/Next
Title/Chapter **and a second copy of Play/Stop/Previous/Next/Record** (visible in the video, sheet `s006` frame 6,
and in master, §2). Hiding the interface exists (Minimal View, Ctrl+H) but is buried in View ›.

**After (05:13–05:30, sheet `s007` frame 1).** The same menu, cleaned:
Play `Space` · Stop `S` · Previous Item `P` · Next Item `N` · Record `R` — each with its **current hotkey** right
aligned — then **Hide UI** with its shortcut, then Playback › · Video › · Audio › · Subtitle › · More Settings ›
(Tools + View) · Playlist › · Open Media › · Quit VLC `Ctrl+Q`. The Playback submenu no longer repeats the
top-level transport entries. Styling (two-tone icons, accent) belongs to PKT-3001/3002, not here.

One MR, three commits:
1. `qt: describe the video context menu as a pure layout` — `popup_menu_layout.*` + QtTest.
2. `qt: stop repeating transport entries in the playback submenu` — the `NavigMenu` duplication.
3. `qt: show hotkeys and a hide interface entry in the context menu` — shortcuts, Hide UI, regrouping.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- Path of a right-click on embedded video: `modules/control/hotkeys.c:1098-1107` (`MouseButtonCallback`) sets
  `intf-popupmenu` → `modules/gui/qt/maininterface/mainctx.cpp:264-265` registers `PopupMenuCB`, `:963-979` forwards
  `INTF_DIALOG_POPUPMENU` → `modules/gui/qt/dialogs/dialogs_provider.cpp:206-218` builds
  `VLCMenuBar::PopupMenu(p_intf, true)`. Without embedded video (audio): `modules/gui/qt/player/qml/Player.qml:1361-1376`
  (`QmlAudioContextMenu` + right-button `TapHandler`) → `modules/gui/qt/menus/qml_menu_wrapper.cpp:798-809` → the
  same `PopupMenu(p_intf, false)`. One builder serves both.
- `modules/gui/qt/menus/menus.cpp:746-873` (`VLCMenuBar::PopupMenu`) — `:754` `PopupMenuPlaylistEntries`; `:763-767`
  Fullscreen when a vout exists; Audio/Video/Subtitle/Playback submenus (`:771-801`, Playback = `NavigMenu` at
  `:795`); `:805` `if( !b_isFullscreen )` where `b_isFullscreen` is declared `false` at `:752` and never set (dead
  branch guard); Tools/View submenus for the Qt interface (`:839-846`, the `else` of the skins-only "Interface" submenu);
  tail `:850-867`: Open Media › + Help › + Quit when Qt is only the dialog provider (skins), otherwise
  `PopupMenuStaticEntries` (`:685-705`: Open Media › + Quit) — the Qt interface has no Help › here.
- `modules/gui/qt/menus/menus.cpp:490-528` (`VLCMenuBar::NavigMenu`) — Title/Chapter/Program, bookmarks, renderer,
  then `:525` `PopupMenuControlEntries` (Speed, Jump ±, Jump to Time) and `:527` `RebuildNavigMenu`, which ends with
  `:547` `PopupMenuPlaylistEntries` → **Play/Stop/Previous/Next/Record repeated** inside Playback ›. The same
  `NavigMenu` feeds the classic Playback menu (`modules/gui/qt/menus/qml_menu_wrapper.cpp:437-442`) where the repetition is intended.
- `modules/gui/qt/menus/menus.cpp:577-624` (`PopupMenuPlaylistEntries`) — the transport actions, no shortcut set
  (contrast `addDPStaticEntry(…, "Ctrl+Q")` which shows one).
- `modules/gui/qt/menus/menus.cpp:342-347` — "&Minimal View" with `setShortcut("Ctrl+H")` inside `ViewMenu`;
  `modules/gui/qt/menus/qml/GlobalShortcuts.qml:50` — the actual Ctrl+H binding toggling `MainCtx.minimalView`;
  `modules/gui/qt/maininterface/mainctx.cpp:703-710` (`setMinimalView`); `modules/gui/qt/player/qml/MinimalView.qml`
  shows only the video surface and auto-hiding CSD — this is the "hide the UI" the designer asks for.
- Hotkey texts come from core options, not Qt: `key-play-pause` `src/libvlc-module.c:2592` (default "Space",
  `:2454`/`:2459` per platform), `key-stop` (`:2457`/`:2462` "s"), `key-next` `:2604`, `key-prev` `:2605`,
  `key-stop` `:2607`, `key-record` `:2714` (default "Shift+r", `:2539`). The menu reads them with `var_InheritString(p_intf, "key-…")` at build time; the first
  `\t`-separated binding is shown.

Settings / variables involved: `key-play-pause`, `key-stop`, `key-prev`, `key-next`, `key-record`
(`src/libvlc-module.c:2592`, `:2604`, `:2605`, `:2607`, `:2714`), `intf-popupmenu` (libvlc variable).
Mock boundary: none for the layout rows (pure function of a `PopupMenuContext` struct: `hasInput`, `hasVout`,
`isDialogProvider`, and a map action → hotkey string).

### Overlaps & contradictions
- **PKT-3013** reworks `QmlGlobalMenu` in `qml_menu_wrapper.cpp` and adds `app_menu_layout.*`; this packet copies
  that helper pattern and touches only `PopupMenu`/`NavigMenu`/`PopupMenuPlaylistEntries` in `menus.cpp`. Rebase
  after 3013.
- **PKT-3008** ("top bar kept in fullscreen, bars reveal on hover") and **PKT-3041** (layout editor) may want
  "Hide UI" to mean something else than Minimal View (e.g. hide bars but keep library); §5.2.
- **PKT-3026** (one-key bookmark) may add a Bookmark entry to this menu later; layout helper leaves room.
- **PKT-3035** (hotkeys dialog) changes where hotkeys are edited, not how they are read.

## 2b. Upstream state
Master already has every action of the designed menu and already lets the user hide the interface (Minimal View,
Ctrl+H) — so "add an option to completely hide the UI, with a shortcut too" is shipped; only its visibility in
the right-click menu is missing. The duplicated transport entries inside Playback › are a real master defect
(`modules/gui/qt/menus/menus.cpp:527` → `:547`). Shortcuts are not shown on the transport entries.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `popupMenuLayout` | `hasInput=true`, Qt interface | build | top-level ids: Play, Stop, Previous, Next, Record, Separator, HideInterface, Separator, Playback, Video, Audio, Subtitle, MoreSettings, Playlist, OpenMedia, Separator, Quit | `test_popup_menu_layout.cpp::testOrderWithInput` |
| 2 | `popupMenuLayout` | `hasInput=false` | build | Playback/Video/Audio/Subtitle absent; Play present (opens file when queue empty, `modules/gui/qt/menus/menus.cpp:584-590`) | `::testNoInput` |
| 3 | `popupMenuLayout` | hotkeys `{play:"Space\tMedia Play Pause", stop:"s"}` | build | Play shortcut text "Space", Stop "S" (first binding, display-cased) | `::testShortcutText` |
| 4 | `popupMenuLayout` | `key-record` empty | build | Record has no shortcut text, entry still present | `::testUnboundHotkey` |
| 5 | `popupMenuLayout` | `isDialogProvider=true` (skins) | build | layout equals today's dialog-provider tail (Interface ›, Open Media, Help, Quit) — skins unaffected | `::testDialogProviderUnchanged` |
| 6 | `playbackSubmenuLayout` | any | build | contains no Play/Stop/Previous/Next/Record id | `::testPlaybackSubmenuHasNoTransport` |
| 7 | classic Playback menu | — | open menu bar (F10) → Playback | transport entries still there (`NavigMenu` path for the bar unchanged) | MANUAL — note in MR |
| 8 | right-click on video | playing video | right-click → Hide UI | Minimal View on; Ctrl+H restores; screenshot of the menu | MANUAL — `pkt3014-context-menu.png` |

## 4. Fixtures & mocks
- Pattern to copy: the pure-helper QtTest PKT-3013 adds (`tests/test_app_menu_layout.cpp`), itself modelled on
  `tests/test_thread_runner.cpp` + `modules/gui/qt/meson.build:1384-1401`.
- Stubs: none; the hotkey map is passed in, `PopupMenu` fills it from `var_InheritString`.
- Sample media: any video for row 8.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): Fullscreen leaves the top level and Tools + View fold into "More Settings". Removing Fullscreen from the top level and folding Tools › + View › into
   "More Settings ›" reorganises a menu VideoLAN maintains for all desktop platforms; 
2. **RULED** RULED 2026-10-14 (R0): keep Ctrl+H; the entry is labelled "Hide Interface" and shows "Ctrl+H". "Hide UI" shortcut: the design shows Ctrl+\\ ; master binds Ctrl+H (`modules/gui/qt/menus/qml/GlobalShortcuts.qml:50`)
   for the same feature. Proposed: keep Ctrl+H, label the entry "Hide Interface", show "Ctrl+H".
3. NOTE — `b_isFullscreen` (`modules/gui/qt/menus/menus.cpp:752`) is dead; the new layout drops it rather than keeping a constant branch.
4. NOTE — `NavigMenu` keeps calling `RebuildNavigMenu` for the classic bar; only the context-menu Playback ›
   gets the transport-free variant (row 6 vs row 7).
5. NOTE — macOS shows no menu icons (`#ifndef __APPLE__` throughout `menus.cpp`); shortcut text is native QAction
   shortcut display, so it works there too.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`.
- Strings added are marked for translation (`qtr`) and `po/POTFILES.in` lists the new files.
- Keyboard navigation works in the menu and every entry keeps its accelerator.

### Evidence
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3014-context-menu-cleanup-hide-ui` from
      the merge of PKT-3013; §6 command → green.
- [ ] **Step 2: Dry-run** — re-read §2; open the menu once on a running build (nzxt, X forwarding or Xvfb +
      screenshot) to record today's tree in the MR as the "before" picture.
- [ ] **Step 3: Row 1 watched failing** — `menus/popup_menu_layout.hpp` (`PopupMenuContext`,
      `popupMenuLayout()`, `playbackSubmenuLayout()` returning `{}`), `tests/test_popup_menu_layout.cpp` row 1;
      register `test_qt_popup_menu_layout` in `meson.build` → FAIL.
- [ ] **Step 4: Rows 2–6** — implement; one TDD cycle per row. Commit 1
      `qt: describe the video context menu as a pure layout`.
- [ ] **Step 5: Duplication fix** — split `NavigMenu` so the context-menu Playback › skips `:527`'s
      `RebuildNavigMenu`→`PopupMenuPlaylistEntries`; keep the classic path. Row 7. Commit 2
      `qt: stop repeating transport entries in the playback submenu`.
- [ ] **Step 6: Shortcuts + Hide UI** — `PopupMenu` builds from the layout; transport actions get
      `setShortcut(QKeySequence(firstBinding(var_InheritString(p_intf, "key-…"))))` with
      `setShortcutContext(Qt::WidgetShortcut)` so the menu only displays them (the core hotkey still acts);
      Hide Interface toggles `MainCtx::setMinimalView`. Row 8. Commit 3
      `qt: show hotkeys and a hide interface entry in the context menu`.
- [ ] **Step 7: Whole gate** — §6 command; summary into §6 Evidence.
- [ ] **Step 8: MR** — Draft MR, §5.1–5.2 as recorded rulings; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
