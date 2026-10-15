---
id: PKT-3002
title: "Two icon sets: a consistent two-tone set for menus, monochrome everywhere else"
section: "A. Visual system"
video_ref: "00:59–01:14"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/pixmaps
branch: master-pkt-3002-two-icon-sets
base_sha: ce4af2f
target_files:
  - modules/gui/qt/pixmaps/menu/*.svg
  - modules/gui/qt/util/colorizedsvgicon.cpp
  - modules/gui/qt/util/colorizedsvgicon.hpp
  - modules/gui/qt/tests/test_menu_icons.cpp
  - modules/gui/qt/meson.build
estimate_h: 24.0
priority: P2
depends_on: [PKT-3001]
upstream_ruling: needed
target: fork
mr_title: "qt: make the menu icons a two-tone set that follows the palette"
status: ready
---

# PKT-3002 — Two icon sets: a consistent two-tone set for menus, monochrome everywhere else

## 1. Scope & intent
At 00:59–01:14 the designer keeps VLC's "detailed two-tone icons with gray and orange", modernises them and makes
them consistent, and uses them **only where the icon is not clickable** (menu entries). Everything clickable gets a
**monochrome** twin. The frame at ~01:05 shows the same eight pictograms twice — "decorative" (grey outline with
orange inner parts: disc, gear, subtitles, broadcast, folder, save, queue, renderer) and "clickable" (the same shapes
in one light grey).

Master already has exactly this split (§2b): menus use SVGs under `pixmaps/menu/`, QML uses the monochrome
`VLCIcons` icon font. The defect is inside the menu set: of its **42** SVGs, **12** are the old two-tone drawings
with a hard-coded orange (`#ff950d`), **17** are monochrome Fluent glyphs recoloured at runtime, and **13** are
fixed-colour monochrome drawings that ignore the palette. A menu therefore mixes three styles, the two-tone ones do not
follow `qt-accent-color`, and the fixed ones do not follow light/dark.

After this packet every menu SVG uses only the colour placeholders the colouriser already understands (`#FF00FF`
body, `#FF8800` accent), every menu icon is built through one helper that passes the window text colour **and** the
accent, and a QtTest fails if a new menu SVG ships a hard-coded colour. New artwork from the Figma file replaces the
drawings in a second MR (§5.4).

MR A (this packet's T2 half), three commits:
1. `qt: test that menu icons only use colour placeholders` — the test, red on master (lists the 25 offenders).
2. `qt: colorize menu icons with the accent colour` — `ColorizedSvgIcon::colorizedIconForWidget` passes the accent.
3. `qt: use colour placeholders in the menu icons` — the 25 SVGs edited (colours only, no path change); test green.

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/pixmaps/menu/` — 42 SVGs, all registered in `assets.qrc` under the `/menu` prefix (`assets.qrc:18-61`).
  Two-tone with literal orange `#ff950d` + grey `#747474` + white `#fafafa` (e.g. `file.svg`, `folder.svg`): capture-card,
  disc, file, folder, help, info, movie, music, network, profile_new, volume-high, volume-low. Placeholder `#FF00FF`:
  add, download and the 15 `ic_fluent_*` / `ic_pause_filled` / `ic_playlist`. Fixed colour, no placeholder: clear, exit,
  ic_fluent_record, media_library, messages, preferences, remove, repeat_off, search_clear, stream, sync, update, volume-muted.
- `modules/gui/qt/util/color_svg_image_provider.hpp:29-31` — the placeholder keys `_res_C1`, `_res_C2`, `_res_ACCENT`;
  `modules/gui/qt/util/color_svg_image_provider.cpp:33-37` (`predefinedSubst`) maps them to `#FF00FF`, `#00FFFF`, `#FF8800`.
- `modules/gui/qt/util/color_svg_image_provider.cpp:39-70` (`colorizeSvg`) — reads the file and replaces each
  placeholder case-insensitively; a free function declared at `modules/gui/qt/util/color_svg_image_provider.hpp:33`, testable without a GUI.
- `modules/gui/qt/util/colorizedsvgicon.cpp:32-94` (`ColorizedSvgIcon` ctor) — takes `color1`, `color2`, `accentColor`;
  `:96-100` (`colorizedIconForWidget`) passes **only** `widget->palette().text().color()` as `color1`: the accent is never set
  for menu icons.
- `modules/gui/qt/menus/menus.cpp:181-185` — `QIcon`/path pairs for Open File / Open Directory (`file.svg`, `folder.svg`,
  two-tone, not colourised); `:302-306` `colorizedIconForWidget(":/menu/ic_playlist.svg", menu)` (colourised).
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:750`, `:929`, `:1102`, `:1110` — `QIcon(":/menu/…")` without colourising;
  `:755`, `:1120`, `:1126`, `:1132`, `:1196` with `colorizedIconForWidget`. Both styles side by side in one menu.
- `modules/gui/qt/pixmaps/VLCIcons.json` + `pixmaps/makeIconFont.py` → `VLCIcons.ttf` and the generated
  `style/vlcicons.hpp` (`:1-3` "generated … do not edit by hand") — the monochrome set, 100+ glyphs, coloured by the
  QML `ColorContext` at draw time. Already the "clickable" set of the video.

Settings / variables involved: `qt-accent-color` (`modules/gui/qt/qt.cpp:480`).
Mock boundary: none; the test reads the SVGs from the source tree (or the compiled `/menu` resources).

### Overlaps & contradictions
- **PKT-3001** decides which orange is the accent (§5.2 there); the placeholder approach here is neutral to that ruling.
- **PKT-3004** adds a `queue` glyph to `VLCIcons.json` and regenerates `vlcicons.hpp`; this packet does **not** touch the
  icon font, so the two MRs never conflict on the generated header.
- **PKT-3013 / PKT-3014** (menu button, context-menu cleanup) edit `menus.cpp` / `qml_menu_wrapper.cpp` entries; this
  packet edits only how icons are coloured (`colorizedsvgicon.cpp`) and the SVG files, not the menu code. If 3014 lands
  first, Step 2 re-lists the call sites.

## 2b. Upstream state
- Two icon families (menu SVGs vs. monochrome icon font for QML): **shipped**.
- Runtime recolouring with a body colour and an accent: **shipped** in the colouriser (`_res_ACCENT`), **not used** by
  `colorizedIconForWidget`.
- A consistent, palette-following menu set: **not shipped** (three styles, §2).
The video's "two sets" is therefore master's design; the packet narrows to consistency and palette-following.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. New QtTest file
`modules/gui/qt/tests/test_menu_icons.cpp`, class `TestMenuIcons`, registered in `vlc_tests` as `test_qt_menu_icons`.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | every SVG in `pixmaps/menu` | the source tree | each file's `fill`/`stroke` colours are collected | only `#FF00FF`, `#00FFFF`, `#FF8800`, `none`, `currentColor` appear (red on master: 25 files listed) | `test_menu_icons.cpp::onlyPlaceholderColours` |
| 2 | `colorizeSvg` | `file.svg` after commit 3 | replacements `{_res_C1: #102030, _res_ACCENT: #2E7DFF}` | output contains `#102030` and `#2e7dff` and no `#ff950d` / `#747474` | `test_menu_icons.cpp::twoToneIconTakesBodyAndAccent` |
| 3 | `colorizeSvg` | a monochrome placeholder icon (`add.svg`) | same replacements | output contains `#102030`, accent absent (nothing to replace) | `test_menu_icons.cpp::monochromeIconTakesBodyOnly` |
| 4 | `ColorizedSvgIcon::colorizedIconForWidget` | a `QWidget` whose palette has `Text = #102030`, `Accent = #2E7DFF` | the icon for `file.svg` is rendered to a 16×16 pixmap | the pixmap contains a pixel of the accent hue (red on master: accent not passed) | `test_menu_icons.cpp::widgetIconUsesPaletteAccent` |
| 5 | menus | `--qt-accent-color=#2E7DFF`, dark and light | Media menu opened | every icon is grey/white body + blue accent, one style | MANUAL: two screenshots of the Media menu |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp:18-34` (QtTest class, `QTEST` main) and its meson
  entry `modules/gui/qt/meson.build:1342-1360` (`test_qt_navigation_model`: sources, `moc_sources`, `qtest_qt6_dep`).
- The test needs `util/color_svg_image_provider.cpp` and `util/colorizedsvgicon.cpp` in its sources and the Qt `Svg`
  module; the SVG directory is passed as a compile definition (`-DMENU_ICON_DIR="…/pixmaps/menu"`), like
  `QUICK_TEST_SOURCE_DIR` at `modules/gui/qt/meson.build:1439`.
- Row 4 needs a `QApplication` (widget palette) — `QTEST_MAIN` with `QT_QPA_PLATFORM=offscreen`, as `modules/gui/qt/tests/qml_test.cpp:44`.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED — placeholders for the two-tone set.** RULED 2026-10-14 (R0): placeholders accepted (`#FF00FF` window text, `#FF8800` accent, `#00FFFF` window background); the two-tone menu icons follow the palette. Body grey → `#FF00FF` (window text colour), orange → `#FF8800`
   (accent), white highlight → `#00FFFF` (window background). This makes the old drawings theme-aware without redrawing;
   it changes their look on light theme (grey body becomes the dark text colour). 
2. **RULED — `colorizedIconForWidget` accent source.** RULED 2026-10-14 (R0): the accent is a `MainCtx`-level value passed to `colorizedIconForWidget`, fallback `#FF8800`. `QPalette::Accent` (Qt ≥ 6.6) vs. the QML `SystemPalette`
   accent (`orange500`/`orange800`, honours `qt-accent-color`). Proposed: a `MainCtx`-level accent passed in, so menus
   and QML agree; fallback `#FF8800`.
3. NOTE — `QIcon(":/menu/…")` call sites that do not colourise (`qml_menu_wrapper.cpp:750/929/1102/1110`,
   `modules/gui/qt/menus/menus.cpp:181-185`) switch to `colorizedIconForWidget` in commit 2; otherwise placeholders would render magenta.
   Step 2 must list every `":/menu/` user (`grep -rn '":/menu/' modules/gui/qt`) before commit 3.
4. **T3 split.** MR A (this brief, ~10 h): test, colouriser, placeholder conversion. MR B (~14 h, separate packet
   `PKT-3002b` once the Figma artwork licence is confirmed): redraw the 42 menu icons in the new two-tone style and
   refresh the matching monochrome glyphs in `VLCIcons.json` (regenerate with `makeIconFont.py`, which needs
   `fontforge` — run it on nzxt in a throwaway container, commit the generated `vlcicons.hpp`/`VLCIcons.ttf`).
5. NOTE — macOS menus show no icons for some entries (`modules/gui/qt/menus/menus.cpp:303` `#ifndef __APPLE__`); unchanged.

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
- [ ] **Step 1: Work clone** — on `nzxt`, `~/src/vlc`: `git switch -c master-pkt-3002-two-icon-sets ce4af2f` (or a
      worktree under `~/src/vlc-wt/pkt-3002` if the clone is busy); §6 once → green.
- [ ] **Step 2: Dry-run** — re-read §2; `grep -rn '":/menu/' modules/gui/qt` and paste the call-site list into §5.3;
      re-run the colour census of `pixmaps/menu/*.svg` and update the counts in §1 if they moved.
- [ ] **Step 3: Row 1 watched failing** — write `tests/test_menu_icons.cpp` with `onlyPlaceholderColours`, register
      `test_qt_menu_icons` in `meson.build` after `test_qt_dialog_model` (`:1404-1423`); §6 → FAIL listing 25 files.
      Commit `qt: test that menu icons only use colour placeholders` is made only after Step 5 turns it green (the red
      run goes in §6 Evidence).
- [ ] **Step 4: Rows 4 then 2** — add `widgetIconUsesPaletteAccent` → FAIL; change `modules/gui/qt/util/colorizedsvgicon.cpp:96-100` to pass
      the accent (§5.2) and switch the non-colourised call sites (§5.3) → green.
      Commit `qt: colorize menu icons with the accent colour`.
- [ ] **Step 5: Rows 1–3** — edit the 25 SVGs (colours only; `sed` on the hex values, then eyeball each in a browser);
      rows 1–3 green. Commit `qt: use colour placeholders in the menu icons`.
- [ ] **Step 6: Row 5** — screenshots of the Media menu, light/dark, default and blue accent.
- [ ] **Step 7: Whole gate** — §6; paste the summary.
- [ ] **Step 8: MR** — PR to the fork github.com/oscarolar/vlc (base `master`), §5.1/§5.2 as recorded rulings; `status: review`; open `PKT-3002b`
      for the artwork.
