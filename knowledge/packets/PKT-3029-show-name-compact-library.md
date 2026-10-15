---
id: PKT-3029
title: "The player's top bar names the show's folder; clicking it opens a compact library beside the video, drag to the queue"
section: "D. Library"
video_ref: "11:00–11:58"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3029-show-name-compact-library
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/TopBar.qml
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/player/player_controller.hpp
  - modules/gui/qt/player/player_controller.cpp
  - modules/gui/qt/network/folderbrowsetree.hpp
  - modules/gui/qt/network/folderbrowsetree.cpp
  - modules/gui/qt/player/qml/CompactLibraryPane.qml
  - modules/gui/qt/tests/test_folder_browse_tree.cpp
  - modules/gui/qt/meson.build
estimate_h: 26.0
priority: P2
depends_on: [PKT-3004, PKT-3027]
upstream_ruling: needed
target: fork
mr_title: "qml: compact library pane beside the video, opened from the folder name"
status: ready
---

# PKT-3029 — The player's top bar names the show's folder; clicking it opens a compact library beside the video, drag to the queue

## 1. Scope & intent
At 11:00–11:58 the designer rejects a home icon and a library icon as entry points ("too vague") and settles on the
most familiar thing: **the name of the folder of what you are watching** ("the show"), shown in the player's top
bar (frame `s013` 11:30–11:42: `cone · … · [▯] · 📁 S01`). Next to it a sidebar icon opens the library **in compact
mode on the left of the video**, while the queue stays on the right; you drag episodes from the library into the
queue "so you never have to leave the video" (frame `s014` 11:42–11:58: left pane `← → The Wand ▸ S01`, list of
`The_Wand_S01E05.mp4…`, right pane the queue). A full-screen library with a mini player stays available (master's
existing library view).

After this packet, while a local file plays:
1. The player top bar shows a folder chip with the parent folder's name (`S01`) after the menu button.
2. Clicking the chip (or the sidebar-toggle icon beside it) slides a compact pane in from the left listing that
   folder (address line, back/forward, list mode only).
3. Items of the pane drag into the queue pane (existing drop support) and double-click plays.

One MR, three commits:
1. `qt: expose the current media's parent folder to QML`
2. `qt: build a browse tree from a local folder url`
3. `qml: compact library pane beside the video`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/player/qml/TopBar.qml:266-300` (`logoGroup`: `backBtn` `:276-293` → `root.backRequested()`, then
  the cone) and `:337-375` (`centerTitleText`, bound to `root.title`) — the top bar knows the media title only.
- `modules/gui/qt/player/qml/Player.qml:932-1002` — `TopBar { title: Player.title … }`; `:997-1002`
  `onBackRequested` stops embedded playback when PiP is impossible and sets `MainCtx.playerView = false`: the only
  way from the video to the library today is to leave the video.
- `modules/gui/qt/player/player_controller.hpp:141` — `Q_PROPERTY(QUrl url READ getUrl NOTIFY inputChanged)`; `:156`
  `title`. No folder property.
- `modules/gui/qt/medialibrary/mlhelper.cpp:66-71` (`getParentURLFromURL`) — `QUrl::adjusted(RemoveFilename)`; the
  helper to reuse for the folder URL (its name is the last path segment).
- `modules/gui/qt/network/standardpathmodel.cpp:166-188` (`addItem`) — the recipe to make a browsable tree from a
  local directory: `input_item_NewDirectory(url, name, ITEM_LOCAL)` + `vlc_media_tree_New()` +
  `vlc_media_tree_Add`; `:239` wraps it as `NetworkTreeItem` for `NetworkMediaModel.tree`
  (`modules/gui/qt/network/networkmediamodel.hpp:129`).
- `modules/gui/qt/network/qml/BrowseTreeDisplay.qml:35-49` — the browse view with `BrowseTreeHeader`
  (address line) reused by the pane in list mode.
- `modules/gui/qt/playlist/qml/PlaylistPane.qml:401-430` (`isDropAcceptableFunc` / `acceptDropFunc`) — the queue
  already accepts `Widgets.DragItem` drops from library views and inserts at the drop index; nothing to add for
  "drag to the queue".
- `modules/gui/qt/player/qml/Player.qml:146-173` (`playlistVisibility`, `PlayerPlaylistVisibilityFSM`) — governs the
  right-hand queue in the player; a left pane needs its own visibility state that does not fight this FSM.
- `include/vlc_media_library.h:1362-1364` (`vlc_ml_get_folder`) and `:2035` (`VLC_ML_LIST_FOLDER_MEDIA`) — the ML can
  list a folder's media **by folder id**, but there is no get-folder-by-MRL wrapper, so an ML-backed pane would need
  a lookup the core does not offer (§5.1).

Settings / variables involved: none new (pane width could reuse the `MainCtx.navigationPanel` pattern).
Mock boundary: the tree builder is tested in QtTest with a temporary directory and no player; the QML pane is
visual (MANUAL), drag/drop already covered upstream.

### Overlaps & contradictions
- **PKT-3008/3009/3013/3014** edit `TopBar.qml` (bars in fullscreen, PiP button, single menu button). This packet adds
  one chip after the menu button; whoever lands second rebases. 3013 must keep a slot left of the title.
- **PKT-3004** turns the queue toggle into "Queue"; the compact library must sit opposite to it — dependency kept.
- **PKT-3018** (settings sidebar) also slides in over the video, from the right; the compact library is on the left.
- **PKT-3027** provides the address line component reused here.

## 2b. Upstream state
Master has: the queue beside the video (`PlayerPlaylistVisibilityFSM`), drag from library to queue, a mini player
when the library is open over a playing video (`modules/gui/qt/maininterface/qml/MainDisplay.qml:39`), and the title in the top bar. It has **no**
folder name in the player and **no** library pane inside the player. Both are new.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PlayerController::parentFolderName/Url` (new) | url `file:///tmp/The%20Wand/S01/E05.mp4` | property read | name `S01`, url `file:///tmp/The%20Wand/S01/` | `tests/test_folder_browse_tree.cpp::testParentOfLocalFile` |
| 2 | same | url `https://example.org/live.m3u8` | property read | empty name (chip hidden for non-`file` schemes) | `test_folder_browse_tree.cpp::testRemoteHasNoFolder` |
| 3 | `FolderBrowseTree::fromUrl` (new, extracted from `modules/gui/qt/network/standardpathmodel.cpp:166-188`) | a temp dir with 3 files | build tree | `NetworkTreeItem` valid, root input item is `ITEM_TYPE_DIRECTORY` with that URI | `test_folder_browse_tree.cpp::testTreeFromLocalDir` |
| 4 | `StandardPathModel` regression | after extraction | model built | still lists the standard paths (same item count as before) | `test_folder_browse_tree.cpp::testStandardPathsUnchanged` |
| 5 | TopBar chip | local episode playing | screenshot | `📁 S01` after the menu button, hidden for a stream | MANUAL `pkt3029-chip.png` |
| 6 | Compact pane | chip clicked | screenshot | pane left, queue right, video visible between | MANUAL `pkt3029-pane.png` |
| 7 | Drag | pane open | drag E06 onto queue row 2 | E06 inserted at 2 | MANUAL `pkt3029-drag.gif` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:148-170` (`VLCTestingEnv` from `tests/vlc_stub_modules.hpp`) for a
  libvlc instance; `QTemporaryDir` for the folder.
- Rows 1–2 test a static helper (`parentFolderOf(QUrl)`) rather than a live `PlayerController`, which needs a player.
- Sample media: three empty files created in the temp dir (`.mp4` names); not parsed.

## 5. Design decisions & open questions
1. **RULED — filesystem or media library.** RULED 2026-10-14 (R0): the pane browses the filesystem folder, not the media-library folder. The pane browses the **filesystem folder** (works for any local
   file, indexed or not) instead of the ML folder (needs a by-MRL lookup absent from `vlc_media_library.h`, and
   only works for indexed folders). The ML variant would add a core packet.
2. **RULED — new top-bar element.** RULED 2026-10-14 (R0): the new top-bar chip is accepted, behind the option `qt-player-folder-chip` (default on). VideoLAN's player top bar is title-centred; a chip and a second pane change
   the player layout. Ships behind an option `qt-player-folder-chip` (default on) if asked.
3. NOTE — "show name" is the parent folder, not the ML `show` entity (`VLC_ML_GET_SHOW` exists but is filled only by
   libmedialibrary's show parsing). For `The Wand/S01/E05.mp4` the chip says `S01`, as in the video.
4. NOTE — the full-screen library with mini player (11:46) already exists; not touched.
5. **Split (T3, 26 h)**: 3029a folder property + chip (rows 1–2, 5; ~6 h); 3029b tree builder extraction (rows 3–4;
   ~6 h); 3029c compact pane + visibility state (rows 6–7; ~14 h, after 3004).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3029-show-name-compact-library` on top of merged PKT-3004 and PKT-3027; §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges, especially `TopBar.qml` after 3008/3009/3013 landed.
- [ ] **Step 3: First row watched failing** — new `tests/test_folder_browse_tree.cpp` + `vlc_tests` block (copy `modules/gui/qt/meson.build:1342-1360`), row 1 against a stub returning `""` → FAIL.
- [ ] **Step 4: Folder property** — static `parentFolderOf(QUrl)` next to `getParentURLFromURL` usage; `PlayerController` `Q_PROPERTY parentFolderName/parentFolderUrl NOTIFY inputChanged`; rows 1–2 green. Commit `qt: expose the current media's parent folder to QML`.
- [ ] **Step 5: Tree builder** — extract `modules/gui/qt/network/standardpathmodel.cpp:166-188` into `network/folderbrowsetree.{hpp,cpp}`; rows 3–4 green. Commit `qt: build a browse tree from a local folder url`.
- [ ] **Step 6: Pane** — `player/qml/CompactLibraryPane.qml` (a `BrowseTreeDisplay` in list mode with `NetworkMediaModel { tree: … }`), chip in `TopBar.qml` after `menuSelector`, pane state in `Player.qml` left of the video; screenshots rows 5–7. Commit `qml: compact library pane beside the video`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
