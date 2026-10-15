---
id: PKT-3030
title: "Music plays in a cover-first view with an optional waveform seek bar"
section: "D. Library"
video_ref: "11:58–12:22"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3030-music-view-waveform-seek
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/player/qml/SliderBar.qml
  - modules/gui/qt/player/qml/WaveformBackground.qml
  - modules/gui/qt/player/loudness_envelope.hpp
  - modules/gui/qt/player/loudness_envelope.cpp
  - modules/gui/qt/player/player_controller.hpp
  - modules/gui/qt/player/player_controller.cpp
  - modules/gui/qt/tests/test_loudness_envelope.cpp
  - modules/gui/qt/meson.build
estimate_h: 24.0
priority: P3
depends_on: [PKT-3006]
upstream_ruling: needed
target: fork
mr_title: "qml: cover-first audio view and a loudness waveform seek bar"
status: ready
---

# PKT-3030 — Music plays in a cover-first view with an optional waveform seek bar

## 1. Scope & intent
At 11:58–12:22 the designer plays her own songs from the Music section (Songs / Albums / Artists / Genres) and points
out that audio "looks completely different from a video, with a massive cover art", and that "you have the
**waveform seek bar** if you want" (frame `s014` 12:10–12:22: blurred artwork background, title
`untangle - juxtopp…` bottom-left, a seek bar drawn as vertical amplitude bars in orange up to the play position and
grey after it; frame `s015` the queue `1 item - 1m`).

After this packet:
1. The audio player view gives the cover the dominant share of the height (today ~37 %).
2. A `PlayerController.loudnessEnvelope` model collects the player's momentary loudness (already computed by the
   core every ~400 ms while audio plays) into fixed-width buckets over the track duration.
3. `SliderBar` can draw that envelope as its background ("waveform" style), gated by a setting that PKT-3025's
   Playback tab toggles. Unplayed parts are drawn flat until heard (§5.1).

One MR, three commits:
1. `qml: give the cover the larger share of the audio player view`
2. `qt: collect the momentary loudness into a per-track envelope`
3. `qml: draw the loudness envelope as a seek bar background`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/player/qml/Player.qml:43-50` (`coverSource`) — `Player.artwork`, else the no-art cover.
- `modules/gui/qt/player/qml/Player.qml:349-400` (`audioComponent`) — the audio view already exists: a
  `DualKawaseBlur` of the cover as background (`:374-400`), lyrics mode (`:362-372`).
- `modules/gui/qt/player/qml/Player.qml:584-640` (`coverItem` / `cover`) — `Layout.preferredHeight:
  rootPlayer.height / sizeConstant` with `sizeConstant: 2.7182` (`:591-596`): the cover takes ~37 % of the height;
  title/album/artist labels and audio controls stack below (`:679-790`).
- `modules/gui/qt/player/qml/SliderBar.qml:28` (`T.ProgressBar`) and `:317-400` (`background`) — a flat `sliderRect`
  (`:323-324`) or, with chapters, a `Repeater` of chapter segments (`:330-400`). No waveform anywhere in
  `modules/gui/qt` (`grep -ri waveform` → none).
- `modules/gui/qt/player/qml/ControlBar.qml:217-220` — the `SliderBar` instance the waveform style applies to.
- `include/vlc_player.h:2275-2284` (`VLC_PLAYER_METADATA_LOUDNESS_MOMENTARY` / `_FULL`) and `:2294-2348`
  (`vlc_player_metadata_cbs`, `vlc_player_AddMetadataListener`) — the core already pushes EBU R128 momentary
  loudness "approximately every 400ms" while audio plays; `grep AddMetadataListener modules/gui/qt` → none: the Qt
  UI never listens.
- `test/src/player/loudness.c:22-59` (`test_audio_loudness_meter`) — proves the listener fires with a mock audio
  media; the pattern for a core-side check if one is needed.

Settings / variables involved: new `qt-seekbar-waveform` (bool, default false) owned by PKT-3025; nothing in
`src/libvlc-module.c`.
Mock boundary: `LoudnessEnvelope` is a plain `QAbstractListModel` fed by `addSample(vlc_tick_t date, double lufs)`;
QtTest feeds samples directly, no player. The `vlc_player` listener wiring in `PlayerController` is covered by the
MANUAL row.

### Overlaps & contradictions
- **PKT-3006** (thick seek bar) rewrites `SliderBar.qml`'s geometry; the waveform is a background variant of that
  bar, hence the dependency.
- **PKT-3025** owns the "show waveform on seek bar" toggle (09:13); this packet reads the setting, never adds UI for it.
- **PKT-3007** (thumbnail preview while seeking) also draws over `SliderBar`; the waveform stays under the preview.
- **PKT-3001** tokens give the orange accent used for the played part.

## 2b. Upstream state
Master already ships the "completely different" audio view: blurred-cover background, centred cover, title/album/
artist, lyrics (`modules/gui/qt/player/qml/Player.qml:349-860`). The packet is narrowed to the cover proportion and the waveform. The waveform
has no upstream counterpart; the momentary loudness feed it can use exists in the core and is unused by Qt.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `LoudnessEnvelope` | duration 100 s, 200 buckets | `addSample(10 s, -23)` | bucket 20 holds the normalised value of -23 LUFS; others 0 | `tests/test_loudness_envelope.cpp::testSampleLandsInBucket` |
| 2 | same | two samples in one bucket | add -30 then -10 | bucket keeps the max (-10) | `test_loudness_envelope.cpp::testBucketKeepsPeak` |
| 3 | same | samples present | `setDuration` / new media (`reset()`) | all buckets 0, `dataChanged` / `modelReset` emitted | `test_loudness_envelope.cpp::testResetOnNewMedia` |
| 4 | same | silence `-inf` | add | bucket stays 0, no NaN | `test_loudness_envelope.cpp::testSilenceIsZero` |
| 5 | same | duration unknown (live) | add | ignored, `available == false` | `test_loudness_envelope.cpp::testLiveDisabled` |
| 6 | audio view | 1000×700 window, track with cover | screenshot | cover ≥ 55 % of height, labels below | MANUAL `pkt3030-audio-view.png` |
| 7 | waveform bar | setting on, 30 s played | screenshot | bars orange up to position, flat grey after | MANUAL `pkt3030-waveform.png` |

## 4. Fixtures & mocks
- Pattern to copy: `tests/test_base_model.cpp` + meson block `modules/gui/qt/meson.build:1287-1302` (`base_model_test`, no libvlc
  instance) — `LoudnessEnvelope` needs only Qt and `vlc_tick.h`.
- Stubs: none; samples are synthetic.
- Sample media for MANUAL: any MP3 with embedded artwork.

## 5. Design decisions & open questions
1. **RULED — live envelope vs full waveform.** RULED 2026-10-14 (R0): ship the cheap live envelope (filled as the track plays, cached by ML media id); the core pre-scan packet is not opened now. The video shows the whole track's waveform from the start. The
   core has no peak-extraction API; a full waveform needs a pre-scan (a second input with a null audio output and an
   analysis filter, or a preparser extension) — a **core packet** with CPU and battery cost that VideoLAN would have
   to accept. This packet ships the cheap variant (filled as the track plays; re-plays of the same track can reuse a
   cache keyed by ML media id). The core packet is not opened now.
2. **RULED — cover share.** RULED 2026-10-14 (R0): the cover takes about 0.55 x the height. Raising the cover from `height / e` to ~0.55 × height changes a VideoLAN layout
   constant (`modules/gui/qt/player/qml/Player.qml:596`); trivial code, 
3. NOTE — loudness (LUFS, ~400 ms) is an envelope, not a sample-accurate waveform; at 200 buckets over a 3-minute song
   one bucket ≈ 0.9 s ≈ 2 samples, enough for the look of the video.
4. NOTE — the listener is registered only while `Player.hasVideoOutput === false` and the setting is on, so video
   playback pays nothing.
5. **Split (T3, 24 h)**: 3030a cover proportion (row 6, ~2 h, T1, independent of 3006); 3030b `LoudnessEnvelope` +
   `PlayerController` wiring (rows 1–5, ~10 h); 3030c waveform background in `SliderBar` (row 7, ~12 h, after 3006 and
   3025).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3030-music-view-waveform-seek` (3030a can start from `ce4af2f`; 3030c after PKT-3006/3025 merged); §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm `SliderBar.qml` shape after PKT-3006.
- [ ] **Step 3: Cover share** — `modules/gui/qt/player/qml/Player.qml:591-596`; screenshot row 6. Commit `qml: give the cover the larger share of the audio player view`.
- [ ] **Step 4: First row watched failing** — `tests/test_loudness_envelope.cpp` + `vlc_tests` block, row 1 with a stub `addSample` doing nothing → FAIL.
- [ ] **Step 5: Envelope** — `player/loudness_envelope.{hpp,cpp}`; rows 1–5 green; `PlayerController` registers `vlc_player_AddMetadataListener(…, VLC_PLAYER_METADATA_LOUDNESS_MOMENTARY, …)` and forwards to the UI thread; `Q_PROPERTY loudnessEnvelope`. Commit `qt: collect the momentary loudness into a per-track envelope`.
- [ ] **Step 6: Bar** — `player/qml/WaveformBackground.qml` (a `Repeater`/`Canvas` over the model), used by `SliderBar.background` when the setting is on; screenshot row 7. Commit `qml: draw the loudness envelope as a seek bar background`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
