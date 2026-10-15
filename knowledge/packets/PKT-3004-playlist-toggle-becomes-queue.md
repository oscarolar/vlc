---
id: PKT-3004
title: "The play-queue toggle looks like a queue, and the queue header carries repeat, shuffle, remove and add"
section: "A. Control bar"
video_ref: "02:23–02:55"
pipeline: MANUAL
kind: ui
track: T2
component: modules/gui/qt/playlist
branch: master-pkt-3004-playlist-toggle-becomes-queue
base_sha: ce4af2f
target_files:
  - modules/gui/qt/pixmaps/VLCIcons.json
  - modules/gui/qt/pixmaps/icons/ic_queue.svg
  - modules/gui/qt/pixmaps/VLCIcons.ttf
  - modules/gui/qt/style/vlcicons.hpp
  - modules/gui/qt/playlist/qml/PlaylistPane.qml
  - modules/gui/qt/player/qml/controlbarcontrols/PlaylistButton.qml
  - modules/gui/qt/player/qml/PlayerControlbarControls.qml
  - modules/gui/qt/player/qml/TopBar.qml
estimate_h: 8.0
priority: P2
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qml: give the play queue its own icon and header actions"
status: ready
---

# PKT-3004 — The play-queue toggle looks like a queue, and the queue header carries repeat, shuffle, remove and add

## 1. Scope & intent
At 02:23–02:55 the designer stumbles on VLC 3's "Toggle playlist": VLC says *playlist* for what is really a
*queue* (things that play next), while "add to playlist" / "save playlist" mean a saved collection. She asks for a
queue that plays things automatically and separate playlists, turns the button into a **queue icon** (the "queue"
title card at ~02:45 shows three lines with bullet dots) and puts it on the right. The panel at ~02:48 reads
"2 items - 48m" with five icons in its header — sort, repeat, shuffle, **minus** (remove) and **plus** (add) — and
rows with a drag handle, file name and duration. "Ideally you just drag files into the queue or use these icons to add."

**Master already made the vocabulary change** (§2b): the toggle, the pane, the menu and the save action all say
*Play Queue*, the pane header already shows a count and total duration, sort and search, and dropping files on the
toggle button already enqueues them. Playlists as collections exist in the media library. What is left is visual and
small: the toggle still draws the old *playlist* glyph, and repeat/shuffle/remove/add are only reachable from the
control bar or the right-click menu, not from the queue header.

One MR, three commits:
1. `qt: add a queue glyph to the icon font` — SVG + `VLCIcons.json` + regenerated `VLCIcons.ttf` / `vlcicons.hpp`.
2. `qml: use the queue glyph for the play queue toggles` — `PlaylistButton`, the controls registry, `TopBar`, the
   pane's `PlaylistToggleButton`; also unify the tooltip casing.
3. `qml: add repeat, shuffle, remove and add to the play queue header` — `PlaylistPane.qml`.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/controlbarcontrols/PlaylistButton.qml:27-41` — control-bar toggle: `text: VLCIcons.playlist`
  (`:30`), toggles `MainCtx.playqueuePanel.visible` (`:31-36`), `description: qsTr("Play Queue")` (`:39`).
- `modules/gui/qt/player/qml/PlayerControlbarControls.qml:42` — registry entry `PLAYLIST_BUTTON`, `label: VLCIcons.playlist`,
  `text: qsTr("Play Queue")` (what the toolbar editor shows).
- `modules/gui/qt/player/qml/TopBar.qml:482-508` — top-bar `PlaylistToggleButton`, `text: VLCIcons.playlist` (`:488`),
  `description: qsTr("Play Queue")` (`:489`).
- `modules/gui/qt/playlist/qml/PlaylistPane.qml:625-691` (`component PlaylistToggleButton`) — `text: VLCIcons.playlist`
  (`:631`), `description: qsTr("Play queue")` (`:632`, lower-case q, inconsistent with `:148`); its `DropArea`
  (`:642-690`) accepts drops and appends them at the end of the queue (`acceptDropFunc(plListView.model.count, drop)`,
  `:678`) and opens the panel after `duration_humanMoment` (`:682-689`). Also used by `modules/gui/qt/maininterface/qml/LocalTopbar.qml:186`.
- `modules/gui/qt/playlist/qml/PlaylistPane.qml:135-254` (header) — title `qsTr("Play Queue")` (`:148`), `SortControl`
  (`:154-198`), `SearchBox` (`:200-226`), caption `"%1 elements, %2"` count + total duration (`:229-253`). No repeat,
  shuffle, remove or add button.
- `modules/gui/qt/playlist/playlist_controller.hpp:101-103` — `random`, `repeatMode`, `empty` properties; `:123` `clear()`,
  `:135` `shuffle()`; `modules/gui/qt/dialogs/dialogs_provider.hpp:268` `PLAppendDialog(...)` — every action the header
  needs already exists.
- `modules/gui/qt/pixmaps/VLCIcons.json:55` — `{"key":"playlist", "path": "./icons/ic_playlist.svg"}`; the font is built by
  `pixmaps/makeIconFont.py` (needs `fontforge`) into `VLCIcons.ttf` + `style/vlcicons.hpp` (generated, `:1-3`).
- `modules/gui/qt/menus/menus.cpp:302-310` — View menu "Play &Queue" (Ctrl+L) and `:312-316` "Docked Play Queue";
  `include/vlc_intf_strings.h:65` `I_PL_SAVE` "Save Play Queue to &File...".
- `modules/gui/qt/player/control_list_model.hpp:50` — the id stays `PLAYLIST_BUTTON` (`:37` warns the enum is persisted).

Settings / variables involved: none.
Mock boundary: none — no automatable behaviour (every action already exists in C++ and is covered by its owner).

### Overlaps & contradictions
- **PKT-3002** must not touch the icon font in parallel (it does not; §2 overlaps there). If 3002b refreshes glyphs, it
  regenerates `vlcicons.hpp` after this lands.
- **PKT-3009** and **PKT-3013** edit `TopBar.qml` (`playlistGroup`, `:423-509`); this packet changes one property line
  (`:488`) there. Whoever lands second rebases.
- **PKT-3029** (drag to queue from a compact library) and **PKT-3032** (playlists as collections, "add to playlist")
  own the queue/playlist split at the library level; this packet does not touch the media library.

## 2b. Upstream state
- "Playlist" → "Play Queue" rename: **shipped** (all strings in §2).
- Drag files onto the toggle to enqueue: **shipped** (`modules/gui/qt/playlist/qml/PlaylistPane.qml:642-690`).
- Count + duration, sort, search in the queue header: **shipped**.
- Saved playlists distinct from the queue: **shipped** in the media library (Playlists view).
- Queue glyph, header repeat/shuffle/remove/add: **not shipped** — this packet.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. The header binds to existing
`PlaylistController` properties and the pane imports `VLC.Playlist`/`VLC.MainInterface`, which `qml_test` cannot load
(`modules/gui/qt/tests/qml_test.cpp:38-41` initialises only `util_assets`), so all rows are MANUAL with a screenshot.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | Icon font | commit 1 built | `VLCIcons.queue` is used in a scratch QML | the glyph renders (three lines with dots), `VLCIcons.playlist` still exists | MANUAL: toolbar editor screenshot showing both glyphs |
| 2 | Toggles | a video plays | control bar, top bar and library top bar are shown | all three toggles draw the queue glyph; tooltips read "Play Queue" | MANUAL: three crops |
| 3 | Header repeat | queue with 2 items | repeat button clicked three times | cycles none → all → one (icon `repeat_all` / `repeat_one`), control-bar `LoopButton` follows | MANUAL: screenshots per state |
| 4 | Header shuffle | same | shuffle clicked | `MainPlaylistController.random` toggles; control-bar `RandomButton` checked state follows | MANUAL |
| 5 | Header remove | one row selected | minus clicked / Delete key | the row is removed; button disabled with no selection | MANUAL |
| 6 | Header add | queue open | plus clicked | the open dialog appends to the queue (`PLAppendDialog`), nothing starts playing | MANUAL |
| 7 | Keyboard | header focused | Tab / arrows | sort → search → repeat → shuffle → remove → add are reachable, each has an `Accessible.name` | MANUAL: focus screenshots |

## 4. Fixtures & mocks
- Pattern to copy for the buttons: `player/qml/controlbarcontrols/LoopButton.qml` and `RandomButton.qml` (they already
  bind to `MainPlaylistController`); copy, do not reparent the control-bar components.
- Icon regeneration: `python3 makeIconFont.py VLCIcons.json` in `modules/gui/qt/pixmaps`, run on nzxt in a throwaway
  container with `fontforge` (`docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc/modules/gui/qt/pixmaps debian:sid
  sh -c 'apt-get update && apt-get install -y python3-fontforge && python3 makeIconFont.py VLCIcons.json'`).
- Sample media: two local files to fill the queue.

## 5. Design decisions & open questions
1. **RULED — the glyph.** RULED 2026-10-14 (R0): the Queue button uses the bullet-list glyph; the old glyph stays in the font. Bullet list ("queue", video ~02:45) vs. today's list-with-play-triangle. The old glyph is
   kept in the font (other views may use it — Step 2 greps `VLCIcons.playlist`).
2. **RULED — header density.** RULED 2026-10-14 (R0): repeat and shuffle are always shown; remove/add move to an overflow when the pane is narrower than `maxWidthPlaylist` (400 dp). Five icons plus search may not fit at `VLCStyle.defaultWidthPlaylist` (300 dp,
   `modules/gui/qt/style/VLCStyle.qml:148`). Proposed: repeat and shuffle always, remove/add in an overflow when the pane is narrower than
   `maxWidthPlaylist` (400 dp).
3. NOTE — the `PLAYLIST_BUTTON` enum id and the file name `PlaylistButton.qml` stay: renaming would break saved
   profiles (`modules/gui/qt/player/control_list_model.hpp:37`) for no user-visible gain.
4. NOTE — the casing fix `"Play queue"` → `"Play Queue"` (`modules/gui/qt/playlist/qml/PlaylistPane.qml:632`) changes one translatable string;
   `po/POTFILES.in` needs no edit (file already listed).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`: `git switch -c master-pkt-3004-playlist-toggle-becomes-queue ce4af2f`
      (worktree if busy; rebase on PKT-3003 once merged); §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; `grep -rn "VLCIcons.playlist" modules/gui/qt` and list the users in §5.1.
- [ ] **Step 3: Commit 1** — add `pixmaps/icons/ic_queue.svg` (48×48 viewBox, no overlaps — `makeIconFont.py` DOC), key
      `queue` in `VLCIcons.json`, regenerate (§4); build; row 1. Commit `qt: add a queue glyph to the icon font`.
- [ ] **Step 4: Commit 2** — `VLCIcons.queue` at `modules/gui/qt/player/qml/controlbarcontrols/PlaylistButton.qml:30`, `modules/gui/qt/player/qml/PlayerControlbarControls.qml:42`,
      `modules/gui/qt/player/qml/TopBar.qml:488`, `modules/gui/qt/playlist/qml/PlaylistPane.qml:631`; casing at `:632`; row 2. Commit
      `qml: use the queue glyph for the play queue toggles`.
- [ ] **Step 5: Commit 3** — in `PlaylistPane.qml` `titleRowLayout` (`:135-227`) add the four `IconToolButton`s with
      `Navigation.left/rightItem` chained after `searchBox`; rows 3–7. Commit
      `qml: add repeat, shuffle, remove and add to the play queue header`.
- [ ] **Step 6: Whole gate** — §6; paste the summary in §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`) with the screenshots and §5.1/§5.2 as recorded rulings; `status: review`.
