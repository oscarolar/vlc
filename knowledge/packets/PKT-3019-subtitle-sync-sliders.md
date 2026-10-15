---
id: PKT-3019
title: "Subtitle delay, speed and duration are set with sliders in the Subtitles tab"
section: "C. Subtitles"
video_ref: "07:16–07:22"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/player
branch: master-pkt-3019-subtitle-sync-sliders
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/TracksPageSubtitle.qml
  - modules/gui/qt/player/qml/SubtitleSyncSection.qml
  - modules/gui/qt/util/qml/SliderValueMapper.qml
  - modules/gui/qt/player/subs_duration_controller.cpp
  - modules/gui/qt/player/subs_duration_controller.hpp
  - modules/gui/qt/dialogs/extended/extended_panels.cpp
  - modules/gui/qt/tests/tst_SliderValueMapper.qml
  - modules/gui/qt/tests/test_subs_duration_controller.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 12.0
priority: P1
depends_on: [PKT-3018]
upstream_ruling: not-needed
target: upstream
mr_title: "qml: subtitle synchronization sliders in the settings sidebar"
status: ready
actual_h_bugfix: 0.3
---

# PKT-3019 — Subtitle delay, speed and duration are set with sliders in the Subtitles tab

## 1. Scope & intent
At 07:13–07:22 the designer opens the **Subtitles** tab of the sidebar (PKT-3018) and asks for the three timing
options of the old *Synchronization* tab — **Subtitle Track Delay, Subtitle Speed, Subtitle Duration Factor** —
"but imagine having to type, when it could be an easy slider instead". Frame `s009` (≈07:18) and `s010`
(≈08:40) show the target: section title *Synchronization*; each option is a label on the left, a text field on the
right with its unit (`-2.000 s`, `1.000 fps`, `0.000`), and under it a full-width slider in the accent colour;
the delay slider is bipolar with tick labels `-10 s · 0 s · +10 s` and its thumb starts at the centre.

Before: QML has delay (primary/secondary) and speed only as `SpinBoxExt` rows inside the tracks popup; the
duration factor exists only in the Widgets dialog. After: one *Synchronization* section in the Subtitles tab with
three slider+field rows (and the secondary delay row when a secondary track is active), each with Reset.

One MR, four commits:
1. `qml: fix the subtitle speed spinbox refresh` — the bug of §2 (standalone, can go first upstream).
2. `qml: add SliderValueMapper` — the value↔slider mapping in `VLC.Util` + `TestCase`.
3. `qt: move the subtitle duration factor out of the effects dialog` — `SubsDurationController` + QtTest;
   `SyncControls` uses it.
4. `qml: subtitle synchronization sliders in the settings sidebar` — `SubtitleSyncSection.qml` in the
   Subtitles tab.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/TracksPageSubtitle.qml:77-196` — primary delay: `DelayEstimator`
  (`:77-83`, "Voice Heard / Text Seen" sync helper, buttons `:168-194`), `SpinBoxExt spinBoxA` `:99-141`
  (`stepSize 50`, `from -10000`, `to 10000` ms, bound to `Player.subtitleDelayMS`), Reset `:143-158`.
- `modules/gui/qt/player/qml/TracksPageSubtitle.qml:198-323` — the same for the secondary delay
  (`Player.secondarySubtitleDelayMS`).
- `modules/gui/qt/player/qml/TracksPageSubtitle.qml:342-383` (`spinBoxC`, subtitle speed) — `from 0, to 100`,
  value = `Player.subtitleFPS * 10`. **Bug**: its `Connections` (`:372-382`) listens to
  `onSecondarySubtitleDelayChanged` instead of `onSubtitleFPSChanged`, and writes `Player.subtitleFPS / 10`
  where `:360` uses `* 10`. The field therefore never follows an external FPS change and jumps to 1/100 of the
  value whenever the secondary delay moves.
- `modules/gui/qt/player/player_controller.hpp:167-172` — `subtitleDelay`, `secondarySubtitleDelay`,
  `subtitleDelayMS`, `secondarySubtitleDelayMS`, `subtitleFPS` properties; `modules/gui/qt/player/player_controller.cpp:1472-1487`
  (`setSubtitleDelay`, `addSubtitleDelay` absolute/relative), `:1541-1548` (`setSubtitleFPS` →
  `vlc_player_SetAssociatedSubsFPS`), `:776-784` the FPS callback emitting `subtitleFPSChanged`.
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:1471-1582` (`SyncControls::SyncControls`) — the Widgets
  sync tab: A/V spin, primary/secondary subtitle spins, `subSpeedSpin` 1–100 fps (`:1519-1527`) and
  `subDurationSpin` 0–20 (`:1533-1540`).
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:1647-1706` — `adjustSubsDuration` sets
  `subsdelay-factor` on every vout and adds/removes the `subsdelay` sub-filter (`:1651-1652`);
  `initSubsDuration` (`:1656-1680`) picks unit and tooltip from `subsdelay-mode`; `subsdelayClean`
  (`:1682-1686`) removes the filter — called from the destructor and `clean()` (`:1584-1597`), so the duration
  factor dies with the dialog.
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:1547` — the refresh button's tooltip reads
  "Eject the disc" (copy-paste); fixed in commit 3 since the file is touched.
- `modules/spu/subsdelay.c:100-101,272-277` — modes (`0` absolute seconds, `1` relative to source delay, default
  `1`), `subsdelay-factor` float 0–20, default 2.

Settings / variables involved: `sub-delay` (`src/libvlc-module.c:1814`, integer in 1/10 s — the startup default,
not touched here), `sub-fps` (`:1813`), `subsdelay-mode` / `subsdelay-factor` (`modules/spu/subsdelay.c:272-275`),
the `sub-filter` chain (`src/libvlc-module.c:1835`).
Mock boundary: `SliderValueMapper` is pure QML (no `Player`); `SubsDurationController` is tested against the
real `vlc_player` of `VLCTestingEnv` (`tests/vlc_stub_modules.hpp`, `intf->p_player`) with no vout, asserting
the `sub-filter` string and the factor variable.

### Overlaps & contradictions
- **PKT-3018** provides the Subtitles tab `Loader`; this packet only adds `SubtitleSyncSection.qml` to it.
- **PKT-3020** (subtitle timeline) writes the same `Player.subtitleDelay`; both read it through the property, no
  shared file except `TracksPageSubtitle.qml` if PKT-3020 adds a shortcut there — merge order 3019 then 3020.
- **PKT-3021/3022/3023** reuse `SliderValueMapper` (font size, audio delay). It lands here first.
- **PKT-3023** removes the duplicate audio delay row; this packet leaves `TracksPageAudio.qml` alone.
- The video's sidebar frame drops the "Voice Heard / Text Seen" estimator. Master's estimator is a real feature
  (`modules/gui/qt/player/qml/TracksPageSubtitle.qml:168-194`); it is **kept** under the slider (§5.2).

## 2b. Upstream state
- Delay and speed **are already adjustable in QML** (spin boxes, ±10 s at 50 ms steps), plus a guided
  estimator — the video's "you have to type" is half true: the spin boxes take wheel and arrow keys, but there is
  no slider and the controls sit in a popup that closes when you click the video.
- The **duration factor** is not in QML at all, and only lives while the Widgets dialog exists.
- Hotkeys for delay exist (`key-subdelay-up/down`, `src/libvlc-module.c:2655-2658`, `H`/`G` on Linux) — the
  "press this or this" of 06:49.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `modules/gui/qt/player/qml/TracksPageSubtitle.qml:372-382` speed refresh | — (logic moved to row 2's mapper) | `Player.subtitleFPS` changes outside the page | the speed field shows the new value ×1, not ÷100 | MANUAL — `pkt3019-fps-refresh.png` (Player singleton can't load in the QML runner) |
| 2 | `SliderValueMapper` | `from -10000, to 10000, step 50` | `positionFor(-2000)` / `valueAt(0.4)` | `0.4` / `-2000`; round-trip exact on steps | `tst_SliderValueMapper.qml::test_linear_roundtrip` |
| 3 | `SliderValueMapper` | same, `snapToZero 150` | `valueAt(0.504)` (≈ +80 ms) | `0`: the centre detent the bipolar slider needs | `tst_SliderValueMapper.qml::test_snap_to_zero` |
| 4 | `SliderValueMapper` | same | `valueAt(-0.2)` / `valueAt(1.3)` | clamped to `-10000` / `10000` | `tst_SliderValueMapper.qml::test_clamp` |
| 5 | `SliderValueMapper` | `from 0.25 to 4` speed, `log: true` | `positionFor(1.0)` | the middle of the track is 1× (log scale), so slow-down and speed-up get equal room | `tst_SliderValueMapper.qml::test_log_scale_centres_one` |
| 6 | `SliderValueMapper` | delay mapper | `format(-2000)` / `parse("-2.000 s")` | `"-2.000 s"` / `-2000` with the locale decimal | `tst_SliderValueMapper.qml::test_format_parse` |
| 7 | `SubsDurationController` | player with an input, mode absolute | `setFactor(1.5)` | `subsdelay-factor` = 1.5 on the player, `sub-filter` contains `subsdelay` | `test_subs_duration_controller.cpp::testSetFactorEnablesFilter` |
| 8 | `SubsDurationController` | factor 1.5 set | `setFactor(0)` | `sub-filter` no longer contains `subsdelay` | `test_subs_duration_controller.cpp::testZeroDisablesFilter` |
| 9 | `SubsDurationController` | factor set | the controller is destroyed | the filter **stays** (unlike `SyncControls` today, `modules/gui/qt/dialogs/extended/extended_panels.cpp:1584-1597`) | `test_subs_duration_controller.cpp::testFactorOutlivesController` |
| 10 | `SubsDurationController` | `subsdelay-mode` = 1 | `unit()` | `""` with a "multiply" hint; mode 0 → `"s"` | `test_subs_duration_controller.cpp::testUnitFollowsMode` |
| 11 | `SubtitleSyncSection.qml` | sidebar open, video with external `.srt` | drag delay slider to −2 s | subtitles shift live, field reads `-2.000 s`, Reset returns to 0 | MANUAL — `pkt3019-sync-sliders.png` |

## 4. Fixtures & mocks
- Pattern to copy (QML): `modules/gui/qt/tests/tst_FSM.qml:18-60`; register `tst_SliderValueMapper` in
  `qml_tests` (`modules/gui/qt/meson.build:954-956`) and `SliderValueMapper.qml` in `VLC.Util` (`modules/gui/qt/meson.build:837-856`).
- Pattern to copy (C++): `modules/gui/qt/tests/test_navigation_model.cpp:34-170` + its `vlc_tests` entry
  `modules/gui/qt/meson.build:1342-1360` (`tests/vlc_stub_modules.cpp` + sources + `moc_headers`, suite `qt`).
- `VLCTestingEnv` (`tests/vlc_stub_modules.hpp`) gives `intf->p_player` (`modules/gui/qt/tests/vlc_stub_modules.cpp:92-93`);
  rows 7–10 need no vout: the controller writes the variable on the player and the filter chain string, which is
  what the vout reads when it starts. Step 2 confirms `GetVFilterType`/`ChangeFiltersString` helpers
  (`modules/gui/qt/dialogs/extended/extended_panels.cpp:1697-1706`) are callable without a vout, or the controller writes `sub-filter` itself.
- Sample media for MANUAL rows: any local video + an `.srt` next to it.

## 5. Design decisions & open questions
1. NOTE — the speed slider is labelled "Subtitle Speed" with an `fps` field as in the video, but the value is
   the factor `vlc_player_SetAssociatedSubsFPS` takes (1.000 = unchanged). The field keeps master's
   `textFromValueB` formatting.
2. NOTE — the delay estimator ("Voice Heard / Text Seen") stays as a compact row under the delay slider; the
   design omits it but it is the best tool for the exact problem of 06:30.
3. NOTE — slider range ±10 s like master's spin box; values beyond stay reachable by typing in the field.
4. NOTE — the duration factor moves to a C++ controller owned by `MainCtx`/player side so it no longer dies with
   the dialog; the Widgets dialog uses the same controller (one implementation).
5. NOTE — row 1 is MANUAL because `TracksPageSubtitle.qml` imports `VLC.Player` (Player singleton) which the
   runner does not load (`modules/gui/qt/tests/qml_test.cpp:38-41`); after commit 4 the field reads through
   `SliderValueMapper`, which rows 2–6 cover.

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

#### Bugfix part (commit 1) — done; redesign part (commits 2–4) still pending
- Bug confirmed on HEAD ce4af2f: the `spinBoxC` Connections handler listened to `onSecondarySubtitleDelayChanged`,
  wrote `Player.subtitleFPS / 10` (inverse of the `* 10` used on init) and assigned an unqualified `value`
  (not a property of the Connections scope), so the field never followed external FPS changes.
  The C++ NOTIFY signal is `PlayerController::subtitleFPSChanged(float)`; the factor is 1.0 = unchanged.
- Fix: handler renamed to `onSubtitleFPSChanged`, writes `spinBoxC.value = Player.subtitleFPS * 10` (2-line diff).
- Branch `master-pkt-3019-subtitle-sync-sliders-bugfix`, commit `e2f454ef380d952e30373b4e5bf3f53fa802bffc`
  "qml: fix the subtitle speed spinbox refresh".
- PR: https://github.com/oscarolar/vlc/pull/1 (bugfix part only).
- Build: CI image `vlc-debian-unstable:20260611225331`, `-Dextra_checks=true`, `ninja -j8`: no warning from the
  touched file (only the pre-existing cc1plus `-Werror=unterminated-string-initialization` notice, also in baseline).
  `meson test --suite qt`: 7/7 OK before (`~/src/logs/pkt-3019-baseline.log`) and after (`~/src/logs/pkt-3019-fix.log`).
- Row 1 is MANUAL (no automated test: `qml_test.cpp` loads only `VLC.Util`, not the `Player` singleton):
  1. play a video with an external `.srt`; open the tracks popup, subtitle page: Subtitle Speed reads `1.000 fps`;
  2. Tools > Effects and Filters > Synchronization: set Subtitle speed to `2.000 fps`;
  3. expected: the QML field shows `2.000 fps` (before the fix it stayed at `1.000 fps`);
  4. with a secondary subtitle track, change the secondary delay: the speed field must not change.
  Screenshots not taken (no GUI session on nzxt); to attach when the MANUAL rows are run.

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3019-subtitle-sync-sliders` from the merged PKT-3018 (or `ce4af2f` + the 3018 branch); build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; confirm the `spinBoxC` bug still exists on HEAD; confirm whether the filter-string helpers need a vout.
- [x] **Step 3: Commit 1** — in `modules/gui/qt/player/qml/TracksPageSubtitle.qml:372-382` listen to `onSubtitleFPSChanged` and use `* 10`; MANUAL row 1 screenshot before/after; commit `qml: fix the subtitle speed spinbox refresh`.
- [ ] **Step 4: Row 2 watched failing** — `tests/tst_SliderValueMapper.qml` against an empty `util/qml/SliderValueMapper.qml` → FAIL; implement; rows 3–6 one cycle each; commit `qml: add SliderValueMapper`.
- [ ] **Step 5: Rows 7–10** — `player/subs_duration_controller.{hpp,cpp}` + `tests/test_subs_duration_controller.cpp` registered in `meson.build` like `test_qt_navigation_model`; `SyncControls::adjustSubsDuration/initSubsDuration/subsdelayClean` delegate to it; fix the "Eject the disc" tooltip (`:1547`); commit `qt: move the subtitle duration factor out of the effects dialog`.
- [ ] **Step 6: Section** — `player/qml/SubtitleSyncSection.qml` (three `SliderExt` + field rows, secondary delay row when a secondary track is selected, estimator row) added to the Subtitles tab of PKT-3018; register in `VLC.Player` and `po/POTFILES.in`; MANUAL row 11; commit `qml: subtitle synchronization sliders in the settings sidebar`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MR** — push to the fork, MR to videolan/vlc master (Draft) (commit 1 may be split out as its own MR); `status: review`.
