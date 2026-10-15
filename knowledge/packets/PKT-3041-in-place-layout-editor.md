---
id: PKT-3041
title: "Customize the control bar in place by dragging buttons on the live player"
section: "E. Customization"
video_ref: "18:58–19:28"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/dialogs/toolbar + modules/gui/qt/player
branch: master-pkt-3041-in-place-layout-editor
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/control_list_model.cpp
  - modules/gui/qt/player/control_list_model.hpp
  - modules/gui/qt/player/qml/ControlBar.qml
  - modules/gui/qt/player/qml/PlayerControlLayout.qml
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/dialogs/toolbar/qml/InPlaceEditorOverlay.qml   # new
  - modules/gui/qt/dialogs/toolbar/qml/EditorDNDView.qml
  - modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorButtonList.qml
  - modules/gui/qt/dialogs/dialogs/qml/Dialogs.qml
  - modules/gui/qt/dialogs/dialogs_provider.hpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/tests/test_control_list_model.cpp             # new
  - modules/gui/qt/meson.build
estimate_h: 32.0
priority: P2
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qml: edit the control bar in place on the player view"
status: ready
---

# PKT-3041 — Customize the control bar in place by dragging buttons on the live player

## 1. Scope & intent
At 18:58–19:28 the designer calls the customization dialog "very hard to use" and replaces it with one large frame
over the centre of VLC, filled with every available button as an outlined tile; the user drags tiles onto the real
control bar, reorders them there, and drags them back into the frame to remove them, then Save/Cancel (frame s022:
"Customize Player — Drag items around and out of the box below to re-order them. Drag items into the box to remove
them from the player.", 31 tiles from *Spacer*, *Divider*, *Playback Speed*, *Forward 10s* … to *Bookmarks*,
*Playlists*; the player's own bar stays visible at the bottom and updates while dragging).

**The video shows VLC 3's widget editor** ("Toolbars Editor", Line 1/Line 2, Flat Button/Big Button/Native Slider,
frame s022 top). Master already replaced it with a QML drag-and-drop editor (§2b). What remains is the *in place*
part: today's editor is a separate modal window with a mock bar per player (L/C/R zones) instead of the real bar.

After this packet:
- "Customize Interface…" (and PKT-3013's menu entry) enters an **edit mode on the player view**: a translucent overlay
  with the button palette (reusing `ToolbarEditorButtonList`) above the live `ControlBar`; the bar's left/centre/right
  zones become drop targets with outlines; Save keeps, Cancel/Esc reloads.
- Edits go through the same `ControlbarProfileModel` / `PlayerControlbarModel` / `ControlListModel` as today, so
  profiles, persistence and the old dialog keep working; the old window remains reachable for audio and mini player
  layouts (the in-place mode edits the player currently on screen).
- `ControlListModel::insert/move/remove` get bounds checks (today an out-of-range index reaches
  `beginInsertRows`/`beginMoveRows` unchecked) and their first unit tests.

T3 — split (§5.5):
1. `qt: check bounds in ControlListModel and cover it with tests` (3041a).
2. `qml: make PlayerControlLayout zones accept toolbar drops in edit mode` (3041b).
3. `qml: add the in-place customize overlay and its menu entry` (3041c).

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorDialog.qml:29-195` (`WindowDialog`) — modal window "Toolbar
  Editor", 825×600; saves on open (`:43-46`) so Cancel can `reload()` (`:53-57`); profile combo + clone/new/delete
  (`:90-184`); hosts `ToolbarEditor` (`:187-192`).
- `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditor.qml:51-85` — a `TabBar` with one tab per `PlayerListModel` entry
  (video, audio, mini player); `:87-140` stacks one `RowLayout` of three `EditorDNDView` (left/centre/right) per
  player, with "L E F T / C E N T E R / R I G H T" watermarks (`:91-107`).
- `modules/gui/qt/dialogs/toolbar/qml/EditorDNDView.qml:80-99` (`dropEvent`) — the three drop cases: reorder inside a
  zone (`model.move`), insert from the palette (`model.insert({"id": mIndex})`), move between zones
  (`insert` + `remove` on the source view).
- `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorButtonList.qml:75-95` — the palette is itself a `DropArea`;
  dropping a bar button there removes it (`:88-93`).
- `modules/gui/qt/dialogs/dialogs/qml/Dialogs.qml:596-608` — `Loader` that instantiates the dialog on
  `DialogsProvider.showToolbarEditorDialog`; `modules/gui/qt/dialogs/dialogs_provider.hpp:290` the signal;
  `modules/gui/qt/menus/menus.cpp:270-272` "Customi&ze Interface…" (hidden when `b_isDialogProvider`);
  `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:868-870` the same from the Interface panel button.
- `modules/gui/qt/player/qml/ControlBar.qml:314-325` — the live bar is a `PlayerControlLayout` with
  `identifier: root.identifier`; `modules/gui/qt/player/qml/PlayerControlLayout.qml:53-56` its lock/menu signals
  (auto-hide must be locked while editing).
- `modules/gui/qt/player/player_controlbar_model.hpp:36-55` — `left`/`center`/`right` `ControlListModel`s, `dirty`;
  `PlayerIdentifier {Videoplayer, Audioplayer, Miniplayer}`.
- `modules/gui/qt/player/control_list_model.hpp:39-89` (`ControlType`) — persisted integer ids with reserved
  ranges; `:151-154` the invokables. `modules/gui/qt/player/control_list_model.cpp:127-150` — `insert`, `move`,
  `remove`, `clear` with **no index validation**.
- `modules/gui/qt/tests/qml_test.cpp:38-40` — the QML test runner only initialises `util_assets`; `VLC.Dialogs` and
  `VLC.PlayerControls` cannot be imported by a `TestCase` today, and their files use `MainCtx` singletons. DnD rows
  are therefore MANUAL; the model rows are QtTest.

Settings / variables involved: QSettings group `ControlbarProfileModel`
(`modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:696-750`); no vlcrc option.
Mock boundary: none for `ControlListModel` (pure Qt model); the overlay is MANUAL.

### Overlaps & contradictions
- **PKT-3003** (default layout) changes the default profile content; this packet edits whatever profile is selected.
  Depends on it so the screenshots match the redesign.
- **PKT-3006** (single thin control row, thick seek bar) changes `ControlBar.qml`; edit mode must target the zones it
  leaves. Rebase after it if it lands first.
- **PKT-3042** (profiles carrying layout and settings) builds on the same profile model; no file overlap with 3041a/b.
- **PKT-3013** (one menu button) owns where "Customize Player" sits in the menu.
- New tiles in the mock-up (*HDR*, *File Name*, *Artist Name*, *Folder Name*, *Playlist Name*, *Favorite*,
  *Thumbnail*, *Go Home*, *Right Sidebar*, *Bookmarks*, *Playlists*, *Forward/Backward 15s*) are **not**
  `ControlType`s today; each needs its own control component and id — out of scope, listed for the sibling packets
  that own those features (PKT-3009 PiP, PKT-3026 bookmarks, PKT-3032 playlists).

## 2b. Upstream state
Master (VLC 4) ships: a QML drag-and-drop toolbar editor, per-player layouts (video/audio/mini), named profiles with
clone/new/delete, five built-in styles (`modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:39-380`), dirty markers and Cancel-reload. The
video's complaint targets the VLC 3 editor and is largely solved in master. This packet is narrowed to the in-place
mode (edit the real bar instead of a mock in another window) plus the model hardening it needs.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `ControlListModel::insert` | model with [PLAY, NEXT] | insert `{id: STOP}` at 1 | [PLAY, STOP, NEXT], one `rowsInserted(1,1)` | `tests/test_control_list_model.cpp::testInsertMiddle` |
| 2 | same | count 2 | insert at 5 or -1 | rejected: no change, no signal, warning logged | `::testInsertOutOfRange` |
| 3 | `move` | [A, B, C] | move 0 → 2 | [B, C, A]; `rowsMoved` destination row 3 (Qt's convention, as `modules/gui/qt/player/control_list_model.cpp:137`) | `::testMoveForward` |
| 4 | `move` | [A, B, C] | move 2 → 0, and move 1 → 1 | [C, A, B]; second call no signal | `::testMoveBackwardAndNoop` |
| 5 | `remove` | [A] | remove 3 | rejected, no signal | `::testRemoveOutOfRange` |
| 6 | dirty | `PlayerControlbarModel` with three lists | insert in `center` | `dirty == true`; `getControls()` of the profile reflects it | `::testDirtyPropagates` |
| 7 | in-place mode | video playing | Customize Player → drag *Snapshot* onto the right zone, reorder, drag *Stop* back to the palette, Save; restart | bar shows the change live and after restart; Cancel/Esc restores; keyboard can reach Save/Cancel | MANUAL (GIF or 3 screenshots) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_thread_runner.cpp` + meson block `modules/gui/qt/meson.build:1384-1401` (QtTest, no
  libvlc). `test_qt_control_list_model` compiles `player/control_list_model.cpp` and
  `player/player_controlbar_model.cpp` (Step 2 checks the latter's includes; if it pulls `MainCtx`, row 6 tests
  `ControlListModel` signals only).
- `QSignalSpy` on `rowsInserted`, `rowsMoved`, `rowsRemoved`, `countChanged`.
- Sample media: any local video for row 7.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): complement: the editor window stays and the in-place mode becomes the default entry. Replace or complement the editor window. Proposal: complement (window stays for audio/mini player
   layouts and profile management; in-place mode is the default entry). Replacing it needs an in-place story for the
   mini player and audio view.
2. **NOTE** — Overlay vs. window: the overlay lives in the main window's QML scene (`Player.qml`), so it works with
   the compositor and CSD; it locks auto-hide through `requestLockUnlockAutoHide(true)`.
3. **NOTE** — Bounds checks change behaviour only for invalid calls that would hit Qt asserts in debug builds today.
4. **NOTE** — The mock-up's tile grid is the existing palette restyled (outlined tiles, labels under icons); tokens from
   PKT-3001.
5. **NOTE (split)** — 3041a model hardening + rows 1–6 (T1, ~4 h); 3041b drop targets on the live
   `PlayerControlLayout` in edit mode (T2, ~14 h); 3041c overlay, entry points, Save/Cancel (T2, ~14 h, row 7).

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3041-in-place-layout-editor ce4af2f`
      (rebase on PKT-3003 when merged); §6 command once → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm whether PKT-3006 changed `modules/gui/qt/player/qml/ControlBar.qml:314-325`; decide row 6's scope
      (see §4).
- [ ] **Step 3 (3041a): Row 1 watched failing** — `tests/test_control_list_model.cpp`, `test_qt_control_list_model`
      in `meson.build`; wrong expected order → FAIL, then right.
- [ ] **Step 4 (3041a): Rows 2–6** — rows 2 and 5 fail first (they crash/assert today), then add index guards in
      `modules/gui/qt/player/control_list_model.cpp:127-150`. Commit `qt: check bounds in ControlListModel and cover it with tests`; MR.
- [ ] **Step 5 (3041b)** — add `editMode` to `PlayerControlLayout`; in edit mode each zone wraps its repeater in an
      `EditorDNDView`-style `DropArea` reusing `dropEvent` (`modules/gui/qt/dialogs/toolbar/qml/EditorDNDView.qml:80-99`), outlines visible. Commit
      `qml: make PlayerControlLayout zones accept toolbar drops in edit mode`.
- [ ] **Step 6 (3041c)** — `InPlaceEditorOverlay.qml` (palette from `ToolbarEditorButtonList`, Save/Cancel, Esc) in
      `Player.qml`; `DialogsProvider` signal `showInPlaceEditor`; menu entry in `modules/gui/qt/menus/menus.cpp:270-272` routes to it,
      window kept under "Advanced layout editor…". Save = `controlbarProfileModel.save()`, Cancel = `reload()` (as
      `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorDialog.qml:43-57`). Row 7. Commit `qml: add the in-place customize overlay and its menu entry`; MR.
- [ ] **Step 7: Whole gate** — §6 command per MR; summaries in §6 Evidence; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
