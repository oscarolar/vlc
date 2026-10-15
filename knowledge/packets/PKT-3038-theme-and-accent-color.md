---
id: PKT-3038
title: "Light/dark theme and a pickable accent colour in the preferences"
section: "E. Settings"
video_ref: "17:39–17:50"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/style + modules/gui/qt/dialogs/preferences
branch: master-pkt-3038-theme-and-accent-color
base_sha: ce4af2f
target_files:
  - modules/gui/qt/style/accent_color.hpp                     # new
  - modules/gui/qt/style/accent_color.cpp                     # new
  - modules/gui/qt/style/systempalette.cpp
  - modules/gui/qt/style/systempalette.hpp
  - modules/gui/qt/style/systempalettethemeprovider.cpp
  - modules/gui/qt/util/color_scheme_model.cpp
  - modules/gui/qt/dialogs/preferences/sprefs_interface.ui
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
  - modules/gui/qt/maininterface/mainctx.hpp
  - modules/gui/qt/maininterface/mainctx.cpp
  - modules/gui/qt/tests/test_accent_color.cpp               # new
  - modules/gui/qt/meson.build
estimate_h: 10.0
priority: P2
depends_on: [PKT-3001, PKT-3036]
upstream_ruling: not-needed
target: upstream
mr_title: "qt: pick the accent colour from the preferences and apply it live"
status: ready
---

# PKT-3038 — Light/dark theme and a pickable accent colour in the preferences

## 1. Scope & intent
At 17:39–17:50 the designer puts theme selection (light, dark) in the General settings and adds an accent colour
setting "so you could change the orange to whatever colour you like". Frame s021 (bottom-right) shows the target:
*Global Settings → Theme* as a three-way segmented control **Dark / Light / System**, and *Accent color* as a row of
nine round swatches (orange, red, yellow, green, teal, blue, purple, pink, grey).

**Master already ships most of this** (§2b): a System/Day/Night colour scheme selector (preferences, View menu,
first-run wizard) and a `qt-accent-color` option. What is missing is narrow, and this packet does only that:
- a swatch row for the accent colour in the Interface panel, under the colour scheme combo, plus "System" (Qt ≥ 6.6)
  and "Custom…" (`QColorDialog`); today the option is a free text field in the All view only;
- applying the accent **live** — today it is read once when the palette object completes;
- one parser shared by the two places that read the option, with a fallback for invalid values (today an invalid
  string yields an invalid `QColor`);
- the scheme labels say "Light" / "Dark" instead of "Day" / "Night" (§5.2).

Where the row lives ("General" vs "Interface") follows PKT-3039; this packet adds it to today's Interface panel.

One MR, three commits:
1. `qt: share the accent colour parsing and fall back on invalid values` — helper + QtTest.
2. `qt: apply the accent colour without restarting`.
3. `qt: add an accent colour picker to the interface preferences`.

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/qt.cpp:297-298` (`ACCENT_COLOR_TEXT`, `ACCENT_COLOR_LONGTEXT`) and `:480`
  `add_string( "qt-accent-color", "default", … )` — free string, special values `default` and `system`, no list.
- `modules/gui/qt/style/systempalette.cpp:304-336` (`SystemPalette::componentComplete`) — reads `qt-accent-color`
  once, sets `orange500`/`orange800`; `"system"` uses `QPalette::Accent` under Qt ≥ 6.6; anything else is passed to
  `QColor(const char*)` with no validity check.
- `modules/gui/qt/style/systempalettethemeprovider.cpp:134-165` (`updatePalette` of the system-palette provider) —
  a second, slightly different copy of the same parsing (`#FF8800`/`#FF610A` defaults by darkness, pressed shade).
- `modules/gui/qt/style/windowsthemeprovider.cpp:166-169` — the Windows provider ignores the option and uses
  `COLOR_HIGHLIGHT` for `VQTC_NAME_ACCENT` (with a NOTE asking whether to keep the orange).
- `modules/gui/qt/style/systempalette.cpp:366-410` (`setSource`, `setCtx`, `updatePalette`) — the palette is rebuilt
  on scheme change and on provider `paletteChanged`, never on an option change.
- `modules/gui/qt/style/VLCStyle.qml:38-49` — two `SystemPalette` instances (current and forced dark).
- `modules/gui/qt/util/color_scheme_model.cpp:23-28` (`ColorSchemeModel` ctor) — items `System`, `Day`, `Night`;
  `:78-91` `setCurrentIndex` clamps and emits `currentChanged`.
- `modules/gui/qt/maininterface/mainctx.cpp:162-187` — the scheme also drives `QStyleHints::setColorScheme` on
  Qt ≥ 6.8; `:336` saves `color-scheme-index`, `:497-498` restores it.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:832-836` — Interface panel binds
  `colorSchemeComboBox` to the model live, with a `PropertyResetter` for Cancel; `modules/gui/qt/dialogs/preferences/sprefs_interface.ui:337-342`,
  `:408` hold the label and combo inside `LooknfeelBox` (`:47`).
- `modules/gui/qt/menus/menus.cpp:354` — "Color Scheme" checkable sub-menu in View.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:66-77`, `:142`, `:286-305` — the first-run wizard's colour
  scheme page.
- `modules/gui/qt/maininterface/mainctx.cpp:511-514` (`reloadPrefs`) — called by `PrefsDialog::save`
  (`modules/gui/qt/dialogs/preferences/preferences.cpp:371-372`); the natural hook to re-read the accent.

Settings / variables involved: `qt-accent-color` (`modules/gui/qt/qt.cpp:480`); QSettings
`MainWindow/color-scheme-index` (`modules/gui/qt/maininterface/mainctx.cpp:497`).
Mock boundary: none — the helper is a pure function of (string, isDark, QPalette); rows 1–5 need no libvlc.

### Overlaps & contradictions
- **PKT-3001** (design tokens) defines the orange and greys; the swatch set and the "default" values come from its
  tokens, not from literals here. Depends on it.
- **PKT-3036/3039** decide where the row lives (Interface today, "General" in the 13-category map).
- **PKT-3043** (onboarding) shows the theme choice; the wizard page already exists in master — PKT-3043 may add the
  swatch row there reusing this packet's widget.
- **PKT-3002** (icon sets) reads accent through `ColorContext`; live update must reach those bindings too (row 6).

## 2b. Upstream state
Shipped: System/Day/Night scheme in Preferences → Interface, View → Color Scheme, and the first-run wizard; scheme
changes apply live. `qt-accent-color` exists since VLC 4 with `default`/`system`/any colour name. Not shipped: a
picker, live accent update, validation, a Windows-provider path. The video's "add an accent colour setting" is
therefore a UI-exposure and live-update packet, not a new feature in core.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `vlc::accentColor(spec, isDark, palette)` | `"default"` | dark / light | `#FF8800` / `#FF610A`, pressed `#e67a30` / `#e65609` (today's values) | `tests/test_accent_color.cpp::testDefault` |
| 2 | same | `"#3b82f6"` | — | accent = that colour, pressed = `lighter(150)` | `::testHex` |
| 3 | same | `"banana"`, `""`, `"#12"` | — | falls back to row 1's default, valid colours | `::testInvalidFallsBack` |
| 4 | same (Qt ≥ 6.6) | `"system"`, palette with Accent = `#00aa00` | — | accent `#00aa00`; under Qt < 6.6 → default | `::testSystem` |
| 5 | `ColorSchemeModel` | — | read row texts | "System", "Light", "Dark"; `scheme()` values unchanged | `::testSchemeLabels` |
| 6 | live update | running UI | pick the blue swatch, Save | buttons, sliders and focus rings turn blue without restart; Cancel restores | MANUAL (before/after screenshots, light and dark) |
| 7 | picker | Interface panel | Tab through swatches, Space | selection moves and is announced; "Custom…" opens a colour dialog | MANUAL |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_thread_runner.cpp` + its meson block `modules/gui/qt/meson.build:1384-1401`
  (QtTest with no libvlc and few sources) — `test_qt_accent_color` compiles `style/accent_color.cpp`,
  `util/color_scheme_model.cpp` (needs `qt.hpp` for `qtr`; Step 2 checks the include chain).
- Stubs: none. Sample media: none.

## 5. Design decisions & open questions
1. **NOTE** — Swatches store `#rrggbb` in `qt-accent-color`; the option stays a string so existing configs keep
   working and `system`/`default` stay valid.
2. **RULED** RULED 2026-10-14 (R0): "Day/Night" is renamed to "Light/Dark". Rename "Day/Night" to "Light/Dark". The designer and every OS use Light/Dark; it changes three
   translated strings and the first-run labels (they read from the model). Small, but user-visible wording.
3. **NOTE** — Windows provider (`modules/gui/qt/style/windowsthemeprovider.cpp:166-169`) keeps `COLOR_HIGHLIGHT`; honouring the option
   there is a follow-up (the NOTE in that file asks the same question). The picker shows a hint on Windows when that
   provider is active.
4. **NOTE** — Contrast: `textOnAccent` is white (`modules/gui/qt/style/systempalettethemeprovider.cpp:167`); a light swatch (yellow) needs
   dark text. The helper also returns `textOnAccent` by relative luminance; covered by row 2 variants.
5. **NOTE** — `upstream_ruling: not-needed`: exposes an existing option; ruling 5.2 is wording only.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3038-theme-and-accent-color ce4af2f`
      (after PKT-3001 is merged, rebase on it); §6 command once → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm the token names PKT-3001 introduced for the default accent.
- [ ] **Step 3: Row 1 watched failing** — `tests/test_accent_color.cpp`, `test_qt_accent_color` in `meson.build`;
      empty `style/accent_color.cpp` → FAIL; implement.
- [ ] **Step 4: Rows 2–5** — one cycle each; replace both parsers (`modules/gui/qt/style/systempalette.cpp:304-336`,
      `modules/gui/qt/style/systempalettethemeprovider.cpp:139-165`) by the helper; rename labels in `modules/gui/qt/util/color_scheme_model.cpp:25`. Commit
      `qt: share the accent colour parsing and fall back on invalid values`.
- [ ] **Step 5: Live** — `MainCtx` emits `accentColorChanged` from `reloadPrefs` when `qt-accent-color` changed;
      `SystemPalette` re-runs the accent block and `updatePalette()`; the provider path gets `paletteChanged`. Row 6.
      Commit `qt: apply the accent colour without restarting`.
- [ ] **Step 6: Picker** — swatch row (`QButtonGroup` of round `QToolButton`s + "System" + "Custom…") in
      `sprefs_interface.ui` under `colorSchemeComboBox`; bound in `simple_preferences.cpp` near `:832` with a
      `PropertyResetter`; row 7. Commit `qt: add an accent colour picker to the interface preferences`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MR** — MR to videolan/vlc master (Draft); `status: review`.
