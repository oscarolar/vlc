---
id: PKT-3024
title: "Video tab: flip, rotate, zoom, aspect ratio, crop and filters with previews"
section: "C. Video"
video_ref: "08:56–09:07"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3024-video-tab-transforms
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/video_transform_model.cpp
  - modules/gui/qt/player/video_transform_model.hpp
  - modules/gui/qt/player/qml/VideoSettingsPage.qml
  - modules/gui/qt/player/qml/VideoFilterGrid.qml
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/tests/test_video_transform_model.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 22.0
priority: P2
depends_on: [PKT-3018]
upstream_ruling: needed
target: fork
mr_title: "qml: video tab with transforms and filters in the settings sidebar"
status: ready
---

# PKT-3024 — Video tab: flip, rotate, zoom, aspect ratio, crop and filters with previews

## 1. Scope & intent
08:54–09:07: "the same exact thing for video filters too … you need to see a preview of whatever you're
doing", plus **flip, rotate, zoom, HDR, resolution, aspect ratio** and similar settings in the Video tab. Frames
`s010` (≈08:58, bottom right) and `s011` (≈09:00, top left) show the target page *Defaults*: **Flip** as a
three-way segmented control `None | Horizontal | Vertical` (None in orange), **Rotate** `0 deg` field, then
dropdowns **Zoom** `1:1 Original`, **HDR** `Auto`, **Resolution** `Best Available`, **Aspect Ratio** `Default`,
**Crop** `Default`, **Render** `Local`. The filter list with previews is described but not drawn.

Before: zoom/aspect/crop/deinterlace are reachable from the Video menu and an optional bar widget; flip/rotate
and all filters live in *Adjustments and Effects → Video Effects* (Widgets, checkbox per filter, no preview).
After: a Video page in the sidebar with the controls above, wired to existing player models and a new
`VideoTransformModel` for flip×rotate; a filter grid with preview tiles (§5.2).

T3, split in §5.6. Commits: `qt: add VideoTransformModel`, `qml: video tab with transforms in the settings
sidebar`, `qml: video filter grid with previews`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `modules/gui/qt/player/player_controller.hpp:201-211` — `hasVideoOutput`, `videoStereoMode`, `zoom`,
  `aspectRatio`, `crop`, `fit`, `deinterlace`, `deinterlaceMode` as `VLCVarChoiceModel` (vout variable choice
  lists), `fullscreen`, `wallpaperMode`, `autoscale`: Zoom, Aspect Ratio and Crop of the frame are **already**
  QML-bindable models.
- `modules/gui/qt/player/qml/controlbarcontrols/AspectRatioWidget.qml:25-40` — an existing `ComboBoxExt` on
  `Player.aspectRatio` with `toggleIndex` — the exact control to reuse for the dropdowns.
- `modules/video_filter/transform.c:60-96` (`Open`) — in VLC 4 the transform filter only **changes the output
  orientation** (`dst->orientation = video_format_GetTransform(...)`) and does not touch pixels: cheap, any
  combination is one orientation.
- `modules/video_filter/transform.c:106-125` — `transform-type` is a single choice among `90, 180, 270, hflip,
  vflip, transpose, antitranspose`. With identity, those are the 8 elements of the dihedral group: every
  *flip × rotate(0/90/180/270)* the frame offers maps to exactly one value (e.g. horizontal + 90° = `transpose`
  or `antitranspose` depending on convention) — the model's whole job.
- `modules/gui/qt/dialogs/extended/extended_panels.cpp:154-282` (`ExtVideo::ExtVideo`) — the Widgets video
  effects: adjust, extract, colorthres, sepia, gradient, motionblur, sharpen, ripple, wave, **transform**
  (`:207-208`), **rotate** dial for free angles (`:210-213`), puzzle, …; `:338-349`
  (`changeVFiltersString`) toggles a module in `video-filter`/`sub-source` via `ChangeFiltersString` /
  `UpdateVFiltersString`.
- `src/player/medialib.c:96-122` — per-media restore (when `restore-playback-states` is on) already covers zoom,
  aspect ratio, crop, deinterlace and the `video-filter` string — the page's values are persisted per media by
  core for library media.
- `modules/video_output/libplacebo/utils.h:75-81` — HDR tone mapping is a **libplacebo vout** option
  (`*-tone-mapping-function`, `*-inverse-tone-mapping`), read when that vout opens; other vouts (D3D11 shaders,
  OpenGL) have their own or none. There is no player-level "HDR" variable.
- `src/libvlc-module.c:1889-1892` — `preferred-resolution` is an **input** option (adaptive stream variant
  choice: `-1` best available, 1080, 720, 576, 360, 240), applied at open.

Settings / variables involved: `zoom` (`src/libvlc-module.c:1773`), `aspect-ratio` (`:1753`), `crop` (`:1749`),
`video-filter` (`:1792`), `transform-type` (`modules/video_filter/transform.c:120`), `rotate-angle`
(`modules/video_filter/rotate.c:74`, float, default 30), `preferred-resolution` (`src/libvlc-module.c:1889`).
Mock boundary: `VideoTransformModel` is tested as a pure mapping plus a fake "filter string sink"; the dropdowns
reuse `VLCVarChoiceModel` and get MANUAL rows only.

### Overlaps & contradictions
- **PKT-3012** (video tracks tab) owns the track list; this page shows only settings.
- **PKT-3022** presets may save these values; core per-media restore already does (§2) — precedence in 3022 §5.3.
- The Widgets *Video Effects* tab stays for the long tail of filters and for "Save" to config.

## 2b. Upstream state
- Zoom, aspect ratio, crop, deinterlace: complete as models; only the page is missing.
- Flip/rotate: available as filters in the Widgets dialog (one choice at a time in a combo, so "flip **and**
  rotate" needs knowing that `transpose` exists). No QML.
- Filter previews: none. HDR: vout-specific, open-time. Resolution: input option for adaptive streams.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `VideoTransformModel::transformType()` | flip None, rotate 0 | read | `""` (no transform filter) | `test_video_transform_model.cpp::testIdentityHasNoFilter` |
| 2 | same | the 12 combinations flip {None,H,V} × rotate {0,90,180,270} | read | each maps to one of `{"", 90, 180, 270, hflip, vflip, transpose, antitranspose}` and composing the returned orientation equals flip∘rotate (table-driven, checked with `video_format_GetTransform`) | `test_video_transform_model.cpp::testAllCombinationsMapToOneType_data/…` |
| 3 | same | vertical + 180 | read | `hflip` (V∘180 = H) | covered by row 2 data |
| 4 | `apply()` | flip H | apply | `video-filter` chain contains `transform`, `transform-type == hflip`; other filters kept | `test_video_transform_model.cpp::testApplyEditsFilterChain` |
| 5 | `apply()` | back to identity | apply | `transform` removed from the chain | `test_video_transform_model.cpp::testIdentityRemovesFilter` |
| 6 | free rotate | rotate 15° | apply | `rotate` filter with `rotate-angle 15`, **not** `transform` | `test_video_transform_model.cpp::testFreeAngleUsesRotateFilter` |
| 7 | `VideoSettingsPage.qml` | sidebar Video tab | Flip Horizontal, Zoom 2:1, AR 16:9 | picture flips immediately, dropdowns reflect `Player.zoom`/`aspectRatio` current values | MANUAL — `pkt3024-video-tab.png` |
| 8 | `VideoFilterGrid.qml` | Video tab, Filters | hover "Sepia" tile | tile shows its preview; click toggles the filter | MANUAL — `pkt3024-filters.png` |

## 4. Fixtures & mocks
- Pattern: `modules/gui/qt/tests/test_navigation_model.cpp` + `modules/gui/qt/meson.build:1342-1360`; table rows with
  `QTest::addColumn` (`_data` function) for row 2.
- The model writes through a small sink interface; production sink = the same helpers `ExtVideo` uses
  (`modules/gui/qt/dialogs/extended/extended_panels.cpp:338-349`), test sink = a recorder. `video_format_GetTransform` is `VLC_API` (`include/vlc_es.h:513`, `src/misc/es_format.c:124`):
  the test links `libvlccore` like the other Qt tests; no libvlc instance needed.
- Sample media for MANUAL rows: any video.

## 5. Design decisions & open questions
1. NOTE — flip and rotate are one `transform` filter instance; the model owns the mapping so the user never
   sees "transpose".
2. **RULED — filter previews.** RULED 2026-10-14 (R0): option (a): static preview images bundled for the 6-8 common filters; the rest stay in the dialog. Live previews (current frame through each filter) need a frame grab and N
   filter chains — expensive and not available from the UI today. Options: (a) static preview images bundled per
   filter (assets, maintenance), (b) a snapshot of the current frame processed by each filter on demand
   (new core/Qt plumbing), (c) icons only. Recommend (a) for the 6–8 common filters (adjust/sepia/sharpen/
   gradient/motion blur/invert), the rest stay in the dialog. 
3. **RULED — HDR.** RULED 2026-10-14 (R0): HDR is out of scope here and its row stays omitted. No player-level HDR switch exists (`modules/video_output/libplacebo/utils.h:75-81` is vout-specific). Showing
   "HDR Auto" implies a new vout-agnostic variable; out of scope here, row omitted.
4. NOTE — "Resolution" maps to `preferred-resolution` and is shown only for adaptive streams (it applies at
   open: changing it re-opens the input, the UI says so).
5. NOTE — "Render Local" in the frame is the renderer (Chromecast) picker; PKT-3011's audio-output style
   belongs to the renderer button, not reimplemented here.
6. **Split (T3):** **3024a** `VideoTransformModel` + Flip/Rotate/Zoom/Aspect/Crop page, rows 1–7 (12 h,
   `upstream_ruling: not-needed`); **3024b** filter grid with previews, row 8 (10 h, after §5.2, ruled for static previews);
   HDR stays omitted per §5.3.

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
- [ ] **Step 0: Split** — write 3024a/b from §5.6.
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: branch `master-pkt-3024-video-tab-transforms` (suffix per split) from merged PKT-3018; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; read `include/vlc_es.h` for the orientation enum and `video_format_GetTransform`; fix the exact H+90 mapping in row 2's table.
- [ ] **Step 3: Row 1 watched failing** — `tests/test_video_transform_model.cpp` against an empty `player/video_transform_model.{hpp,cpp}` registered in `meson.build` → FAIL; rows 2–6; commit `qt: add VideoTransformModel`.
- [ ] **Step 4: Page** — `player/qml/VideoSettingsPage.qml` (segmented flip, rotate field, `ComboBoxExt` on `Player.zoom/aspectRatio/crop` like `modules/gui/qt/player/qml/controlbarcontrols/AspectRatioWidget.qml:25-40`); register the model in `mainui.cpp`; MANUAL row 7; commit `qml: video tab with transforms in the settings sidebar`.
- [ ] **Step 5 (3024b, after §5.2)** — `player/qml/VideoFilterGrid.qml`; MANUAL row 8; commit `qml: video filter grid with previews`.
- [ ] **Step 6: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 7: MRs** — PR to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
