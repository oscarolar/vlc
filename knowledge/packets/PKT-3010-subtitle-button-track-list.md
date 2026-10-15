---
id: PKT-3010
title: "A subtitle button opens the subtitle track list in one click"
section: "B. Tracks & menus"
video_ref: "04:06–04:35"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/player
branch: master-pkt-3010-subtitle-button-track-list
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/control_list_model.hpp
  - modules/gui/qt/player/qml/PlayerControlbarControls.qml
  - modules/gui/qt/player/qml/controlbarcontrols/SubtitleButton.qml        # new
  - modules/gui/qt/player/qml/TracksMenu.qml
  - modules/gui/qt/player/qml/TracksTabPage.qml                           # new
  - modules/gui/qt/util/qml/TrackTabsModel.qml                            # new, pure logic, testable
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp
  - modules/gui/qt/tests/tst_TrackTabs.qml                                # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 12.0
priority: P1
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qml: add a subtitle button that opens the subtitle track list"
status: ready
---

# PKT-3010 — A subtitle button opens the subtitle track list in one click

## 1. Scope & intent
**Before (master `ce4af2f`).** The video player's left control group starts with one "Languages and tracks"
button (`LangButton`, icon `VLCIcons.audiosub`). It opens `TracksMenu`, a 296 dp high popup anchored at the
left edge of the player, whose first page is three side-by-side columns — Subtitle, Audio, Video Tracks — plus a
playback-speed button. Picking a subtitle is two clicks, but the popup is wide, mixes three track kinds and sits
bottom-left, where (04:22–04:31) the designer says people do not look for it.

**After.** A dedicated **subtitle button** sits on the right group of the control bar, next to the audio button
of PKT-3011. One click opens a compact, tabbed popup anchored above the button, with the **Subtitles** tab
selected (sheet `s006`, first frame, ~04:30):

- header: tab strip with an icon + "Subtitles" label, the active tab underlined in the accent colour;
- section "Embedded Tracks": "None" then each track ("Track 1 - English"), the selected one with an accent check;
- section "External Tracks" with a "+" button on its right (adds a subtitle file; the richer add dialog is PKT-3017);
- footer: a full-width "Show Preferences" button (opens the extended settings for now; PKT-3018 re-targets it to
  the sidebar);
- the button itself is shown in the accent colour while the popup is open.

The tab strip, the popup and its anchoring are built here once and reused by PKT-3011 (Audio tab) and PKT-3012
(Video tab). The current three-column page (`TracksListPage`) stays reachable from `LangButton`, which is not
removed — users with a custom layout keep it.

One MR, three commits:
1. `qt: add a subtitle track button control id` — `SUBTITLE_BUTTON` takes the reserved slot `0x1F`.
2. `qml: add a tabbed track popup with a subtitle tab` — `TrackTabsModel`, `TracksTabPage`, `SubtitleButton`, the
   QML test.
3. `qt: put the subtitle button in the default video layout` — `controlbar_profile_model.cpp`.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`. Step 2 re-reads every range.

- `modules/gui/qt/player/qml/controlbarcontrols/LangButton.qml:27-93` (`LangButton`) — `IconToolButton` with
  `text: VLCIcons.audiosub` (`:46`), description "Languages and tracks" (`:50`); `:56` `onClicked:
  menuLoader.item.open()`; `:60-87` a `Loader` whose `TracksMenu` is parented to `rootPlayer` (`:32-37`), placed
  at `x: 0`, `y: parent.positionSliderY - height` (`:68-69`) — i.e. always flush left above the seek bar; `:76-85`
  locks the control-bar auto-hide while open (`requestLockUnlockAutoHide`).
- `modules/gui/qt/player/qml/TracksMenu.qml:30-144` (`TracksMenu`, a `T.Popup`) — width from the current page's
  `preferredWidth` capped by the parent (`:34-38`), fixed height `VLCStyle.dp(296)` (`:40`); content is a
  `T.StackView` (`:72-118`) whose `initialItem` is `TracksListPage` (`:86-88`); `:120-143` the
  `trackMenuController` that pushes `TracksPageAudio`/`TracksPageSubtitle`/`TracksPageSpeed` (sync pages).
  `:29` carries `// FIXME: Keyboard navigation needs to be fixed for this Popup.`
- `modules/gui/qt/player/qml/TracksListPage.qml:70-107` — the three `TrackColumn`s bound to
  `Player.subtitleTracks`, `Player.audioTracks`, `Player.videoTracks`; `:109-254` the `TrackColumn` component:
  header label + menu button, a `ListViewExt` of `Widgets.CheckedDelegate` that two-way binds `model.checked`
  (`:223-251`). This delegate is the one to reuse for every tab.
- `modules/gui/qt/player/qml/TracksListPage.qml:256-273` (`QmlSubtitleMenu`) — "Open file" →
  `DialogsProvider.loadSubtitlesFile()`, "Synchronize" → sync page, "Search online" → `Player.openVLsub()`.
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:743-776` (`QmlSubtitleMenu::beforePopup`) — builds those three
  actions plus a checkable "Select multiple" toggling `TrackListModel::multiSelect`.
- `modules/gui/qt/player/player_controller.hpp:162-164` — `videoTracks`, `audioTracks`, `subtitleTracks`
  (`TrackListModel*`, CONSTANT). `modules/gui/qt/player/input_models.hpp:44` `class TrackListModel` exposes
  `count` and `multiSelect` (`:47-48`).
- `modules/gui/qt/player/control_list_model.hpp:37-110` (`ControlListModel::ControlType`) — `:37-38` the rule:
  never reorder or insert before the end, reuse a `RESERVED_0x__`; `:61` `LANG_BUTTON`; `:72` `RESERVED_0X1F`;
  `:113-114` the `static_assert`s that pin `0x1F` and `0x2B`.
- `modules/gui/qt/player/qml/PlayerControlbarControls.qml:55` — `LANG_BUTTON` entry, label text "Open subtitles"
  (it actually opens all tracks; NOTE in §5).
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:95-122` — "Default Style" video layout: left
  `LANG_BUTTON, BOOKMARK_BUTTON, EXTENDED_BUTTON, NAVIGATION_BUTTONS`, centre skip/prev/play/next/skip, right
  `VOLUME, RENDERER_BUTTON, FULLSCREEN_BUTTON`. `:128-133` the audio-player layout also starts with `LANG_BUTTON`.
- `modules/gui/qt/player/qml/Player.qml:41` — `positionSliderY`, the y the popup anchors on today.
- `modules/gui/qt/meson.build:954-956` (`qml_tests`) and `:1426-1444` (the `qml_test` runner).
  `modules/gui/qt/tests/qml_test.cpp:34-46` initialises **only** `util_assets`: a `TestCase` can import
  `VLC.Util`, not `VLC.Player` (whose `Player` singleton is a C++ object registered by `mainui.cpp`). The tab
  logic therefore lives in `VLC.Util` (`util/qml/`, module block `modules/gui/qt/meson.build:837-856`) and is model-agnostic.

Settings / variables involved: none new. Subtitle track selection goes through `TrackListModel::setData`
(already used by the delegate). `sub-track`/`sub-language` (`src/libvlc-module.c`) are untouched.
Mock boundary: the QML `TestCase` feeds `TrackTabsModel` plain `ListModel`s with `display`/`checked` roles in
place of `TrackListModel`.

### Overlaps & contradictions
- **PKT-3003** owns the default layout (`controlbar_profile_model.cpp`); this packet only inserts
  `SUBTITLE_BUTTON` into the right group of whatever 3003 merged. Rebase on 3003 and touch only that line.
- **PKT-3011 / PKT-3012** reuse `TracksTabPage` and `TrackTabsModel` and add the Audio and Video tabs. They take
  the next reserved id (`RESERVED_0X2B` for `AUDIO_BUTTON`); this packet must not take it.
- **PKT-3017** replaces the "+" of "External Tracks" with its add-subtitles dialog; here "+" calls
  `DialogsProvider.loadSubtitlesFile()` (same as today's "Open file").
- **PKT-3018** turns "Show Preferences" into "open the sidebar"; here it calls `DialogsProvider.extendedDialog()`.
- **PKT-3001** supplies the accent colour token; until it lands the check and underline use `theme.accent`.
- Upstream direction: the VLC 4 design deliberately groups all tracks behind one left-side button (`LangButton`).
  Adding a second, subtitle-only button on the right is a design change → `upstream_ruling: needed`.

## 2b. Upstream state
Master already lets the user pick a subtitle in two clicks (LangButton → subtitle column) and already lists
embedded and external subtitle tracks in one `TrackListModel` (external ones are just more rows). What is missing:
a subtitle-only entry point on the right, a popup anchored to its button, a tab strip, and the
embedded/external split. The split needs to know whether a track is external: `TrackListModel` has no such role
today (Step 2 checks `vlc_player_track`'s `fmt.i_id`/`es_id` origin; if the information is not reachable, the
packet ships a single "Tracks" section and §5.3 records it).

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `TrackTabsModel` | subtitle model with 3 rows, audio and video models empty, `requested: "subtitle"` | the model is built | `tabs` is `["subtitle"]`, `currentTab` is `"subtitle"` | `tst_TrackTabs.qml::test_subtitle_only` |
| 2 | `TrackTabsModel` | audio 2 rows, subtitle 1 row, `requested: "subtitle"` | built | tab order is `["audio","subtitle"]` (Video, Audio, Subtitles order of the design), current `"subtitle"` | `tst_TrackTabs.qml::test_order_and_current` |
| 3 | `TrackTabsModel` | `requested: "subtitle"`, subtitle model 0 rows | built | the subtitle tab is still present (so "+" stays reachable) | `tst_TrackTabs.qml::test_empty_subtitle_tab_kept` |
| 4 | `TrackTabsModel` | subtitle rows where `checked` is false everywhere | `noneChecked("subtitle")` | `true` (drives the check on "None") | `tst_TrackTabs.qml::test_none_checked` |
| 5 | `ControlListModel` | — | build | `SUBTITLE_BUTTON == 0x1F` (`static_assert` next to `:113`) | compile-time |
| 6 | `SubtitleButton` + popup | video with 2 subtitle tracks | click the button | popup above the button, Subtitles tab, "None" + 2 rows; picking a row selects it and closes nothing else | MANUAL — screenshot `pkt3010-subtitle-popup.png` |
| 7 | keyboard | popup open | Tab / arrows / Esc | focus walks tab strip → rows → "+" → footer; Esc closes and returns focus to the button | MANUAL — note in MR |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/tst_FSM.qml:18-30` (`TestCase` importing `VLC.Util`), registered in
  `qml_tests` at `modules/gui/qt/meson.build:954-956`.
- Stubs: `ListModel { ListElement { display: "Track 1 - English"; checked: true } }` stands for a `TrackListModel`.
- Sample media: for the MANUAL rows, any mkv with two subtitle streams (e.g. generated with
  `ffmpeg -f lavfi -i testsrc -i a.srt -i b.srt -map 0 -map 1 -map 2 out.mkv` on nzxt).

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): `LangButton` stays available in the editor and leaves the default layout as PKT-3003 decides. A second track entry point on the right contradicts the VLC 4 grouping in `LangButton`. Proposed:
   keep `LangButton` available in the editor, drop it from the default layout only if VideoLAN agrees (PKT-3003
   decides the default). 
2. **RULED** RULED 2026-10-14 (R0): the popup is anchored to the button and clamped inside `rootPlayer`. Popup anchoring: anchored to the button (design) vs. flush-left full-width as today
   (`modules/gui/qt/player/qml/controlbarcontrols/LangButton.qml:68-69`). Proposed: anchored to the button, clamped inside `rootPlayer`.
3. NOTE — embedded vs external split depends on a role `TrackListModel` lacks (§2b). If Step 2 shows it needs a
   core change, ship one section and open a follow-up; do not grow this packet.
4. NOTE — `modules/gui/qt/player/qml/PlayerControlbarControls.qml:55` labels `LANG_BUTTON` "Open subtitles" although it opens all tracks;
   the new `SUBTITLE_BUTTON` takes "Subtitles" and `LANG_BUTTON` is relabelled "Languages and tracks" to match
   its tooltip (`modules/gui/qt/player/qml/controlbarcontrols/LangButton.qml:50`).
5. NOTE — `modules/gui/qt/player/qml/TracksMenu.qml:29` FIXME (keyboard navigation of the popup). The new tab page must not inherit it:
   row 7 is the check.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`.
- Strings added are marked for translation (`qsTr` / `qtr`) and `po/POTFILES.in` is updated for the new QML files.
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on every new control.

### Evidence
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc && git switch -c master-pkt-3010-subtitle-button-track-list ce4af2f`
      (after PKT-3003 is merged: from its merge commit); run the §6 command → green before any change.
- [ ] **Step 2: Dry-run** — re-read every range of §2; grep `TrackListModel::data` in `player/input_models.cpp`
      for anything telling an external track apart (§2b, §5.3) and write the answer into §5.3.
- [ ] **Step 3: First row watched failing** — create `modules/gui/qt/tests/tst_TrackTabs.qml` with row 1 only,
      register `'tst_TrackTabs': files('tests/tst_TrackTabs.qml')` in `qml_tests`; with an empty
      `util/qml/TrackTabsModel.qml` (registered in the `VLC.Util` block `modules/gui/qt/meson.build:837-856`) →
      `meson test -C build-meson tst_TrackTabs` FAILS.
- [ ] **Step 4: Rows 1–4** — implement `TrackTabsModel` (pure QML: input models + `requested`, output `tabs`,
      `currentTab`, `noneChecked()`), one TDD cycle per row.
- [ ] **Step 5: Row 5** — `SUBTITLE_BUTTON` replaces `RESERVED_0X1F` in `control_list_model.hpp`, add its
      `static_assert`; entry in `PlayerControlbarControls.qml` (`file: "SubtitleButton.qml"`, `label:
      the subtitle glyph of PKT-3002 — today only `VLCIcons.audiosub` exists (`modules/gui/qt/style/vlcicons.hpp:128`), use it as placeholder — `text: qsTr("Subtitles")`). Commit 1.
- [ ] **Step 6: Popup** — `player/qml/TracksTabPage.qml` (tab strip + per-tab `ListViewExt` reusing the
      `CheckedDelegate` pattern of `modules/gui/qt/player/qml/TracksListPage.qml:223-251`, "+" and "Show Preferences" footer);
      `SubtitleButton.qml` copies `LangButton.qml` (auto-hide lock, `forceUnlock`) and opens `TracksMenu` with
      `initialItem` = `TracksTabPage { requested: "subtitle" }`; anchor x on the button (§5.2). Add both files to
      the `VLC.Player`/`VLC.PlayerControls` blocks of `meson.build` and to `po/POTFILES.in`. Rows 6–7 by hand,
      screenshots. Commit 2 `qml: add a tabbed track popup with a subtitle tab`.
- [ ] **Step 7: Default layout** — add `ControlListModel::SUBTITLE_BUTTON` to the right group of the Default
      Style video layout in `controlbar_profile_model.cpp` (as left by PKT-3003). Commit 3
      `qt: put the subtitle button in the default video layout`.
- [ ] **Step 8: Whole gate** — §6 command; paste the `meson test` summary into §6 Evidence.
- [ ] **Step 9: MR** — push to the fork, PR to the fork github.com/oscarolar/vlc (base `master`) titled as `mr_title`, body cites
      §5.1–5.2 as recorded rulings; `status: review`.
