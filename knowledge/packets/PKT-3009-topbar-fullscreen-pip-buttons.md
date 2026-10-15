---
id: PKT-3009
title: "Fullscreen and picture-in-picture buttons in the player top bar"
section: "A. Chrome"
video_ref: "03:54–04:04"
pipeline: MANUAL
kind: ui
track: T2
component: modules/gui/qt/player/qml
branch: master-pkt-3009-topbar-fullscreen-pip-buttons
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/TopBar.qml
  - modules/gui/qt/player/qml/Player.qml
estimate_h: 6.0
priority: P2
depends_on: [PKT-3008]
upstream_ruling: needed
target: fork
mr_title: "qml: add fullscreen and mini player buttons to the player top bar"
status: ready
---

# PKT-3009 — Fullscreen and picture-in-picture buttons in the player top bar

## 1. Scope & intent
At 03:54–04:04 the designer likes "putting the fullscreen icon and maybe even a picture-in-picture icon up here", in
the top bar, and asks viewers whether they prefer it up or down. The frames at ~03:57–04:01 show the top-right cluster
growing from minimise / maximise / close to **minimise, PiP (a square with a filled corner), maximise, fullscreen,
close**; the bottom bar at ~04:04 still carries its own fullscreen button.

Master's player top bar has, on the right, a "more" menu button and the Play Queue toggle, then the client-side window
buttons; on the left a Back button. There is no fullscreen button in the top bar. **Picture-in-picture exists, but
not under that name**: when the platform supports it (`qt-pip-mode`, compositor capability), leaving the player view —
the top bar's Back button — keeps the video playing in a draggable 320×180 mini player over the library. The video's
PiP icon therefore maps to an existing action that is hidden behind an unrelated label.

After this packet the top bar's right group shows a **mini player (PiP)** button, only when `MainCtx.canShowVideoPIP`
and a video is embedded, that leaves the player view exactly like Back does, and a **fullscreen** toggle bound to
`Player.fullscreen`, both before the window buttons. The control-bar fullscreen control stays (it is a profile entry).
A floating, always-on-top OS window is out of scope (§5.2).

One MR, two commits:
1. `qml: add a fullscreen toggle to the player top bar`
2. `qml: add a mini player button to the player top bar`

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/TopBar.qml:423-509` (`playlistGroup` `Row`) — right-aligned group: `menuSelector`
  ("Menu", `VLCIcons.more`, `QmlGlobalMenu`, `:437-480`) and `PlaylistToggleButton` ("Play Queue", `:482-508`); each button
  locks auto-hide while hovered (`:462`, `:507`), and is chained with `Navigation.leftItem/rightItem` (`:456-458`, `:500-501`).
- `modules/gui/qt/player/qml/TopBar.qml:401-421` (`csdDecorations`) — window buttons loaded only when `showCSD`
  (`CSDWindowButtonSet.qml` / `CSDThemeButtonSet.qml`), anchored top-right; `playlistGroup.extraRightMargin` (`:426`) leaves
  room for them.
- `modules/gui/qt/player/qml/TopBar.qml:266-293` (`logoGroup`, `backBtn`) — "Back" emits `backRequested()` (`:292`).
- `modules/gui/qt/player/qml/Player.qml:997-1002` (`onBackRequested`) — stops playback when PiP is unavailable, then
  `MainCtx.playerView = false`.
- `modules/gui/qt/maininterface/qml/MainDisplay.qml:79-85` — on leaving the player, `pipPlayerComponent` is created when
  `MainCtx.canShowVideoPIP`, otherwise an embedded video is stopped; `:979-1010` the `PIPPlayer` (320×180, draggable,
  bottom-left, `z: 4`).
- `modules/gui/qt/player/qml/PIPPlayer.qml:28-80` — the mini player: tap toggles play/pause, double-tap returns to the
  player view (`MainCtx.playerView = true`).
- `modules/gui/qt/maininterface/compositor.cpp:300-303` — `setCanShowVideoPIP(true)` only with a depth buffer (or
  software), the compositor flag `CAN_SHOW_PIP`, and `qt-pip-mode`; `modules/gui/qt/maininterface/mainctx.hpp:121` exposes it as a constant.
- `modules/gui/qt/player/qml/controlbarcontrols/FullscreenButton.qml:26-35` — the control-bar toggle: enabled with
  `Player.hasVideoOutput`, glyph `fullscreen` / `defullscreen`, `Player.fullscreen = !Player.fullscreen`.
- `modules/gui/qt/player/qml/Player.qml:961` — `showCSD` is false in fullscreen: in fullscreen the new buttons must sit in
  `playlistGroup`, not in the CSD set, or they vanish with it.

Settings / variables involved: `qt-pip-mode` (`modules/gui/qt/qt.cpp:476`, default true); `fullscreen`
(`src/libvlc-module.c:1677`) is the initial state only; `key-toggle-fullscreen` (`src/libvlc-module.c:2588`) for the tooltip.
Mock boundary: none — no automatable behaviour (both buttons call existing setters).

### Overlaps & contradictions
- **PKT-3008** keeps the top bar available in fullscreen with pinned controls; without it the fullscreen toggle would be
  unreachable in that mode. Hard dependency.
- **PKT-3004** changes the queue toggle glyph in the same `Row` (`modules/gui/qt/player/qml/TopBar.qml:488`); **PKT-3013** replaces the menu button
  (`:437-480`). Three packets edit `playlistGroup`; merge order 3004 → 3009 → 3013, each rebases.
- **PKT-3003/3006** keep `FULLSCREEN_BUTTON` in the Compact profile; the video shows fullscreen both up and down
  ("tell me which one you prefer"). Not a conflict: the bottom one is a user-removable profile entry.

## 2b. Upstream state
- In-window picture-in-picture: **shipped** (`PIPPlayer.qml`, `qt-pip-mode`), reached through Back / leaving the player.
- Fullscreen toggle: **shipped** in the control bar, menus and hotkeys; **not** in the top bar.
- OS-level floating PiP window: **not shipped**.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. `TopBar` imports `VLC.Player` and
`VLC.MainInterface`, which `qml_test` cannot load (`modules/gui/qt/tests/qml_test.cpp:38-41`), and the buttons only call existing
setters, so the rows are MANUAL with screenshots.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | Fullscreen toggle | windowed, video playing | the top-bar fullscreen button is clicked | window goes fullscreen, glyph turns to `defullscreen`, tooltip names the hotkey | MANUAL: two screenshots |
| 2 | Fullscreen toggle | fullscreen | button clicked (top bar revealed per PKT-3008) | back to windowed | MANUAL |
| 3 | Fullscreen toggle | audio-only media | top bar shown | button disabled (as `modules/gui/qt/player/qml/controlbarcontrols/FullscreenButton.qml:29`) | MANUAL |
| 4 | Mini player | video, `canShowVideoPIP` true | PiP button clicked | library view with the 320×180 mini player, playback continues | MANUAL: screenshot |
| 5 | Mini player | `--no-qt-pip-mode` | top bar shown | no PiP button (Back keeps its current meaning) | MANUAL |
| 6 | Keyboard / a11y | top bar focused | Tab / arrows through menu → queue → PiP → fullscreen | each focusable, `Accessible.name` "Mini player" / "Fullscreen", auto-hide locked while hovered | MANUAL |
| 7 | Layout | CSD on, window 640 px wide | top bar shown | buttons do not overlap the CSD buttons or the title | MANUAL: screenshot |

## 4. Fixtures & mocks
- Pattern to copy: the `menuSelector` / `playlistButton` blocks in `modules/gui/qt/player/qml/TopBar.qml:437-508` (size `bannerButton_*`, hover lock,
  navigation chain); the fullscreen logic of `modules/gui/qt/player/qml/controlbarcontrols/FullscreenButton.qml:29-31`.
- Stubs: none. Sample media: one local video, one audio file.

## 5. Design decisions & open questions
1. **RULED — placement.** RULED 2026-10-14 (R0): the two buttons go at the end of `playlistGroup`, next to the CSD button set, so they exist in fullscreen and without CSD. The video puts PiP and fullscreen *inside* the window-button cluster. Native/themed CSD
   buttons (`CSDThemeButtonSet.qml`) come from the platform theme and vanish in fullscreen; proposed: the two buttons go at
   the end of `playlistGroup`, visually adjacent to the CSD set, so they also exist in fullscreen and without CSD.
2. **RULED — what "PiP" means.** RULED 2026-10-14 (R0): PiP means the existing in-window mini player (label "Mini player", tooltip "Picture-in-picture"); an OS-level PiP is a separate packet. Proposed: the existing in-window mini player (label "Mini player", tooltip
   "Picture-in-picture"). A real OS-level PiP (separate always-on-top window holding the vout) needs a new window per
   compositor (`maininterface/compositor*.cpp`) and is a separate T3 packet if VideoLAN wants it.
3. NOTE — Back keeps its behaviour; the PiP button is an explicit alias only when PiP can be shown, so users discover the
   feature without learning that "Back" means "keep playing small".
4. NOTE — new strings "Mini player", "Picture-in-picture", "Fullscreen" (exist elsewhere; `TopBar.qml` already in
   `po/POTFILES.in`).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`, on top of PKT-3008 (merged or its branch):
      `git switch -c master-pkt-3009-topbar-fullscreen-pip-buttons <base>`; §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; check `grep -n "TopBar.qml" po/POTFILES.in`; rebase on 3004 if merged.
- [ ] **Step 3: Commit 1** — in `TopBar.qml` `playlistGroup` after `playlistButton` (`:508`) an `IconToolButton`
      `fullscreenButton` (logic of `modules/gui/qt/player/qml/controlbarcontrols/FullscreenButton.qml:29-31`, hover lock, navigation chain, `AccessibleCompat.id:
      "topBarFullscreenButton"`); rows 1–3, 6. Commit `qml: add a fullscreen toggle to the player top bar`.
- [ ] **Step 4: Commit 2** — `pipButton` visible when `MainCtx.canShowVideoPIP && MainCtx.hasEmbededVideo`, emitting
      `backRequested()` (handled at `modules/gui/qt/player/qml/Player.qml:997-1002`); rows 4–5, 7. Commit
      `qml: add a mini player button to the player top bar`.
- [ ] **Step 5: Whole gate** — §6; paste the summary in §6 Evidence.
- [ ] **Step 6: MR** — PR to the fork github.com/oscarolar/vlc (base `master`) with screenshots, §5.1/§5.2 as recorded rulings; `status: review`.
