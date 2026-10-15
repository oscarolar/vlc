---
id: PKT-3020
title: "A subtitle timeline on the seek bar to sync subtitles by eye"
section: "C. Subtitles"
video_ref: "06:54–07:03"
pipeline: CI
kind: core
track: T3
component: modules/gui/qt/player
branch: master-pkt-3020-subtitle-timeline
base_sha: ce4af2f
target_files:
  - modules/demux/subtitle.c
  - modules/gui/qt/player/subtitle_cue_model.cpp
  - modules/gui/qt/player/subtitle_cue_model.hpp
  - modules/gui/qt/player/qml/SubtitleTimeline.qml
  - modules/gui/qt/player/qml/SliderBar.qml
  - modules/gui/qt/util/qml/TimelineMapper.qml
  - modules/gui/qt/tests/test_subtitle_cue_model.cpp
  - modules/gui/qt/tests/tst_TimelineMapper.qml
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 28.0
priority: P2
depends_on: [PKT-3018, PKT-3006, PKT-3019]
upstream_ruling: needed
target: fork
mr_title: "qml: show subtitle cues on the seek bar and shift them by dragging"
status: ready
---

# PKT-3020 — A subtitle timeline on the seek bar to sync subtitles by eye

## 1. Scope & intent
06:53–07:03: instead of guessing a delay value, the designer wants a **subtitle timeline** "that showed the times
that texts appear": wait for the line to be spoken, pause, and move the subtitles back until the block sits under
the playhead. Frame `s008` (≈06:57–07:00, bottom row) shows it: the seek bar becomes a strip of brown blocks (one
per cue, width = cue duration) over the orange progress, the white playhead cutting through it, and two small
nudge buttons **‹ ›** at each end of the strip; zoomed in (middle frame) a cue block and the playhead are a few
pixels apart. Frame `s011` (≈09:12) puts a toggle *Show Subtitle Timeline on Seekbar* in the Playback tab
(PKT-3025 owns the toggle).

Before: the only visual sync aid is the "Voice Heard / Text Seen" estimator; the UI never knows when cues happen.
After: with the toggle on and a text subtitle track selected, the seek bar draws the cues shifted by the current
subtitle delay; dragging the strip (or the nudge buttons, 50 ms per click) changes `Player.subtitleDelay` live.

This is **T3** (§5.5 split). The blocking question is where the cue times come from (§5.1).

Commits (per split): `demux: subtitle: publish the cue list`, `qt: add SubtitleCueModel`,
`qml: add TimelineMapper`, `qml: show subtitle cues on the seek bar`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `include/vlc_player.h` — no API returns subtitle cue times; the player only exposes tracks, delays
  (`vlc_player_SetSubtitleDelay`), subs FPS and text scale (`:1954-1963`). The UI cannot read cues today.
- `modules/demux/subtitle.c:110-184` — the text-subtitle demuxer (SRT, SSA, MicroDVD, SubViewer, VPlayer, SAMI,
  MPL2, …, parsers declared `:185-198`) loads the **whole file** into `subtitles.p_array` of `subtitle_t
  {i_start, i_stop, psz_text}` (`:120-126`, `:168-174`). The cue list exists in memory for every external text
  subtitle; `:849` handles `DEMUX_GET_META`.
- `modules/demux/mpeg/es.c:583-627` (`SyltEmitMeta`) — the precedent: the ES demuxer packs synchronized lyrics
  as `"ms\x1Ftext\x1E"` records into the item meta extra `sylt-data` / `sylt-data:<lang>` via
  `es_out_SetMeta`.
- `modules/gui/qt/player/player_controller.cpp:195-260` — the consumer of that precedent: `sylt-data` /
  `uslt-data` read from `vlc_meta_GetExtra`, kept raw with a hash, parsed lazily into `QList<TimedText>` for
  `syltLyrics` (`modules/gui/qt/player/player_controller.hpp:173-176`), shown by `player/qml/LyricsFlickable.qml`.
- `modules/gui/qt/player/qml/SliderBar.qml:317-345` (`background`, `Repeater seekpointsRptr` over
  `Player.chapters`) — the seek bar already draws per-chapter segments from a model with `startPosition` /
  `endPosition`; the cue strip is the same pattern on another model.
- `modules/gui/qt/player/player_controller.hpp:167,170` — `subtitleDelay` / `subtitleDelayMS`, the value the
  drag writes.

Settings / variables involved: `sub-delay` (`src/libvlc-module.c:1814`) only as startup default; a new Qt
setting for the toggle lives in PKT-3025.
Mock boundary: the cue model is fed cue lists directly in QtTest (no demux); `TimelineMapper` is pure QML; the
core row uses a sample `.srt` and the demux test harness.

### Overlaps & contradictions
- **PKT-3006** rebuilds the seek bar (thick bar at the bottom edge) and **PKT-3030** adds a waveform to it — all
  three draw inside `SliderBar.qml` `background`. Order: 3006 → 3020 → 3030; the cue strip is a separate
  `SubtitleTimeline.qml` item so the conflict is one line.
- **PKT-3007** (thumbnail preview while seeking) uses hover on the same bar; drag on the cue strip must not
  seek (§5.3).
- **PKT-3019** owns the delay slider; both write `Player.subtitleDelay`, no shared file.
- **PKT-3025** owns the toggle "Show Subtitle Timeline on Seekbar".
- **PKT-3026** draws bookmarks on the seek bar (`medialibrary/qml/Bookmarks.qml`, loaded by
  `modules/gui/qt/player/qml/ControlBar.qml:240-256`) — z-order must keep bookmarks above cues.

## 2b. Upstream state
Nothing: no cue access in the player API, no cue drawing in any VLC interface. The lyrics path (SYLT/USLT via
meta extra, PR in master) is the only existing "timed text to UI" channel and is the model to follow.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | subtitle demux cue export (option A of §5.1) | `test/samples/subs/three-cues.srt` (3 cues) opened as a slave of a sample video | input started | the item meta extra (key per §5.1) holds 3 records with the right `start`/`stop` ms | new test in `test/src/input/` following `demux-run.c`, or `test/libvlc/` — Step 2 picks the harness |
| 2 | `SubtitleCueModel` | cue list `[(1000,2000),(5000,6500)]`, length 10 s | `rowCount()` / `data(row, StartPositionRole)` | 2 rows, positions `0.1` / `0.5` | `test_subtitle_cue_model.cpp::testPositionsFromTimes` |
| 3 | `SubtitleCueModel` | same, `delay = -500 ms` | positions read | shifted by −0.05 (`0.05`, `0.45`), clamped at 0 | `test_subtitle_cue_model.cpp::testDelayShiftsCues` |
| 4 | `SubtitleCueModel` | 5 000 cues, bar 800 px | `setVisibleWidth(800)` | adjacent cues closer than 1 px are merged (row count ≤ 800) so the strip never draws more items than pixels | `test_subtitle_cue_model.cpp::testMergesSubPixelCues` |
| 5 | `SubtitleCueModel` | selected subtitle track changes / is a bitmap track | `onTrackChanged` | model resets; bitmap track → 0 rows and `available == false` | `test_subtitle_cue_model.cpp::testResetOnTrackChange` |
| 6 | `TimelineMapper` | width 800, length 600 000 ms | `msForDx(40)` | `30000` ms; `dxForMs` inverse | `tst_TimelineMapper.qml::test_px_ms_roundtrip` |
| 7 | `TimelineMapper` | drag from x=400 to x=380, step 50 ms | `delayForDrag(400, 380, startDelay 0)` | `-15000` snapped to 50 ms; dragging never changes position | `tst_TimelineMapper.qml::test_drag_to_delay` |
| 8 | `SubtitleTimeline.qml` | external SRT playing, toggle on | seek bar | cue blocks drawn, nudge ‹ › change delay by 50 ms, OSD shows "Subtitle delay …" | MANUAL — `pkt3020-cues.png` |

## 4. Fixtures & mocks
- Core: copy the input/demux test style of `test/src/input/demux-run.c` (Step 2 confirms it can open a slave)
  or extend `test/libvlc/slaves.c:128-205`, which already opens a video with subtitle slaves; add
  `test/samples/subs/three-cues.srt`.
- Qt: copy `modules/gui/qt/tests/test_navigation_model.cpp` + `modules/gui/qt/meson.build:1342-1360`; the model takes a
  `QList<TimedText>`-like input, so no player is needed for rows 2–5 (row 5 may need `VLCTestingEnv`).
- QML: `tst_FSM.qml` pattern; `TimelineMapper.qml` in `VLC.Util`.

## 5. Design decisions & open questions
1. **RULED — cue source.** RULED 2026-10-14 (R0): option (A): the core publishes the cues through a player-level API (`vlc_player_GetSubtitleCues`); the Qt-only parser (B) is the fallback if that is refused. (A) **core**: `modules/demux/subtitle.c` publishes its `p_array` like
   `SyltEmitMeta` does (`modules/demux/mpeg/es.c:583-627`) — works for every text format the demux parses, but meta extras of a
   **slave** source may not reach the main item (Step 2 must check `es_out_SetMeta` for non-master sources) and
   embedded tracks (MKV/MP4 text) are never covered because their cues are not known ahead of playback; may need
   a proper player API (`vlc_player_GetSubtitleCues`) instead of meta, which is a VideoLAN API decision.
   (B) **Qt-only**: parse the external file in the UI (SRT/VTT/ASS times only) — no core change, duplicates a
   parser, external files only. Recommendation: (A) with a player-level API; (B) as fallback if refused.
2. NOTE — embedded subtitle tracks show no strip (`available == false`); the toggle explains why.
3. NOTE — drag on the strip changes delay, click on the bar outside the strip still seeks; the strip sits in the
   upper half of the thick bar of PKT-3006 so both gestures have a target.
4. NOTE — cues are drawn shifted by the **current** delay so "move until under the playhead" is literal.
5. **Split (T3):** **3020a** cue source (core player API per §5.1, row 1, 10 h);
   **3020b** `SubtitleCueModel` + `TimelineMapper` (rows 2–7, 10 h); **3020c** QML strip + nudge buttons
   (row 8, 8 h, after PKT-3006).

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs && meson test -C build-meson --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt` or `modules/demux`.
- Strings added are marked for translation (`qsTr` / `qtr`) and `po/POTFILES.in` is updated when a file is new.
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on every new control.

### Evidence
<Filled by the executor: build tail, `meson test` summary, screenshots, MR URL.>

## 7. Agent brief
- [ ] **Step 0: Split** — write 3020a/b/c from §5.5; 3020a follows the §5.1 ruling (core cue API).
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3020-subtitle-timeline` (suffix per split) from the merged PKT-3006/3018/3019; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; trace `es_out_SetMeta` from a slave source in `src/input/es_out.c` and write the answer in §5.1; pick the core test harness for row 1.
- [ ] **Step 3 (3020b): Row 2 watched failing** — `tests/test_subtitle_cue_model.cpp` against an empty `player/subtitle_cue_model.{hpp,cpp}` → FAIL; then rows 3–5; commit `qt: add SubtitleCueModel`.
- [ ] **Step 4 (3020b): Rows 6–7** — `util/qml/TimelineMapper.qml` + `tests/tst_TimelineMapper.qml` in `qml_tests`; commit `qml: add TimelineMapper`.
- [ ] **Step 5 (3020a): Row 1** — per ruling; commit `demux: subtitle: publish the cue list` (or `qt: parse external subtitle cues`).
- [ ] **Step 6 (3020c): Row 8** — `player/qml/SubtitleTimeline.qml` inside `SliderBar.qml` background, bound to the toggle of PKT-3025; register in `meson.build`, `po/POTFILES.in`; commit `qml: show subtitle cues on the seek bar`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MRs** — MRs per split to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
