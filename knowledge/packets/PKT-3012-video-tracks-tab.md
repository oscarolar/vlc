---
id: PKT-3012
title: "Video tracks get a tab in the track popup, without a bar button"
section: "B. Tracks & menus"
video_ref: "04:43–04:55"
pipeline: CI
kind: ui
track: T1
component: modules/gui/qt/player
branch: master-pkt-3012-video-tracks-tab
base_sha: ce4af2f
target_files:
  - modules/gui/qt/util/qml/TrackTabsModel.qml
  - modules/gui/qt/tests/tst_TrackTabs.qml
  - modules/gui/qt/player/qml/TracksTabPage.qml
estimate_h: 4.0
priority: P3
depends_on: [PKT-3010]
upstream_ruling: not-needed
target: upstream
mr_title: "qml: add a video tab to the track popup"
status: ready
---

# PKT-3012 — Video tracks get a tab in the track popup, without a bar button

## 1. Scope & intent
**Before.** Video tracks are the third column of `TracksListPage` (opened by `LangButton`) and the
"Video Track" submenu of the right-click/Video menu. PKT-3010/3011 introduce a tabbed popup with Audio and
Subtitles tabs only.

**After (04:43–04:55, sheet `s006` third frame).** The tabbed popup shows "Video | Audio | Subtitles". The video
tab lists the video tracks (checkable, multi-select as today) and an "Add" (+) that loads a video file. No button
on the bar: the designer's survey (sheet `s006`, fourth frame: 55.2 % "Never" change video tracks) justifies
keeping it one tab away. The tab is only shown when the media has **more than one** video track, so the
common case keeps two tabs.

One MR, one commit: `qml: add a video tab to the track popup`.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- `modules/gui/qt/player/qml/TracksListPage.qml:98-106` — the existing "Video Tracks" `TrackColumn` on
  `Player.videoTracks`, menu icon `VLCIcons.add`, action `DialogsProvider.loadVideoFile()` (`:104`). This is the
  behaviour the tab copies.
- `modules/gui/qt/menus/menus.cpp:448-454` (`VLCMenuBar::VideoMenu`) — "Video &Track" `CheckableListMenu` with
  `ExclusionPolicy::None` (`:452`): video tracks are multi-select in the classic menu; the tab keeps that.
- `modules/gui/qt/player/player_controller.hpp:162` — `videoTracks` (`TrackListModel*`);
  `modules/gui/qt/player/input_models.hpp:47` — `count`, the property the visibility rule reads.
- `TrackTabsModel.qml` / `TracksTabPage.qml` / `tst_TrackTabs.qml` do not exist at `ce4af2f`; they are created by
  PKT-3010 (Step 2 re-reads them on the branch).

Settings / variables involved: none (`video-track`, `src/libvlc-module.c`, untouched).
Mock boundary: `ListModel`s with `display`/`checked` roles in the QML `TestCase`.

### Overlaps & contradictions
- **PKT-3010 / PKT-3011** own the same three files; this packet adds rows and one tab only. Merge order
  3010 → 3011 → 3012 (3011 is not a hard dependency: if 3012 lands first it adds Video next to Subtitles).
- **PKT-3024** (Video tab of the sidebar: filters, flip/rotate, aspect) is a different surface; no file shared.
- None detected upstream.

## 2b. Upstream state
Master already exposes video track selection in the Qt UI (third column of `TracksListPage`, Video menu). Nothing
is missing functionally; this packet only keeps it reachable once the tabbed popup replaces the three-column page
as the default entry point.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `TrackTabsModel` | video model 1 row, audio 1, subtitle 1 | built | no `"video"` tab | `tst_TrackTabs.qml::test_single_video_hidden` |
| 2 | `TrackTabsModel` | video model 2 rows | built | tabs `["video","audio","subtitle"]` in that order | `tst_TrackTabs.qml::test_video_tab_first` |
| 3 | `TrackTabsModel` | video tab visible and current; video model drops to 1 row | `countChanged` | current falls back to `requested` (or `"audio"`) | `tst_TrackTabs.qml::test_video_tab_disappears` |
| 4 | Video tab | mkv with 2 video streams | open the popup, select both | both checked (multi-select as `modules/gui/qt/menus/menus.cpp:452`) | MANUAL — screenshot `pkt3012-video-tab.png` |

## 4. Fixtures & mocks
- Pattern to copy: the functions PKT-3010 adds to `modules/gui/qt/tests/tst_TrackTabs.qml`.
- Stubs: `ListModel` for the three track models.
- Sample media: two-video-stream mkv generated on nzxt with
  `ffmpeg -f lavfi -i testsrc -f lavfi -i smptebars -map 0 -map 1 -t 10 twovideo.mkv`.

## 5. Design decisions & open questions
1. NOTE — "only when more than one video track" is this packet's reading of "rarely used"; the video shows the tab
   always present. If the owner prefers always-visible, rows 1 and 3 flip; no VideoLAN ruling needed since master
   already exposes the tracks.
2. NOTE — "Add" (+) calls `DialogsProvider.loadVideoFile()` exactly as `modules/gui/qt/player/qml/TracksListPage.qml:104`.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`.
- Strings added are marked for translation (`qsTr`).
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on the new tab.

### Evidence
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3012-video-tracks-tab` from the merge of
      PKT-3010 (3011 if merged); §6 command → green.
- [ ] **Step 2: Dry-run** — re-read §2 and the three files PKT-3010 created; rewrite §3 rows if the tab API differs.
- [ ] **Step 3: Row 1 watched failing** — add `test_single_video_hidden` expecting a `"video"` tab → FAIL, then the
      real expectation.
- [ ] **Step 4: Rows 2–3** — `TrackTabsModel`: video tab when `videoModel.count > 1`, ordered first; current-tab
      fallback on `countChanged`.
- [ ] **Step 5: Tab content** — `TracksTabPage.qml` Video tab: `Player.videoTracks` list + "+"
      (`DialogsProvider.loadVideoFile()`); row 4 by hand.
- [ ] **Step 6: Whole gate** — §6 command; summary into §6 Evidence.
- [ ] **Step 7: Commit and MR** — `qml: add a video tab to the track popup`; Draft MR; `status: review`.
  Destination: MR to videolan/vlc master (Draft).
