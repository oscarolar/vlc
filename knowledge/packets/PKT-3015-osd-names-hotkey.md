---
id: PKT-3015
title: "The on-screen message also names the key that triggered it"
section: "B. Feedback"
video_ref: "15:18–15:32"
pipeline: CI
kind: core
track: T2
component: modules/control (hotkeys)
branch: master-pkt-3015-osd-names-hotkey
base_sha: ce4af2f
target_files:
  - modules/control/hotkeys.c
  - modules/control/hotkeys_osd.h                                        # new, pure helpers
  - test/modules/control/hotkeys_osd.c                                   # new
  - test/modules/meson.build
estimate_h: 12.0
priority: P2
depends_on: []
upstream_ruling: needed
target: fork
mr_title: "hotkeys: show the pressed key under the action OSD"
status: review
actual_h: 0.6
---

# PKT-3015 — The on-screen message also names the key that triggered it

## 1. Scope & intent
**Before.** A hotkey changes something and the video shows one OSD line top-right for one second, e.g.
"Crop: 2.35:1". If the user hit the key by accident they do not know which key to press to undo it (15:18–15:28:
"what did I press?").

**After (sheet `s018`, frame 2).** The same message gets a second line naming the key: "Crop: 2.35:1" /
"Pressed C". It appears only when the action came from a key (or mouse-wheel binding), never for the same change
made from a menu, and it can be turned off. The redesign shows the two lines centred and large; position and
size stay VLC's (top-right, OSD text style) — restyling the OSD is not part of this packet.

One MR, two commits:
1. `hotkeys: track which key triggered an action` — `hotkeys_osd.h` + unit test.
2. `hotkeys: show the pressed key under the action OSD` — rendering + option.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- Where the key is lost: `src/misc/actions.c:423-439` (`vlc_key_to_action`, callback of the libvlc `key-pressed`
  variable registered at `:574`) looks the keycode up and does `var_SetInteger(obj, "key-action", ent->action)`:
  only the action id travels on. Sources of `key-pressed`: Qt `modules/gui/qt/maininterface/mainctx.cpp:543-553`
  (`MainCtx::sendHotkey`/`sendVLCHotkey`), vout windows `src/video_output/video_window.c:222`, mouse wheel
  `modules/control/hotkeys.c:1108-1113`.
- `modules/control/hotkeys.c:1218-1225` (`ActionCallback`) — receives only `newval.i_int` (action id) and calls
  `handle_action`; `:1021-1068` (`handle_action`) dispatches to intf/playlist/player/vout handlers through the table
  at `:961-1019`; `:1231-1262` (`Open`) registers `key-action` (`:1256`); `:1294-1304` module block, **no options**.
- Where the visible text comes from: most messages are **not** written by hotkeys.c. Changing a vout variable
  triggers `src/player/osd.c:283-350` (`vlc_player_vout_OSDCallback`), e.g. crop → "Crop: %s" (`:298-300`), aspect
  (`:290-292`); hotkeys.c writes only some itself (subtitle sync `:505-550`, "Audio device: %s" `:674`). All go
  through `vout_OSDMessageVa` (`src/video_output/video_text.c:134-144`): channel `VOUT_SPU_CHANNEL_OSD`,
  `SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_RIGHT`, 1 s. `include/vlc_vout.h:138`: the OSD channel is auto-cleared,
  so a second message on it **replaces** the first.
- `src/misc/variables.c:766-800` (`AddCallback`) — new callbacks are **prepended** (`entry->next = *pp`), so a
  `key-pressed` callback added by hotkeys.c at `Open` runs **before** core's `vlc_key_to_action`. That is the hook:
  record the key, then `ActionCallback` (nested inside the same `var_SetInteger`) consumes it.
- `modules/control/hotkeys.c:1157-1172` (`player_on_vout_started`) — hotkeys already tracks every vout (and adds
  per-vout callbacks), the place to register a private SPU channel with `vout_RegisterSubpictureChannel`
  (`include/vlc_vout.h:145`) for the key line.
- `src/misc/actions.c:246` (`vlc_keycode2str(code, locale)`) — the localized key name ("C", "Ctrl+Shift+S",
  "Mouse Wheel Up").
- Qt-only shortcuts (`modules/gui/qt/menus/qml/GlobalShortcuts.qml:40-52`: Ctrl+P, Ctrl+H, F11 …) never reach the
  core and never show an OSD — out of scope.

Settings / variables involved: `osd` (`src/libvlc-module.c:1808`, master switch, respected by `vout_OSDText`
`src/video_output/video_text.c:99`); new `hotkeys-osd-key` (bool) in the hotkeys module block
(`modules/control/hotkeys.c:1294-1304`).
Mock boundary: the key-attribution logic and the label formatting live in `hotkeys_osd.h` as pure functions; the
unit test links libvlccore only for `vlc_keycode2str`.

### Overlaps & contradictions
- **PKT-3035** (hotkeys dialog) and **PKT-3036** (preferences tree) decide where `hotkeys-osd-key` is shown in the
  preferences; here it is only an advanced option of the hotkeys module.
- **PKT-3026** (one-key bookmark) adds an action; it gets the key line for free.
- **PKT-3043** (onboarding) may mention the feature; no file shared.
- No upstream work detected on the OSD text.

## 2b. Upstream state
Nothing in master names the key. The OSD exists, is centralised in the player for variable changes, and the
keycode → name function exists (`vlc_keycode2str`). The packet is additive, behind an option.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `hotkeys_osd_KeyLabel` | keycode `'c'` | format | "Pressed C" (localized prefix, key via `vlc_keycode2str(…, true)`) | `test/modules/control/hotkeys_osd.c::test_label_letter` |
| 2 | `hotkeys_osd_KeyLabel` | `KEY_MODIFIER_CTRL|KEY_MODIFIER_SHIFT|'s'` | format | "Pressed Ctrl+Shift+S" (exact modifier spelling read in Step 2) | `::test_label_modifiers` |
| 3 | `hotkeys_osd_KeyLabel` | `KEY_MOUSEWHEELUP` | format | "Pressed Mouse Wheel Up" | `::test_label_wheel` |
| 4 | tracker | `KeyPressed('c')` then `TakeForAction()` | — | returns `'c'`; a second `TakeForAction()` returns 0 | `::test_tracker_consumes_once` |
| 5 | tracker | `TakeForAction()` with no key recorded (action set directly, e.g. Qt `modules/gui/qt/player/player_controller.cpp:1349`) | — | returns 0 → no key line | `::test_tracker_direct_action` |
| 6 | tracker | `KeyPressed(unmapped)`, then a direct action | — | 0: a key with no action does not leak into the next action (tracker cleared at the start of each `KeyPressed` and invalidated after the dispatch) | `::test_tracker_unmapped_key` |
| 7 | rendering | video playing, `hotkeys-osd-key` on | press C | "Crop: 2.35:1" stays; "Pressed C" shows beneath for the same 1 s | MANUAL — screenshot `pkt3015-osd.png` |
| 8 | option | `--no-hotkeys-osd-key` / `--no-osd` | press C | no key line / no OSD at all | MANUAL — note in MR |

## 4. Fixtures & mocks
- Pattern to copy: a plain C test under `test/modules/` with a `vlc_tests += {…}` block, e.g. `test/modules/meson.build`
  `test_modules_packetizer_helpers` (sources one `.c`, `link_with : [libvlc, libvlccore]`, suite `modules`). New
  directory `test/modules/control/`.
- Stubs: none; `hotkeys_osd.h` is header-only (`static inline`), included by both `hotkeys.c` and the test.
- Sample media: any video for rows 7–8.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): new option `hotkeys-osd-key`, default on. New user-visible option `hotkeys-osd-key`; default **on** (designer) or **off** (no behaviour
   change for existing users). Proposed: on, since the line only appears after a key press. 
2. **RULED** RULED 2026-10-14 (R0): (a) a private SPU channel per vout with a newline-prefixed second line (spike in Step 2); (b) only if the spike fails; (c) rejected. Rendering of the second line. (a) Private SPU channel per vout, `vout_OSDText(…,
   SUBPICTURE_ALIGN_TOP|SUBPICTURE_ALIGN_RIGHT, VLC_TICK_FROM_SEC(1), "\nPressed C")` — the leading newline puts it
   under the action line; pure module change, but relies on the text renderer keeping an empty first line (spike in
   Step 2). (b) Bottom-right placement — robust, but not under the message. (c) A core helper that composes both
   lines in `src/player/osd.c` — touches libvlccore API, rejected unless (a) fails.
   **Spike outcome (2026-10-15): (a) failed, (b) taken.** The text renderer drops a leading empty line (also a leading NBSP line), so `"\nPressed C"` on a private channel is drawn over "Crop: 16:10" (Xvfb capture). The label is shown bottom-right in the OSD text style. Also found in the spike: snapshots skip the OSD channels but not private channels, so the label channel is flushed before `ACTIONID_SNAPSHOT`.
3. NOTE — attribution relies on prepend order in `src/misc/variables.c:766-800`; if VideoLAN considers callback
   order an implementation detail, the alternative is a new `key-action-source` libvlc variable set by
   `vlc_key_to_action` (core change).
4. NOTE — the label is shown even for actions that print nothing themselves (e.g. play/pause icon OSD); acceptable
   per the video ("show the keyboard shortcut that was just pressed").

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite modules --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/control`.
- The new option text is `N_()`-marked; `po/POTFILES.in` already lists `modules/control/hotkeys.c` (check) and gets
  `hotkeys_osd.h` if it carries strings.
- No change to `include/` (public API) unless §5.2 (c) or §5.3 is taken up.

### Evidence
Executed 2026-10-15 on nzxt, worktree ~/src/vlc-wt/pkt-3015, CI image vlc-debian-unstable:20260611225331,
`-Dextra_checks=true`, ninja -j8. Logs in ~/src/logs/pkt-3015-*.log, spike captures in ~/src/logs/pkt-3015-spike.

- Branch `master-pkt-3015-osd-names-hotkey`, commits 6dd805c `hotkeys: track which key triggered an action`,
  2cc6253 `hotkeys: show the pressed key under the action OSD`.
- PR: https://github.com/oscarolar/vlc/pull/9
- Rows 1-6 (+ extra `test_keymap_lookup`): `test_modules_control_hotkeys_osd` has 7 cases. All 7 failed against stubs
  (row 1: `got "(null)", expected "Pressed C"`; rows 4-6: tracker assertions), log pkt-3015-rows-fail.log. After the
  implementation: OK, log pkt-3015-rows-pass.log.
- `meson test --suite modules`: 25 OK / 0 fail / 2 skip (pkt-3015-suite-modules.log).
- `meson test --suite qt`: 7/7 OK. `--suite src`: 43 OK / 1 fail, the known `test_src_player_lifecycle` SIGSEGV
  (pkt-3015-suite-qt-src.log).
- extra_checks: forced rebuild of `modules/control/hotkeys.c` and the new test, 0 warnings (pkt-3015-extra-checks.log).
- Step 2 prepend order: a throw-away `msg_Dbg` showed the hotkeys `key-pressed` callback logging 0x63 before
  `key-action 95 key 0x63` (pkt-3015-spike run logs).
- Rows 7-8 (headless, Xvfb + xcb_x11 + luaintf script driving `key-pressed`): C gives "Crop: 16:10" top-right and
  "Pressed C" bottom-right; wheel up gives "Pressed Mouse Wheel Up"; a direct `key-action` crop shows no label;
  `--no-hotkeys-osd-key` shows no label; `--no-osd` shows nothing; a snapshot taken right after a labelled key has no label.
- PENDING: desktop screenshot pkt3015-osd.png with the Qt interface.
- Deviations: §5.2 fell back to (b), bottom-right (see the spike outcome). Unbound keys are detected with a sorted
  keycode list built at Open from `vlc_actions_get_keycodes` plus the wheel modes, and the tracker is also scoped to
  the calling thread. Letters are upper-cased in the label (`vlc_keycode2str` returns "c"). Autotools files
  (`modules/control/Makefile.am`, `test/Makefile.am`) and `po/POTFILES.in` were updated too. The commit and PR titles
  keep the packet wording "under the action OSD"; the bodies explain the bottom-right placement.

## 7. Agent brief
- [x] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3015-osd-names-hotkey` from `ce4af2f`;
      §6 command → green.
- [x] **Step 2: Dry-run + spike** — re-read §2; confirm the prepend order (`src/misc/variables.c:766-800`) with a
      `msg_Dbg` in a throw-away build; spike §5.2 (a) by hard-coding `"\nPressed X"` on a private channel and
      looking at a frame (screenshot to the MR). Write the outcome into §5.2.
- [x] **Step 3: Row 1 watched failing** — `modules/control/hotkeys_osd.h` with
      `char *hotkeys_osd_KeyLabel(uint32_t keycode)` returning `NULL`; `test/modules/control/hotkeys_osd.c` row 1;
      `vlc_tests += {'name': 'test_modules_control_hotkeys_osd', …}` in `test/modules/meson.build` → FAIL.
- [x] **Step 4: Rows 2–6** — label formatting (`vlc_keycode2str`, `_("Pressed %s")`) and the tracker
      (`struct hotkeys_key_tracker { uint32_t key; bool valid; }` + `KeyPressed`/`TakeForAction`/`Invalidate`).
      Commit 1 `hotkeys: track which key triggered an action`.
- [x] **Step 5: Wire it** — in `Open` add a `key-pressed` callback on `vlc_object_instance(intf)` that records the
      key (mutex in `intf_sys_t`, callbacks can come from the vout window thread); in `ActionCallback` take the key
      before `handle_action` and, after it, render the label on each tracked vout's private channel (registered in
      `player_on_vout_started`, `:1157-1172`); `add_bool("hotkeys-osd-key", true, …)` in the module block. Remove
      the callback in `Close`. Rows 7–8 by hand. Commit 2 `hotkeys: show the pressed key under the action OSD`.
- [x] **Step 6: Whole gate** — §6 command (suite `modules`) plus `meson test -C build-meson --suite qt` to prove Qt
      hotkeys are unaffected; summaries into §6 Evidence.
- [x] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`), §5.1–5.3 as recorded rulings; `status: review`.
