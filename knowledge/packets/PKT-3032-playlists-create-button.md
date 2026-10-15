---
id: PKT-3032
title: "Playlists get a New playlist button (name + files) and \"Add to playlist\" in every media menu"
section: "D. Library"
video_ref: "13:21–13:31"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/medialibrary
branch: master-pkt-3032-playlists-create-button
base_sha: ce4af2f
target_files:
  - modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml
  - modules/gui/qt/medialibrary/qml/NewPlaylistDialog.qml
  - modules/gui/qt/medialibrary/mlplaylistlistmodel.cpp
  - modules/gui/qt/dialogs/playlists/playlists.cpp
  - modules/gui/qt/menus/qml_menu_wrapper.cpp
  - modules/gui/qt/tests/test_ml_playlist_create.cpp
  - modules/gui/qt/tests/tst_NewPlaylist.qml
  - modules/gui/qt/meson.build
estimate_h: 12.0
priority: P2
depends_on: [PKT-3027]
upstream_ruling: not-needed
target: upstream
mr_title: "qml: create playlists from a button, add to playlist from every menu"
status: ready
actual_h_bugfix: 0.2
---

# PKT-3032 — Playlists get a New playlist button (name + files) and "Add to playlist" in every media menu

## 1. Scope & intent
At 13:21–13:31: "Click on this button, write a name for your playlist, and select your files. That's it. Or, if you're
somewhere else, select a file and add it to a playlist." Frames `s015` 13:24 (a **New Playlist** dialog: *Playlist
details* with a name field `Fav Episodes` and a thumbnail; *Select files* with a drop zone "Drag files into the box or
click here to select files" and a `+`; Cancel / Save) and `s016` 13:30 (a media "…" menu with Play, Mark as Watched,
Favorite, Stream, Convert, Add to Queue, **Add to Playlist**, Show in File Explorer, Add Subtitle/Audio Tracks).

After this packet:
1. Video ▸ Playlists and Music ▸ Playlists (and PKT-3027's Playlists section) show a **New playlist** action in the
   header and in the empty state; it opens a dialog with a name and an optional list of files (drop or pick).
2. A playlist can be created **empty** (today impossible, §2).
3. "Add to playlist" is offered in the play queue's context menu and in the network/browse media menu, where it is
   missing today.
4. Bug fix: dropping files from the desktop onto the playlists list (outside a row) no longer throws.

One MR, four commits:
1. `qml: fix dropping external files on the playlists list`
2. `qt: allow creating an empty media library playlist`
3. `qml: new playlist dialog with name and files`
4. `qt: add "Add to playlist" to the queue and network context menus`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml:525-534` (`EmptyLabelHint`) — "No playlists found / Right
  click on a media to add it to a playlist": the only documented way to make a playlist is the media context menu.
  `:105` sort model, `:140-147` `_actionAtIndex`; no create action anywhere in the page.
- `modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml:165-188` (`_dropAction`) — for an external drop outside a
  row (`index === undefined`) the URL branch calls `DialogsProvider.playlistsDialog(inputItems, …)` (`:181`), but
  `inputItems` is not defined in that branch (it built `urlList`): a `ReferenceError`, the drop is lost.
- `modules/gui/qt/dialogs/playlists/playlists.cpp:48-138` (`PlaylistsDialog`) — the "Add to Playlist" widget dialog:
  existing-playlist tree + "New Playlist" line edit; `:133-137` `assert(!media.isEmpty())` — the dialog cannot be
  opened without media; `:178-207` `accept()` calls `create(text, m_ids)` or `append`.
- `modules/gui/qt/medialibrary/mlplaylistlistmodel.cpp:87-116` (`create`) — creates on the ML thread then, on the UI
  thread, `append(createdId, initialItems)` unconditionally (`:111-114`); `:118-122` `append` starts with
  `assert(ids.size() > 0)`: `create(name, [])` aborts in debug builds and appends nothing in release.
- `modules/gui/qt/medialibrary/qml/MLContextMenu.qml:85-90` + `:164-169` — library media already have "Add to a
  playlist" (and an audio-only/video-only variant); `modules/gui/qt/menus/qml_menu_wrapper.cpp:901-905` — so do playlist items.
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:1052-1215` (`PlaylistContextMenu::popup`, the queue) and `:940-1035`
  (`NetworkMediaContextMenu`, `NetworkDeviceContextMenu`) — no "Add to playlist" entry.
- `include/vlc_media_library.h:98-102` — playlist types: `VIDEO` = "containing at least one video track". The Video ▸
  Playlists page lists `PLAYLIST_TYPE_VIDEO` (`modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml:112-113`), so a freshly created **empty**
  playlist may not show on that page until it gets a video (§5.2).

Settings / variables involved: none.
Mock boundary: QtTest with `VLCTestingEnv` + `MediaLib` as in `modules/gui/qt/tests/test_ml_model.cpp:130-135`; the dialog's
validation (name trimmed and non-empty, file list de-duplicated) is a pure function tested in a QML `TestCase`.

### Overlaps & contradictions
- **PKT-3028** adds a page "…" menu to the same header; "New playlist" goes there on playlist pages and also as a
  visible `+` (the video shows a button). Rebase on 3028 if it lands first.
- **PKT-3031** calls `MLPlaylistListModel::create` with URLs for "Save as playlist"; it relies on commit 2.
- **PKT-3014** cleans up the player context menu; the queue context menu (`PlaylistContextMenu`) is not the player
  menu — no conflict, but 3014's owner should know a new entry appears in the queue menu.

## 2b. Upstream state
Master has playlists pages (Video and Music), "Add to a playlist" on library items with a widget dialog that can also
create a playlist by name, drag-and-drop of media onto a playlist, rename and delete. Missing: a create button, an
empty playlist, the entry in the queue/network menus, and the drop fix. Narrowed accordingly.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `MLPlaylistListModel::create` | ML test instance | `create("Empty", {})` | no assertion; `transactionPending` returns to false; one playlist "Empty" exists | `tests/test_ml_playlist_create.cpp::testCreateEmpty` |
| 2 | same | 2 external URLs | `create("Two", urls)` | playlist with 2 media (guards today's path) | `test_ml_playlist_create.cpp::testCreateWithUrls` |
| 3 | `NewPlaylist.validate` | name `"  "` | validate | invalid, Save disabled | `tests/tst_NewPlaylist.qml::test_blankName` |
| 4 | same | two drops with the same URL | merge | list has it once | `tst_NewPlaylist.qml::test_dedupFiles` |
| 5 | `PlaylistMediaList._dropAction` | external drop of 2 files, no index | drop | playlists dialog receives the 2 URLs, no `ReferenceError` | `tst_NewPlaylist.qml::test_dropOutsideRow` (loads the function with a stub `DialogsProvider`) |
| 6 | Queue context menu | 2 queue items selected | open menu | "Add to playlist…" present, opens dialog with 2 items | MANUAL `pkt3032-queue-menu.png` |
| 7 | New playlist dialog | Video ▸ Playlists | click New playlist, name, drop 3 files, Save | playlist listed with 3 items | MANUAL `pkt3032-new-dialog.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_ml_model.cpp:115-140` + meson `modules/gui/qt/meson.build:1306-1338`. Rows 1–2 need a real
  `vlc_medialibrary_t`: Step 2 checks whether `VLCTestingEnv` can load the `medialibrary` module with a temp DB as
  `test/modules/misc/medialibrary.c:109-138` does; if not, rows 1–2 move to a C test next to it calling
  `vlc_ml_playlist_create` and the fixed guard is a static helper `MLPlaylistListModel::needsAppend(ids)` tested in Qt.
- QML: `tests/tst_FSM.qml` pattern; `qml_tests` (`modules/gui/qt/meson.build:954-956`).
- Sample media: two `file://` URLs of empty files in a `QTemporaryDir`.

## 5. Design decisions & open questions
1. NOTE — the bug fix (commit 1) is independent and can go upstream alone first.
2. **RULED — where an empty playlist shows.** RULED 2026-10-14 (R0): option (a): an empty playlist shows under `PLAYLIST_TYPE_ALL` on the top-level Playlists section. With the ML's type rule, an empty playlist is neither audio nor video.
   Options: (a) show `PLAYLIST_TYPE_ALL` on the new top-level Playlists section (PKT-3027) and keep typed lists in
   Video/Music; (b) create from Video ▸ Playlists only with files. Proposal (a); 
3. NOTE — the dialog is QML (matches the redesign's look; `dialogs/dialogs/qml/ModalDialog.qml` is the base); the
   widget `PlaylistsDialog` stays for "Add to playlist" from menus.
4. NOTE — the thumbnail in the video's dialog is the ML's generated playlist cover (`coverPrefix`,
   `modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml:120`); no custom thumbnail picker.

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
**Bugfix part done** (commits 1 and 2 of §1 only; dialog and menus pending on PKT-3027/3028). Status stays `ready`.
- Branch `master-pkt-3032-playlists-create-button-bugfix` (from `ce4af2f`), PR https://github.com/oscarolar/vlc/pull/3
- `95f346b` `qt: allow creating an empty media library playlist` — guard in `create()`: append only when `initialItems` is
  not empty. Row 1 (`testCreateEmpty`) and row 2 (`testCreateWithItems`, with MLItemIds instead of URLs) live in
  `modules/gui/qt/tests/test_ml_playlist_create.cpp` (`test_qt_ml_playlist_create`). §4 settled: no real ML needed; the
  stub ML module of `VLCTestingEnv` got an optional `mlControl` callback, and `vlc_tests` got a `ui_files` key (the model
  includes `dialogs_provider.hpp`, which needs `ui_open*.h`). Red: `Assertion 'ids.size() > 0' failed`, SIGABRT
  (`~/src/logs/pkt-3032-red-testlog.txt`); green: 4 passed (`~/src/logs/pkt-3032-green-testlog.txt`).
- `ccc7bfc` `qml: fix undefined inputItems in PlaylistMediaList` — pass `urlList`. Row 5 replaced for now by qmllint
  evidence (`181:49: Unqualified access` on `inputItems` before, gone after: `~/src/logs/pkt-3032-qmllint-{before,after}.log`)
  plus MANUAL: Video ▸ Playlists, drag 2 files from the file manager onto the empty list area → "Add to Playlist" dialog
  with "2 Media", no `ReferenceError` in the console.
- Gate: `-Dextra_checks=true` build, `meson test --suite qt` 8/8 OK, no new warnings in touched files
  (`~/src/logs/pkt-3032-final.log`).

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3032-playlists-create-button` (from `ce4af2f`; rebase on 3027/3028 when merged); §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2; settle §4's question (Qt test with a real ML vs C test).
- [ ] **Step 3: First row watched failing** — row 5 (`test_dropOutsideRow`) → FAIL with `ReferenceError: inputItems is not defined`; fix `modules/gui/qt/medialibrary/qml/PlaylistMediaList.qml:181` to pass `urlList`. Commit `qml: fix dropping external files on the playlists list`.
- [ ] **Step 4: Empty create** — row 1 red (abort on `assert(ids.size() > 0)`); guard `modules/gui/qt/medialibrary/mlplaylistlistmodel.cpp:111-114` with `!initialItems.isEmpty()`; row 2 green too. Commit `qt: allow creating an empty media library playlist`.
- [ ] **Step 5: Dialog** — rows 3–4 red; `medialibrary/qml/NewPlaylistDialog.qml` + header/empty-state action in `PlaylistMediaList.qml`; screenshot row 7. Commit `qml: new playlist dialog with name and files`.
- [ ] **Step 6: Menus** — `qml_menu_wrapper.cpp` `PlaylistContextMenu::popup` and `NetworkMediaContextMenu::popup`: "Add to playlist…" → `DialogsProvider::playlistsDialog(inputItems)`; screenshot row 6. Commit `qt: add "Add to playlist" to the queue and network context menus`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: MR to videolan/vlc master (Draft).
