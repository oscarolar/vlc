---
id: PKT-3023
title: "Audio tab: default track, delay slider and an equalizer with friendly presets"
section: "C. Audio"
video_ref: "08:41–08:56"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3023-audio-tab-equalizer
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/equalizer_model.cpp
  - modules/gui/qt/player/equalizer_model.hpp
  - modules/gui/qt/player/qml/AudioSettingsPage.qml
  - modules/gui/qt/player/qml/EqualizerSection.qml
  - modules/gui/qt/player/qml/TracksPageAudio.qml
  - modules/gui/qt/dialogs/extended/extended_panels.cpp
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/tests/test_equalizer_model.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 22.0
priority: P2
depends_on: [PKT-3018, PKT-3019]
upstream_ruling: not-needed
target: upstream
mr_title: "qml: audio tab with delay and equalizer presets in the settings sidebar"
status: ready
---

# PKT-3023 — Audio tab: default track, delay slider and an equalizer with friendly presets

## 1. Scope & intent
08:39–08:56: the Audio tab mirrors the Subtitles tab — **default audio track/language** and **audio delay** —
then the audio filters, which "could be expanded, like the equalizer, which is for audio nerds", while a casual
user benefits from **basic friendly presets with preview icons**. Frame `s010` (≈08:42–08:52, bottom row) shows
it: a channel-mode dropdown (`Mono`), *Synchronization → Audio Track Delay* `0.000 s` with a bipolar slider
`-10 s · 0 s · +10 s`, *Filters → Equalizer* dropdown (`None`) and an *Expand Settings ›* row; expanded
(middle column) *Collapse Settings*, *Preamp 12.0 dB* and one horizontal slider per band (`60 Hz 0.0 dB`,
`170 Hz`, `310 Hz`, `600 Hz` …); below, *2 Pass* and *Compressor* toggles.

Before: audio delay is a spin box in the tracks popup; the equalizer, its 18 presets and the other filters live in
the Widgets *Adjustments and Effects* dialog. After: an Audio page in the sidebar with stereo mode, delay
slider + estimator, defaults rows (stored by PKT-3022), and an equalizer section driven by a C++
`EqualizerModel` shared with the Widgets dialog. Compressor/spatializer/etc. stay in the dialog (link "More
audio effects…").

T3, split in §5.5. Commits: `qml: audio delay slider in the settings sidebar`, `qt: add EqualizerModel`,
`qt: use EqualizerModel in the effects dialog`, `qml: equalizer presets in the settings sidebar`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/TracksPageAudio.qml:69-138` — `DelayEstimator` (`:69-72`) and `SpinBoxExt`
  `-10000…10000` ms step 50 on `Player.audioDelayMS` (`:83-124`), Reset (`:126-138`).
- `modules/gui/qt/player/qml/TracksPageAudio.qml:141-174` — the estimator buttons have **crossed bindings**:
  "Sound Heard" shows `isSpottedTimeMarked` and calls `markSpottedTime()`, "Sound Spotted" shows
  `isHeardTimeMarked` and calls `markHeardTime()`, with `//method name should be changed` comments (`:155`,
  `:169`). Behaviour is consistent but the naming is inverted; kept as is, renamed only if the estimator code is
  touched (§5.4).
- `modules/gui/qt/player/player_controller.hpp:166,169,216-219` — `audioDelay`/`audioDelayMS`,
  `audioDevices` (PKT-3011), `audioStereoMode` (`VLCVarChoiceModel`, the "Mono" dropdown), `audioMixMode`,
  `audioVisualization`.
- `modules/gui/qt/dialogs/extended/extended.cpp:61-106` — the Widgets audio tab: Equalizer, Compressor,
  Expander, Limiter, Spatializer, Stereo Widener, Advanced (pitch shifter, stereo panner).
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:998-1117` (`EqualizerSliderData`) — reads bands from the
  aout (`getBandsFromAout` `:1018-1049`) and writes `equalizer-bands` / config.
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:1161-1263` (`Equalizer::build`) — builds the band
  sliders and the preset combo; **`:1203-1227` already draws a 40×40 curve icon for every preset** from
  `eqz_preset_10b` and puts it in the combo: the "preview icons" of the video exist in master's dialog.
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:1265-1284` (`setCorePreset`) — applies preamp and bands
  to the sliders, sets `equalizer-preset` on the aout, emits `configChanged`.
- `modules/audio_filter/equalizer_presets.h:46-57` — 18 presets (`flat … techno`) with translatable labels;
  `:59-155` their 10-band values and preamp.
- `modules/audio_filter/equalizer.c:84-95` — `equalizer-preset`, `equalizer-bands`, `equalizer-2pass`,
  `equalizer-vlcfreqs`, `equalizer-preamp`.
- `modules/gui/qt/player/player_controller.cpp:1801` (`PlayerController::getAout`) — `SharedAOut`
  (`modules/gui/qt/player/player_controller.hpp:46`) used by the dialog; the model needs the same.

Settings / variables involved: `audio-desync` (`src/libvlc-module.c:1635`, ms, startup default),
`audio-filter` (`:1659`), `audio-language` (`:1867`), `audio-track` (`:1861`), the `equalizer-*` options above.
Mock boundary: `EqualizerModel` against `VLCTestingEnv` — Step 2 checks whether the player's aout exists
without playback (`vlc_player_aout_Hold`); if not, the model is tested on its config path and a fake aout
accessor interface.

### Overlaps & contradictions
- **PKT-3011** (audio button: tracks + output device) owns track list and device; this page links to it, does not
  duplicate it.
- **PKT-3019** owns `SliderValueMapper` (delay slider reuse) and the subtitle estimator; same estimator here.
- **PKT-3022** stores the Defaults rows and the selected equalizer preset in presets.
- **PKT-3018** removes the bar entry to the Widgets dialog; the "More audio effects…" link keeps it reachable.

## 2b. Upstream state
- Audio delay: in QML (spin box + estimator) — needs only the slider form.
- Equalizer: complete in Widgets, including 18 presets **with curve icons** (`modules/gui/qt/dialogs/extended/extended_panels.cpp:1203-1227`)
  and 2-pass. None of it is in QML, and the logic is inside the widget class, not a model.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `EqualizerModel` presets | — | `rowCount()` / `data(i, NameRole)` | 18 rows + a leading "Custom"; names are the translated `preset_list_text`; `CurveRole` holds 10 normalised values | `test_equalizer_model.cpp::testPresetList` |
| 2 | `applyPreset("rock")` | equalizer enabled | apply | `bands()` and `preamp()` equal `eqz_preset_10b[rock]`; `equalizer-preset == "rock"` written | `test_equalizer_model.cpp::testApplyPreset` |
| 3 | `setBand(2, 4.5)` | preset rock applied | change one band | `currentPreset()` becomes "Custom"; `equalizer-bands` string has 4.5 at index 2 | `test_equalizer_model.cpp::testEditMakesCustom` |
| 4 | `setEnabled(true/false)` | `audio-filter` empty | toggle | `equalizer` added/removed in the `audio-filter` chain, other filters kept | `test_equalizer_model.cpp::testEnableTogglesFilterChain` |
| 5 | `setBand` range | — | `setBand(0, 30)` | clamped to ±20 dB (the range the dialog uses, `:1209-1212`) | `test_equalizer_model.cpp::testBandClamped` |
| 6 | Widgets dialog regression | dialog open | pick a preset | sliders move as before (dialog now delegates to the model) | MANUAL — `pkt3023-dialog-preset.png` |
| 7 | `AudioSettingsPage.qml` | sidebar Audio tab, playing | drag delay slider, pick "Rock" preset tile | audio shifts; band sliders reflect Rock; tiles show curve previews | MANUAL — `pkt3023-audio-tab.png` |

## 4. Fixtures & mocks
- Pattern: `modules/gui/qt/tests/test_navigation_model.cpp` + `modules/gui/qt/meson.build:1342-1360`; include
  `modules/audio_filter/equalizer_presets.h` exactly as `extended_panels.cpp` does.
- `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.cpp:92-93`) for the player; config reads/writes on a throw-away
  config (Step 2 confirms `--ignore-config` in the test defaults).
- Sample media for MANUAL rows: any audio/video file.

## 5. Design decisions & open questions
1. NOTE — "friendly presets with preview icons": tiles generated from the preset values (same data the dialog
   uses), drawn in QML (`Canvas` or `Shape`) from `CurveRole`; no bitmaps added.
2. NOTE — Compressor/Expander/Limiter/Spatializer/Stereo Widener/pitch/panner are **not** ported (the frame
   shows only toggles; porting each is its own work). The page links to the Widgets dialog.
3. NOTE — the stereo-mode dropdown uses the existing `Player.audioStereoMode` model.
4. NOTE — estimator naming in `modules/gui/qt/player/qml/TracksPageAudio.qml:141-174` is inverted but consistent; renaming
   `markSpottedTime/markHeardTime` touches `delay_estimator.cpp` and is out of scope unless the page is rewritten
   on top of it.
5. **Split (T3):** **3023a** Audio page with stereo mode + delay slider + estimator + defaults rows (6 h);
   **3023b** `EqualizerModel` + dialog delegation, rows 1–6 (10 h); **3023c** equalizer section with preset
   tiles and expandable bands, row 7 (6 h).

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
- [ ] **Step 0: Split** — write 3023a/b/c from §5.5.
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: branch `master-pkt-3023-audio-tab-equalizer` (suffix per split) from merged PKT-3018/3019; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; check `vlc_player_aout_Hold` without an input in the test env and record the mock boundary.
- [ ] **Step 3 (3023a)** — `player/qml/AudioSettingsPage.qml` (stereo mode, delay slider via `SliderValueMapper`, estimator, defaults rows) in the Audio tab; MANUAL; commit `qml: audio delay slider in the settings sidebar`.
- [ ] **Step 4 (3023b): Row 1 watched failing** — `tests/test_equalizer_model.cpp` against an empty `player/equalizer_model.{hpp,cpp}` registered in `meson.build` → FAIL; rows 2–5; commit `qt: add EqualizerModel`.
- [ ] **Step 5 (3023b)** — `Equalizer` in `modules/gui/qt/dialogs/extended/extended_panels.cpp:1121-1300` delegates presets/bands/enable to the model; MANUAL row 6; commit `qt: use EqualizerModel in the effects dialog`.
- [ ] **Step 6 (3023c)** — `player/qml/EqualizerSection.qml` (preset tiles with curves, Expand/Collapse to band sliders, 2-pass toggle); register in `mainui.cpp` and `meson.build`, `po/POTFILES.in`; MANUAL row 7; commit `qml: equalizer presets in the settings sidebar`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MRs** — MR to videolan/vlc master (Draft); `status: review`.
