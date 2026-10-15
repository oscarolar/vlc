---
id: PKT-3031
title: "Network streams: an \"Add network stream\" entry, saved streams, and a pasted .m3u URL listed as a playlist"
section: "D. Library"
video_ref: "12:22–13:21"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/network
branch: master-pkt-3031-network-streams-url-playlist
base_sha: ce4af2f
target_files:
  - modules/gui/qt/network/qml/NetworkStreamsDisplay.qml
  - modules/gui/qt/network/qml/DiscoverUrlDisplay.qml
  - modules/gui/qt/network/folderbrowsetree.hpp
  - modules/gui/qt/network/folderbrowsetree.cpp
  - modules/gui/qt/medialibrary/mlplaylistlistmodel.cpp
  - modules/gui/qt/maininterface/qml/MainDisplay.qml
  - modules/gui/qt/tests/test_folder_browse_tree.cpp
  - modules/gui/qt/tests/tst_StreamUrl.qml
  - modules/gui/qt/meson.build
estimate_h: 16.0
priority: P2
depends_on: [PKT-3034, PKT-3027]
upstream_ruling: needed
target: fork
mr_title: "qml: network streams page with add, saved streams and URL playlists"
status: ready
---

# PKT-3031 — Network streams: an "Add network stream" entry, saved streams, and a pasted .m3u URL listed as a playlist

## 1. Scope & intent
At 12:22 the designer opens **Network streams** ("yes, VLC can stream"): by default a few saved sources, and an
**Add network stream** button (frame `s014` 12:28: `Network · 2 items · + Add Network Stream · Podcasts · Radio`).
The button opens the open-media dialog she then takes apart (PKT-3034). At 13:11–13:21 she pastes the URL of the
public iptv-org list (`https://iptv-org.github.io/iptv/index.m3u`) into the new single-purpose dialog and "now it's
an **automatic playlist** with all the videos listed" (frame `s015` 13:18: `Network ▸ Index · 110 items · TV Channel
1 … TV Channel 11`).

After this packet the Network streams section (created by PKT-3027) shows:
1. a first row **+ Add network stream** that opens the network single-source dialog of PKT-3034;
2. the saved/played streams (the media library's network history, today on Discover ▸ URL);
3. the service sources (Podcasts, radio directories — today Discover ▸ Services);
4. a stream whose URL is a playlist (`.m3u`, `.m3u8` master lists, `.pls`, `.xspf`) opens as a browsable list
   (address line `Network ▸ <name>`) with **Play all**, per-item play/enqueue, and **Save as playlist** (persists it
   in the media library's Playlists).

One MR, three commits:
1. `qt: build a browse tree from a remote playlist url`
2. `qml: network streams page with an add entry, saved streams and sources`
3. `qml: save a browsed URL playlist into the media library`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/network/qml/DiscoverDisplay.qml:32-42` (`pageModel`) — Discover = Services
  (`ServicesHomeDisplay.qml`) + URL (`DiscoverUrlDisplay.qml`).
- `modules/gui/qt/network/qml/DiscoverUrlDisplay.qml:66-81` (`searchField.onAccepted`) — a text field "Paste or write
  the URL here": with ML it calls `MLUrlModel.addAndPlay(text)`, else `MainPlaylistController.append([text], true)`;
  `:97-104` loads `UrlListDisplay.qml` (the saved list) only when the ML is available.
- `modules/gui/qt/medialibrary/mlurlmodel.cpp:60-85` (`MLUrlModel::addAndPlay`) — gets or creates an ML **stream**
  (`vlc_ml_new_stream`) then plays it; `:155-162` lists `VLC_ML_HISTORY_TYPE_NETWORK`: the "saved streams" exist,
  as a history. `:88-105` `deleteStream` (`vlc_ml_remove_stream`) is the remove action (`modules/gui/qt/medialibrary/qml/MLContextMenu.qml:103-105`).
- Playing an `.m3u` URL today expands it **in the queue** (playlist demuxer sub-items), not in the library; nothing
  lists it as a browsable collection.
- `src/media_source/media_tree.c:333-352` (`vlc_media_tree_Preparse`) — preparses an item with
  `VLC_PREPARSER_TYPE_PARSE | OPTION_INTERACT | OPTION_SUBITEMS`: the mechanism Browse uses to list a directory's
  children, applicable to a playlist URL's sub-items; `modules/gui/qt/network/networkmediamodel.cpp:451` calls it
  for the current tree node.
- `modules/gui/qt/network/standardpathmodel.cpp:166-188` — builds a `vlc_media_tree` around an input item; PKT-3029
  extracts it to `network/folderbrowsetree.cpp`, this packet adds a remote (`ITEM_NET`, non-directory) variant.
- `modules/gui/qt/medialibrary/mlplaylistlistmodel.cpp:87-116` (`create(name, initialItems)`) and `:118-175`
  (`append`) — `append` accepts input items/URLs, creating external media with `vlc_ml_new_external_media`
  (`:150-163`): "Save as playlist" needs no core change.

Settings / variables involved: none.
Mock boundary: the tree builder is unit-tested on a local `.m3u` file (`file://` URL) so CI needs no network;
the URL classification ("is this a playlist URL?") is a pure helper tested in a QML `TestCase`.

### Overlaps & contradictions
- **PKT-3034** provides the single-source network dialog that "+ Add network stream" opens.
- **PKT-3027** creates the Network streams section and its address line; this packet fills its page.
- **PKT-3029** introduces `network/folderbrowsetree.{hpp,cpp}`; whichever merges first creates the file, the other adds
  a function. Coordinate in Step 2.
- **PKT-3032** owns "Add to playlist" in menus; "Save as playlist" here calls the same `MLPlaylistListModel::create`.

## 2b. Upstream state
Master can already: play any network URL (Media ▸ Open Network Stream, Discover ▸ URL), keep a list of played
streams (ML network history, `UrlListDisplay.qml`), and browse service discovery sources (Podcasts, radio). It cannot:
show these under one "Network streams" page with an add entry, nor present a playlist URL as a browsable, savable
collection. The packet is narrowed to those.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `FolderBrowseTree::fromPlaylistUrl` | a temp `list.m3u` with 3 `http://` entries | build tree and preparse | 3 children with those URIs and `#EXTINF` names | `tests/test_folder_browse_tree.cpp::testPlaylistUrlChildren` |
| 2 | same | an unreachable `http://127.0.0.1:9/x.m3u` | preparse | tree ends with 0 children and a failed state, no crash | `test_folder_browse_tree.cpp::testPlaylistUrlFailure` |
| 3 | `Helpers.isPlaylistUrl` | `…/index.m3u`, `…?x=1.pls`, `…/a.mp4`, `rtsp://…` | classify | true, true, false, false | `tests/tst_StreamUrl.qml::test_classify` |
| 4 | `MLPlaylistListModel::create` with URLs | ML stub not needed: guard only | `create("Index", [])` | no assertion (empty list is skipped, see PKT-3032 row 1) | covered by PKT-3032 `test_ml_playlist_create.cpp` |
| 5 | Network streams page | ML on | screenshot | "+ Add network stream", saved streams, Podcasts/Radio rows | MANUAL `pkt3031-page.png` |
| 6 | Paste iptv-org index | network on | add the URL | `Network ▸ Index`, ~N items, Play all works | MANUAL `pkt3031-index.png` |
| 7 | Save as playlist | row 6 state | Save as playlist | "Index" appears under Playlists with the same count | MANUAL `pkt3031-saved.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:148-170` (`VLCTestingEnv` gives a libvlc instance with modules;
  the preparser needs the `m3u` demux and `file` access, which `vlc_stub_modules.cpp` does not stub — Step 2 checks
  whether the test env loads real modules; if not, rows 1–2 become a `test/src/media_source` test, as
  `src/media_source/media_tree.c` already has `TEST_MEDIA_SOURCE` hooks at `:336-341`).
- QML: `tests/tst_FSM.qml` pattern, `qml_tests` registration (`modules/gui/qt/meson.build:954-956`).
- Sample media: a 3-line `.m3u` written by the test into `QTemporaryDir`.

## 5. Design decisions & open questions
1. **RULED — merge Browse/Discover into Network streams** RULED 2026-10-14 (R6): Browse and Discover are merged under "Network streams"; this packet assumes it and the "keep Discover" alternative is void. is PKT-3027's ruling; this packet assumes it. If the
   ruling is "keep Discover", the page lives at Discover ▸ URL instead and only rows 1–3, 6–7 remain.
2. **RULED — "automatic playlist" persistence.** RULED 2026-10-14 (R0): browsing is ephemeral (re-fetched each time) and "Save as playlist" is an explicit snapshot into the media library. The video implies the pasted list *becomes* a playlist. Proposal:
   browsing is ephemeral (re-fetched each time, so the 110 live channels stay current) and **Save as playlist** is an
   explicit action that snapshots the entries into the ML. An auto-refreshing ML playlist would need a core
   "remote playlist" entity in libmedialibrary — out of scope.
3. NOTE — "by default a bunch of streams saved here" (12:24): master ships no default stream list; Podcasts/Radio
   (service discovery) fill that role. No hard-coded URLs will be added (VideoLAN does not ship third-party stream
   lists).
4. NOTE — preparsing network items may be gated by the preparser's network policy; Step 2 verifies with
   `vlc_preparser` defaults that an `http` `.m3u` yields sub-items through `vlc_media_tree_Preparse`; if not, the
   tree is fed by playing the URL into a hidden queue — record which in §5.

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
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3031-network-streams-url-playlist` on top of merged PKT-3027 and PKT-3034; §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2; decide where rows 1–2 live (Qt test vs `test/src/media_source`) per §4; check §5.4.
- [ ] **Step 3: First row watched failing** — row 1 with a builder returning an empty tree → FAIL.
- [ ] **Step 4: Remote tree** — `FolderBrowseTree::fromPlaylistUrl` (input item `ITEM_NET`, not a directory) feeding `NetworkMediaModel.tree`; rows 1–2 green. Commit `qt: build a browse tree from a remote playlist url`.
- [ ] **Step 5: Page** — `tst_StreamUrl.qml` row 3 red, `Helpers.isPlaylistUrl`; `network/qml/NetworkStreamsDisplay.qml` (add row → `DialogsProvider.openNetDialog()`, `UrlListDisplay`, services list); `MainDisplay.qml` page entry for the section; screenshots rows 5–6. Commit `qml: network streams page with an add entry, saved streams and sources`.
- [ ] **Step 6: Save** — "Save as playlist" header action → `MLPlaylistListModel.create(name, inputItems)`; row 7. Commit `qml: save a browsed URL playlist into the media library`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
