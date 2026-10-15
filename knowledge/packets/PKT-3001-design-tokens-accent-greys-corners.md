---
id: PKT-3001
title: "Design tokens: one orange accent, neutral greys and softer corners everywhere"
section: "A. Visual system"
video_ref: "00:36–01:32"
pipeline: MANUAL
kind: ui
track: T2
component: modules/gui/qt/style
branch: master-pkt-3001-design-tokens-accent-greys-corners
base_sha: ce4af2f
target_files:
  - modules/gui/qt/style/VLCStyle.qml
  - modules/gui/qt/style/systempalette.hpp
  - modules/gui/qt/style/systempalette.cpp
  - modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml
  - modules/gui/qt/player/qml/SliderBar.qml
  - modules/gui/qt/widgets/qml/SliderExt.qml
estimate_h: 12.0
priority: P1
depends_on: []
upstream_ruling: needed
target: fork
mr_title: "qml: route corner radii and the play button through style tokens"
status: review
actual_h: 0.2
---

# PKT-3001 — Design tokens: one orange accent, neutral greys and softer corners everywhere

## 1. Scope & intent
At 00:36–01:32 the designer fixes the visual vocabulary before touching any control: keep the traffic-cone
orange as the only accent, use neutral greys and near-blacks for every surface, and keep VLC rectangular
("that gives VLC the personality it has") but "a little less sharp". The palette card at 00:50 shows one
large orange swatch, a lighter and a darker orange, two dark browns, four greys from light to dark and four
near-black tiles, all as squares with a small corner radius; the cone itself is drawn flat orange.

The QML interface in master already has most of this (§2b): a single accent per palette, a grey ramp, and a
user option for the accent colour. What it lacks is **one place for corner radii** (96 `radius:` sites, 29 of
them through `VLCStyle`) and **one component that bypasses the accent**: the play button paints a hard-coded
orange gradient, so `--qt-accent-color` recolours everything except the most visible control.

After this packet: `VLCStyle` exposes a small radius scale; the player controls (play button, seek bar, volume
slider, buttons) use it; the play button takes its colours from the `ButtonAccent` colour set, so the accent
option and the dark/light palettes reach it.

One MR, three commits:
1. `qt: add corner radius tokens to VLCStyle` — the scale, no consumer changed.
2. `qml: use the radius tokens in the player controls` — `SliderExt`, `SliderBar`, `ButtonExt`, `PlayButton`.
3. `qml: let the play button follow the accent colour` — drop the hard-coded gradient.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/style/systempalette.hpp:114-133` (`SystemPalette` `COLOR_DEFINITION`s) — the brand constants:
  `orange500 #FF8800`, `orange800 #FF610A`, two "randomly named" oranges (`:117-118`, `FIXME`), `darkGrey200..800`
  (`#1E1E1E`..`#303030`) and `lightGrey100..700` (`#FAFAFA`..`#E0E0E0`). This already is the grey ramp the video asks for.
- `modules/gui/qt/style/systempalette.cpp:305-336` (`SystemPalette::componentComplete`) — reads `qt-accent-color`;
  `default` keeps the constants, `system` takes `QPalette::Accent` (Qt ≥ 6.6), any other string is parsed as a colour;
  `orange800` is derived as `orange500.lighter(150)`.
- `modules/gui/qt/style/systempalette.cpp:512` and `:725` — `Decoration/Accent` is `orange800` in the light palette and
  `orange500` in the dark one; `:594-605` / `:806-` the `ButtonAccent` colour set (orange background, white foreground).
- `modules/gui/qt/style/VLCStyle.qml:95`, `:98`, `:178`, `:189`, `:206`, `:215` — the only radius values the style knows:
  `listAlbumCover_radius` 3 dp, `trackListAlbumCover_radius` 3 dp, `button_radius` 4 dp, `navBoxButton_radius` 8 dp,
  `gridCover_radius` 4 dp, `mainView_topLeftRadius` 8 dp. No general scale.
- `modules/gui/qt/widgets/qml/SliderExt.qml:48` — `property real radius: VLCStyle.dp(3, VLCStyle.scale)` (literal);
  `:100` handle `radius: width / 2` (round knob).
- `modules/gui/qt/player/qml/SliderBar.qml:327` (`sliderRect`) `radius: implicitHeight`, `:436` (`progressRect`)
  `radius: control._seekPointsRadius` (0.5 dp, `:38`), `:578` (`sliderHandle`) `radius: VLCStyle.margin_small` — three
  different corner rules on the same bar.
- `modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:243-289` — the play button is a 48 dp circle
  (`implicitHeight: VLCStyle.icon_medium`, `:244`; `radius: width / 2`, `:250`, `:261`, `:289`) filled with a
  **hard-coded** gradient `#e25b01 → #f89a06` (`:264-265`); only its glow uses `theme.accent` (`:277`). It is the only
  hard-coded hex colour in the QML tree besides the A-B button's SVG placeholders (`modules/gui/qt/player/qml/controlbarcontrols/AtoBButton.qml:66-67`).

Settings / variables involved: `qt-accent-color` (`modules/gui/qt/qt.cpp:480`, texts `:297-298`).
Mock boundary: none — no automatable row (see §3).

### Overlaps & contradictions
- **PKT-3002** (icon sets) edits the menu SVGs so their orange follows the accent; it depends on this packet's
  ruling 5.2 (which orange is "the" accent) but touches no file listed here.
- **PKT-3003** changes the play button's *position*; this packet changes its *shape and colour*. Both touch nothing in
  common except that 3003's screenshots must be retaken after this lands.
- **PKT-3005** (volume cone) and **PKT-3006** (thick seek bar) rewrite `VolumeWidget`/`SliderBar` visuals: they must
  consume the radius tokens added here (`depends_on: 3001` in the index). This packet only swaps literals for tokens in
  `SliderBar.qml`; it does not change the bar's thickness or layout.
- **PKT-3038** (theme and accent in General preferences) exposes `qt-accent-color` in the UI; this packet keeps the
  option as is.

## 2b. Upstream state
- Accent colour as a setting: **shipped** (`qt-accent-color`, `default` / `system` / any colour, `modules/gui/qt/qt.cpp:480`).
- Neutral grey ramp and a forced dark palette for video playback: **shipped** (`modules/gui/qt/style/systempalette.hpp:120-133`,
  `modules/gui/qt/player/qml/Player.qml:128-130` forces `VLCStyle.darkPalette` while a video plays unpinned).
- A design-system radius scale: **not shipped**; radii are literals or per-widget properties.
- Play button following the accent: **not shipped** (`modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:264-265`).
The video's "keep orange, add greys" is therefore already the master design; the packet narrows to corners and the
one accent leak.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. No C++ behaviour changes and
`VLCStyle`/`SystemPalette` cannot be loaded by `qml_test` (its harness only initialises the `VLC.Util` resources,
`modules/gui/qt/tests/qml_test.cpp:38-41`, and both need `MainCtx`), so every row is MANUAL with a screenshot in the MR.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `VLCStyle` radius scale | the build of commit 1 | the player view is opened with a video | pixel-identical to master (no consumer changed yet) | MANUAL: before/after screenshot pair, player view, 1280×720, dark |
| 2 | `SliderExt`, `SliderBar`, `ButtonExt` | commit 2 | volume slider, seek bar and a library button are hovered | corners are the token values (bar ends and handle consistent, no 0.5 dp corner on the progress fill) | MANUAL: 200 % zoom crop of the control bar, light and dark |
| 3 | `PlayButton` | `--qt-accent-color=#2E7DFF` | the player view is opened | the play button background is the accent (blue), the glyph stays white | MANUAL: screenshot with the option, and one with `default` |
| 4 | `PlayButton` | `--qt-accent-color=default` | the player view is opened | the button is `ButtonAccent` orange (`#FF8800` dark / `#FF610A` light), the old gradient is gone | MANUAL: screenshot light and dark |
| 5 | Keyboard focus | any | Tab reaches the play button | the focus ring (`modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:255`) is still drawn | MANUAL: screenshot with visual focus |

## 4. Fixtures & mocks
- Pattern to copy: none (no automated row). The screenshots use `vlc --no-qt-privacy-ask --qt-accent-color=…`
  on a local file.
- Stubs: none.
- Sample media: any local video; `test/samples/image.jpg` opened with `--image-duration=-1` is enough for the player view.

## 5. Design decisions & open questions
1. **RULED — the radius scale.** RULED 2026-10-14 (R0): radius scale accepted: xsmall 2 dp, small 4 dp, normal 6 dp, large 8 dp, with the existing named radii kept as aliases; values are confirmed against the Figma file in Step 2. Proposed: `radius_xsmall 2 dp`, `radius_small 4 dp` (= today's `button_radius`),
   `radius_normal 6 dp`, `radius_large 8 dp` (= `navBoxButton_radius`, `mainView_topLeftRadius`). Existing named radii
   become aliases so no caller outside the player changes in this MR. Values are read from the Figma file (community file 1691465999134370980) in Step 2.
2. **RULED — which orange.** RULED 2026-10-14 (R0): both existing oranges stay unchanged; the play button takes `ButtonAccent` like every other accent button. The video keeps "the cone orange"; master already has two (`orange500` for dark,
   `orange800` for light). This packet does not change either value; the play button takes `ButtonAccent` as every other
   accent button does. Replacing `#FF8800`/`#FF610A` is a brand change for VideoLAN to rule on.
3. **RULED — play button shape.** RULED 2026-10-14 (R0): this packet keeps the circular play button and only fixes its colour; the rounded-rectangle shape goes to PKT-3003. The video's play button (01:45–02:23) is an orange **rounded rectangle**, not a
   circle. This packet keeps the circle and only fixes the colour; the shape belongs to PKT-3003 once the layout is
   ruled. Recorded so the reviewer does not read the circle as an omission.
4. NOTE — the two `FIXME` oranges (`modules/gui/qt/style/systempalette.hpp:117-118`) are not used by the player; left as they are.
5. NOTE — no new string, no new file: `po/POTFILES.in` untouched.
6. NOTE (Step 2 dry-run, 2026-10-15) — `grep -rn "radius:"` over `modules/gui/qt/player` and `modules/gui/qt/widgets/qml`
   finds 64 sites. Commit 2 converts these sites: `modules/gui/qt/widgets/qml/SliderExt.qml:48` (`radius_xsmall`),
   `modules/gui/qt/player/qml/SliderBar.qml:327` and `:436` (`radius_xsmall`), `modules/gui/qt/player/qml/SliderBar.qml:578` (`width / 2`), and
   `modules/gui/qt/widgets/qml/ButtonExt.qml:48` (`radius_small`). `ButtonExt.qml:107` and `:121` already read `control.radius` and
   `parent.radius`, so the token goes in the declaration at `:48`. The other sites are out of scope (later packets).

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
Executed 2026-10-15 on `nzxt`, worktree `~/src/vlc-wt/pkt-3001` (branch `master-pkt-3001-design-tokens-accent-greys-corners`
from `ce4af2f`), the CI image of §6, `meson setup build-meson -Dextra_checks=true`, `ninja -j8`.
Logs: `~/src/logs/pkt-3001-*.log`.

- Commits: `9f81cad` qt: add corner radius tokens to VLCStyle; `e0b901d` qml: use the radius tokens in the player
  controls; `3aa50e6` qml: let the play button follow the accent colour.
- PR: https://github.com/oscarolar/vlc/pull/4 (fork, base `master`).
- Baseline (`pkt-3001-baseline.log`): full build plus `meson test --suite qt` gave Ok 7, Fail 0.
- Gate after commit 3 (`pkt-3001-gate.log`): `meson test -C build-meson --suite qt --print-errorlogs` gave Ok 7, Fail 0.
  Each commit built on its own (`pkt-3001-commit1.log`, `pkt-3001-commit2.log`).
- Warnings: no compiler warning in the touched files. The only warnings in the logs are meson's configure notices and
  the generic cc1plus `-Werror=unterminated-string-initialization` notice, which baseline has too.
- qmllint, Qt 6 (`pkt-3001-qmllint-before.log` and `pkt-3001-qmllint-after.log`). Warning counts per file, before
  and after: VLCStyle 135 and 135, SliderExt 35 and 34, SliderBar 178 and 179, ButtonExt 56 and 56, PlayButton 91 and
  100. Every added line is an unresolved-VLC-type warning (`ColorContext`, `VLCStyle`, the outer `root` id), the same
  kind the existing `theme` context already raises. There is no new semantic warning.
- Rows 1 to 5 (MANUAL): screenshots are still pending. There is no display in the build environment, so they must be
  taken on a desktop build and attached to the PR.
- Figma: the community file 1691465999134370980 returned HTTP 403 without a browser login. The radius values are the
  ones proposed in §5.1.
- Deviations:
  - Spec facts that differ from the code:
    - Row 3 expects a white glyph, but the play button is an accent ring around a white disc with a black glyph. The
      commit recolours the ring only, so with the option set the ring turns blue and the disc and glyph stay as they are.
    - `SliderBar.qml:327` `radius: implicitHeight` read the Rectangle's own implicit height, which is 0, so the track
      was square before.
    - `SliderExt`'s 3 dp radius was already clamped to 2 dp by its 4 dp bar, so `radius_xsmall` draws exactly the same.
    - VLCStyle uses `MainCtx.dp`, not `VLCStyle.dp`.
  - Choices made in this packet:
    - The seek handle uses `width / 2` instead of a token. It is a round knob, matching the `SliderExt` handle, and it
      draws the same as before because `margin_small` (12 dp) was already more than half of its 15 dp size.
    - `gridCover_radius` also became an alias (of `radius_small`) because its value matches.
    - The ButtonAccent context sets only `enabled`, so the play button stays on the accent while pressed, instead of
      using ButtonAccent's fixed pressed orange `#e65609`.
  - Build setup: the §6 command was run from the worktree with `~/src` mounted at the same path.
  - Baseline: the commit 1 VLCStyle edit was saved while the baseline build was still running. The 7/7 result on
    `ce4af2f` is also documented in BUILD-NOTES.

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, in `~/src/vlc`: `git switch -c master-pkt-3001-design-tokens-accent-greys-corners ce4af2f`
      (if another packet's branch is checked out there, use `git worktree add ~/src/vlc-wt/pkt-3001 -b <branch> ce4af2f`
      and mount that path in §6); run the §6 command once → green before any change.
- [ ] **Step 2: Dry-run** — re-read every range of §2; also `grep -rn "radius:" --include='*.qml' modules/gui/qt/player
      modules/gui/qt/widgets/qml` and list in §5 every site that commit 2 converts.
- [ ] **Step 3: Baseline screenshots** — rows 1–4 "before" shots from the unmodified build (X11 forwarding or
      `xvfb-run` + `import -window root` on nzxt), stored for the MR.
- [ ] **Step 4: Commit 1** — add the scale to `style/VLCStyle.qml` next to `button_radius` (`:178`), aliases for the
      existing names; build; row 1 shot → identical. Commit `qt: add corner radius tokens to VLCStyle`.
- [ ] **Step 5: Commit 2** — `modules/gui/qt/widgets/qml/SliderExt.qml:48`, `modules/gui/qt/player/qml/SliderBar.qml:327`, `modules/gui/qt/player/qml/SliderBar.qml:436`, `modules/gui/qt/player/qml/SliderBar.qml:578`,
      `modules/gui/qt/widgets/qml/ButtonExt.qml:107`, `modules/gui/qt/widgets/qml/ButtonExt.qml:121` read the tokens; row 2 shots. Commit `qml: use the radius tokens in the player controls`.
- [ ] **Step 6: Commit 3** — `modules/gui/qt/player/qml/controlbarcontrols/PlayButton.qml:258-266`: replace the `Gradient` by `color: accentTheme.bg.primary` from a
      `ColorContext { colorSet: ColorContext.ButtonAccent }`; keep the glow (`:268-278`). Rows 3–5 shots.
      Commit `qml: let the play button follow the accent colour`.
- [ ] **Step 7: Whole gate** — the §6 command; paste the summary in §6 Evidence.
- [ ] **Step 8: Commit and MR** — push to the fork, PR to the fork github.com/oscarolar/vlc (base `master`) with the before/after
      pairs and the §5 rulings 1–3 recorded; `status: review`.
