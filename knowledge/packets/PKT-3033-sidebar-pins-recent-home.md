---
id: PKT-3033
title: "Pinned items and a Recent entry in the sidebar; Home adds Listen again and Frequently visited folders"
section: "D. Library"
video_ref: "13:31–13:56"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/maininterface
branch: master-pkt-3033-sidebar-pins-recent-home
base_sha: ce4af2f
target_files:
  - modules/gui/qt/maininterface/navigationmodel.hpp
  - modules/gui/qt/maininterface/navigationmodel.cpp
  - modules/gui/qt/maininterface/qml/SideNavigationPane.qml
  - modules/gui/qt/maininterface/qml/MainDisplay.qml
  - modules/gui/qt/medialibrary/mlrecentaudiomodel.hpp
  - modules/gui/qt/medialibrary/mlrecentaudiomodel.cpp
  - modules/gui/qt/medialibrary/recentfoldersmodel.hpp
  - modules/gui/qt/medialibrary/recentfoldersmodel.cpp
  - modules/gui/qt/medialibrary/qml/HomePage.qml
  - modules/gui/qt/medialibrary/qml/HomeDisplay.qml
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/tests/test_navigation_model.cpp
  - modules/gui/qt/tests/test_recent_folders_model.cpp
  - modules/gui/qt/meson.build
estimate_h: 28.0
priority: P2
depends_on: [PKT-3027]
upstream_ruling: needed
target: fork
mr_title: "qt: sidebar pins and recent, listen again and folders on home"
status: ready
---

# PKT-3033 — Pinned items and a Recent entry in the sidebar; Home adds Listen again and Frequently visited folders

## 1. Scope & intent
At 13:31–13:56: "this sidebar is hella long and empty, but I have three more things to add. One, **pin** whatever you
want here — your favourite playlists, your most used folders. Two, a **recent files** tab. And three, a **homepage**…
with all the latest files and playlists and streams. Instead of this blank cone, you come back to VLC and see a full
media library ready to continue wherever you left off." Frames `s016` 13:36 (icon rail with a `+` at the bottom) and
13:48 (Home: sidebar Home, Videos, Music, Network, Playlists, Recent, `+ Add item`; content rows **Continue
Watching ›**, **Listen Again ›**, **Frequently Visited Folders ›**).

After this packet:
1. The sidebar ends with a **Recent** entry (all recently played media, local and network) and a list of **pinned**
   items, plus a `+ Pin…` row; any playlist, folder or network source can be pinned/unpinned from its context menu.
2. Home keeps Continue Watching / Favorites / New videos / New music and adds **Listen again** (audio history) and
   **Frequently visited folders** (parent folders of recent media, by play frequency).

One MR, four commits:
1. `qt: add a recent audio history model`
2. `qt: derive frequently visited folders from the playback history`
3. `qml: listen again and folders rows on the home page`
4. `qt: recent and pinned entries in the navigation model`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/maininterface/qml/MainDisplay.qml:107-113` + `:87-89` — `home` is the default page
  (`History.update(["home"])` when empty) and resolves to `HomeDisplay.qml` with an ML, `NoMedialibHome.qml`
  without: the "blank cone" of the video is the no-ML / VLC 3 case; master already opens on Home.
- `modules/gui/qt/medialibrary/qml/HomeDisplay.qml:31-47` (`pageModel`) — Home sub-pages continueWatching, favorites,
  newVideo, newMusic; `modules/gui/qt/medialibrary/qml/HomePage.qml:191-470` — four rows built on `MLRecentVideoModel` (`:233`),
  `MLMediaModel { favoriteOnly }` (`:313`), `MLVideoModel` (`:389`), `MLAudioModel` (`:469`), each with "See all".
- `include/vlc_media_library.h:1835-1849` (`vlc_ml_list_audio_history`, `vlc_ml_count_audio_history`) — the core
  exposes an audio history; `grep -rn audio_history modules/gui/qt` → none: no Qt model. The video counterpart to copy
  is `modules/gui/qt/medialibrary/mlrecentvideomodel.cpp:53-80` (`Loader::count/load` on `vlc_ml_*_video_history`).
- `modules/gui/qt/medialibrary/mlrecentmediamodel.cpp:83-106` — global history (`VLC_ML_HISTORY_TYPE_GLOBAL`), mixing
  `MLVideo`/`MLAudio`; used only by Media ▸ Open Recent Media (`modules/gui/qt/menus/menus.cpp:196-206`, limit 10, gated by
  `save-recentplay`, `src/libvlc-module.c:1926`): the "Recent" list exists only as a menu.
- `modules/gui/qt/medialibrary/mlhelper.cpp:66-71` (`getParentURLFromURL`) — parent folder of an MRL; the ML has no
  "folder visit" statistic (`include/vlc_media_library.h:506-508` lists entry points and folders, no play counts).
- `modules/gui/qt/maininterface/navigationmodel.cpp:266-295` — static entries only; `modules/gui/qt/maininterface/navigationmodel.hpp:26-33`
  `NavigationModelItem` carries name/icon/uri but **no view properties**, so a pinned playlist (which needs
  `{parentId}` in `History.push(path, props)`) cannot be represented today.
- `modules/gui/qt/medialibrary/qml/MLContextMenu.qml:77-84` + `:171-177` — Add/Remove favorites exist for media;
  `include/vlc_media_library.h:392` and `:2077` show folders and playlists also carry a favourite flag in the ML.
- `modules/gui/qt/tests/test_navigation_model.cpp:172-192` — pins today's static rows; extended for Recent and pins.

Settings / variables involved: `save-recentplay` (`src/libvlc-module.c:1926`) gates Recent; pins persist either as ML
favourites (§5.1) or under `MainCtx.setSettingValue("navigation/pins", …)` (same store as `modules/gui/qt/widgets/qml/PageExt.qml:72-85`).
Mock boundary: `RecentFoldersModel` aggregates a list of MRLs + play dates handed to it (no ML) — QtTest;
`MLRecentAudioModel` mirrors the video one and is covered by a MANUAL row plus the ML test pattern of PKT-3032.

### Overlaps & contradictions
- **PKT-3027** owns the section list; this packet appends Recent and pins after the sections (merge after 3027).
- **PKT-3029** adds a library pane in the player; pins are not shown there.
- **PKT-3034 / 3013**: Media ▸ Open Recent stays (menu) — two entry points to the same history; acceptable.
- **PKT-3016** (resume prompt) touches "continue where you left off" in the player, not Home.

## 2b. Upstream state
Master already opens on a Home page with **Continue Watching**, Favorites, New Videos and New Music
(`HomePage.qml`), so "a homepage instead of the blank cone" is shipped whenever the media library is enabled. Missing:
Listen again, frequently visited folders, a Recent page/entry, and pins in the sidebar. Narrowed to those.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `RecentFoldersModel` | history MRLs `a/S01/E1`, `a/S01/E2`, `b/x.mp3` | `setHistory` | rows `a/S01/` (2) then `b/` (1), names `S01`, `b` | `tests/test_recent_folders_model.cpp::testGroupsByParent` |
| 2 | same | equal counts | `setHistory` | ties ordered by most recent play | `test_recent_folders_model.cpp::testTieByRecency` |
| 3 | same | network MRLs (`http://`) | `setHistory` | ignored (folders are local only) | `test_recent_folders_model.cpp::testIgnoresNetwork` |
| 4 | same | `limit = 4`, 6 folders | `setHistory` | 4 rows, `maximumCount = 6` (drives "See all") | `test_recent_folders_model.cpp::testLimit` |
| 5 | `NavigationModel` with ML | default | built | static sections then `Recent` | `test_navigation_model.cpp::testInitialValues` (extended) |
| 6 | `NavigationModel::addPin` | 2 pins | `addPin(playlist "Fav", ["playlists"], {parentId})`; `removePin(0)` | rows inserted/removed with correct signals; `PROPERTIES` role returns `{parentId}` | `test_navigation_model.cpp::testPins` |
| 7 | pins persistence | pins set | model destroyed and rebuilt with the same settings stub | same pins | `test_navigation_model.cpp::testPinsRestored` |
| 8 | Home | 3 songs played, 2 folders | screenshot | Listen again and Frequently visited folders rows | MANUAL `pkt3033-home.png` |
| 9 | Sidebar | 1 pinned playlist | screenshot | Recent + pinned playlist + `+ Pin…` at the bottom | MANUAL `pkt3033-sidebar.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:148-170` (model + `QAbstractItemModelTester` + insert/remove
  spies) for rows 5–7; persistence through an injected `std::function` settings getter/setter so the test needs no
  `MainCtx`.
- `RecentFoldersModel` input is `QList<{QUrl mrl; QDateTime lastPlayed}>`; in the app it is filled from
  `MLRecentMediaModel` (`modules/gui/qt/medialibrary/mlrecentmediamodel.cpp:83-106`); `base_model_test` block (`modules/gui/qt/meson.build:1287-1302`) shows a
  Qt-only test target.
- Sample media: none for CI.

## 5. Design decisions & open questions
1. **RULED — what a pin is.** RULED 2026-10-14 (R0): pins are a UI-only list in settings (option b), with "Pin" in context menus. (a) ML favourites of playlists/folders (already in the ML schema, synced with the
   Favorites row) or (b) a UI-only list in settings that can also hold network sources and Browse paths. Proposal: (b)
   for flexibility, with "Pin" in context menus;  (a) would make "pin" a synonym of "favourite".
2. **RULED — sidebar length.** RULED 2026-10-14 (R0): at most 8 pins are visible, with a "More" overflow. VideoLAN keeps the navigation short; pins grow it unbounded. Cap visible pins (8)
   with a "More" overflow; 
3. NOTE — "Frequently visited folders" is derived from the playback history (play count per parent folder over the
   last N history entries, N = 500), not tracked by libmedialibrary; a true per-folder statistic would be a core
   change.
4. NOTE — the video's Home shows no Favorites/New rows; master's rows are kept (removing them is not asked for).
5. **Split (T3, 28 h)**: 3033a `MLRecentAudioModel` + Listen again (~6 h); 3033b `RecentFoldersModel` + folders row
   (rows 1–4, 8; ~8 h); 3033c Recent page + entry (row 5; ~4 h); 3033d pins (rows 6–7, 9; ~10 h, rulings 1–2 recorded).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3033-sidebar-pins-recent-home` on top of merged PKT-3027; §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm the section rows left by 3027 for row 5.
- [ ] **Step 3: Audio history model** — `medialibrary/mlrecentaudiomodel.{hpp,cpp}` copying `modules/gui/qt/medialibrary/mlrecentvideomodel.cpp:53-80` with `vlc_ml_*_audio_history`; register in `mainui.cpp` next to `MLRecentMediaModel` (`:490`); add sources to `meson.build`. Commit `qt: add a recent audio history model`.
- [ ] **Step 4: First row watched failing** — `tests/test_recent_folders_model.cpp` row 1 against an empty model → FAIL; implement `RecentFoldersModel`; rows 1–4 green. Commit `qt: derive frequently visited folders from the playback history`.
- [ ] **Step 5: Home rows** — `HomePage.qml` two rows after Continue Watching (`ViewHeader` + list, "See all" → new `HomeDisplay` sub-pages `listenAgain`, `folders`); screenshot row 8. Commit `qml: listen again and folders rows on the home page`.
- [ ] **Step 6: Navigation** — rows 5–7 red; `NavigationModel` `PROPERTIES` role, `addPin/removePin`, Recent entry; `SideNavigationPane.qml` passes properties to `History.push`; Pin/Unpin in `MLContextMenu.qml` and playlist/network menus; screenshot row 9. Commit `qt: recent and pinned entries in the navigation model`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
