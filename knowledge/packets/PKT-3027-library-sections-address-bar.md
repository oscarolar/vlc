---
id: PKT-3027
title: "The library shows Videos, Music, Network streams and Playlists, with a clickable address line and back/forward"
section: "D. Library"
video_ref: "09:24–10:28"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/maininterface
branch: master-pkt-3027-library-sections-address-bar
base_sha: ce4af2f
target_files:
  - modules/gui/qt/maininterface/navigationmodel.cpp
  - modules/gui/qt/maininterface/qml/MainDisplay.qml
  - modules/gui/qt/maininterface/qml/LocalTopbar.qml
  - modules/gui/qt/util/navigation_history.hpp
  - modules/gui/qt/util/navigation_history.cpp
  - modules/gui/qt/network/qml/NetworkAddressbar.qml
  - modules/gui/qt/widgets/qml/PageExt.qml
  - modules/gui/qt/tests/test_navigation_model.cpp
  - modules/gui/qt/tests/test_navigation_history.cpp
  - modules/gui/qt/tests/tst_NetworkAddressbar.qml
  - modules/gui/qt/meson.build
estimate_h: 20.0
priority: P1
depends_on: []
upstream_ruling: needed
target: fork
mr_title: "qt: library sections, forward history and an address line on every page"
status: review
actual_h: 0.4
---

# PKT-3027 — The library shows Videos, Music, Network streams and Playlists, with a clickable address line and back/forward

## 1. Scope & intent
At 09:24–10:28 the designer walks through the VLC 4 library she found and names three complaints: (a) the
sidebar mixes library sections with a "Browse" entry that wears a Wi-Fi icon and is the only place where folders
get added; (b) there is only a back button, no forward and no address line to jump up a folder; (c) "how are you
supposed to get to your library? The back button, of course" — the library is not exposed. Her cleaned-up target
(frames `s012`–`s014`, 10:12–10:28 and 11:40–12:10): a narrow icon rail with **Videos, Music, Network streams,
Playlists**, and, above the content, a row with **← →** and an address line `Music ▸ Songs ▸ My Songs` whose
segments are clickable.

After this packet:
1. The sidebar top level is Home, Video, Music, Network streams, Playlists (Browse and Discover move under
   Network streams — §5.1 is ruled (R6)).
2. `History` gains a forward stack; the local top bar shows a forward button next to back.
3. Every `PageExt` header can show the page's path as an address line (the existing `NetworkAddressbar`, made
   generic), not only the Browse tree.
4. The dead `mlsettings` page entry of `MainDisplay.qml` is removed.

One MR, four commits:
1. `qt: add a forward stack to the navigation history`
2. `qml: show a forward button in the local top bar`
3. `qt: regroup the navigation model into library sections`
4. `qml: show the address line in every page header`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/maininterface/navigationmodel.cpp:266-295` (`NavigationModelPrivate::initilializeModel`) — builds
  the sidebar tree: Home; Video {All, Playlists}; Music {Artists, Albums, Tracks, Genres, Playlists} (ML only);
  Browse (`"network"`, `ic_fluent_wifi_1.svg` — the Wi-Fi icon of the video, `:286`); Discover {Services, URL}.
  Playlists exist twice, once per media type (`:274`, `:282`), never as a section of their own.
- `modules/gui/qt/maininterface/qml/MainDisplay.qml:107-129` (`pageModel`) — maps the top-level names `home`,
  `video`, `music`, `network`, `discover`, `mlsettings` to QML pages. `:126-128` points `mlsettings` at
  `qrc:///qt/qml/VLC/MediaLibrary/MLFoldersSettings.qml`, a file that does not exist at `ce4af2f`
  (`git ls-files | grep MLFoldersSettings` → 0): a dead entry.
- `modules/gui/qt/maininterface/qml/MainDisplay.qml:554-623` (`SideNavigationPane` + `onItemClicked`) — pushes the
  clicked URI on `History`; collapsed items pop a `NavigationBarContextMenu` instead (`:607-614`).
- `modules/gui/qt/maininterface/qml/LocalTopbar.qml:95-157` (`navigationButtonsRow`) — sidebar toggle, one
  `historyBack` button bound to `History.previous()` / `!History.previousEmpty` (`:124-147`), then the cone.
  No forward button.
- `modules/gui/qt/util/navigation_history.cpp:16-43` (`push`) and `:137-155` (`previous`) — a single
  `std::vector` stack (`modules/gui/qt/util/navigation_history.hpp:84`); `previous()` pops and discards, so there is nothing to go
  forward to. `:11-14` `isPreviousEmpty` is the only exposed state.
- `modules/gui/qt/network/qml/NetworkAddressbar.qml:51-71` (`createContentModel`) — pure JS that splits `path`
  into visible crumbs and an overflow menu by measured width; `:94-117` home button, `:175-188` crumb buttons that
  emit `browse(tree)` for every crumb but the last. Used only by `modules/gui/qt/network/qml/BrowseTreeHeader.qml:76-92`.
- `modules/gui/qt/widgets/qml/PageExt.qml:112-132` + `:134-196` (`DefaultPageHeader`) — every library page header
  is a title label plus `GridSortFilterControls`; no path.
- `modules/gui/qt/tests/test_navigation_model.cpp:172-192` (`testInitialValues`, `testInitialValuesNoML`) — pin
  today's 5 rows (Home, Video, Music, Browse, Discover) and 3 rows without ML; they must change with commit 3.

Settings / variables involved: `MainCtx.navigationPanel` (sidebar width, `modules/gui/qt/maininterface/qml/MainDisplay.qml:625`); none in
`src/libvlc-module.c`.
Mock boundary: `NavigationModel` and `NavigationHistory` are plain QObjects — QtTest without a media library
(`modules/gui/qt/tests/test_navigation_model.cpp:158-163` already does so). `NetworkAddressbar.createContentModel` runs in a QML
`TestCase` with a fixed `availableWidth`.

### Overlaps & contradictions
- **PKT-3033** (pins, recent, home) also edits `navigationmodel.cpp` and `SideNavigationPane.qml`: this packet owns
  the section list, 3033 appends pinned/recent rows after it. Merge 3027 first.
- **PKT-3028, 3032** edit `PageExt.qml`'s header (sort/view/search and a "+" button). This packet only adds the
  address line to the left of `GridSortFilterControls`; the others add to the right.
- **PKT-3031** owns what "Network streams" shows; this packet only creates the section and routes it to the
  existing `BrowseDisplay`/`DiscoverDisplay` pages.
- **PKT-3008/3009/3013** touch the player `TopBar.qml`, not `LocalTopbar.qml`. None detected on the latter.

## 2b. Upstream state
VLC 4 master already ships most of what the video calls "the new design": a sidebar with Home, Video, Music,
Browse, Discover (`modules/gui/qt/maininterface/navigationmodel.cpp:266-295`), a Home page (`medialibrary/qml/HomeDisplay.qml`), back
navigation, and a clickable breadcrumb with overflow menu — but **only inside Browse**
(`modules/gui/qt/network/qml/BrowseTreeHeader.qml:76`). What master does not have: a forward history, an address line in library pages, a
top-level Playlists section and a "Network streams" name. The packet is narrowed to those four plus the dead
`mlsettings` entry. The "folders appear mixed regardless of folder" complaint (09:43) is already answered by
master's grouping-by-folder in the Video sort menu (`modules/gui/qt/medialibrary/qml/VideoAllSubDisplay.qml:39-54`); not redone.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `NavigationHistory::previous`/`next` | history `[home]→[video]→[music]` | `previous()` | viewPath `video`, `nextEmpty == false` | `tests/test_navigation_history.cpp::testPreviousKeepsForward` |
| 2 | `NavigationHistory::next` | state of row 1 | `next()` | viewPath `music`, `nextEmpty == true`, `navigate` emitted once | `test_navigation_history.cpp::testNextRestores` |
| 3 | `NavigationHistory::push` | state of row 1 | `push([network])` | forward stack cleared, `nextEmpty == true` | `test_navigation_history.cpp::testPushClearsForward` |
| 4 | `NavigationHistory::previous` invalid node | a node whose property is not valid (`isNodeValid`, `modules/gui/qt/util/navigation_history.cpp:45-60`) | `previous()` then `next()` | the invalid node is skipped in both directions | `test_navigation_history.cpp::testSkipInvalidBothWays` |
| 5 | `NavigationModel` top level with ML | `hasMedialib = true` | model built | rows Home, Video, Music, Network streams, Playlists | `test_navigation_model.cpp::testInitialValues` (rewritten) |
| 6 | `NavigationModel` without ML | `hasMedialib = false` | model built | rows Home, Network streams | `test_navigation_model.cpp::testInitialValuesNoML` (rewritten) |
| 7 | `NavigationModel` URI | expand Playlists | `URI` role of its children | `["playlists","video"]`, `["playlists","music"]` | `test_navigation_model.cpp::testURIPlaylists` |
| 8 | `NetworkAddressbar.createContentModel` | path of 5 crumbs, width for 2 | `availableWidth` set | 2 crumbs visible, 3 in the overflow menu, last crumb always visible | `tests/tst_NetworkAddressbar.qml::test_overflow` |
| 9 | `NetworkAddressbar` click | 3 crumbs | click crumb 0 | `browse` emitted with crumb 0's tree; last crumb click emits nothing | `tst_NetworkAddressbar.qml::test_click` |
| 10 | Library page header | Music ▸ Albums ▸ an album | screenshot | address line `Music ▸ Albums ▸ <album>` left of sort/search; ← → in top bar | MANUAL `pkt3027-addressline.png` |
| 11 | Sidebar | default window | screenshot | five sections, no Wi-Fi icon for the library | MANUAL `pkt3027-sidebar.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:148-170` (`VLCTestingEnv`, `QAbstractItemModelTester`,
  row-insert spies) for rows 5–7; meson block `modules/gui/qt/meson.build:1342-1360` for the new `test_qt_navigation_history`
  (sources `util/navigation_history.cpp`; it includes `network/networkmediamodel.hpp` and
  `medialibrary/mlqmltypes.hpp` at `modules/gui/qt/util/navigation_history.cpp:3-4` — only the headers are needed for `isNodeValid`;
  if the link pulls more, move `isNodeValid` behind a small header, noted in §5).
- QML: `modules/gui/qt/tests/tst_FSM.qml:1-7` pattern (`import QtTest`, `TestCase`), registered in `qml_tests`
  (`modules/gui/qt/meson.build:954-956`). The test instantiates `NetworkAddressbar` with a plain JS `path` array; `VLC.Network`
  must be importable by `qml_test` (`modules/gui/qt/meson.build:1433-1443` links `qmllibs`).
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED — sidebar sections.** RULED 2026-10-14 (R6): Browse and Discover merge as children of "Network streams"; local folders are reachable from the address line's home and "Add folder". Replacing Browse/Discover with "Network streams" and promoting Playlists
   changes a VideoLAN design choice (`modules/gui/qt/maininterface/navigationmodel.cpp:266-295`). Browse also lists local folders and devices
   (`BrowseHomeDisplay.qml`), which "Network streams" does not name. Proposal: Network streams = Browse + Discover
   as children; local folders reachable from the address line's home and from PKT-3028's "Add folder". 
2. **RULED — forward history.** RULED 2026-10-14 (R0): a forward-history stack, capped at 50 entries. Additive, but `History` is a singleton used by every page; a second stack
   changes how `previous()` releases `QQmlPropertyMap`s (they are now kept). Memory cost is bounded by the stack
   depth; cap at 50 entries.
3. NOTE — `mlsettings` dead entry is a bug-level fix and can go upstream alone as `qml: remove dead mlsettings page`.
4. NOTE — the address line on library pages derives its crumbs from `History.viewPath` + the page `title`, not
   from a filesystem path: ML pages have no tree. `NetworkAddressbar` gets a `crumbs` input of `{display, uri}`;
   Browse keeps passing `{display, tree}`.
6. NOTE (executor) — the link did pull more: `isNodeValid` needs `NetworkTreeItem`'s moc, which drags in
   the whole network model. It moved to its own file (navigation_history_node.cpp, function
   `isNavigationNodeValid`), and `NavigationHistory` now takes the validator in its constructor. The QtTest
   passes a stand-in validator for row 4.
5. **Split (T3, 20 h)**: 3027a forward history + button (rows 1–4, ~6 h, T2); 3027b sections (rows 5–7, 11, ~6 h,
   follows ruling 1 (R6)); 3027c address line everywhere (rows 8–10, ~8 h). 3027a can merge without any ruling.

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
Executed 2026-10-15 on `nzxt`, worktree `~/src/vlc-wt/pkt-3027` (branch `master-pkt-3027-library-sections-address-bar`
from `ce4af2f`), the CI image of §6, `meson setup build-meson -Dextra_checks=true`, `ninja -j8`.
Logs: `~/src/logs/pkt-3027-*.log`.

- Commits (one PR, the four commits of §1):
  - `e186c63` qt: add a forward stack to the navigation history
  - `52182ea` qml: show a forward button in the local top bar
  - `18afee8` qt: regroup the navigation model into library sections
  - `1e7fcaf` qml: show the address line in every page header
- PR: https://github.com/oscarolar/vlc/pull/5 (fork, base `master`).
- Step 2 dry-run: every range of §2 matched at `ce4af2f`. MLFoldersSettings.qml is still absent
  (`git ls-files | grep -c MLFoldersSettings` gives 0).
- Watched failing, then passing:
  - Rows 1 to 4 (`pkt-3027-stepA-red.log`, `pkt-3027-stepB-red.log`, `pkt-3027-stepC-green.log`).
    - Row 1 first failed with the placeholder expectation (Actual "video", Expected "home").
    - The real tests then failed to compile because `next`, `isNextEmpty` and `nextEmptyChanged` did not exist.
    - After the change, `test_qt_navigation_history` passed 6 of 6 functions: `testPreviousKeepsForward`,
      `testNextRestores`, `testNextKeepsProperties`, `testPushClearsForward`, `testSkipInvalidBothWays` and
      `testForwardCapped` (the 50-entry cap).
  - Rows 5 to 7 (`pkt-3027-c3-red.log`, `pkt-3027-c3-green.log`).
    - With the rewritten `test_qt_navigation_model`, 14 of 17 functions failed against the old model.
    - After the change, 17 of 17 passed. The new functions are `testURIPlaylists`, `testURINetworkStreams`,
      `testExpandNetworkStreams` and `testRetractNetworkStreams`.
  - Rows 8 and 9 (`pkt-3027-c4-red.log`, `pkt-3027-c4-dedupe-red.log`, `pkt-3027-c4-green.log`).
    - `tst_NetworkAddressbar` failed first because the Helpers functions did not exist.
    - After the change, `test_overflow`, `test_click` and `test_pathCrumbs` passed.
    - One more assertion, for a parent and default sub page with the same name, was added red and then passed.
- Gate (`pkt-3027-gate.log`): `meson test -C build-meson --suite qt --print-errorlogs` gave Ok 9, Fail 0 (7 at base,
  plus `test_qt_navigation_history` and `tst_NetworkAddressbar`).
- Warnings: the touched C++ files were recompiled in the gate run with no compiler warning. The only warnings are the
  generic cc1plus `-Werror=unterminated-string-initialization` notice and meson's configure notices, which baseline
  has too.
- qmllint, Qt 6 (`pkt-3027-qmllint-c2-*`, `-c3-*` and `-c4-*` logs). Warning counts per file, before and after:

  | File | Before | After |
  |---|---|---|
  | LocalTopbar | 144 | 170 |
  | MainDisplay | 492 | 506 |
  | EmptyLabelButton | 32 | 32 |
  | SideNavigationDelegate | 83 | 83 |
  | Helpers | 5 | 5 |
  | NetworkAddressbar | 131 | 133 |
  | PageExt | 77 | 97 |
  | tst_NetworkAddressbar (new) | n/a | 14 |

  Every added line is an unresolved-VLC-type warning (`Widgets.*`, `Navigation`, `AccessibleCompat`, `Helpers`,
  `VLC.Network` without qmltypes). There is no new semantic warning.
- Runtime smoke: `vlc-static -I qt --aout dummy` with the offscreen platform loaded the interface with no QML error.
  - This build has no media library module, so only the no-media-library sections could be reached.
  - For one local run that was not committed, the start page was set to the Discover page and a console trace was
    added. The loaded page was `streams/discover/services/all`, and its address line was
    `Network streams > Discover > Services > Services`. That run led to the duplicate-crumb rule in `Helpers.pathCrumbs`.
- Rows 10 and 11 (MANUAL): screenshots are still pending. The build environment has no display, so they must be taken
  on a desktop build and attached to the PR.
- Deviations:
  - Tests:
    - Rows 8 and 9 test the Helpers functions that NetworkAddressbar delegates to (`splitAddressCrumbs`,
      `addressCrumbAction`, `pathCrumbs`), not an instance of the control. The QML test runner only loads VLC.Util,
      and the control needs MainCtx and VLCStyle.
    - Row 4 uses a stand-in validator (see §5.6). The real `NetworkTreeItem` check cannot be linked into a QtTest
      without the network model.
  - New files outside target_files:
    - navigation_history_node.cpp and navigation_history_node.hpp hold the node validity check moved out of the history.
    - Makefile.am also gets the new sources, the new QtTest and the new QML test, to keep the autotools lists in step.
      This is not built here.
  - Choices made in this packet:
    - Forward button: the icon font has no forward arrow, so the back glyph is mirrored with a Scale transform.
    - Section routing:
      - Network streams is a `streams` page and Playlists a `playlists` page. Each is an inline `SectionLoader`
        (a PageLoader that forwards the display margins) in MainDisplay.qml, so no new QML file was needed.
      - Browse pages move from `network/...` to `streams/network/...`. Sort settings saved under the old keys are not
        migrated.
      - `EmptyLabelButton` now pushes `streams/network`.
    - Sidebar: the `AccessibleCompat` ids become `networkStreamsNavBar` and `playlistsNavBar`, in place of
      `browseNavBar` and `discoverNavBar`. Video and Music keep their own Playlists sub entries.
    - Address line:
      - It replaces the title label when `pagePrefix.length > 1`.
      - Its crumbs come from the page path and the page title (§5.4). A last crumb named like its parent replaces
        the parent.
      - Its home button pushes `streams/network`, the Browse home where local folders are (R6).
  - Baseline: the commit 1 edits were saved while the baseline build was running, which broke that build's link step.
    The 7/7 result on `ce4af2f` is documented in BUILD-NOTES.

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3027-library-sections-address-bar ce4af2f`; build once with the §6 command → green.
- [ ] **Step 2: Dry-run** — re-read every range of §2; update §2/`base_sha` if moved. Confirm `MLFoldersSettings.qml` is still absent.
- [ ] **Step 3: First row watched failing** — create `tests/test_navigation_history.cpp` + its `vlc_tests` block; row 1 with `QCOMPARE(viewPath, "home")` → FAIL, then the real expectation (fails until `next()` exists).
- [ ] **Step 4: Forward stack** — `navigation_history.hpp/.cpp`: `m_forward`, `Q_PROPERTY(bool nextEmpty …)`, `Q_INVOKABLE next()`, `push()` clears forward; rows 1–4 green. Commit `qt: add a forward stack to the navigation history`.
- [ ] **Step 5: Forward button** — `modules/gui/qt/maininterface/qml/LocalTopbar.qml:124-147`, add `historyForward` after `historyBack` (`VLCIcons` forward glyph, `Navigation.left/right`, `AccessibleCompat.id: "historyForwardButton"`). Commit `qml: show a forward button in the local top bar`.
- [ ] **Step 6: Sections** (§5.1 is ruled: R6) — rewrite rows 5–7 first (red), then `modules/gui/qt/maininterface/navigationmodel.cpp:266-295` and `modules/gui/qt/maininterface/qml/MainDisplay.qml:107-129` (add `playlists`, rename network, drop `mlsettings`). Commit `qt: regroup the navigation model into library sections`.
- [ ] **Step 7: Address line** — `tests/tst_NetworkAddressbar.qml` rows 8–9 red; generalise `NetworkAddressbar` input; put it in `PageExt.DefaultPageHeader` (`modules/gui/qt/widgets/qml/PageExt.qml:175-195`) when `pagePrefix.length > 1`. Commit `qml: show the address line in every page header`.
- [ ] **Step 8: Whole gate** — §6 command; paste the summary in §6 Evidence; screenshots rows 10–11.
- [ ] **Step 9: Commit and MR** — push to the fork, PR to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
