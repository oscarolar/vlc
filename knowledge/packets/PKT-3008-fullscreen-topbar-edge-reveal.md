---
id: PKT-3008
title: "In fullscreen the title bar stays available, and both bars appear only near their edge"
section: "A. Chrome"
video_ref: "03:30–03:54"
pipeline: CI
kind: ui
track: T2
component: modules/gui/qt/player/qml
branch: master-pkt-3008-fullscreen-topbar-edge-reveal
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/PlayerToolbarVisibilityFSM.qml
  - modules/gui/qt/player/qml/Player.qml
  - modules/gui/qt/util/qml/Helpers.qml
  - modules/gui/qt/qt.cpp
  - modules/gui/qt/maininterface/mainctx.hpp
  - modules/gui/qt/maininterface/mainctx.cpp
  - modules/gui/qt/tests/tst_PlayerToolbarVisibilityFSM.qml
  - modules/gui/qt/meson.build
estimate_h: 15.0
priority: P1
depends_on: []
upstream_ruling: needed
target: fork
mr_title: "qml: reveal the player bars when the pointer nears their edge"
status: review
actual_h: 0.7
---

# PKT-3008 — In fullscreen the title bar stays available, and both bars appear only near their edge

## 1. Scope & intent
At 03:30–03:54 the designer restyles the top bar (menu, media title, window buttons) and objects that it
"disappears on fullscreen": to read the title you must leave fullscreen. Her rule: keep **both** the top and bottom UI
in fullscreen, show each **only when you hover near it**, and hide it **immediately when you move away**. The frames
at ~03:40–03:50 show a dark window with a thin top bar (logo, menus, centred title "video.mp4", window buttons) and the
one-row control bar, with arrows pointing at the top and bottom edges.

Master already keeps the top bar in fullscreen in the default (unpinned) mode — it shows the media title in large type
under the logo and fades in and out **together with the control bar** on any mouse move over the video, after
`mouse-hide-timeout` (1 s). Two things differ from the video: (1) with **pinned controls** the top bar is hidden in
fullscreen, title included; (2) the reveal is **anywhere / timeout**, not **edge / leave**.

After this packet: the pinned-controls exception is removed (the top bar is available in fullscreen in both modes), and a
new reveal mode **"near edges"** shows the top bar when the pointer enters a band at the top, the control bar when it
enters a band at the bottom, and hides each as soon as the pointer leaves its band and the bar (locks for open menus and
hovered controls still apply). The default stays today's "anywhere" mode (ruled, §5.1).

One MR, three commits:
1. `qml: inject the player toolbar FSM dependencies` — the FSM reads properties instead of `MainCtx`/`Player`, so
   `qml_test` can load it; behaviour unchanged, pinned by tests.
2. `qml: add an edge reveal mode to the player toolbars` — new FSM signals + `Helpers.edgeZone`, option, `Player.qml` wiring.
3. `qml: keep the player top bar in fullscreen with pinned controls`.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/PlayerToolbarVisibilityFSM.qml:45-209` (`FSM`) — states `Visible{NormalVisible{AudioMode,
  VideoMode}, Locked}` and `Hidden`; inputs `askShow, mouseMove, keyboardMove, lock, unlock, timeout, updateVideoEmbed`
  (`:49-56`, map `:67-75` — note `forceUnlock` is an *output*, not in the map); `timeoutDuration: MainCtx.mouseHideTimeout`
  (`:62`); initial state reads `Player.isInteractive && MainCtx.hasEmbededVideo` (`:64-65`); `VideoMode` restarts the
  timer on `mouseMove` (`:130-136`), `Hidden` goes visible on `mouseMove` (`:177-183`), lock counting (`:148-165`). It imports
  `VLC.MainInterface` (`:20`) for those singletons — the reason it cannot be loaded by `qml_test`.
- `modules/gui/qt/player/qml/Player.qml:135-143` — the FSM instance; `:105-111` `lockUnlockAutoHide`; `:278-291`
  `videoSurface.onMouseEvent` sends `mouseMove()` for every move/press/release/double-click (no position used).
- `modules/gui/qt/maininterface/videosurface.cpp:185-197` (`VideoSurface::hoverMoveEvent`) — emits
  `mouseMoved(x * dpr, y * dpr)`; signal declared at `modules/gui/qt/maininterface/videosurface.hpp:127`: the position is available.
- `modules/gui/qt/player/qml/Player.qml:932-1000` (`TopBar` instance) — `visible` is false only when
  `pinVideoControls && fullscreen && !resumeVisible` (`:948-954`); `showCSD` / `showToolbar` are off in fullscreen
  (`:961-962`); state follows the FSM (`:986-989`).
- `modules/gui/qt/player/qml/TopBar.qml:377-399` (`leftTitleText`) — the title, visible when `!pinControls`; `:337-346`
  `centerTitleText` for pinned mode.
- `modules/gui/qt/player/qml/Player.qml:1324-1327`, `:1334` — the control bar shares the FSM state and locks it while hovered.
- `modules/gui/qt/maininterface/mainctx.cpp:458` — `m_mouseHideTimeout` from `mouse-hide-timeout`.
- `modules/gui/qt/qt.cpp:459-460` — `qt-fs-sensitivity` ("Fullscreen controller mouse sensitivity", pixels, default 3):
  declared, **read nowhere** in `modules/gui/qt` (grep at `base_sha`); `modules/gui/qt/qt.cpp:388` `qt-fs-controller` is read only by the
  simple preferences page (`modules/gui/qt/dialogs/preferences/simple_preferences.cpp:808`), not by the QML player.

Settings / variables involved: `mouse-hide-timeout` (`src/libvlc-module.c:1720`, default 1000 ms); `qt-pin-controls`
(`modules/gui/qt/qt.cpp:430`); new `qt-controls-reveal` (integer list `anywhere` / `edges`) next to `qt-fs-sensitivity`;
`qt-fs-sensitivity` (`modules/gui/qt/qt.cpp:459`) — candidate for the band size (§5.2).
Mock boundary: the FSM's injected properties (`isInteractive`, `hasEmbededVideo`, `hideTimeout`, `revealMode`) in the
`TestCase`; nothing else.

### Overlaps & contradictions
- **PKT-3009** adds buttons to the top bar's right group; it depends on this packet only for the "top bar exists in
  fullscreen with pinned controls" change.
- **PKT-3014** ("Hide interface" entry) adds a state where neither bar shows; it will add a transition to this FSM —
  coordinate on the refactored property names.
- **PKT-3006** makes the control bar one row; the bottom band height must follow the bar height (row 4 uses the bar's
  actual height, not a constant).
- `Player.qml` is also touched by PKT-3006/3016; changes here are confined to `:135-143`, `:278-291`, `:948-954`.

## 2b. Upstream state
- Top bar with title in fullscreen (unpinned mode): **shipped**.
- Auto-hide after `mouse-hide-timeout`, lock while a menu is open or a control hovered: **shipped**.
- Top bar in fullscreen with pinned controls: **not shipped** (explicitly hidden, `modules/gui/qt/player/qml/Player.qml:948-954`).
- Edge-proximity reveal with immediate hide: **not shipped**.
The video's main complaint (no title in fullscreen) is a VLC 3 behaviour; master already fixes it in its default mode.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. QML `TestCase`
`tests/tst_PlayerToolbarVisibilityFSM.qml` registered in `qml_tests` (`modules/gui/qt/meson.build:954-956`); it creates the FSM with
`Qt.createComponent("../player/qml/PlayerToolbarVisibilityFSM.qml")` after commit 1 removes its `VLC.MainInterface` import,
and drives time with `wait()` on a 50 ms `hideTimeout`. `Helpers.edgeZone` rows load `VLC.Util` directly.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | FSM, anywhere mode (regression) | video, not interactive, hidden | `mouseMove()` | visible; hidden again after `hideTimeout` | `::test_anywhere_moveShowsThenTimesOut` |
| 2 | FSM locks (regression) | visible | `lock(); lock(); unlock()` then wait > timeout | still visible; after the second `unlock()` and timeout, hidden | `::test_lockCounting` |
| 3 | FSM interactive media (regression) | `isInteractive: true` | `mouseMove()` | stays hidden (guards at `:131`, `:178`) | `::test_interactiveIgnoresMouse` |
| 4 | `Helpers.edgeZone(y, height, band)` | `height 1000, band 80` | `y = 10`, `500`, `990`, `80`, `920` | `Top`, `None`, `Bottom`, `Top` (inclusive), `Bottom` | `::test_edgeZone` |
| 5 | FSM, edges mode | hidden | `mouseMove()` in the middle (`pointerZone(None)`) | stays hidden | `::test_edges_middleDoesNotReveal` |
| 6 | FSM, edges mode | hidden | `pointerZone(Bottom)` | `isVisible`, and `revealedEdge === Bottom` | `::test_edges_bottomReveals` |
| 7 | FSM, edges mode | visible from bottom, not locked | `pointerZone(None)` | hidden at once (no wait) | `::test_edges_leaveHidesImmediately` |
| 8 | FSM, edges mode | visible from bottom, `lock()` (control bar hovered) | `pointerZone(None)` | stays visible until `unlock()` | `::test_edges_lockKeepsVisible` |
| 9 | Pinned + fullscreen | `--qt-pin-controls`, fullscreen | move to the top edge | top bar shows the title (center title variant) | MANUAL: screenshot |
| 10 | Edges mode, fullscreen | `--qt-controls-reveal=edges` | pointer to top, middle, bottom | top band → top bar only; middle → nothing; bottom band → control bar only (expected text final after §5.3) | MANUAL: three screenshots |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/tst_FSM.qml:18-60` (TestCase holding an `FSM`, signal-driven assertions) and the
  `qml_tests` entry at `modules/gui/qt/meson.build:955`.
- Stubs: none; injected properties replace `MainCtx`/`Player` (commit 1), bound back in `modules/gui/qt/player/qml/Player.qml:135-143`:
  `isInteractive: Player.isInteractive`, `hasEmbededVideo: MainCtx.hasEmbededVideo`, `hideTimeout: MainCtx.mouseHideTimeout`.
- Sample media: any local video for rows 9–10.

## 5. Design decisions & open questions
1. **RULED — default reveal mode.** RULED 2026-10-14 (R0): ship `edges` as an option and keep `anywhere` as the default. The video asks for edges; master's "anywhere" is VideoLAN's current choice and is
   friendlier to touchpads. Proposed: ship `edges` as an option, default `anywhere`.
2. **RULED — band size option.** RULED 2026-10-14 (R0): new option `qt-controls-reveal-band` (dp, default 80); `qt-fs-sensitivity` and `qt-fs-controller` are made obsolete in a separate cleanup MR. Reuse the dead `qt-fs-sensitivity` (pixels; its name says fullscreen, its default 3 px
   is too small for a band) or add `qt-controls-reveal-band` (dp, default 80). Proposed: new option; mark
   `qt-fs-sensitivity` and `qt-fs-controller` obsolete in a separate cleanup MR (`add_obsolete_*` keeps old configs loading).
3. **RULED — independent bars.** RULED 2026-10-14 (R0): one FSM with `revealedEdge` (Top / Bottom / Both via keyboard); `TopBar.state` and `ControlBar.state` bind to it separately. The video hides each bar separately ("show them when you hover near them"). Today one FSM
   drives both. Proposed: one FSM with `revealedEdge` (Top / Bottom / Both via keyboard); `TopBar.state` and
   `ControlBar.state` bind to it separately. Row 10's expected text follows this ruling.
4. NOTE — commit 1 is a pure refactor; rows 1–3 are written first against master behaviour and must stay green across it.
5. NOTE — keyboard (`keyboardMove`) keeps showing both bars with `2 × timeout`, in both modes.

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
Executed 2026-10-15 on nzxt, worktree ~/src/vlc-wt/pkt-3008, branch `master-pkt-3008-fullscreen-topbar-edge-reveal` from ce4af2f,
CI image registry.videolan.org vlc-debian-unstable:20260611225331, `meson setup build-meson -Dextra_checks=true && ninja -j8 -C build-meson && meson test -C build-meson --suite qt`.

- PR: https://github.com/oscarolar/vlc/pull/10 (fork, base master).
- Commits:
  - e6113b5 `qml: inject the player toolbar FSM dependencies`
  - 133abec `qml: add an edge reveal mode to the player toolbars`
  - 3365c11 `qml: keep the player top bar in fullscreen with pinned controls`
- Baseline at ce4af2f: `--suite qt` 7/7 OK (log pkt-3008-baseline.log).
- Step 2 dry-run: every §2 range matched at HEAD. `grep -rn fs-sensitivity modules/gui/qt` hits only the declaration in `modules/gui/qt/qt.cpp`, so it has no reader.
- Rows 1–3, red: `tst_PlayerToolbarVisibilityFSM` added before the refactor. 3 failed with `ReferenceError: MainCtx is not defined` (log pkt-3008-red1.log).
- Rows 1–3, green after commit 1: suite 8/8 OK (log pkt-3008-green1.log).
- Rows 4–8, red: the new tests failed 7/12 with `edgeZone is not a function` and `non-existent property "revealMode"` (log pkt-3008-red2.log).
- Rows 4–8, green after commit 2: 12/12 test functions passed (log pkt-3008-green2.log).
- Test functions in `tst_PlayerToolbarVisibilityFSM` (10):
  - Rows 1–8: `test_anywhere_moveShowsThenTimesOut`, `test_lockCounting`, `test_interactiveIgnoresMouse`, `test_edgeZone`, `test_edges_middleDoesNotReveal`, `test_edges_bottomReveals`, `test_edges_leaveHidesImmediately`, `test_edges_lockKeepsVisible`.
  - Extra: `test_edges_keyboardShowsBoth` (§5.5) and `test_anywhere_ignoresPointerZone`.
- Final gate after commit 3 (`-Dextra_checks=true`, reconfigured): `--suite qt` Ok 8, Fail 0 (log pkt-3008-gate.log).
- Compiler warnings: none in `modules/gui/qt/qt.cpp`, `modules/gui/qt/maininterface/mainctx.cpp` or `modules/gui/qt/maininterface/mainctx.hpp`. The only lines are the image-wide `-Werror=unterminated-string-initialization` notice.
- qmllint (Qt 6), before and after, on the FSM, `modules/gui/qt/player/qml/Player.qml` and `modules/gui/qt/util/qml/Helpers.qml`:
  - Warning counts: FSM 38 → 37, Player.qml 546 → 566, Helpers.qml 5 → 5.
  - Every new warning is unresolved-import noise (`MainCtx`, `VLCStyle`, `Helpers` unqualified, `mapToItem` on unknown `VideoSurface`) or an id accessed from inside `videoComponent`, the same pattern as the existing `playerToolbarVisibilityFSM` accesses.
  - Logs: pkt-3008-qmllint-before.log and pkt-3008-qmllint-after.log.
- Rulings applied:
  - §5.1 (R0): default `anywhere`.
  - §5.2 (R0): new `qt-controls-reveal-band`, 80 dp. The obsolete cleanup of `qt-fs-sensitivity` and `qt-fs-controller` is left to a separate MR.
  - §5.3 (R0): one FSM with `revealedEdge`, exposed as `isTopVisible` and `isBottomVisible` for the two bars.
- Deviations:
  1. Test file: the caller suggested extending `tst_FSM`. That file tests the generic `FSM`, which this packet does not change, so the player FSM rows live in the new `modules/gui/qt/tests/tst_PlayerToolbarVisibilityFSM.qml`, as §3 specifies.
  2. `edgeZone` takes separate top and bottom depths: `edgeZone(y, height, topBand, bottomBand)`. The bottom zone uses `max(band, controlBar.height)`, as §2 asks for PKT-3006; the top zone does the same with the top bar.
  3. Option type: `qt-controls-reveal` is an integer list (0 anywhere, 1 edges), as specified. Row 10 therefore uses `--qt-controls-reveal=1`, not `=edges`.
  4. Commit 3 also touches `modules/gui/qt/player/qml/TopBar.qml`, which is not in target_files. It adds a `fullScreen` input so `_showCenterText` also holds in fullscreen. Without it, the pinned top bar in fullscreen has no title, because `showCSD` is false there and `leftTitleText` is unpinned-only. This is required for row 9.
  5. In edges mode a pointer-revealed bar has no timeout: it stays while the pointer is in its zone, per the "only when you hover near it" rule. Keyboard reveal keeps the 2 × timeout.
  6. Mouse press, release and double-click no longer reveal the bars in edges mode, because `mouseMove` is ignored there. Only pointer moves are used.
- MANUAL rows 9 and 10: pending, they need a desktop for screenshots.
- Known test outside this gate: `test_src_player_lifecycle` in `--suite src` still segfaults on master in this image. Not run here.

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`: `git switch -c master-pkt-3008-fullscreen-topbar-edge-reveal ce4af2f`
      (worktree if busy); §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm `qt-fs-sensitivity` has no reader (`grep -rn fs-sensitivity modules/gui/qt`).
- [ ] **Step 3: Commit 1 (refactor under tests)** — add `tst_PlayerToolbarVisibilityFSM.qml` + `qml_tests` entry; it fails to
      load (import of `VLC.MainInterface`) → FAIL recorded; replace the singleton reads (`:62`, `:64-65`, `:85`, `:99-133`,
      `:178-195`) by properties, bind them in `modules/gui/qt/player/qml/Player.qml:135-143`; rows 1–3 green.
      Commit `qml: inject the player toolbar FSM dependencies`.
- [ ] **Step 4: Row 4** — `Helpers.edgeZone` (`util/qml/Helpers.qml`, beside `pointInRadius` `:56`); red/green.
- [ ] **Step 5: Rows 5–8** — `pointerZone(zone)` signal, `revealMode` property, `revealedEdge`; option in `qt.cpp` + `MainCtx`
      property (pattern `modules/gui/qt/maininterface/mainctx.cpp:458`); `modules/gui/qt/player/qml/Player.qml:278-291` passes `edgeZone(y / dpr, height, band)` from
      `mouseMoved(x, y)`. Commit `qml: add an edge reveal mode to the player toolbars`.
- [ ] **Step 6: Commit 3** — `modules/gui/qt/player/qml/Player.qml:948-954` drops the pinned/fullscreen exception; row 9.
      Commit `qml: keep the player top bar in fullscreen with pinned controls`.
- [ ] **Step 7: Whole gate** — §6; paste the summary in §6 Evidence; row 10 screenshots.
- [ ] **Step 8: MR** — PR to the fork github.com/oscarolar/vlc (base `master`); §5.1–§5.3 as recorded rulings; `status: review`.
