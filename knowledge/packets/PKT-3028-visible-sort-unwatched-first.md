---
id: PKT-3028
title: "Video pages sort by Unwatched / Watched / Newest / Oldest / Title, show the item count and a page menu"
section: "D. Library"
video_ref: "10:28–10:57"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/medialibrary
branch: master-pkt-3028-visible-sort-unwatched-first
base_sha: ce4af2f
target_files:
  - modules/gui/qt/medialibrary/qml/VideoAll.qml
  - modules/gui/qt/medialibrary/mlvideogroupsmodel.cpp
  - modules/gui/qt/medialibrary/mlvideofoldersmodel.cpp
  - modules/gui/qt/widgets/qml/SortControl.qml
  - modules/gui/qt/widgets/qml/GridSortFilterControls.qml
  - modules/gui/qt/util/qml/Helpers.qml
  - modules/gui/qt/dialogs/dialogs_provider.hpp
  - modules/gui/qt/dialogs/dialogs_provider.cpp
  - modules/gui/qt/tests/test_ml_sort_criteria.cpp
  - modules/gui/qt/tests/tst_SortSelection.qml
  - modules/gui/qt/meson.build
estimate_h: 10.0
priority: P2
depends_on: [PKT-3027]
upstream_ruling: not-needed
target: upstream
mr_title: "qml: watched/unwatched and date sorting for videos, item count"
status: ready
---

# PKT-3028 — Video pages sort by Unwatched / Watched / Newest / Oldest / Title, show the item count and a page menu

## 1. Scope & intent
At 10:28–10:57 the designer asks where sorting is ("only in the right-click menu and you're just supposed to
know"), wants "standard sorting, view mode and search items", explicitly adds **Unwatched first / Watched first**,
and a "…" menu with Add file / Add folder (frame `s013` 10:48–10:57: header row `15 items  ⇅  ▦  Search  …`; the
sort popup lists Unwatched First ✓, Watched First, Newest First, Oldest First, Title A-Z, Title Z-A; the "…" popup
lists Add File, Add Folder, Edit Subtitle Tracks, Edit Audio Tracks, Resync, Info).

Master already shows sort, grid/list and search in every page header (§2b), so this packet is narrowed to:
1. Sort entries that carry their own direction ("Title A-Z" and "Title Z-A" are two entries), and the four new
   video entries Unwatched first, Watched first, Newest first, Oldest first — on the flat video list **and** on
   the grouped (by name / by folder) lists.
2. An item count left of the sort button.
3. A page "…" menu with **Add folder** (adds a media-library entry point) and **Rescan** (reloads the library).
   Add file, subtitle/audio track editing and Info are left to PKT-3017/3034 (§5.4).

One MR, three commits:
1. `qt: allow playcount and insertion sorting on video groups and folders`
2. `qml: sort entries carry their own order; add watched and date sorting to videos`
3. `qml: show the item count and a page menu in library headers`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/widgets/qml/PageExt.qml:134-196` (`DefaultPageHeader`) — title + `GridSortFilterControls`;
  `modules/gui/qt/widgets/qml/GridSortFilterControls.qml:20-79` — grid/list toggle (`:20-32`), `SortControl` (`:34-57`),
  `SearchBox` (`:59-79`). This is the "standard sorting, view mode and search" of the video — already shipped.
- `modules/gui/qt/widgets/qml/SortControl.qml:85-95` (`onSelected`) — a different key → `sortSelected(key)` +
  `Ascending`; the same key → toggles order. An entry cannot impose a direction, so "Watched first" and "Unwatched
  first" (one criterion, two orders) cannot be expressed.
- `modules/gui/qt/medialibrary/qml/VideoAll.qml:87-90` (`sortModel`) — only Alphabetic and Duration for videos.
- `modules/gui/qt/medialibrary/mlvideomodel.cpp:100-108` (`MLVideoModel::nameToCriteria`) — already maps
  `"playcount"` → `VLC_ML_SORTING_PLAYCOUNT` and `"insertion"` → `VLC_ML_SORTING_INSERTIONDATE`; QML never uses them
  for videos.
- `modules/gui/qt/medialibrary/mlvideogroupsmodel.cpp:145-152` and `modules/gui/qt/medialibrary/mlvideofoldersmodel.cpp:101-107` — the grouped
  models know only title/duration(/date): playcount silently falls back to `VLC_ML_SORTING_DEFAULT`.
- `modules/gui/qt/medialibrary/qml/VideoAllSubDisplay.qml:45-54` + `:130-135` — the model is switched by
  `MainCtx.grouping` and all three take `root.sort.criteria`; `:39-43` custom `SortMenuVideo` adds the grouping
  entries (`modules/gui/qt/menus/qml_menu_wrapper.cpp:176-215`).
- `include/vlc_media_library.h:431-443` — the ML sorting enum has `VLC_ML_SORTING_PLAYCOUNT`, no progress/last-played
  criterion; `modules/misc/medialibrary/medialibrary.cpp:1748-1749` maps it to `medialibrary::SortingCriteria::PlayCount`.
- `modules/gui/qt/medialibrary/mlvideo.cpp:81` — `m_isNew = (m_playCount == 0 && m_progress <= 0)`: the UI's own
  definition of "unwatched".
- `modules/misc/medialibrary/medialibrary.cpp:1883-1887` (`VLC_ML_MEDIA_UPDATE_PROGRESS` → `setLastPosition`) and
  `:1953-1962` (`VLC_ML_MEDIA_SET_PLAYED` → `markAsPlayed`) — playcount is raised by libmedialibrary when a position
  near the end is stored or on "Mark as seen" (`modules/gui/qt/medialibrary/qml/MLContextMenu.qml:91-97`).
- `modules/gui/qt/medialibrary/mlfoldersmodel.cpp:194-200` (`MLFoldersModel::add`) and
  `modules/gui/qt/medialibrary/medialib.cpp:241-248` (`MediaLib::reload`) — the two actions the page menu needs already exist;
  today Add folder is reachable only from Browse (`modules/gui/qt/network/qml/BrowseTreeHeader.qml:94-118`) and preferences.

Settings / variables involved: per-page sort persisted under `sortCriteria/<pagePrefix>` and
`sortOrder/<pagePrefix>` (`modules/gui/qt/widgets/qml/PageExt.qml:62-86`); none in `src/libvlc-module.c`.
Mock boundary: QtTest calls `getMLSortingCriteria()` (public, `modules/gui/qt/medialibrary/mlbasemodel.hpp:108`) after `setSortCriteria`; no
media library instance needed. The selection logic is moved to a pure function in `VLC.Util` `Helpers` and tested
in a QML `TestCase` like `tests/tst_FSM.qml`.

### Overlaps & contradictions
- **PKT-3027** adds the address line on the left of the same header (`PageExt.qml`); this packet adds the count and
  "…" on the right. Rebase on 3027.
- **PKT-3032** adds a "+ New playlist" entry to playlist page headers; it should reuse this packet's page-menu
  slot rather than a second button.
- **PKT-3002** (icon sets) may restyle `VLCIcons.topbar_sort`; no conflict on behaviour.

## 2b. Upstream state
Master already has visible sort, grid/list and search controls in every page header (`GridSortFilterControls`).
The video's "sorting only in the right-click menu" describes VLC 3.0's playlist, not master. What is missing:
watched/unwatched and date sorting for videos (the criteria exist in C++ but are not offered), direction-carrying
entries, an item count, and a library-level Add folder / Rescan outside Browse and preferences.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `MLVideoModel::nameToCriteria` | model, no ML | `setSortCriteria("playcount")` | `getMLSortingCriteria() == VLC_ML_SORTING_PLAYCOUNT` (guards existing behaviour) | `tests/test_ml_sort_criteria.cpp::testVideoPlaycount` |
| 2 | `MLVideoGroupsModel::nameToCriteria` | same | `"playcount"` / `"insertion"` | `PLAYCOUNT` / `INSERTIONDATE` (today `DEFAULT`) | `test_ml_sort_criteria.cpp::testGroupsPlaycount` |
| 3 | `MLVideoFoldersModel::nameToCriteria` | same | `"playcount"` / `"insertion"` | `PLAYCOUNT` / `INSERTIONDATE` (today `DEFAULT`) | `test_ml_sort_criteria.cpp::testFoldersPlaycount` |
| 4 | `Helpers.resolveSortSelection` | entry `{criteria:"playcount", order: Qt.AscendingOrder}`, current `title`/asc | select it | returns `{criteria:"playcount", order: Asc}` | `tests/tst_SortSelection.qml::test_entryWithOrder` |
| 5 | same | current is that entry | select it again | order unchanged (fixed-direction entries do not toggle) | `tst_SortSelection.qml::test_reselectFixed` |
| 6 | same | entry without `order` (legacy) | select same key twice | toggles as today (`modules/gui/qt/widgets/qml/SortControl.qml:85-95`) | `tst_SortSelection.qml::test_legacyToggle` |
| 7 | `SortControl.show` check mark | `playcount`/desc active | build menu model | only "Watched first" is checked | `tst_SortSelection.qml::test_checkedEntry` |
| 8 | Video page | 15 videos, 3 played | sort "Unwatched first" | played ones last; header shows "15 items" | MANUAL `pkt3028-sort-unwatched.png` |
| 9 | Page "…" menu | Video page | Add folder → pick dir | folder appears in ML entry points, scan progress bar shows | MANUAL `pkt3028-page-menu.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_ml_model.cpp:115-140` (`VLCTestingEnv`, model `classBegin()` without ML) and its meson
  block `modules/gui/qt/meson.build:1306-1338`; the new test adds `mlvideomodel.cpp`, `mlvideogroupsmodel.cpp`,
  `mlvideofoldersmodel.cpp` and their item sources to `sources`/`moc_headers` (or extends `test_qt_ml_model` if the
  link set is the same — decide in Step 2).
- QML: `modules/gui/qt/tests/tst_FSM.qml:1-7` (`import VLC.Util`, `TestCase`); register `tst_SortSelection` in `qml_tests`
  (`modules/gui/qt/meson.build:954-956`).
- Sample media: none for CI; MANUAL rows use any 15 local clips.

## 5. Design decisions & open questions
1. NOTE — **"Unwatched" = playcount ascending.** libmedialibrary has no progress-based sort; a half-watched episode has
   playcount 0 and sorts with the unwatched ones, while the grid marks it with a progress bar
   (`modules/gui/qt/medialibrary/qml/VideoGridItem.qml:84`). Acceptable for the video's use (next episode). A true "in progress first" needs a new
   `VLC_ML_SORTING_LASTPLAYBACKDATE`/progress criterion in libmedialibrary — out of scope, would be a core packet.
2. NOTE — Newest/Oldest first use insertion date (when the file entered the library), not file mtime;
   `VLC_ML_SORTING_LASTMODIFICATIONDATE` exists if the owner prefers file date.
3. NOTE — direction-carrying entries are additive: entries without `order` keep today's toggle behaviour, so the
   other pages (music, playlists) are untouched unless they opt in.
4. NOTE — the video's "…" also lists Add File, Edit Subtitle/Audio Tracks, Info: Add File belongs to the open
   dialogs (PKT-3034), track editing to PKT-3017/3023; not duplicated here.
5. NOTE — the item count reads `model.count`; for grouped views it counts groups, matching what is on screen.

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
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3028-visible-sort-unwatched-first` from the merged PKT-3027 (or `ce4af2f` if 3027 is not merged; rebase later); §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; decide new test file vs extending `test_qt_ml_model`.
- [ ] **Step 3: First row watched failing** — row 2 (`MLVideoGroupsModel`, `"playcount"`) → FAIL with `DEFAULT`.
- [ ] **Step 4: C++ criteria** — add `playcount`/`insertion` to `modules/gui/qt/medialibrary/mlvideogroupsmodel.cpp:145-152` and `modules/gui/qt/medialibrary/mlvideofoldersmodel.cpp:101-107`; rows 1–3 green. Commit `qt: allow playcount and insertion sorting on video groups and folders`.
- [ ] **Step 5: Selection helper** — `tst_SortSelection.qml` rows 4–7 red; add `Helpers.resolveSortSelection(model, index, key, order)`; make `modules/gui/qt/widgets/qml/SortControl.qml:85-95` and `:100-119` use it; extend `modules/gui/qt/medialibrary/qml/VideoAll.qml:87-90` with the six entries of the video. Commit `qml: sort entries carry their own order; add watched and date sorting to videos`.
- [ ] **Step 6: Count and page menu** — `GridSortFilterControls.qml`: count label + "…" `IconToolButton` opening a `NativeMenu` with Add folder (a new `DialogsProvider` slot `addMediaLibraryFolder()` that wraps the static, QML-invisible `getDirectoryDialog(p_intf)` of `modules/gui/qt/dialogs/dialogs_provider.hpp:142` and calls `vlc_ml_add_folder` the way `MLFoldersModel::add` does) and Rescan (`MediaLib.reload()`); rows 8–9 screenshots. Commit `qml: show the item count and a page menu in library headers`.
- [ ] **Step 7: Whole gate** — §6 command; paste summary in §6 Evidence.
- [ ] **Step 8: Commit and MR** — MR to videolan/vlc master (Draft); `status: review`.
