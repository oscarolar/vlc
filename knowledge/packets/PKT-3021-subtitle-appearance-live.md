---
id: PKT-3021
title: "Subtitle font, size, colours and outline change live from the Subtitles tab"
section: "C. Subtitles"
video_ref: "07:22–07:46"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/player
branch: master-pkt-3021-subtitle-appearance-live
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/subtitle_style_controller.cpp
  - modules/gui/qt/player/subtitle_style_controller.hpp
  - modules/gui/qt/player/qml/SubtitleAppearanceSection.qml
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
  - modules/text_renderer/freetype/freetype.c
  - modules/gui/qt/tests/test_subtitle_style_controller.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 16.0
priority: P1
depends_on: [PKT-3018, PKT-3019]
upstream_ruling: needed
target: fork
mr_title: "qml: live subtitle appearance controls in the settings sidebar"
status: ready
---

# PKT-3021 — Subtitle font, size, colours and outline change live from the Subtitles tab

## 1. Scope & intent
07:22–07:46: the Subtitles tab also holds the **appearance** — font, font size, outline, position — "so you
could change them while watching". Viewers asked for **exact font values, not small or big**; the designer says
VLC already has them "hidden deep inside advanced settings" (frame `s009` ≈07:33 shows *Advanced Preferences →
Video → Subtitles/OSD → Text renderer* with "Font size in pixels" and "Relative font size") and proposes a
**Relative / Pixels** switch for sizes. Frame `s009` (≈07:24–07:45) shows the target section *Appearance*:
Embedded Sub Format (toggle, OFF) · Subtitle Font (Helvetica) · Text Color `#B8B8B8` + swatch · Outline Color
`#0C0C0C` + swatch · Subtitle Offset `0px` · **Size Values  Relative | Pixels** (segmented, orange when active) ·
Font Size (slider with *Auto* in relative mode, `18px` field in pixel mode) · Outline Thickness (slider *Normal* /
`2px`) · Background Opacity `0 %`, then a preset dropdown (PKT-3022) and an *Open Advanced Settings* link.

**The video's premise is outdated for master** (§2b): the pixel and relative font size options are
`add_obsolete_integer` since 4.0.0; master only has a percentage scale. This packet therefore ships the
section with a **relative** size control (percent of the default, live), the live colour/outline/background
controls, and drops **Pixels** per the ruling in §5.1. It also fixes the simple-preferences font size control,
which is bound to the obsolete option.

One MR, four commits:
1. `qt: bind the subtitle size preference to sub-text-scale` — simple prefs fix (standalone).
2. `freetype: apply outline colour and opacity live` — small core change so the outline colour of the video
   updates without restarting the vout (§2, §5.2).
3. `qt: add SubtitleStyleController` — C++ model + QtTest.
4. `qml: live subtitle appearance controls in the settings sidebar`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/text_renderer/freetype/freetype.c:198-199` — `add_obsolete_integer("freetype-fontsize")` and
  `("freetype-rel-fontsize")`, both "since 4.0.0". The Advanced Preferences screenshot in the video is VLC 3.
- `modules/text_renderer/freetype/freetype.c:136-196` — the options that remain: `freetype-font` /
  `-monofont` (`:141-147`), `freetype-opacity` (`:151`), `freetype-color` (`:156`), `freetype-bold` (`:160`),
  `freetype-background-opacity/-color` (`:163-168`), `freetype-outline-opacity/-color` (`:170-175`),
  `freetype-outline-thickness` 0–50 with a named list (`:176-179`), shadow opacity/colour/angle/distance
  (`:181-192`).
- `modules/text_renderer/freetype/freetype.c:710-721` (`UpdateDefaultLiveStyles`) — read **on every render**
  (`:1042`): font colour, background alpha/colour, outline thickness. These are already live.
- `modules/text_renderer/freetype/freetype.c:723-770` (`FillDefaultStyles`) — read **once at open**: font
  family, mono family, text opacity, **outline opacity/colour**, shadow, blending, bold. Changing them needs a new
  text renderer (vout restart) — the video's live "Outline Color" is not possible today.
- `modules/text_renderer/freetype/freetype.c:1040` — `p_sys->i_scale = var_InheritInteger(p_filter,
  "sub-text-scale")` on every render (grid/teletext excluded).
- `modules/text_renderer/freetype/platform_fonts.c:546-568` (`ConvertToLiveSize`) — default size is
  `f_font_relsize` % of the render area height (width in portrait) × `i_scale`/100; default relsize 6.25
  (`include/vlc_text_style.h:117`). A pixel size is therefore a function of the output height, not a constant.
- `src/libvlc-module.c:1828-1831` — `sub-margin` (integer px, the "Subtitle Offset"), `sub-text-scale`
  10–500 %, default 100.
- `src/player/player.c:1942-1952`, `include/vlc_player.h:1954-1963` — `vlc_player_SetSubtitleTextScale` /
  `Get…` on the player variable created at `src/player/player.c:2142`; `src/player/osd.c:334` shows the OSD for it;
  hotkeys `key-subtitle-text-scale-normal/-up/-down` (`src/libvlc-module.c:2757-2762`).
- `modules/gui/qt/player/player_controller.hpp:166-176` — no text-scale or style property in `PlayerController`.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:944-972` — the simple Subtitles panel binds
  `sub-language`, **`freetype-rel-fontsize` (`:953-954`, obsolete)**, `freetype-font`, `freetype-color`,
  `freetype-outline-thickness`, `freetype-outline-color`, `sub-margin`; shadow/background are checkboxes
  writing opacities (`:965-972`, `:1309-1321`). The font size combo is bound to a dead option.
- `modules/codec/subsusf.c:54` — `subsdec-formatted` exists only for USF; there is no general switch to ignore
  the styling embedded in ASS/WebVTT (the "Embedded Sub Format" toggle of the frame) — §5.3.

Settings / variables involved: `sub-text-scale` (`src/libvlc-module.c:1830`), `sub-margin` (`:1828`),
`freetype-font`, `freetype-color`, `freetype-outline-color`, `freetype-outline-opacity`,
`freetype-outline-thickness`, `freetype-background-opacity`, `freetype-background-color`
(`modules/text_renderer/freetype/freetype.c:141-179`).
Mock boundary: `SubtitleStyleController` against `VLCTestingEnv` (`tests/vlc_stub_modules.hpp`, real
`p_player`, real config); asserts on `config_GetInt` / `vlc_player_GetSubtitleTextScale`; no vout.

### Overlaps & contradictions
- **PKT-3018** shell; **PKT-3019** owns `SliderValueMapper`, reused for size/thickness/opacity sliders.
- **PKT-3022** saves this section's values into presets; the controller exposes a `snapshot()`/`apply(map)` pair
  for it (§5.4).
- **PKT-3036/3039** (preferences tree) also edit `simple_preferences.cpp`; commit 1 is one line and should land
  first as its own MR.

## 2b. Upstream state
- Size: only **relative** (`sub-text-scale`, %, live, with hotkeys and OSD). Absolute pixel size was removed in
  4.0. The video's "it does exist" is true for VLC 3.0.x, false for master.
- Colour, background, outline thickness: live already (`modules/text_renderer/freetype/freetype.c:710-721`) but only reachable from
  preferences, whose writes land in the config the renderer reads.
- Font family, outline colour/opacity, shadow: applied when the text renderer opens.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `SubtitleStyleController::textScale` | player at default | `setTextScale(150)` | `vlc_player_GetSubtitleTextScale() == 150`; `textScaleChanged` emitted once | `test_subtitle_style_controller.cpp::testTextScaleGoesToPlayer` |
| 2 | same | — | `setTextScale(5)` / `(900)` | clamped to 10 / 500 (core asserts the range, `src/player/player.c:1945`) | `test_subtitle_style_controller.cpp::testTextScaleClamped` |
| 3 | `textColor` | — | `setTextColor("#B8B8B8")` | `config_GetInt("freetype-color") == 0xB8B8B8` | `test_subtitle_style_controller.cpp::testTextColorWritesConfig` |
| 4 | `outlineThickness` | — | `setOutlineThickness(2)` | `freetype-outline-thickness == 2` (list values of `modules/text_renderer/freetype/freetype.c:176-179` accepted, others snapped to nearest) | `test_subtitle_style_controller.cpp::testOutlineThicknessSnapsToList` |
| 5 | `backgroundOpacity` | — | `setBackgroundOpacity(0.5)` | `freetype-background-opacity == 128` | `test_subtitle_style_controller.cpp::testBackgroundOpacityPercent` |
| 6 | `snapshot()` / `apply()` | values set | `apply(snapshot())` on a fresh controller | all properties equal; this is the preset contract of PKT-3022 | `test_subtitle_style_controller.cpp::testSnapshotRoundTrip` |
| 7 | `resetToDefaults()` | values changed | reset | every option back to its module default (`config_FindConfig(...)->orig`) | `test_subtitle_style_controller.cpp::testReset` |
| 8 | simple prefs size | Preferences → Subtitles/OSD | choose "Larger" | `sub-text-scale` changes (not the obsolete key) | MANUAL — `pkt3021-sprefs-size.png` (and `vlcrc` diff) |
| 9 | freetype outline live | video with subtitles playing | outline colour changed in the sidebar | next subtitle line drawn with the new outline, no vout restart | MANUAL — `pkt3021-outline-live.webm` |
| 10 | `SubtitleAppearanceSection.qml` | sidebar Subtitles tab | change font size slider | text grows live, field shows `%`; Pixels segment per §5.1 | MANUAL — `pkt3021-appearance.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_navigation_model.cpp` and its `vlc_tests` block
  (`modules/gui/qt/meson.build:1342-1360`); `VLCTestingEnv::init()` gives `intf->p_player`
  (`modules/gui/qt/tests/vlc_stub_modules.cpp:92-93`).
- Config writes in tests go to a throw-away config (`VLCTestingEnv` runs libvlc with `--ignore-config` — Step 2
  confirms in `vlc_stub_modules.cpp`; if not, add it so the test never writes the user's `vlcrc`).
- Register the controller for QML in `maininterface/mainui.cpp` next to `MLBookmarkModel` (`:483`) style
  registrations (Step 2 picks the right module block — `VLC.Player`).
- Sample media for MANUAL rows: a video with an `.srt`.

## 5. Design decisions & open questions
1. **RULED — "Pixels" mode.** RULED 2026-10-14 (R0): option (c): drop the Pixels segment and keep relative sizes only; an absolute size (b) may be proposed later as a separate core packet. Master has no absolute size (`modules/text_renderer/freetype/freetype.c:198-199`). Options: (a) emulate:
   compute the scale that makes the default size equal N px at the current output height
   (`ConvertToLiveSize`, `modules/text_renderer/freetype/platform_fonts.c:546-568`) — the size then drifts when the window is resized, which
   contradicts "exact"; (b) core: re-introduce an absolute size option in freetype (reverting a 4.0 design choice);
   (c) drop the segment and keep relative only. Recommendation (c) for this packet, (b) proposed to VideoLAN as a
   separate core packet if wanted. 
2. **RULED — core commit 2.** RULED 2026-10-14 (R0): core commit 2 is accepted: outline alpha/colour become live like the text colour; font family stays open-time. Moving `i_outline_alpha` / `i_outline_color` from `FillDefaultStyles`
   (`modules/text_renderer/freetype/freetype.c:750-751`) into `UpdateDefaultLiveStyles` (`:710-721`) makes outline colour live like the text
   colour; needs freetype maintainers' review. Font family stays open-time (face reload on every render is too
   costly); changing it shows "applies to the next video" (§5.5).
3. NOTE — "Embedded Sub Format" toggle: no general option exists (`modules/codec/subsusf.c:54` is USF-only). Out of scope;
   the toggle is not drawn. Could become a forced-style switch in a future core packet.
4. **RULED — session vs. default.** RULED 2026-10-14 (R0): apply to the session through player variables and offer an explicit "Save as default". Writing `freetype-*` with `config_PutInt` changes the saved preference
   when config is saved, i.e. every tweak becomes the new default. Options: apply to session and offer
   "Save as default" (needs player-level variables the renderer inherits — `var_Create` on the player for each
   `freetype-*` option, the way `sub-text-scale` is at `src/player/player.c:2142`), or accept write-through. Recommend
   player variables + explicit "Save as default"; the controller API is the same either way.
5. NOTE — font family change applies at the next vout (text renderer reopen); the field says so.
6. NOTE — "Subtitle Offset" maps to `sub-margin` (px from bottom); the video shows `0px`.

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt` or `modules/text_renderer`.
- Strings added are marked for translation (`qsTr` / `qtr`) and `po/POTFILES.in` is updated when a file is new.
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on every new control.

### Evidence
<Filled by the executor: build tail, `meson test` summary, screenshots, MR URL.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3021-subtitle-appearance-live` from merged PKT-3018/3019; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; confirm `VLCTestingEnv` ignores the user config; re-check the §5.1/§5.4 rulings before Step 5.
- [ ] **Step 3: Commit 1** — `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:953` binds `sub-text-scale` with a relative list (or an integer slider); MANUAL row 8; commit `qt: bind the subtitle size preference to sub-text-scale` (own MR possible).
- [ ] **Step 4: Commit 2** — move outline opacity/colour to `UpdateDefaultLiveStyles`; MANUAL row 9; commit `freetype: apply outline colour and opacity live`.
- [ ] **Step 5: Row 1 watched failing** — `tests/test_subtitle_style_controller.cpp` against an empty `player/subtitle_style_controller.{hpp,cpp}` registered in `meson.build` → FAIL; rows 2–7 one cycle each; commit `qt: add SubtitleStyleController`.
- [ ] **Step 6: Section** — `player/qml/SubtitleAppearanceSection.qml` in the Subtitles tab (font picker, colour fields with swatch, sliders via `SliderValueMapper`, "Open Advanced Settings" → preferences on Text renderer); register in `VLC.Player`, `po/POTFILES.in`; MANUAL row 10; commit `qml: live subtitle appearance controls in the settings sidebar`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MR** — MR(s) to the fork github.com/oscarolar/vlc (base `master`) with the §5 rulings; `status: review`.
