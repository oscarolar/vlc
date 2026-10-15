---
id: PKT-3018
title: "Player settings open as a four-tab sidebar instead of the effects dialog"
section: "C. Sidebar"
video_ref: "06:30–07:16"
pipeline: CI
kind: ui
track: T2
component: modules/gui/qt/player
branch: master-pkt-3018-player-settings-sidebar
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/player/qml/PlayerSettingsSidebar.qml
  - modules/gui/qt/util/qml/RightPaneFSM.qml
  - modules/gui/qt/player/qml/controlbarcontrols/ExtendedSettingsButton.qml
  - modules/gui/qt/tests/tst_RightPaneFSM.qml
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 14.0
priority: P1
depends_on: [PKT-3010]
upstream_ruling: needed
target: fork
mr_title: "qml: add a player settings sidebar with four tabs"
status: ready
---

# PKT-3018 — Player settings open as a four-tab sidebar instead of the effects dialog

## 1. Scope & intent
Today the "Extended settings" button of the control bar opens **Adjustments and Effects**, a separate Qt Widgets
tool window with tabs *Audio Effects / Video Effects / Synchronization / v4l2 controls* and a *Save/Close* button
row. The designer calls it "Pandora's box" (06:34–06:53): half of what you need while watching (subtitle timing
among it) lives there, the window covers the video, so you change a value, close it, rewind, and repeat until it
is right. At 07:03–07:13 the proposal is to move those settings to the **right side of the player** as a sidebar
with four tabs. The frames at 07:12–07:20 and 08:59–09:15 (contact sheets `s009`, `s011`) show the target: a
dark panel docked to the right edge of the video, full height between the top bar and the seek bar, a tab row
**Playback · Video · Audio · Subtitles** with an icon per tab and an orange underline on the active one, then
grouped sections (*Defaults*, *Synchronization*, *Appearance*, *Filters*) with a label on the left and a value
control on the right. The video keeps playing beside it.

This packet is **the shell only**: the docked pane, the tab bar, one empty `Loader` page per tab, open/close
behaviour, keyboard navigation, and the rule that it and the play queue never occupy the right edge at the same
time. The content of each tab is a sibling packet (§5.4). The Widgets dialog is **kept** and stays reachable from
the menu (Tools → Effects and Filters, Ctrl+E) because it still owns compressor, spatializer, v4l2 and the
"Save" to config that no QML page has yet (§5.1).

One MR, three commits:
1. `qml: add RightPaneFSM to arbitrate the player right pane` — the pure state machine in `VLC.Util` and its
   `TestCase`.
2. `qml: add the player settings sidebar` — `PlayerSettingsSidebar.qml`, its placement in `Player.qml`.
3. `qml: open the settings sidebar from the extended settings button` — the button toggles the sidebar; the
   effects dialog is reached from the menu.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/controlbarcontrols/ExtendedSettingsButton.qml:26-36` (`extdSettingsBtn`) — icon
  `VLCIcons.effect_filter`, description "Extended settings", `onClicked: DialogsProvider.extendedDialog()`.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:96-122` — the *Default Style* video player layout
  puts `LANG_BUTTON, BOOKMARK_BUTTON, EXTENDED_BUTTON, NAVIGATION_BUTTONS` on the left of the bar, so the button
  this packet repurposes is visible by default.
- `modules/gui/qt/dialogs/extended/extended.cpp:42-146` (`ExtendedDialog::ExtendedDialog`) — `Qt::Tool` window
  (`:45-49`), title "Adjustments and Effects" (`:52`), audio sub-tabs Equalizer/Compressor/Expander/Limiter/
  Spatializer/Stereo Widener/Advanced (`:61-106`), Video Effects (`:109-118`), Synchronization (`:122-123`),
  v4l2 when the module exists (`:125-129`), Save/Close (`:135-146`).
- `modules/gui/qt/dialogs/extended/extended.cpp:148-158` — geometry restored from `EPanel/geometry`, or a
  400×280 window moved left of the main window: the "covers the video" complaint is this floating window.
- `modules/gui/qt/dialogs/dialogs_provider.cpp:312-320` (`DialogsProvider::extendedDialog`) — the toggle the
  button and `INTF_DIALOG_EXTENDED` (`:199`) call.
- `modules/gui/qt/player/qml/Player.qml:144-148` (`PlayerPlaylistVisibilityFSM playlistVisibility`) and
  `:1032-1140` (`Loader playlistpopup`) — the play queue docked on the **same right edge**: anchored
  top/right/bottom, `expanded`/`retracted` states animating `width`, `PlaylistPane` with
  `maximumWidth = (rootPlayer.width + rightPadding) / 2`, top padding that avoids the `TopBar`, and
  `Navigation.leftAction/cancelAction: closePlaylist`. The sidebar copies this placement.
- `modules/gui/qt/player/qml/PlayerPlaylistVisibilityFSM.qml:17-80` — the playlist FSM imports
  `VLC.MainInterface` and reads `MainCtx.playqueuePanel`; it cannot be instantiated in the QML test runner (see
  mock boundary), which is why the arbitration logic of this packet goes to a new `VLC.Util` component.
- `modules/gui/qt/player/qml/TracksMenu.qml:30-40,120-143` — the current tracks popup: fixed height
  `VLCStyle.dp(296)`, a `StackView` whose `trackMenuController` pushes `TracksPageAudio`, `TracksPageSubtitle`,
  `TracksPageSpeed`. These pages are the seed content the sibling packets move into the sidebar tabs.
- `modules/gui/qt/player/qml/controlbarcontrols/LangButton.qml:56-92` — the popup is parented to `rootPlayer`
  and placed at `positionSliderY - height`; `requestLockUnlockAutoHide(true)` while open. The sidebar must do
  the same lock so the control bar does not hide while it is open.
- `modules/gui/qt/meson.build:730-761` — `VLC.Player` module sources (new QML files are listed here);
  `:837-856` `VLC.Util` sources; `:954-956` `qml_tests` holds only `tst_FSM`.
- `modules/gui/qt/tests/qml_test.cpp:34-47` — the runner initialises **only** `util_assets` (`:38-41`) and runs
  offscreen: a `TestCase` can import `QtQuick`, `QtTest` and `VLC.Util`, nothing that needs `MainCtx`,
  `Player` or `VLCStyle`.
- `modules/gui/qt/tests/tst_FSM.qml:18-60` — the pattern: `TestCase` + an `FSM` with `signalMap`, events
  recorded in an array.

Settings / variables involved: none in core. One Qt setting: the sidebar width, stored like the queue width
(`MainCtx.playqueuePanel.width`) — the executor reads how that is persisted in Step 2.
Mock boundary: `RightPaneFSM` is driven by plain signals in the `TestCase`; nothing from `MainCtx` or `Player`.

### Overlaps & contradictions
- **PKT-3004** (the Playlist toggle becomes the Queue) and **PKT-3026** (bookmarks/chapters): frame `s017`
  (≈15:13) shows the queue pane itself with tabs *Queue · Bookmarks · Chapters*. Two right panes compete for
  `modules/gui/qt/player/qml/Player.qml:1032-1140`. This packet owns the arbitration (`RightPaneFSM`: `none | queue | settings`), PKT-3004
  and PKT-3026 consume it. Whoever merges second rebases on `Player.qml`.
- **PKT-3010/3011/3012** own the track lists opened from the bar buttons (`TracksMenu`); this packet does not
  delete `TracksMenu.qml`. The *Subtitles* tab may later embed the track list (07:16 frame shows "Embedded
  Tracks / External Tracks" in a pane) — decided in PKT-3010, not here.
- **PKT-3019 … 3025** fill the four tabs; each adds its page to the `Loader` this packet creates and must not
  edit the shell beyond that one line.
- **PKT-3013** (one menu button) may move "Effects and Filters" in the menu; the dialog entry point must survive.
- Upstream: VLC 4 already moved the tracks UI to QML (`TracksMenu`), but kept the effects in Widgets; there is no
  open upstream work moving the effects to QML that this packet would collide with (Step 2 checks
  `git log --oneline -- modules/gui/qt/dialogs/extended` for news).

## 2b. Upstream state
- Master ships the **content** in two places: the QML tracks popup (`TracksMenu` + `TracksPage*`, subtitle and
  audio delays, subtitle FPS, playback speed) and the Widgets *Adjustments and Effects* dialog (equalizer, audio
  filters, video filters, sync incl. subtitle duration factor). There is **no** docked settings pane in the player.
- The play queue is already a docked right pane in the player (`modules/gui/qt/player/qml/Player.qml:1032-1140`), so the layout pattern
  exists and is reused rather than invented.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `RightPaneFSM` | started, nothing open | `toggleSettings()` | `current === "settings"`, `settingsVisible`, `!queueVisible` | `tst_RightPaneFSM.qml::test_toggle_settings_opens` |
| 2 | `RightPaneFSM` | `settings` open | `toggleSettings()` | `current === "none"` | `tst_RightPaneFSM.qml::test_toggle_settings_closes` |
| 3 | `RightPaneFSM` | `settings` open | `toggleQueue()` | `current === "queue"`: opening one closes the other, never both | `tst_RightPaneFSM.qml::test_queue_replaces_settings` |
| 4 | `RightPaneFSM` | `queue` open | `openSettings("subtitles")` | `current === "settings"`, `settingsTab === "subtitles"` | `tst_RightPaneFSM.qml::test_open_on_tab` |
| 5 | `RightPaneFSM` | `settings` open | `close()` (Esc / cancel action) | `current === "none"`, `settingsTab` kept for the next open | `tst_RightPaneFSM.qml::test_close_keeps_last_tab` |
| 6 | `RightPaneFSM` | queue undocked (floating window) | `toggleQueue()` | state stays `none` / `settings`: a floating queue never claims the right edge | `tst_RightPaneFSM.qml::test_floating_queue_ignored` |
| 7 | `PlayerSettingsSidebar` in `Player.qml` | a video playing | Extended settings button clicked | pane slides in from the right, four tabs Playback/Video/Audio/Subtitles, video still visible, control bar does not auto-hide | MANUAL — screenshot `pkt3018-sidebar-open.png` |
| 8 | same | sidebar open | Tab / arrows, Esc | focus walks tab bar → page → back; Esc closes and returns focus to the control bar | MANUAL — screen recording `pkt3018-keyboard.webm` |
| 9 | menu | sidebar shipped | Tools → Effects and Filters (Ctrl+E) | the Widgets dialog still opens | MANUAL — screenshot `pkt3018-effects-dialog.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/tst_FSM.qml:18-60` (a `TestCase` driving an `FSM` from `VLC.Util`).
- `RightPaneFSM.qml` lives in `modules/gui/qt/util/qml/` and imports only `QtQuick` and `VLC.Util` (`FSM`,
  `FSMState`), so the runner of `modules/gui/qt/tests/qml_test.cpp:34-47` can load it. Register it in `VLC.Util` sources
  (`modules/gui/qt/meson.build:837-856`) and the test in `qml_tests` (`modules/gui/qt/meson.build:954-956`) as
  `'tst_RightPaneFSM': files('tests/tst_RightPaneFSM.qml')`.
- Stubs: none. Sample media: none (MANUAL rows use any local video).

## 5. Design decisions & open questions
1. **RULED — the effects dialog is not removed.** RULED 2026-10-14 (R0): the effects dialog is kept; only the bar button opens the sidebar (fallback: a new `SETTINGS_SIDEBAR_BUTTON`, leaving `EXTENDED_BUTTON` untouched). The video replaces it; this packet only stops opening it
   from the bar button. Removing `dialogs/extended/` would drop compressor, expander, limiter, spatializer,
   stereo widener, pitch shifter, panner, v4l2 and "Save" to config (`modules/gui/qt/dialogs/extended/extended.cpp:61-146`) that no sibling
   packet ports. the button change is ruled acceptable (some users bind this button to the
   dialog on purpose); fallback: a new `SETTINGS_SIDEBAR_BUTTON` in `ControlListModel` and the default profile
   switched to it, leaving `EXTENDED_BUTTON` untouched.
2. **RULED — one right pane at a time.** RULED 2026-10-14 (R0): one right pane at a time, enforced by `RightPaneFSM`. Queue and settings share the right edge; the design never shows both.
   `RightPaneFSM` enforces exclusivity. Alternative (both side by side) shrinks the video too much below
   ~1280 px; 
3. NOTE — overlay vs. resize: the queue pane overlays the video (`modules/gui/qt/player/qml/Player.qml:1032-1060` anchors on top of it).
   The sidebar does the same; resizing the video to the left is a later option, not this packet.
4. NOTE — tab ids are fixed strings `playback | video | audio | subtitles`; each sibling packet adds its page as
   `Component` for its id. Tab order follows the 09:07 frame (Playback, Video, Audio, Subtitles).
5. NOTE — the pane must lock auto-hide like `modules/gui/qt/player/qml/controlbarcontrols/LangButton.qml:76-85` (`requestLockUnlockAutoHide`), otherwise the
   control bar fades while the user is dragging a slider in the pane.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`.
- Strings added are marked for translation (`qsTr` / `qtr`) and `po/POTFILES.in` is updated when a file is new.
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on every new control.

### Evidence
<Filled by the executor: build tail, `meson test` summary, screenshots, MR URL.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc` (no rsync from the Mac): `git switch -c master-pkt-3018-player-settings-sidebar ce4af2f`; build once with the §6 command → green before any change.
- [ ] **Step 2: Dry-run** — re-read every range of §2; check `git log --oneline ce4af2f..origin/master -- modules/gui/qt/player/qml/Player.qml modules/gui/qt/dialogs/extended` for upstream moves; read how `MainCtx.playqueuePanel.width` is persisted and copy it for the sidebar width.
- [ ] **Step 3: First row watched failing** — write `tests/tst_RightPaneFSM.qml` row 1 against an empty `RightPaneFSM.qml` (register both in `meson.build`) → FAIL; then the real FSM.
- [ ] **Step 4: Rows 2–6** — one TDD cycle each in `RightPaneFSM.qml`; commit 1 `qml: add RightPaneFSM to arbitrate the player right pane`.
- [ ] **Step 5: Sidebar** — `player/qml/PlayerSettingsSidebar.qml` (tab bar with `Widgets` tab buttons, four `Loader`s with placeholder pages), placed in `Player.qml` next to `playlistpopup`, both driven by `RightPaneFSM`; add the file to `VLC.Player` in `modules/gui/qt/meson.build:730-761` and to `po/POTFILES.in`; rows 7–8 screenshots; commit 2 `qml: add the player settings sidebar`.
- [ ] **Step 6: Button** — `ExtendedSettingsButton.qml` toggles the sidebar (or the new button of §5.1, per the ruling); row 9 screenshot; commit 3 `qml: open the settings sidebar from the extended settings button`.
- [ ] **Step 7: Whole gate** — the §6 command; paste the summary in §6 Evidence.
- [ ] **Step 8: Commit and MR** — push to the fork, PR to the fork github.com/oscarolar/vlc (base `master`) with the §5.1/§5.2 questions in the description; `status: review`.
