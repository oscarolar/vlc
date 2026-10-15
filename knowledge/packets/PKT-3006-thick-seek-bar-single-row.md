---
id: PKT-3006
title: "A thick seek bar inside one thin control row"
section: "A. Control bar"
video_ref: "03:10–03:24"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3006-thick-seek-bar-single-row
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/control_list_model.hpp
  - modules/gui/qt/player/player_controlbar_model.hpp
  - modules/gui/qt/player/player_controlbar_model.cpp
  - modules/gui/qt/player/qml/PlayerControlbarControls.qml
  - modules/gui/qt/player/qml/controlbarcontrols/SeekBarWidget.qml
  - modules/gui/qt/player/qml/ControlBar.qml
  - modules/gui/qt/player/qml/SliderBar.qml
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp
  - modules/gui/qt/tests/test_controlbar_profile_model.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 22.0
priority: P1
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qml: allow the seek bar to sit inside the control row"
status: ready
---

# PKT-3006 — A thick seek bar inside one thin control row

## 1. Scope & intent
At 03:10–03:24 the designer questions "the thin, tiny slider" every modern player uses and, "in a controversial
move", moves the seek bar **down into the control row** and makes it **much thicker**, then "cleans the whole thing
up" into **one thin row that doesn't disrupt the video**. Frames ~03:18–03:22 and the final bar at ~04:04 read left to
right: previous, orange play, next, elapsed time, a tall seek bar (dark track, orange fill, flat orange handle),
remaining time, volume cone, settings gear, queue, fullscreen — all on one row about the height of the buttons.

In master the control bar is always **two stacked rows**: the seek bar row (`row2`, with the times beside it only in
pinned mode) above `PlayerControlLayout`. The seek bar is 4 dp high (grows ×1.8 on hover). Nothing lets the slider
live inside the button row.

After this packet the seek bar becomes a **placeable control** (`SEEK_BAR`) of the toolbar editor. When the selected
profile contains it, `ControlBar` drops its own slider row and the control row carries elapsed time, a thick slider and
remaining time where the user put it. PKT-3003's "Compact Style" places it in the centre. Profiles without it keep
today's two-row bar, so no existing user sees a change.

**T3 → two MRs** (§5.5):
- MR A `qt: add a seek bar control to the control bar model` — enum id, `PlayerControlbarModel::hasSeekBar`, registry
  entry with a minimal widget, Compact profile centre, C++ rows.
- MR B `qml: draw a thick seek bar inside the control row` — `SeekBarWidget.qml`, `ControlBar` row hiding, bookmarks
  anchoring, thickness token, MANUAL rows.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/ControlBar.qml:34-38` (`TimeTextPosition` `Hide / AboveSlider / LeftRightSlider`), `:52`
  `sliderHeight: VLCStyle.heightBar_xxsmall` (4 dp), `:130-257` `strayItems` (`mediaTime`, `mediaRemainingTime`,
  `trackPositionSlider`, `bookmarksLoader`) reparented into `pseudoRow` (`:259-285`) or `row2` (`:287-312`);
  `:314-338` `PlayerControlLayout` always **below** `row2`.
- `modules/gui/qt/player/qml/ControlBar.qml:240-256` (`bookmarksLoader`) — the media-library bookmarks bar is positioned from `row2.y` / `row2.height`
  and the slider's `x`/`width`: it must follow the slider when it moves into the row.
- `modules/gui/qt/player/qml/Player.qml:1282-1359` (`ControlBar` instance) — `textPosition` is `LeftRightSlider` only
  when `MainCtx.pinVideoControls` (`:1299-1301`); state bound to the toolbar FSM (`:1324-1327`).
- `modules/gui/qt/player/qml/SliderBar.qml:28-38` — `barHeight` default 5 dp, `_hoveredScalingFactor 1.8`, seek-point
  geometry; `:317-328` track, `:430-448` fill, `:568-631` handle (`_size = barHeight * 3`, round, hidden unless hovered).
  The thin look is these defaults, not a hard limit.
- `modules/gui/qt/player/qml/ControlRepeater.qml:48-63` — a control whose item sets `Layout.minimumWidth > 0` is laid out
  with `fillWidth` (`:57-59`): the mechanism the expanding seek bar uses (as `ExpandingSpacerWidget` does).
- `modules/gui/qt/player/control_list_model.hpp:37-110` (`ControlType`, persisted — `:37`) — widget ids `0x20…`;
  `RESERVED_0X2B` (`:85`) is the first free widget slot.
- `modules/gui/qt/player/player_controlbar_model.cpp:162-181` (`contentChanged`) — recomputes `empty` from the three lists
  on every change: where `hasSeekBar` is computed; `modules/gui/qt/player/player_controlbar_model.hpp:36-41` the exposed properties.
- `modules/gui/qt/player/qml/PlayerControlbarControls.qml:32-73` — registry `{id, file, label, text}`; the editor lists
  every entry, so the new control becomes draggable with no editor change.
- `modules/gui/qt/style/VLCStyle.qml:327-328` — `controlLayoutHeight` 64 dp, `controlLayoutHeightPinned` 32 dp.

Settings / variables involved: `qt-pin-controls` (`modules/gui/qt/qt.cpp:430`) — unchanged meaning; profiles in QSettings.
Mock boundary: `QSettings` on a temporary INI (rows via the profile model); `PlayerControlbarModel` directly for `hasSeekBar`.

### Overlaps & contradictions
- **PKT-3003** creates `tests/test_controlbar_profile_model.cpp` and the Compact profile; this packet adds rows to that file
  and fills the profile's centre. Must land after 3003.
- **PKT-3007** (thumbnail while seeking) hooks into `SliderBar`'s hover tooltip (`modules/gui/qt/player/qml/SliderBar.qml:74-97`); land 3006 first,
  3007 rebases.
- **PKT-3030** (music view, waveform seek bar) also restyles `SliderBar`; coordinate the thickness token.
- **PKT-3001** radius tokens are used for the thick bar's corners.
- `ControlBar.qml` is also touched by **PKT-3008** only through `Player.qml` bindings, not this file.

## 2b. Upstream state
- Times left/right of the slider (pinned mode): **shipped** (`LeftRightSlider`).
- A compact 32 dp row in pinned mode: **shipped** (`controlLayoutHeightPinned`).
- Slider inside the button row, thick slider: **not shipped**.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. C++ rows: `test_controlbar_profile_model.cpp`
(from PKT-3003), new class `TestPlayerControlbarModel` in the same file, meson test `test_qt_controlbar_profile_model`
(its sources gain nothing: `player_controlbar_model.cpp` is already there).
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PlayerControlbarModel::hasSeekBar` | a model with `left = [PLAY]` | read | `false` | `TestPlayerControlbarModel::noSeekBarByDefault` |
| 2 | same | same | `center()->setControls({SEEK_BAR})` | `true`, `hasSeekBarChanged` emitted once (`QSignalSpy`) | `::seekBarInCentre` |
| 3 | same | seek bar in `right` | the row is removed | `false`, signal emitted | `::removingSeekBarClearsFlag` |
| 4 | `ControlType` ids | none | `static_cast<int>(ControlListModel::SEEK_BAR)` | `0x2B`, and `VOLUME == 0x21`, `WIDGET_SPACER == 0x40` unchanged (persisted ids) | `::persistedIdsUnchanged` |
| 5 | Compact profile | empty INI | Compact Videoplayer read | centre `[SEEK_BAR]`; Default still has no `SEEK_BAR` | `TestControlbarProfileModel::compactStyleHasInlineSeekBar` |
| 6 | Visual — Compact | Compact selected, video playing | bar shown | one row: prev/play/next, elapsed, thick bar, remaining, right group; no second slider row | MANUAL: 1280×720 and 640×360 screenshots |
| 7 | Visual — Default | Default selected | bar shown | identical to master (two rows) | MANUAL: before/after pair |
| 8 | Seek behaviour | Compact | click, drag, Shift-drag, wheel, Left/Right on the inline bar | same results as the master bar (chapter snapping, precise seek with Shift) | MANUAL: checklist in the MR |
| 9 | Bookmarks | ML available, file with bookmarks | bar shown | bookmark marks align with the inline slider | MANUAL: screenshot |

## 4. Fixtures & mocks
- Pattern to copy: PKT-3003's `test_controlbar_profile_model.cpp` (QtTest, temporary `QSettings`), `QSignalSpy` usage as in
  `modules/gui/qt/tests/test_navigation_model.cpp`.
- `SeekBarWidget.qml` copies the item-level API of `controlbarcontrols/ExpandingSpacerWidget.qml` (paintOnly, minimum width)
  and instantiates `SliderBar` + two `T.Label` like `modules/gui/qt/player/qml/ControlBar.qml:131-239`.
- Sample media: a local video with chapters (rows 8–9) — any MKV with chapters, or `mock://` is not enough for the UI.

## 5. Design decisions & open questions
1. **RULED — control vs. mode.** RULED 2026-10-14 (R0): a placeable `SEEK_BAR` control; the Default profile is unchanged. Proposed: a placeable `SEEK_BAR` control (users decide; Default unchanged). The
   alternative, a fourth `TimeTextPosition.Inline` mode chosen by the profile, cannot express *where* the bar sits.
2. **RULED — thickness.** RULED 2026-10-14 (R0): new token `VLCStyle.seekBar_thickHeight` = 10 dp, no hover growth when thick, flat rectangular handle; the value is confirmed against Figma in Step 2. New token `VLCStyle.seekBar_thickHeight`; proposed 10 dp (the video's bar is about the height
   of the time text), no ×1.8 hover growth when thick, handle a flat rectangle as tall as the bar. Value to be read from Figma.
3. NOTE — enum id `0x2B` replaces `RESERVED_0X2B`, as the comment at `modules/gui/qt/player/control_list_model.hpp:38` allows; ids stay stable.
4. NOTE — two seek bars: if a profile has `SEEK_BAR`, `ControlBar` hides `row2` and `pseudoRow`; there is never a second
   slider. Keyboard focus order: `PlayerControlLayout` first item → … → seek bar in its place (`Navigation.upItem` at
   `modules/gui/qt/player/qml/ControlBar.qml:329` falls back to `root.Navigation.upItem`).
5. **T3 split.** MR A (~8 h): rows 1–5, `SeekBarWidget.qml` minimal (draws the existing `SliderBar` with times). MR B (~14 h):
   thickness, handle, bookmarks re-anchoring (`modules/gui/qt/player/qml/ControlBar.qml:240-256`), rows 6–9. Each MR is releasable alone.
6. NOTE — the new file `SeekBarWidget.qml` goes into the `VLC.PlayerControls` QML module list in `meson.build` and into
   `po/POTFILES.in` (new `qsTr` strings: "Seek bar" registry text, accessible names).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`, after PKT-3003 merged (or on top of its branch):
      `git switch -c master-pkt-3006-thick-seek-bar-single-row <3003 head>`; §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; check `RESERVED_0X2B` is still free; locate the `VLC.PlayerControls` module list in
      `meson.build` (`grep -n controlbarcontrols modules/gui/qt/meson.build`).
- [ ] **Step 3: Row 4 then 1 watched failing** — add the test class; `SEEK_BAR` undeclared → build FAIL; add the enum value →
      row 4 green; row 1 against a missing property → FAIL; add `hasSeekBar` (`modules/gui/qt/player/player_controlbar_model.hpp:36-41`,
      computed in `contentChanged` `modules/gui/qt/player/player_controlbar_model.cpp:162-181`) → green.
- [ ] **Step 4: Rows 2, 3, 5** — one cycle each; Compact centre in `controlbar_profile_model.cpp`.
- [ ] **Step 5: MR A** — registry entry (`PlayerControlbarControls.qml`) + minimal `SeekBarWidget.qml`; `ControlBar.qml` hides
      `row2`/`pseudoRow` when `playerControlLayout.model.hasSeekBar`; §6 green; commit
      `qt: add a seek bar control to the control bar model`; Draft MR.
- [ ] **Step 6: MR B** — token, thick `SliderBar` variant, bookmarks anchoring; rows 6–9 with screenshots; commit
      `qml: draw a thick seek bar inside the control row`.
- [ ] **Step 7: Whole gate** — §6 for each MR; paste the summaries in §6 Evidence; `status: review`, §5.1/§5.2 as recorded rulings.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
