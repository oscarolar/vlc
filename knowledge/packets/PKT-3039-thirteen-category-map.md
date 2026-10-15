---
id: PKT-3039
title: "Preferences grouped in 13 categories (App / Media / System)"
section: "E. Settings"
video_ref: "17:35–18:27"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/dialogs/preferences (core untouched in the recommended path)
branch: master-pkt-3039-thirteen-category-map
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/preferences/prefs_category_map.hpp   # new
  - modules/gui/qt/dialogs/preferences/prefs_category_map.cpp   # new
  - modules/gui/qt/dialogs/preferences/simple_preferences.hpp
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
  - modules/gui/qt/dialogs/preferences/preferences.cpp
  - modules/gui/qt/dialogs/preferences/prefs_merge.cpp          # from PKT-3036
  - modules/gui/qt/tests/test_prefs_category_map.cpp            # new
  - modules/gui/qt/meson.build
  - modules/gui/qt/pixmaps/prefsmenu/                           # 13 icons, from PKT-3002
estimate_h: 40.0
priority: P3
depends_on: [PKT-3036]
upstream_ruling: needed
target: fork
mr_title: "qt: preferences: present settings in thirteen categories"
status: planned
---

# PKT-3039 — Preferences grouped in 13 categories (App / Media / System)

## 1. Scope & intent
At 17:35–18:27 the designer, after listing every setting one by one, regroups them into thirteen categories shown as
a left sidebar in three groups (frames s020 bottom row and s021): **App** — General (theme, accent, language,
startup, diagnostics/logs), Interface (skin, window, playback controls, notifications/tray), Privacy (history,
network access), Controls (keyboard shortcuts, skip amounts, mouse, remotes); **Media** — Playback, Video, Audio,
Subtitles (subtitles and OSD messages), Library (playlists, folders, metadata); **System** — Network, Performance
(decoding, CPU, smooth playback, developer options), Recording (streaming/casting), Extensions.

Today the Simple view has six/seven icons (Interface, Audio, Video, Subtitles/OSD, Input/Codecs, Hotkeys, Media
Library) and the All view follows core's seven categories (Interface, Audio, Video, Input/Codecs, Stream output,
Advanced, Play Queue) with 33 subcategories.

**Two ways to get there; this packet takes the second** (§5.1):
- A. Remap core: new `CAT_*`/`SUBCAT_*` and a new `set_subcategory` in every module. Size measured at `ce4af2f`:
  **581 `set_subcategory(...)` matches in 518 files** under `modules/` and `src/` (578 active: 3 are commented out in `src/libvlc-module.c`, which has 26 active), plus `include/vlc_plugin.h:150-217`, `src/config/cat.c:34-81`,
  `include/vlc_config_cat.h`, the Rust module macros, the macOS preferences and a plugin ABI bump. Thirteen UI
  categories also do not partition by module: "Privacy" and "General" are a handful of options picked from several
  subcats.
- B. **A presentation map in Qt**: subcategory → one of 13 categories by default, plus a per-option override list
  (e.g. `metadata-network-access` → Privacy, `network-caching` → Network). Core keeps its taxonomy (used by `vlc -H`,
  macOS, skins2, ncurses); the Qt dialog shows the 13 pages, each built with PKT-3036's "basic, then advanced"
  mechanics.

T3 — split (§5.5):
1. `qt: preferences: add the thirteen-category presentation map` (3039a, map + tests, no UI change).
2. `qt: preferences: show the category sidebar` (3039b, sidebar replaces the icon row; pages from the map).
3. `qt: preferences: move curated options to General, Privacy and Controls` (3039c, overrides list).

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `include/vlc_plugin.h:150-217` (`enum vlc_config_cat`, `enum vlc_config_subcat`) — 7 categories (`CAT_INTERFACE`=1 …
  `CAT_PLAYLIST`=7) and 33 subcategories (`SUBCAT_INTERFACE_GENERAL`=101 … `SUBCAT_PLAYLIST_EXPORT`=703) plus
  `HIDDEN`/`UNKNOWN`. `:219-224` the plugin ABI version note ("must be synchronized with
  src/rust/vlcrs-macros/module.rs").
- `src/config/cat.c:34-43` (`categories_array`) — category → general subcat → help; `:45-81`
  (`subcategories_array`) — subcat → parent category, title (`N_()`), help. `:83-131` the lookup API
  (`vlc_config_cat_Find`, `_GetAt`, `vlc_config_subcat_Find`, `_GetAt`, `_Count`).
- `include/vlc_config_cat.h:31-47` — category titles (`INTF_TITLE` "Interface", `INPUT_TITLE` "Input / Codecs",
  `PLAYLIST_TITLE` "Play Queue", `AADVANCED_TITLE` "Advanced", `HOTKEYS_TITLE`, `ML_TITLE`); `:202-266` inline helpers
  (`vlc_config_cat_GetGeneralSubcat`, `vlc_config_cat_FromSubcat`, `vlc_config_subcat_IsGeneral`).
- `src/libvlc-module.c` — 343 `add_*` option lines; subcat markers at `:1610` (AUDIO_GENERAL), `:1670`
  (VIDEO_GENERAL), `:1800` (VIDEO_SUBPIC), `:1848` (INPUT_GENERAL), `:2065` (SOUT_GENERAL), `:2104`
  (ADVANCED_MISC), `:2145` (PLAYLIST_GENERAL), `:2239` (INTERFACE_GENERAL), `:2273` (INTERFACE_HOTKEYS) …;
  cross-cutting options the designer moves: `metadata-network-access` `:2196` (Privacy), `network-caching` `:2010`
  (Network), `dec-dev` `:2056` (Performance), `verbose` `:2241` and `stats` `:2255` (General → Diagnostics).
- `modules/gui/qt/dialogs/preferences/simple_preferences.hpp:52-62` — `SPrefs*` enum (7 panels) that becomes 13.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:223-273` (`SPrefsCatList`) — horizontal row of
  `QToolButton`s with 48 px PNG icons (`:257-264`); the sidebar replaces it.
- `modules/gui/qt/dialogs/preferences/complete_preferences.cpp:47-180` (`PrefsTree` ctor) — builds the All tree from
  core's subcat markers and each plugin's first subcat (`:102-150`); it keeps core's taxonomy (unchanged by path B).
- Other consumers of the core taxonomy (unchanged by path B, all touched by path A):
  `modules/gui/macosx/preferences/prefs.m:116-133`, `modules/gui/skins2/src/skin_main.cpp`, `modules/gui/ncurses.c`,
  `modules/gui/minimal_macosx/macosx.c`, `src/config/help.c` (`vlc -H` sections).
- Count for path A (command in Step 2): `grep -rhoE "set_subcategory\s*\(\s*[A-Z_]+" modules src` → 581 hits;
  largest buckets `SUBCAT_INPUT_ACCESS` 62, `SUBCAT_VIDEO_VOUT` 62, `SUBCAT_INPUT_DEMUX` 65, `SUBCAT_VIDEO_VFILTER`
  68, `SUBCAT_INPUT_VCODEC` 39, `SUBCAT_AUDIO_AFILTER` 31.

Settings / variables involved: none new. The map is code, not an option.
Mock boundary: `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.hpp:46-66`) for the real core option set and module list.

### Overlaps & contradictions
- **PKT-3036** provides the page mechanics (basic panel + Advanced options) and its `panelSubcats()` table; this
  packet replaces that table with the 13-category map. Must merge after PKT-3036b.
- **PKT-3035** keeps a Hotkeys panel; in the map it becomes part of **Controls** (with mouse wheel and remotes).
- **PKT-3038** puts theme + accent in **General**.
- **PKT-3040** owns the **Extensions** page content (add-ons list) and moves the internal plugin list to the advanced
  area; this packet only reserves the page.
- **PKT-3002** supplies the monochrome sidebar icons; until then the existing `prefsmenu` PNGs are reused.
- **PKT-3027/3033** (library sections) decide Library vocabulary; the Library page reuses the ML panel as-is.

## 2b. Upstream state
No grouping beyond core's seven categories exists; the Simple view is a hand-made subset. VideoLAN's taxonomy is a
core API (`vlc_config_cat.h`) used by four interfaces and the CLI help; changing it (path A) is a cross-project
decision, which is why path B is proposed. Path B is a Qt presentation choice but still changes how every user finds
settings, hence `upstream_ruling: needed`.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PrefsCategoryMap::categoryOf(subcat)` | every subcat from `vlc_config_subcat_GetAt(0..Count-1)` except `SUBCAT_HIDDEN` | map | each maps to exactly one of the 13 ids; no subcat unmapped | `tests/test_prefs_category_map.cpp::testEverySubcatMapped` |
| 2 | `categoryOf(option)` | every non-hidden option of the main module (`module_config_get(module_get_main())`) | map | each lands in one category; overrides win over the subcat default | `::testEveryCoreOptionPlaced` |
| 3 | override list | the static override table | `config_FindConfig(name)` for each | non-NULL for all (no typo, no obsolete option) | `::testOverridesExist` |
| 4 | groups | the 13 ids | `groupOf(id)` | App = {General, Interface, Privacy, Controls}, Media = {Playback, Video, Audio, Subtitles, Library}, System = {Network, Performance, Recording, Extensions}, in that order | `::testGroupsAndOrder` |
| 5 | plugins | every module in `module_list_get` with options | map by first subcat (as `PrefsTree` does) | lands in a category; none in General/Privacy | `::testPluginsPlaced` |
| 6 | sidebar | running UI | open preferences | 13 entries under App/Media/System, keyboard navigable, each page non-empty | MANUAL (screenshot per group) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75` (`VLCTestingEnv`), meson block
  `modules/gui/qt/meson.build:1404-1423`. The map is GUI-less → `QTEST_GUILESS_MAIN`.
- Stubs: `tests/vlc_stub_modules.cpp`; row 5 needs the real plugin list — `VLC_PLUGIN_PATH` is set by the meson test
  environment; Step 2 checks the stub env loads the module bank, else row 5 runs over the core module only and is
  marked partial.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R2): Path B: a Qt presentation map, core taxonomy untouched, split 3039a/b/c as written. Path A (core remap: 578 active calls / 518 files, enum + `cat.c` + Rust macros + macOS + ABI bump) vs path B
   (Qt presentation map, core untouched). Recommendation: B. A would also break out-of-tree plugins built against the
   current enum values and change `vlc -H` for everyone.
2. **RULED** RULED 2026-10-14 (R0): the proposed default map is accepted; the final per-setting overrides come with 3039c (see the NOTE below). Default map (proposal): INTERFACE_GENERAL, INTERFACE_MAIN → Interface; INTERFACE_HOTKEYS,
   INTERFACE_CONTROL → Controls; AUDIO_* → Audio; VIDEO_GENERAL/VOUT/VFILTER/SPLITTER → Video; VIDEO_SUBPIC →
   Subtitles; INPUT_GENERAL, PLAYLIST_GENERAL → Playback; INPUT_ACCESS, ADVANCED_NETWORK → Network;
   INPUT_DEMUX/VCODEC/ACODEC/SCODEC/STREAM_FILTER, ADVANCED_MISC → Performance; SOUT_* → Recording;
   PLAYLIST_SD, PLAYLIST_EXPORT and module `medialibrary` → Library; General, Privacy and Extensions are filled by
   overrides and by PKT-3040. Needs the designer's full per-setting list from the Figma file to finalise 3039c.
3. **NOTE** — The All/Expert views keep core's taxonomy (search by core names keeps working, docs stay valid).
4. **NOTE** — "Stream output" naming: the designer's "Recording" covers sout + casting (`SUBCAT_SOUT_RENDERER`);
   the record-path options (`input-record-path`, in INPUT_GENERAL) get an override to Recording.
5. **NOTE (split)** — 3039a map + rows 1–5 (T2, ~12 h); 3039b sidebar + pages (T2, ~16 h, row 6); 3039c override
   curation with the Figma list (T2, ~12 h). Each one MR, in that order.
6. NOTE — **Missing information (keeps `status: planned`)**: 3039c needs the designer's per-setting list from the Figma file (community file 1691465999134370980): for each core setting, the category of the thirteen (Interface, Controls, Audio, Video, Subtitles, Playback, Network, Performance, Recording, Library, General, Privacy, Extensions) it belongs to when it differs from the default map of §5.2. 3039a and 3039b can start from the default map; 3039c cannot be written without that list.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, branch `master-pkt-3039-thirteen-category-map` from the tip that
      contains PKT-3036b (else `ce4af2f`); §6 command once → green. Ruling §5.1 (R2, path B) is recorded.
- [ ] **Step 2: Dry-run** — re-read §2; re-run the path-A count
      `grep -rhoE "set_subcategory\s*\(\s*[A-Z_]+" modules src | wc -l` and update §1 if it moved.
- [ ] **Step 3 (3039a): Row 1 watched failing** — `tests/test_prefs_category_map.cpp`,
      `test_qt_prefs_category_map` in `meson.build`; empty map → FAIL; fill the subcat table of §5.2.
- [ ] **Step 4 (3039a): Rows 2–5** — one cycle each; override table starts with the five options named in §2.
      Commit `qt: preferences: add the thirteen-category presentation map`; MR.
- [ ] **Step 5 (3039b)** — `SPrefsCatList` → vertical sidebar with three group labels; `SPrefs*` enum → 13 ids;
      pages built by PKT-3036's merge from the map; row 6. Commit `qt: preferences: show the category sidebar`; MR.
- [ ] **Step 6 (3039c)** — curate overrides from the Figma list; each moved option keeps one home (row 2 guards it).
      Commit `qt: preferences: move curated options to General, Privacy and Controls`; MR.
- [ ] **Step 7: Whole gate** — §6 command per MR; summaries in §6 Evidence; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
