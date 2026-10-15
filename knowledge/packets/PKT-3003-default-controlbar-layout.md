---
id: PKT-3003
title: "A built-in control bar layout with play on the left and fullscreen/settings on the right"
section: "A. Control bar"
video_ref: "01:45–02:23"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/dialogs/toolbar
branch: master-pkt-3003-default-controlbar-layout
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.hpp
  - modules/gui/qt/tests/test_controlbar_profile_model.cpp
  - modules/gui/qt/meson.build
estimate_h: 7.0
priority: P1
depends_on: [PKT-3001]
upstream_ruling: needed
target: fork
mr_title: "qt: add a compact control bar profile with play on the left"
status: ready
---

# PKT-3003 — A built-in control bar layout with play on the left and fullscreen/settings on the right

## 1. Scope & intent
At 01:45–02:23 the designer refuses the centred play button that "every player" and VLC's own upcoming redesign
use: it makes the bar tall, covers the video, and most people pause with the keyboard anyway (her survey frame at
~02:05: 87 % use the keyboard). She keeps play **on the left**, "a tiny bit bigger", puts previous/next **around**
it, and moves fullscreen and settings to the **right**, "like on most players". Frames 02:10–02:20 show the result:
an orange play button flanked by previous and next on the left, the fullscreen and equaliser icons sliding to the
right edge.

Master's QML player does not need code to *allow* this: the bar is three user-editable lists (left / centre / right)
edited in Tools › Customize Interface, and six built-in profiles ship (§2b). But the profile selected on a fresh install,
**"Default Style", is exactly the centred layout the video rejects**, and no built-in profile matches the video
(Classic has play on the left but keeps stop, fullscreen and extended settings in the left group).

After this packet a seventh built-in profile, **"Compact Style"**, ships: left `PREVIOUS, PLAY, NEXT`; centre empty
(PKT-3006 will place the seek bar there); right `VOLUME, EXTENDED, PLAYLIST, FULLSCREEN`. Whether it becomes the
profile selected on first run is ruled in §5.1 (option A); the code path for both answers is the same one line.

One MR, two commits:
1. `qt: test the built-in control bar profiles` — a QtTest pinning today's Default and the profile list (green on master).
2. `qt: add a compact control bar profile with play on the left` — enum, table entry, new rows.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:39-458` (`ControlbarProfileModel::m_defaults`) — the
  built-in profiles, each a `{Style, name, {Videoplayer, Audioplayer, Miniplayer} × {left, centre, right}}` table.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:96-171` (`DEFAULT_STYLE`, "Default Style") — Videoplayer left
  `LANG, BOOKMARK, EXTENDED, NAVIGATION_BUTTONS`; centre `SKIP_BACK, PREVIOUS, PLAY, NEXT, SKIP_FW`; right
  `VOLUME, RENDERER, FULLSCREEN`. Fullscreen is already on the right; play is centred.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:375-457` (`CLASSIC_STYLE`) — Videoplayer left `PLAY, SPACER, PREVIOUS, STOP, NEXT,
  SPACER, FULLSCREEN, EXTENDED, SPACER, PLAYLIST, LOOP, RANDOM`, right `VOLUME`: the VLC 3 bar.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:491-508` (`insertDefaults`) — creates one profile per table entry except
  `EMPTY_STYLE`, in table order, then `setSelectedProfile(0)`: **the first non-empty entry is the first-run profile**.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:476-488` — `insertDefaults()` runs only when `reload()` finds no saved profile or
  every profile was deleted: existing users never receive a new built-in profile.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:1011-1027` (`generateProfileFromStyle`) — builds a profile from a `Style`, used by
  "new profile" (`:610`, `:937`).
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.hpp:47-55` (`enum class Style`) — `EMPTY … CLASSIC_STYLE`;
  `:38` the constructor takes the `QSettings*` to persist to (so a test can hand it a temporary INI file).
- `modules/gui/qt/dialogs/toolbar/controlbar_profile.hpp:46` (`getModelData(int identifier)`) — returns the three
  `QVector<int>` of a player; the assertion surface for the test.
- `modules/gui/qt/player/control_list_model.hpp:37-110` (`ControlType`) — "changing the enum order … will break user
  configuration" (`:37`): the new profile only uses existing ids.
- `modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:243-244` — the play button is `icon_medium` (48 dp) high,
  already the largest control; "a tiny bit bigger" needs no change here (§5.3).

Settings / variables involved: none in `libvlc-module.c`; profiles live in the Qt `QSettings` file under `Profiles` /
`SelectedProfile` (`modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:30-33`).
Mock boundary: `QSettings` on a temporary INI file (`QTemporaryDir`); `qfut()` needs `vlc_gettext`, so the test links
`libvlccore` like `test_qt_navigation_model`.

### Overlaps & contradictions
- **PKT-3006** adds a seek-bar control and fills the centre of this profile; it depends on this packet.
- **PKT-3010 / PKT-3011 / PKT-3013** add subtitle, audio and menu buttons "on the right"; each appends to this
  profile's right list in its own MR. This packet leaves `LANG_BUTTON` out on purpose (it opens the combined tracks
  menu that 3010 replaces).
- **PKT-3041 / PKT-3042** (in-place editor, profiles carrying settings) build on the same `ControlbarProfileModel`;
  no file conflict with a table entry.
- **Upstream redesign**: VLC 4's own direction is the centred Default (the video says so at 01:51, and master ships
  it). Making Compact the first-run profile contradicts that — hence `upstream_ruling: needed`.

## 2b. Upstream state
- User-configurable layout (three lists, drag-and-drop editor `dialogs/toolbar/qml/ToolbarEditorDialog.qml`, menu entry
  `modules/gui/qt/menus/menus.cpp:272`): **shipped**. A user can build the video's bar today.
- Built-in profiles: **shipped** (Default, Minimalist, One-liner, Simplest, Classic).
- A profile matching the video: **not shipped**; Default is the opposite design.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. New file
`modules/gui/qt/tests/test_controlbar_profile_model.cpp`, class `TestControlbarProfileModel`, meson test
`test_qt_controlbar_profile_model`.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `insertDefaults` (`:491-508`) | an empty temporary `QSettings` | the model is constructed and the queued reload runs (`QTRY_VERIFY(model.rowCount() > 0)`) | profile 0 is "Default Style" and its Videoplayer centre is `SKIP_BACK, PREVIOUS, PLAY, NEXT, SKIP_FW` (pins master; green before the change) | `::firstRunSelectsDefaultStyle` |
| 2 | `m_defaults` | same | names of all rows are read | contains "Compact Style" (red on master) | `::compactStyleIsBuiltIn` |
| 3 | Compact Videoplayer | same | `getModelData(Videoplayer)` of "Compact Style" | left `PREVIOUS, PLAY, NEXT`; centre empty; right `VOLUME, EXTENDED_BUTTON, PLAYLIST_BUTTON, FULLSCREEN_BUTTON` | `::compactStyleVideoLayout` |
| 4 | Compact Audioplayer / Miniplayer | same | `getModelData(Audioplayer)`, `(Miniplayer)` | audio: left `PREVIOUS, PLAY, NEXT`, right `VOLUME, LOOP_BUTTON, RANDOM_BUTTON, PLAYLIST_BUTTON`; mini: equal to Default's mini | `::compactStyleOtherPlayers` |
| 5 | `generateProfileFromStyle(COMPACT_STYLE)` | none | called | non-null, name "Compact Style", not dirty | `::generateCompactFromStyle` |
| 6 | Persistence | a model saved with "Compact Style" selected | a second model is built on the same INI | `selectedProfile` points at "Compact Style" and its data is unchanged | `::compactStyleSurvivesReload` |
| 7 | Visual | "Compact Style" selected in Customize Interface | a video plays, bar shown | play left between previous/next, volume/settings/queue/fullscreen right | MANUAL: screenshot 1280×720 |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:18-34` (QtTest skeleton, `QSignalSpy`) and its meson
  block `modules/gui/qt/meson.build:1342-1360`; sources for the new block: `tests/test_controlbar_profile_model.cpp`,
  `dialogs/toolbar/controlbar_profile_model.cpp`, `dialogs/toolbar/controlbar_profile.cpp`,
  `player/control_list_model.cpp`, `player/player_controlbar_model.cpp` (+ their headers in `moc_headers`).
- Stubs: none; `QSettings(tmpDir.filePath("vlc-qt.ini"), QSettings::IniFormat)`.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED — first-run profile.** RULED 2026-10-14 (R0): Option A: Compact is appended after Classic and Default stays first. Option A (no ruling needed): append Compact after Classic; Default stays first.
   Option B (the video): put Compact first in `m_defaults` so `setSelectedProfile(0)` picks it. Row 1 pins A; under B
   it is rewritten to expect "Compact Style". The packet executes A.
2. **RULED — existing users.** RULED 2026-10-14 (R0): the follow-up is accepted (outside this MR): on `reload()` append any missing built-in profile name once, tracked by a settings version key. `insertDefaults` only runs on an empty profile list (`:476-488`), so upgraded installs
   never see Compact. Proposed follow-up (not in this MR): on `reload()`, append any built-in name missing from the
   saved list once, recorded by a settings version key.
3. NOTE — "a tiny bit bigger" play button: already 48 dp (`modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:244`). Its rounded-rectangle shape is in
   PKT-3001 §5.3.
4. NOTE — **bug found, out of scope**: `modules/gui/qt/player/qml/PlayerControlbarControls.qml:46-47` map `FRAME_PREV_BUTTON` to
   `FrameNextButton.qml` ("Next Frame") and `FRAME_NEXT_BUTTON` to `FramePrevButton.qml`; the files themselves are right
   (`modules/gui/qt/player/qml/controlbarcontrols/FrameNextButton.qml:33` calls `Player.frameNext()`). The One-liner profile therefore shows a *previous*-frame
   button where it asks for *next*. One-line swap, separate T1 MR (`qml: fix swapped frame step controls`).
5. NOTE — the new name `N_("Compact Style")` is picked up by `po/POTFILES.in:738` (file already listed).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`: `git switch -c master-pkt-3003-default-controlbar-layout ce4af2f`
      (worktree under `~/src/vlc-wt/` if busy); §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; confirm `insertDefaults` order and that `getModelData` is public.
- [ ] **Step 3: Row 1** — create the test file and meson block (after `test_qt_dialog_model`, `modules/gui/qt/meson.build:1404-1423`);
      write row 1 with a wrong expected centre → FAIL, then the real one → green. Commit
      `qt: test the built-in control bar profiles`.
- [ ] **Step 4: Rows 2–5, one TDD cycle each** — add `COMPACT_STYLE` at the end of `Style` (`modules/gui/qt/dialogs/toolbar/controlbar_profile_model.hpp:47-55`) and its
      table entry after Classic (`modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:457`); each row red then green.
- [ ] **Step 5: Row 6** — persistence across two model instances on the same INI.
- [ ] **Step 6: Row 7** — select the profile in Customize Interface, screenshot.
- [ ] **Step 7: Whole gate** — §6; paste the summary. Commit `qt: add a compact control bar profile with play on the left`.
- [ ] **Step 8: MR** — PR to the fork github.com/oscarolar/vlc (base `master`); §5.1 and §5.2 as recorded rulings; file the §5.4 bug as
      its own T1 MR; `status: review`.
