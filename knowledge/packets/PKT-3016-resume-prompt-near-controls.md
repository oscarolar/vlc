---
id: PKT-3016
title: "The resume prompt sits by the central controls and says where you stopped"
section: "B. Feedback"
video_ref: "13:58–14:10"
pipeline: CI
kind: core
track: T2
component: modules/gui/qt/player, src/player
branch: master-pkt-3016-resume-prompt-near-controls
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/ResumeDialog.qml
  - modules/gui/qt/player/qml/TopBar.qml
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/player/player_controller.hpp
  - modules/gui/qt/player/player_controller.cpp
  - modules/gui/qt/player/player_controller_p.hpp
  - include/vlc_player.h
  - src/player/player.c
  - src/player/player.h
  - src/player/medialib.c
  - test/src/player/restore.c                                            # new
  - test/src/player/modules.c
  - test/src/player/common.h
  - test/src/meson.build
estimate_h: 14.0
priority: P2
depends_on: [PKT-3003]
upstream_ruling: needed
target: fork
mr_title: "qml: show the resume prompt above the central controls"
status: ready
---

# PKT-3016 — The resume prompt sits by the central controls and says where you stopped

## 1. Scope & intent
**Before (master, and the video at 13:58, sheet `s016` frame 6).** When a known video reopens, the player's top bar
swaps the cone/back button for one bold line top-left: "Do you want to restart the playback where you left off?"
[Continue] [Dismiss]. It is far from where the eyes are (the centre/bottom), it does not say *where* playback would
resume, and it vanishes after 10 s. The designer barely noticed it (14:01–14:08).

**After (sheet `s016` frame 7, ~14:06).** A compact pill centred horizontally just above the control bar, next to
the central controls: "You already watched till **00:00:02**" · [**Continue Watching**] (accent-filled button) ·
"×". Same behaviour otherwise: Enter = continue, Esc/× = dismiss, auto-hide timer, keyboard focus on the button.
The top bar goes back to always showing the cone/back button.

One MR, three commits:
1. `player: pass the resume point to on_playback_restore_queried` — core API + `test/src/player/restore.c`.
2. `qt: expose the resume time in the player controller`.
3. `qml: show the resume prompt above the central controls` — move + new text.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- `modules/gui/qt/player/qml/ResumeDialog.qml:30-201` (`ResumeDialog`, a `FocusScope`) — `:48-53`
  `showResumePanel` (visible, focus on Continue, start timer); `:55-60` `hideResumePanel` (also calls
  `Player.acknowledgeRestoreCallback()`); `:62-69` `resumeTimeout`, **10000 ms**, whose `onTriggered` hides the
  panel **without** `acknowledgeRestoreCallback()` — `canRestorePlayback` stays `true` after a timeout (defect,
  row 4); `:71-80` follows `Player.onCanRestorePlaybackChanged`; `:118` the question text; `:121-164` Continue
  (`Player.restorePlaybackPos()`) and Dismiss; `:166-200` a "small" state wrapping the buttons under the label.
- `modules/gui/qt/player/qml/TopBar.qml:70` (`resumeVisible` alias), `:240-330` (`logoOrResume`) — the dialog
  replaces `logoGroup` (back button + cone) at the top-left (`:270`, `:308-330`), its width bounded by the
  playlist/CSD buttons (`:315-317`); `:346` hides the centred title while it shows.
- `modules/gui/qt/player/qml/Player.qml:948-955` — the top bar is forced visible in fullscreen with pinned controls
  **only because** the resume prompt lives in it (`resumeVisible`); moving the prompt removes that special case.
- `modules/gui/qt/player/qml/Player.qml:41` (`positionSliderY`) and `:1043` — the y every bottom overlay anchors on;
  the pill anchors its bottom there.
- `modules/gui/qt/player/player_controller.hpp:153` (`canRestorePlayback`, NOTIFY `playbackRestoreQueried`),
  `:333-336`; `modules/gui/qt/player/player_controller.cpp:984-992` (`on_player_playback_restore_queried`) — sets the flag, nothing else;
  `:1946-1953` (`restorePlaybackPos`), `:1955-1962` (`acknowledgeRestoreCallback`).
- Core: `src/player/medialib.c:28-75` (`vlc_player_input_RestoreMlStates`) — with `restore-playback-pos` = ASK and
  `media->f_progress > 0` sets `ml.delay_restore` and `ml.pos = media->f_progress` (`:69-73`); the ML media also has
  `i_duration` (`include/vlc_media_library.h:221`). `src/player/player.h:122-124` (`pos`, `delay_restore`).
  `src/player/player.c:95-99` sends `on_playback_restore_queried` **with no argument** right after
  `on_current_media_changed`, i.e. before the input knows its length — so Qt cannot compute "till 00:00:02" from
  `Player.length` at that moment.
- `include/vlc_player.h:3304-3318` — the callback contract; its only consumer in the tree is Qt
  (`grep on_playback_restore_queried` → `modules/gui/qt/player/player_controller.cpp:1150`).
- `src/libvlc-module.c:765-778` (texts and values) and `:1928-1930` — `restore-playback-pos`, default
  `VLC_PLAYER_RESTORE_PLAYBACK_POS_ASK`.
- `modules/gui/qt/util/vlctick.hpp:55` (`VLCTick::formatHMS`) — the formatter for the time.
- Test harness: `test/src/player/common.h:113-142` (the `REPORT_LIST` of recorded callbacks, no
  `on_playback_restore_queried` yet), `test/src/player/modules.c` (stub aout/vout modules; a stub `medialibrary`
  submodule goes there), registration pattern `test/src/meson.build:108-118` (`test_src_player_abloop`).

Settings / variables involved: `restore-playback-pos` (`src/libvlc-module.c:1928`), `restore-playback-states`
(`:1932`), `save-recentplay` (`:1926`). No new option.
Mock boundary: stub `medialibrary` module in the player test returning one video media
(`f_progress = 0.25`, `i_duration = 8 s`) for `vlc_ml_get_media_by_mrl` and `VLC_SUCCESS` for the playback-pref
query.

### Overlaps & contradictions
- **PKT-3003** moves the central controls; the pill is anchored to the control bar's centre group, so it follows.
- **PKT-3006** (thick seek bar, one thin control row) changes `positionSliderY`; the pill anchors on it, not on a
  constant.
- **PKT-3008** keeps the top bar in fullscreen; the `modules/gui/qt/player/qml/Player.qml:948-955` special case removed here must be
  re-checked against 3008's rule (whoever merges second rebases).
- **PKT-3033** (home page "Continue Watching" row in the library) uses the same ML progress; no file shared.

## 2b. Upstream state
Master ships the feature (ask/always/never, Continue/Dismiss, keyboard, auto-timeout). Missing: placement near the
central controls, the resume time in the text, and acknowledging on timeout. The video's "5 seconds" is in fact
10 s in master (`modules/gui/qt/player/qml/ResumeDialog.qml:64`); the timer is kept but paused while hovered (§5.3).

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | core `on_playback_restore_queried` | stub ML: video media, progress 0.25, duration 8 s; `--restore-playback-pos=1` (ASK) | open the media | callback fired once with `position == 0.25f` and `time == VLC_TICK_FROM_SEC(2)` | `test/src/player/restore.c::test_restore_query_carries_point` |
| 2 | core | same media, progress 0 | open | callback not fired | `restore.c::test_no_query_without_progress` |
| 3 | core | `--restore-playback-pos=2` (ALWAYS) | open | no query; position applied | `restore.c::test_always_restores_silently` |
| 4 | `ResumeDialog` timeout | prompt shown | 10 s pass | `Player.canRestorePlayback` becomes false (today stays true) | MANUAL — `QT_LOGGING_RULES` trace + note in MR (no QML harness for `VLC.Player`, §5.4) |
| 5 | placement | known video, controls not pinned / pinned / fullscreen | open it | pill centred above the control bar, never under the play queue; top bar shows the cone | MANUAL — screenshots `pkt3016-resume-{windowed,pinned,fullscreen}.png` |
| 6 | text | progress at 1 h 02 m 03 s | open | "You already watched till 01:02:03"; Enter continues, Esc dismisses | MANUAL — screenshot |

## 4. Fixtures & mocks
- Pattern to copy: `test/src/player/abloop.c` (ctx, `player_set_next_mock_media`, report waits) with its meson block
  `test/src/meson.build:108-118`; add `X(struct report_restore, on_playback_restore_queried)` to the list at
  `test/src/player/common.h:113-142`.
- Stubs: a `medialibrary` submodule in `test/src/player/modules.c` (pattern: the Qt stub
  `modules/gui/qt/tests/vlc_stub_modules.cpp:117-146`, `MLGet`/`MLControl`/`MLOpen`) returning the fixture media.
- Sample media: mock demux (`mock://video_track_count=1;length=8000000`) as the other player tests.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): change `on_playback_restore_queried` to pass `position` and `time` (core API change accepted). Core API change: `on_playback_restore_queried(vlc_player_t *, float position, vlc_tick_t time,
   void *data)`. Only Qt consumes it in-tree, but it is public API (libvlc 4 is unreleased, so the break is
   allowed now). Fallback without the change: Qt shows the time once `Player.length` is known
   (`position * length`), which needs the position anyway → at minimum `position` must be passed.
2. **RULED** RULED 2026-10-14 (R0): the resume prompt moves out of the top bar to sit next to the central controls. Moving the prompt out of the top bar reverses an upstream placement choice; ruling needed.
3. NOTE — timer: keep 10 s, stop it while the pill is hovered or focused, restart on leave; on expiry call
   `hideResumePanel()` so the controller is acknowledged (row 4).
4. NOTE — `qml_test` only initialises `util_assets` (`modules/gui/qt/tests/qml_test.cpp:34-46`), so `ResumeDialog`
   (which uses the `Player` singleton) cannot get a `TestCase` without a Player stub; that harness is not built here.
5. NOTE — wording "Continue Watching" also fits audio; for audio the prompt reads "You already listened till %1"
   only if VideoLAN wants it — default single string.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --suite src --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt`, `src/player`.
- Strings added are marked for translation (`qsTr`).
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on the pill.

### Evidence
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3016-resume-prompt-near-controls` from
      the merge of PKT-3003; §6 command → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm the suite name of `test/src/player` tests in `test/src/meson.build`
      and adapt the §6 `--suite`; grep the tree (incl. `modules/gui/macosx`, `lib/`) for other
      `on_playback_restore_queried` users.
- [ ] **Step 3: Row 1 watched failing** — stub ML + `test/src/player/restore.c` row 1 against the current signature
      (asserting `time`) → does not compile/FAIL; then change `include/vlc_player.h:3318`, store `time =
      media->i_duration * f_progress` next to `ml.pos` (`src/player/player.h:122`, `src/player/medialib.c:69-73`)
      and pass both in `src/player/player.c:97` → PASS. Rows 2–3. Commit 1
      `player: pass the resume point to on_playback_restore_queried`.
- [ ] **Step 4: Controller** — `on_player_playback_restore_queried` stores `m_restorePlaybackTime`; new
      `Q_PROPERTY(VLCTick restorePlaybackTime … NOTIFY playbackRestoreQueried)` in `player_controller.hpp`. Commit 2
      `qt: expose the resume time in the player controller`.
- [ ] **Step 5: QML** — `ResumeDialog.qml`: pill layout (label "You already watched till %1" with
      `Player.restorePlaybackTime.formatHMS()`, accent "Continue Watching", × button), hover-aware timer and
      acknowledge on timeout; remove it from `TopBar.qml` (`logoOrResume` back to `logoGroup` only) and the
      `resumeVisible` case in `modules/gui/qt/player/qml/Player.qml:948-955`; instantiate it in `Player.qml`, horizontally centred, bottom
      anchored at `positionSliderY` minus a margin. Rows 4–6 by hand. Commit 3
      `qml: show the resume prompt above the central controls`.
- [ ] **Step 6: Whole gate** — §6 command; summaries into §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`); §5.1–5.2 as recorded rulings; `status: review`.
