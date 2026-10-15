---
id: PKT-3025
title: "Playback tab: speed slider, reverse, sleep timer and seek-bar overlay toggles"
section: "C. Playback"
video_ref: "09:07–09:18"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/player
branch: master-pkt-3025-playback-tab-sleep-timer
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/sleep_timer.cpp
  - modules/gui/qt/player/sleep_timer.hpp
  - modules/gui/qt/player/qml/PlaybackSettingsPage.qml
  - modules/gui/qt/player/qml/PlaybackSpeed.qml
  - modules/gui/qt/maininterface/mainctx.cpp
  - modules/gui/qt/maininterface/mainctx.hpp
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/tests/test_sleep_timer.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 16.0
priority: P2
depends_on: [PKT-3018, PKT-3020]
upstream_ruling: needed
target: fork
mr_title: "qml: playback tab with speed, sleep timer and overlay toggles"
status: ready
---

# PKT-3025 — Playback tab: speed slider, reverse, sleep timer and seek-bar overlay toggles

## 1. Scope & intent
09:07–09:18: "playback settings will house the extras like speed", then a **reverse** feature, a **sleep timer**,
**show subtitle timeline**, **show waveform on the seek bar** and **show metadata**. Frame `s011` (≈09:08–09:15,
top row) shows the page: tab *Playback* active; toggles *Show Subtitle Timeline on Seekbar* (OFF), *Show
Waveform on Seekbar* (OFF), *Reverse* (OFF); *Sleep Timer* dropdown `Off`; *Speed* field `1x` with a slider and
ticks `0.25 · 0.5 · 0.75 · 1 · 1.25 · 1.5 · 1.75 · 2`. "Show metadata" is said, not drawn.

Before: speed lives in the tracks popup (`PlaybackSpeed.qml`), reverse is an optional bar button, there is no
sleep timer, and the seek-bar overlays do not exist yet. After: one Playback page with speed, a sleep timer
(off / 15 / 30 / 45 / 60 / 90 min / end of current media), the overlay toggles persisted in Qt settings and read
by PKT-3020/3030, and a Reverse toggle **only where the core can do it** (§2b, §5.1).

One MR, three commits:
1. `qt: add a sleep timer` — `SleepTimer` C++ + QtTest.
2. `qt: add player overlay settings` — persisted booleans on `MainCtx` (`subtitleTimelineOnSeekbar`,
   `waveformOnSeekbar`, `metadataOnPause`).
3. `qml: playback tab with speed, sleep timer and overlay toggles`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/PlaybackSpeed.qml:40-51` — preset values `0.25 … 2` (with 0.96/1.04 for video
  enthusiasts) and a slider range `0.25 … 4.0`; `:79-110` snapping to presets. The frame's slider is this
  control, laid out wider.
- `modules/gui/qt/player/qml/TracksPageSpeed.qml:25-35` and `modules/gui/qt/player/qml/TracksListPage.qml:50-68` — speed reached today
  through the `1.00x` button of the tracks popup.
- `modules/gui/qt/player/qml/controlbarcontrols/ReverseButton.qml:29-37` — `enabled: Player.rewindable`,
  `onClicked: Player.reverse()`; `REVERSE_BUTTON` exists in `modules/gui/qt/player/control_list_model.hpp:79` but is not in the
  default layout.
- `modules/gui/qt/player/player_controller.cpp:1293-1303` (`PlayerController::reverse`) — negates the current
  rate with `vlc_player_ChangeRate(-rate)`.
- `src/input/input.c:2177-2207` (`INPUT_CONTROL_SET_RATE`) — a negative rate is refused ("cannot set negative
  rate", rate kept) whenever `b_rescale_ts` is true; `:2990-3009` — `b_rescale_ts` is true for every demuxer
  with pace control (all local files) and for those that cannot control rate. `:839-840` — the
  `REWINDABLE` capability (→ `Player.rewindable`) is only set when the demuxer controls rate itself without
  pace control. **Reverse playback of a local file is impossible in master**; the button is disabled for it.
- `modules/gui/qt/playlist/playlist_controller.hpp:84-90,107` — `MediaStopAction {CONTINUE, STOP, EXIT}` on
  `mediaStopAction` (from `include/vlc_playlist.h:176-183`); `modules/gui/qt/menus/menus.cpp:218-225` uses it
  for "Quit at the end of play queue". "Stop after this media" is therefore available to the sleep timer.
- Sleep timer: `grep -ri "sleep.timer\|sleeptimer"` over `modules/gui/qt`, `src`, `share/lua` finds nothing;
  `stop-time`/`run-time` (`src/libvlc-module.c:1905-1910`) are per-input options, not a wall-clock timer.

Settings / variables involved: `rate` (`src/libvlc-module.c:1914`), `stop-time`/`run-time` (`:1905-1910`,
not used), the playlist media-stopped action (`include/vlc_playlist.h:176-183`); three new Qt settings for the
overlay toggles.
Mock boundary: `SleepTimer` takes an injectable clock/`QTimer` interval and a target interface
(`pause()`, `setMediaStopAction()`), tested with a recording fake; no player.

### Overlaps & contradictions
- **PKT-3020** reads `subtitleTimelineOnSeekbar`; **PKT-3030** reads `waveformOnSeekbar`; neither may define the
  setting — it is defined here (merge this before them or stub the property in whichever lands first).
- **PKT-3016** (resume prompt) and **PKT-3015** (OSD with hotkey) — the sleep timer's expiry OSD uses the OSD
  path PKT-3015 extends; no shared file.
- **PKT-3003** default bar layout may drop the speed button; the page keeps speed reachable.

## 2b. Upstream state
- Speed: complete (`PlaybackSpeed.qml`), only relocated.
- Reverse: core supports negative rates only for demuxers that control rate without pace control; for files the
  UI is right to disable it. A true "play backwards" for files is a core/decoder project (reverse GOP decoding),
  far outside a UI redesign.
- Sleep timer: absent. End-of-media stop exists (`MediaStopAction`).
- Overlay toggles: absent (their features are PKT-3020/3030).

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `SleepTimer` | off | `start(30 min)` | `active`, `remaining` ≈ 30 min, target untouched | `test_sleep_timer.cpp::testStartArms` |
| 2 | `SleepTimer` | armed with 1 s (test interval) | time elapses | target receives `pause()` once; `active == false`; `expired` emitted | `test_sleep_timer.cpp::testExpiryPauses` |
| 3 | `SleepTimer` | armed | `cancel()` | no `pause()` after the interval; `remaining == 0` | `test_sleep_timer.cpp::testCancel` |
| 4 | `SleepTimer` | off | `startEndOfMedia()` | target `setMediaStopAction(STOP)`; after `onMediaEnded()` the action is restored to the previous value | `test_sleep_timer.cpp::testEndOfMediaRestoresAction` |
| 5 | `SleepTimer` | armed 30 min | `start(15 min)` | re-armed to 15, a single pending expiry | `test_sleep_timer.cpp::testRestartReplaces` |
| 6 | overlay settings | fresh settings | set `subtitleTimelineOnSeekbar = true`, recreate the object | value persisted (`QSettings`) | `test_sleep_timer.cpp::testOverlaySettingsPersist` (or a `MainCtx`-free settings holder if `MainCtx` can't be built in the test — Step 2) |
| 7 | `PlaybackSettingsPage.qml` | sidebar Playback tab | set Sleep Timer 15 min | OSD "Sleep timer: 15 min", playback pauses at expiry, dropdown returns to Off | MANUAL — `pkt3025-sleep.png` |
| 8 | Reverse toggle | local file / a demuxer with `rewindable` | open the tab | toggle disabled with tooltip "Not supported for this media" / enabled and works | MANUAL — `pkt3025-reverse.png` |

## 4. Fixtures & mocks
- Pattern: `modules/gui/qt/tests/test_thread_runner.cpp` and its `vlc_tests` block (`modules/gui/qt/meson.build:1384-1401`) —
  a Qt-only test without libvlc; `QSignalSpy` + a short interval or `QTest::qWait`.
- Fake target implementing `pause()` / `mediaStopAction` get/set; production glue binds it to
  `PlayerController::pause` and `PlaylistController::setMediaStopAction`.
- No sample media for tests; any file for MANUAL rows.

## 5. Design decisions & open questions
1. **RULED — Reverse.** RULED 2026-10-14 (R0): Reverse is shown but disabled unless `Player.rewindable`; no core reverse. Shown as in the frame but disabled unless `Player.rewindable`; a working reverse for
   files is not offered (core project). A core reverse would be its own packet.
2. NOTE — Sleep timer action = **pause** (not quit), plus "End of current media" using `MEDIA_STOPPED_STOP`.
   Quitting VLC stays the existing "Quit at the end of play queue" (`modules/gui/qt/menus/menus.cpp:218-225`).
3. **RULED — new settings keys.** RULED 2026-10-14 (R0): three persisted UI booleans in `vlc-qt-interface.conf` (not `vlcrc`), no core option. Three persisted UI booleans in `vlc-qt-interface.conf` (not `vlcrc`), no
   core option. Needs VideoLAN acceptance of the keys' names; they are read by PKT-3020/3030.
4. NOTE — "Show metadata" has no design yet (what metadata, where); only the toggle and its setting are added,
   consumer later (PKT-3030 music view could use it).
5. NOTE — speed slider range stays 0.25–4 with the frame's ticks to 2; values above 2 reachable by the field.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3025-playback-tab-sleep-timer` from merged PKT-3018; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; check whether `MainCtx` can be constructed in a test (decides row 6's holder).
- [ ] **Step 3: Row 1 watched failing** — `tests/test_sleep_timer.cpp` against an empty `player/sleep_timer.{hpp,cpp}` registered in `meson.build` like `test_qt_thread_runner` → FAIL; rows 2–5; commit `qt: add a sleep timer`.
- [ ] **Step 4: Row 6** — overlay settings; commit `qt: add player overlay settings`.
- [ ] **Step 5: Page** — `player/qml/PlaybackSettingsPage.qml` (toggles, sleep dropdown, `PlaybackSpeed` reused), registered in `VLC.Player` and `po/POTFILES.in`; `SleepTimer` registered in `mainui.cpp`; MANUAL rows 7–8; commit `qml: playback tab with speed, sleep timer and overlay toggles`.
- [ ] **Step 6: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
