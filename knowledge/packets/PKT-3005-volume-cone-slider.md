---
id: PKT-3005
title: "The volume slider is drawn as a full cone, green to red past 100 %"
section: "A. Control bar"
video_ref: "02:55–03:10"
pipeline: CI
kind: ui
track: T2
component: modules/gui/qt/player/qml/controlbarcontrols
branch: master-pkt-3005-volume-cone-slider
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml
  - modules/gui/qt/util/qml/ConeGeometry.qml
  - modules/gui/qt/tests/tst_ConeGeometry.qml
  - modules/gui/qt/meson.build
estimate_h: 12.0
priority: P2
depends_on: [PKT-3001]
upstream_ruling: needed
target: fork
mr_title: "qml: draw the volume slider as a cone"
status: ready
---

# PKT-3005 — The volume slider is drawn as a full cone, green to red past 100 %

## 1. Scope & intent
At 02:55–03:10 the designer keeps VLC's characteristic volume control instead of "over-simplifying it like modern
players": VLC 3's wedge (frames ~02:58–03:03: an outlined right triangle, "0%" / "101%", filled green from the left)
"looks like a deformed cone", so she makes it **a full cone** and **keeps the colours**. The frame at ~03:06 shows a
horizontal cone — thin rounded tip on the left, tall rounded base on the right — filled with a green → yellow → red
gradient, a speaker icon on its left.

Master dropped the wedge: the QML `VolumeWidget` is a plain 100 dp rounded bar (`SliderExt`) whose fill turns from the
foreground colour to the "negative" (red) colour once the volume exceeds 100 %, with a 1 dp tick at 100 %.

After this packet the volume slider keeps every behaviour of today's widget (drag, wheel, right-click snapping, Shift
for fine steps, keyboard, mute, tick at 100 %, `qt-max-volume` range) and only its background/fill are drawn as a cone
with a gradient. The geometry and colour-stop maths live in a small `VLC.Util` component so they are unit-tested.

One MR, two commits:
1. `qml: add ConeGeometry helper` — pure QML component in `VLC.Util` + `tst_ConeGeometry.qml`.
2. `qml: draw the volume slider as a cone` — `VolumeWidget.qml` background/fill use it.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml:28-41` — `paintOnly` (editor preview), `sliderColor`
  switches `theme.fg.primary` → `theme.fg.negative` when `position > _fullvolpos` (`:33-36`); `_maxvol = MainCtx.maxVolume`,
  `_fullvolpos = 100 / _maxvol`, `_maxvolpos = _maxvol / 100` (`:39-41`).
- `modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml:60-95` — mute `IconToolButton` with five volume glyphs (`:65-76`).
- `modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml:97-114` — `Widgets.SliderExt` `volControl`, `implicitWidth: VLCStyle.dp(100, …)` with a `FIXME: use
  VLCStyle` (`:100-101`), `from: 0`, `to: root._maxvolpos` (`:130-131`), muted → `opacity: 0.5` (`:132`).
- `modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml:174-190` (`_adjustPlayerVolume`) — snaps to 1.0 within `_clamp` unless Shift; `:238-271` the 100 %
  tick (`tickmark`) with its own clamp helper; `:273-352` `sliderMouseArea` (right-click snapping to 0 / 0.5 / 1 / 1.25,
  `:322-335`; wheel, `:303-320`).
- `modules/gui/qt/widgets/qml/SliderExt.qml:86-104` (`handle`) and `:106-129` (`background`: a rounded `Rectangle` with a
  child fill `Rectangle` of width `position * parent.width`) — what the cone replaces, only for this instance.
- `modules/gui/qt/meson.build:837-856` — `VLC.Util` module sources (`FSM.qml`, …; `Helpers.qml` singleton): the only QML
  module `qml_test` can import (`modules/gui/qt/tests/qml_test.cpp:38-41`).
- `modules/gui/qt/meson.build:954-956` — `qml_tests = { 'tst_FSM': files('tests/tst_FSM.qml') }`; each entry runs as
  `qml_test -input <file>` (`:1441-1443`).
- No QML file in `modules/gui/qt` imports `QtQuick.Shapes` or uses `Canvas` (grep at `base_sha`): a cone outline needs
  either a new module dependency or a mask (§5.1).

Settings / variables involved: `qt-max-volume` (`modules/gui/qt/qt.cpp:457`, 60–300, default 125) via `MainCtx.maxVolume`;
`volume-step` (`src/libvlc-module.c:1617`) via `Player.setVolumeUp/Down`.
Mock boundary: none for `ConeGeometry` (pure); `VolumeWidget` is not loadable in `qml_test` (imports `VLC.Player`).

### Overlaps & contradictions
- **PKT-3001** provides the radius tokens used for the cone's rounded tip/base.
- **PKT-3011** (audio button: tracks + output device) may put the volume in a popup; it reuses this widget unchanged.
- **PKT-3006** (single thin row) constrains the height: the cone must fit `controlLayoutHeightPinned` (32 dp,
  `modules/gui/qt/style/VLCStyle.qml:328`). Row 4 checks it.
- The toolbar editor draws `VolumeWidget` with `paintOnly: true`; the cone must render there too (row 5).

## 2b. Upstream state
- Volume beyond 100 % with a 100 % mark and red overdrive colour: **shipped** (rectangular).
- Cone/wedge shape: **not shipped** in the QML UI (VLC 3 widgets UI only).

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. Rows 1–3 are a QML `TestCase`
`tests/tst_ConeGeometry.qml` registered as `'tst_ConeGeometry'` in `qml_tests` (`modules/gui/qt/meson.build:954-956`); the
component is `ConeGeometry { width; height; tipRatio; maxValue; fullValue }` in `VLC.Util`.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `ConeGeometry.heightAt(x)` | `width 100, height 20, tipRatio 0.2` | `heightAt(0)`, `heightAt(50)`, `heightAt(100)` | `4`, `12`, `20` (linear taper, never 0 so the tip stays clickable) | `tst_ConeGeometry.qml::test_heightTapersLinearly` |
| 2 | `ConeGeometry.fullStop` | `maxValue 1.25, fullValue 1.0` / `maxValue 3.0` / `maxValue 0.6` | read | `0.8` / `0.333…` / `1.0` (clamped: no red zone when max < 100 %) | `::test_fullStopFollowsMaxVolume` |
| 3 | `ConeGeometry.gradientStops` | `maxValue 1.25` | read | `[0 → green, fullStop → yellow, 1 → red]`, positions ascending and inside [0, 1] | `::test_gradientStops` |
| 4 | `VolumeWidget` visual | video playing, volume 66 %, then 120 % | bar shown, pinned and unpinned | cone filled to the handle, green → yellow before the tick, red past it; fits 32 dp row | MANUAL: four crops at 200 % |
| 5 | Toolbar editor | Customize Interface open | volume widget in the list and in a bar | `paintOnly` cone renders at 50 % | MANUAL: screenshot |
| 6 | Behaviour regression | volume 100 % | right-click at 90 % width, wheel ×3, Shift-drag, M key | same values as master (1.25 / +3 × `volume-step` / unclamped / mute) | MANUAL: values from the tooltip, before/after table in the MR |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/tst_FSM.qml:18-30` (`import QtTest`, `import VLC.Util`, `TestCase { name: … }`
  with the object under test declared inline).
- Stubs: none; the component has no singleton dependency by construction (§5.2).
- Sample media: any local audio file for rows 4–6.

## 5. Design decisions & open questions
1. **RULED — how to draw the cone.** RULED 2026-10-14 (R0): draw the cone with `QtQuick.Shapes` (`ShapePath` + `LinearGradient`); the masked-SVG variant is only the fallback if the module cannot be added. (a) `QtQuick.Shapes` `ShapePath` with a `LinearGradient` fill clipped to the
   value (cleanest; adds the `QtQuick.Shapes` module to the Qt dependency of every platform build and the contribs);
   (b) a `Rectangle` gradient masked by a cone SVG through the existing `image://svgcolor` provider (no new module;
   mask edges blur at fractional scale). Proposed: (a) if VideoLAN accepts the module, else (b).
2. NOTE — `ConeGeometry` imports only `QtQuick`, so `qml_test` can load it; `VolumeWidget` binds its properties to
   `MainCtx.maxVolume` and the slider size.
3. **RULED — colours.** RULED 2026-10-14 (R0): colours come from the palette: `fg.positive` -> `fg.neutral` -> `fg.negative` of the `Slider` colour set. "Keep the colours" = VLC 3's green, plus yellow/red past 100 %. Green/yellow are not palette
   roles today (`theme.fg.positive` is `Qt::green` in dark, `#0F7B0F` for sliders in light — `modules/gui/qt/style/systempalette.cpp:637`).
   Proposed: `fg.positive` → `fg.neutral` → `fg.negative` of the `Slider` colour set, so themes can change them.
4. NOTE — the `FIXME: use VLCStyle` width (`modules/gui/qt/player/qml/controlbarcontrols/VolumeWidget.qml:100`) becomes `VLCStyle.volumeCone_width` (new token,
   same 100 dp).

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`: `git switch -c master-pkt-3005-volume-cone-slider ce4af2f`
      (worktree if busy; rebase on PKT-3001 once merged); §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm `qml_test` still only loads `util_assets`; record the master values of row 6.
- [ ] **Step 3: Row 1 watched failing** — create `tests/tst_ConeGeometry.qml`, register it in `qml_tests`, add an empty
      `util/qml/ConeGeometry.qml` to the `VLC.Util` sources (`modules/gui/qt/meson.build:837-856`) → FAIL; then implement `heightAt` → green.
- [ ] **Step 4: Rows 2–3** — one red/green cycle each. Commit `qml: add ConeGeometry helper`.
- [ ] **Step 5: Widget** — in `VolumeWidget.qml` override `volControl.background` (keep `handle`, `tickmark`,
      `sliderMouseArea` untouched); drawing per §5.1; rows 4–6. Commit `qml: draw the volume slider as a cone`.
- [ ] **Step 6: Whole gate** — §6; paste the summary in §6 Evidence.
- [ ] **Step 7: MR** — PR to the fork github.com/oscarolar/vlc (base `master`), §5.1 and §5.3 as recorded rulings; `status: review`.
