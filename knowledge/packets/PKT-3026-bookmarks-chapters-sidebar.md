---
id: PKT-3026
title: "Bookmarks and chapters as sidebar lists, one key creates a bookmark"
section: "C. Bookmarks"
video_ref: "14:52–15:18"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/medialibrary
branch: master-pkt-3026-bookmarks-chapters-sidebar
base_sha: ce4af2f
target_files:
  - modules/gui/qt/medialibrary/mlbookmarkmodel.hpp
  - modules/gui/qt/medialibrary/mlbookmarkmodel.cpp
  - modules/gui/qt/medialibrary/qml/MLGlobalShortcuts.qml
  - modules/gui/qt/player/qml/BookmarksPane.qml
  - modules/gui/qt/player/qml/ChaptersPane.qml
  - modules/gui/qt/tests/test_bookmark_model.cpp
  - modules/gui/qt/tests/vlc_stub_modules.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 14.0
priority: P2
depends_on: [PKT-3018]
upstream_ruling: needed
target: fork
mr_title: "qml: bookmarks and chapters in the player side pane"
status: ready
---

# PKT-3026 — Bookmarks and chapters as sidebar lists, one key creates a bookmark

## 1. Scope & intent
14:52–15:13: the designer wants to save a moment. *Playback → Custom Bookmarks* opens **Edit Bookmarks**, a
small Widgets window with *Create / Delete / Close / Clear* and a table (frame `s017` ≈14:58–15:05). "I just
want to add a bookmark and move on." A shortcut like **Ctrl+B should create a bookmark**, "but Ctrl+B just opens
this". Proposal: bring the bookmark list out into the **sidebar**, where it is easy to create bookmarks and see
the full list; **movie chapters fit there too**. Frame `s017` (≈15:13, last cell) shows the target: a right pane
with tabs **Queue · Bookmarks · Chapters** (icons, orange underline under *Chapters*), the Chapters list numbered
`1 Winter 00:02:15`, `2 Spring 00:29:45` … with the time right-aligned.

Before: bookmarks need the media library, are created from a separate window or the bar's bookmark menu, and
Ctrl+B opens the window. After: Bookmarks and Chapters are lists in the right pane next to the queue; a "+"
creates a bookmark at the current time; Ctrl+B creates one directly (OSD "Bookmark added at 00:42"); clicking a
row seeks; double-click renames; Delete removes. The Widgets dialog remains for bulk management.

One MR, three commits:
1. `qt: make bookmark editing invokable from QML` — `MLBookmarkModel` add/remove/rename as `Q_INVOKABLE`,
   + QtTest with an ML stub that supports bookmarks.
2. `qml: bookmarks and chapters in the player side pane`.
3. `qml: Ctrl+B adds a bookmark` (§5.1).

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/dialogs/bookmarks/bookmarks.cpp:37-101` (`BookmarksDialog`) — title "Edit Bookmarks",
  Create/Delete/Clear/Close, a `QTreeView` on `MLBookmarkModel` (`:65-78`); `add/del/clear/activateItem`
  forward to the model (`:118-141`). The video's screenshot (columns *Description / Bytes / Time*) is VLC 3;
  master shows *Name / Time / Description* (`modules/gui/qt/medialibrary/mlbookmarkmodel.cpp:157-169`, `:201-219`).
- `modules/gui/qt/medialibrary/mlbookmarkmodel.hpp:42-85` — roles `NameRole, TimeRole, PositionRole,
  DescriptionRole` (`:55-60`); `add()`, `remove(QModelIndexList)`, `clear()` are **plain public methods, not
  `Q_INVOKABLE`** (`:81-83`); only `select()` is (`:84`). QML can list and seek but cannot create.
- `modules/gui/qt/medialibrary/mlbookmarkmodel.cpp:243-275` (`add`) — reads the player time, returns silently
  when `m_currentMediaId == 0` (media not in the library), else adds on the ML thread and names it
  "Bookmark at %1"; `:277-326` remove/clear; `:328-335` select seeks with `vlc_player_SetTime`;
  `:384-430` (`updateMediaId`) resolves the playing MRL to an ML media id.
- `modules/gui/qt/maininterface/mainui.cpp:483` — `MLBookmarkModel` is registered for QML.
- `modules/gui/qt/medialibrary/qml/Bookmarks.qml:75-114` — bookmark markers on the seek bar (`Repeater` over
  `MLBookmarkModel`), loaded by `modules/gui/qt/player/qml/ControlBar.qml:240-256` when the media library exists.
- `modules/gui/qt/player/qml/controlbarcontrols/BookmarkButton.qml:30-60` — bar button popping
  `QmlBookmarkMenu` (`modules/gui/qt/menus/qml_menu_wrapper.cpp:565-645`: Titles, Chapters, Bookmarks sections, "Manage"
  with Ctrl+B, one action per bookmark).
- `modules/gui/qt/medialibrary/qml/MLGlobalShortcuts.qml:25` — `ShortcutExt { sequence: "Ctrl+B";
  onActivated: DialogsProvider.bookmarksDialog() }`, loaded by `modules/gui/qt/menus/qml/GlobalShortcuts.qml:55-58`; the same
  shortcut is advertised by `modules/gui/qt/menus/custom_menus.cpp:417` and `modules/gui/qt/menus/qml_menu_wrapper.cpp:629`.
- `modules/control/hotkeys.c:230-245` (`PlaylistBookmark`) and `src/libvlc-module.c:2775-2814` —
  `key-set-bookmark1..10` (Ctrl+F1…F10, `:2563-2582`) are **playlist** bookmarks (remember an MRL), not time
  bookmarks: naming trap, not reused.
- `modules/gui/qt/player/input_models.hpp:159-195` (`ChapterListModel`) — `count`, roles `TimeRole`,
  `StartPositionRole`, `EndPositionRole`, `selectChapter(int)`, `getClosestChapterFromPos`; exposed as
  `Player.chapters` and already drawn as seek-bar segments (`modules/gui/qt/player/qml/SliderBar.qml:331-345`).
- `modules/gui/qt/tests/vlc_stub_modules.cpp:122-140` — the test ML module answers `VLC_EGENERIC` to every
  `list`/`control`: a bookmark test needs the stub to keep bookmarks in memory.

Settings / variables involved: none in core (`bookmarks` input option, `src/libvlc-module.c:1922`, is the
legacy per-input seekpoint list, not used). One shortcut binding (Ctrl+B).
Mock boundary: ML stub extended for `VLC_ML_MEDIA_ADD_BOOKMARK`, `…_REMOVE_BOOKMARK`,
`…_UPDATE_BOOKMARK`, `VLC_ML_LIST_MEDIA_BOOKMARKS`, `VLC_ML_GET_MEDIA_BY_MRL`; player from `VLCTestingEnv`.

### Overlaps & contradictions
- **The frame puts Bookmarks/Chapters next to the *Queue*, not in the four-tab settings sidebar.** So this
  packet plugs into the right-pane arbitration of **PKT-3018** (`RightPaneFSM`) and into the queue pane of
  **PKT-3004** (tabs Queue · Bookmarks · Chapters). If PKT-3004 has not landed, the two panes are added as a
  third right-pane state `bookmarks` of `RightPaneFSM` and merged into the queue tabs when 3004 lands.
- **PKT-3020** draws cues on the seek bar; bookmark markers must stay above them (z-order).
- **PKT-3035** (hotkeys dialog) must list the new Ctrl+B action; **PKT-3015** (OSD names the hotkey) shows
  "Bookmark added (Ctrl+B)".

## 2b. Upstream state
- Bookmarks exist (ML-backed, per media, with names and seek-bar markers) and a menu lists them; creation is in
  the dialog or the menu's Manage. Without the media library there are no bookmarks at all
  (`modules/gui/qt/player/qml/controlbarcontrols/BookmarkButton.qml:43`, `modules/gui/qt/player/qml/ControlBar.qml:244`).
- Chapters exist as a model and on the seek bar, and as a menu section — not as a list with times.
- Ctrl+B is bound to *open the dialog* in three places.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `MLBookmarkModel::add` (invokable) | ML stub, media id 7 playing at 42 s | `add()` from `QMetaObject::invokeMethod` | after the ML round-trip `rowCount() == 1`, `NameRole == "Bookmark at 00:42"`, `TimeRole == 42000` | `test_bookmark_model.cpp::testAddFromQml` |
| 2 | same | media **not** in the library (`m_currentMediaId == 0`) | `add()` | returns `false` and emits `addFailed` (today: silent return, `modules/gui/qt/medialibrary/mlbookmarkmodel.cpp:251-252`) so the UI can say why | `test_bookmark_model.cpp::testAddWithoutLibraryReports` |
| 3 | `removeAt(int)` | 3 bookmarks | `removeAt(1)` | 2 rows, the middle one gone | `test_bookmark_model.cpp::testRemoveAt` |
| 4 | `setData(NameRole)` / `rename(int, QString)` | 1 bookmark | rename "Cool shot" | `NameRole == "Cool shot"` persisted in the stub | `test_bookmark_model.cpp::testRename` |
| 5 | `select(index)` | bookmark at 42 s | select | player time set to 42 s (`vlc_player_SetTime` path) | `test_bookmark_model.cpp::testSelectSeeks` |
| 6 | `BookmarksPane.qml` | right pane, Bookmarks tab | "+" twice, click the first | two rows with times; video seeks to the first | MANUAL — `pkt3026-bookmarks.png` |
| 7 | `ChaptersPane.qml` | a file with 5 chapters | Chapters tab | numbered list with start times as in frame `s017`; current chapter highlighted; click seeks | MANUAL — `pkt3026-chapters.png` |
| 8 | Ctrl+B | playing a library video | press Ctrl+B | bookmark created, OSD confirms; the dialog does **not** open (per §5.1) | MANUAL — `pkt3026-ctrl-b.webm` |

## 4. Fixtures & mocks
- Pattern: `modules/gui/qt/tests/test_ml_model.cpp` and its `vlc_tests` block (`modules/gui/qt/meson.build:1306-1340`), which
  already links `medialibrary/medialib.cpp` and `mlthreadpool.cpp`; add `medialibrary/mlbookmarkmodel.cpp`.
- Extend `MLControl`/`MLList`/`MLGet` in `modules/gui/qt/tests/vlc_stub_modules.cpp:122-140` with an in-memory bookmark map
  (keyed by media id) and one fake media for `GET_MEDIA_BY_MRL`; keep the existing tests' behaviour (other
  requests still `VLC_EGENERIC`).
- Sample media: a fake MRL known to the stub; MANUAL rows need a chaptered MKV (Step 2 picks one from
  `test/samples` or generates it with `mkvmerge --chapters`).

## 5. Design decisions & open questions
1. **RULED — rebinding Ctrl+B.** RULED 2026-10-14 (R0): Ctrl+B creates a bookmark and Ctrl+Shift+B opens the manager. VideoLAN bound Ctrl+B to "Manage bookmarks" in three places
   (`modules/gui/qt/medialibrary/qml/MLGlobalShortcuts.qml:25`, `modules/gui/qt/menus/custom_menus.cpp:417`, `modules/gui/qt/menus/qml_menu_wrapper.cpp:629`). The video asks Ctrl+B =
   *create*. Proposal: Ctrl+B creates, Ctrl+Shift+B opens the manager.  Alternatively keep Ctrl+B and add the creation on the "+" only.
2. **RULED — bookmarks without the media library.** RULED 2026-10-14 (R0): no auto-add to the library: keep the media-library requirement and explain it (row 2). Today none. Options: auto-add the playing media to the
   library as external media on first bookmark, or keep the requirement and explain it (row 2). Recommend
   explaining; 
3. NOTE — the playlist "bookmarks" hotkeys (`key-set-bookmark1..10`) are a different feature and are left alone;
   their names must not appear in the new UI.
4. NOTE — the Widgets dialog stays for bulk delete/clear; the bar's bookmark menu stays (titles + chapters
   for DVDs).

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3026-bookmarks-chapters-sidebar` from merged PKT-3018 (and PKT-3004 if merged); build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm which ML requests `MLBookmarkModel` issues (`vlc_ml_media_add_bookmark`, `vlc_ml_list_media_bookmarks`, `vlc_ml_get_media_by_mrl`) and extend the stub to exactly those.
- [ ] **Step 3: Row 1 watched failing** — `tests/test_bookmark_model.cpp` + stub extension, registered in `meson.build` → FAIL (`add` not invokable); then make `add/removeAt/rename` `Q_INVOKABLE` and return/emit failure; rows 2–5; commit `qt: make bookmark editing invokable from QML`.
- [ ] **Step 4: Panes** — `player/qml/BookmarksPane.qml`, `player/qml/ChaptersPane.qml` (list + times, current item, click to seek), plugged into the right pane per §2 overlaps; register in `VLC.Player`, `po/POTFILES.in`; MANUAL rows 6–7; commit `qml: bookmarks and chapters in the player side pane`.
- [ ] **Step 5: Shortcut (§5.1 ruled)** — `modules/gui/qt/medialibrary/qml/MLGlobalShortcuts.qml:25` and the two menu labels; MANUAL row 8; commit `qml: Ctrl+B adds a bookmark`.
- [ ] **Step 6: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`) with §5.1/§5.2 questions; `status: review`.
