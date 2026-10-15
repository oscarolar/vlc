---
id: PKT-3022
title: "Per-show presets that remember subtitle/audio defaults, delays and appearance"
section: "C. Subtitles"
video_ref: "07:46–08:41"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3022-per-show-presets
base_sha: ce4af2f
target_files:
  - modules/gui/qt/player/player_preset_store.cpp
  - modules/gui/qt/player/player_preset_store.hpp
  - modules/gui/qt/player/qml/PresetSelector.qml
  - modules/gui/qt/player/qml/TrackDefaultsSection.qml
  - modules/gui/qt/maininterface/mainui.cpp
  - modules/gui/qt/tests/test_player_preset_store.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 26.0
priority: P2
depends_on: [PKT-3021, PKT-3023]
upstream_ruling: needed
target: fork
mr_title: "qml: save and apply per-show player presets"
status: ready
---

# PKT-3022 — Per-show presets that remember subtitle/audio defaults, delays and appearance

## 1. Scope & intent
07:46–08:01: once delay and formatting are right, the designer does not want to redo them every episode:
"save all these settings for the specific show through a preset", then pick the preset later and everything
comes back. 08:01–08:41: the same for the **subtitle track** — today the default track is set in *Input/Codecs*
deep in advanced preferences (frame `s010` ≈08:12–08:20 shows *Advanced Preferences → Input / Codecs → Track
settings* with *Subtitle track*, *Subtitle language*, *Subtitle track ID* boxed in red), "oddly irrelevant"
when the subtitle options live elsewhere. Target (frames `s009` ≈07:58 and `s010` ≈08:30–08:40): at the top
of the Subtitles tab a **Defaults** section with *Subtitle Track* (`1`) and *Subtitle Language* (`English`);
at the bottom a dropdown **"The Wand Subtitle Preset"** that saves/loads the tab. 08:39–08:44: "the same things
apply to audio" (default audio track, audio delay) — the Audio tab gets the same Defaults rows (PKT-3023 draws
them, this packet stores them).

This is **T3**, split in §5.6. Commits (per split): `qml: add track defaults to the settings sidebar`,
`qt: add PlayerPresetStore`, `qml: save and apply per-show player presets`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `src/libvlc-module.c:1861-1885` — `audio-track`, `sub-track` (index, `-1`), `audio-language`,
  `sub-language`, `menu-language` (comma lists of ISO codes), `audio-track-id`, `sub-track-id`; all
  `change_safe()`, section *Track settings* of `SUBCAT_INPUT_GENERAL` — the "Input/Codecs" location the video
  complains about.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:530` and `:950-951` — **simple** preferences
  already expose *Preferred audio language* (Audio panel) and *Preferred subtitle language* (Subtitles/OSD
  panel). The video's search result is the advanced path; the language default is reachable in simple prefs,
  the track **index** is not.
- `src/libvlc-module.c:1932-1933` — `restore-playback-states` (bool, default **false**): "resume the last
  playback state, such as the selected tracks, rate, aspect-ratio".
- `src/player/medialib.c:30-126` (`vlc_player_input_RestoreMlStates`) — with the option on, for a **video** media
  known to the media library, restores rate, video/audio/subtitle track string ids, zoom, aspect ratio,
  deinterlace, video filter (`:79-125`). `:171-293` (`vlc_player_UpdateMLStates`) saves them on stop.
- `include/vlc_media_library.h:677-711` — `enum vlc_ml_playback_state` includes `AUDIO_DELAY`, `SUBTITLE_DELAY`,
  `GAIN`, `APP_SPECIFIC` (`:691-695`) but `vlc_ml_playback_states_all` (`:698-711`) carries neither delay; the
  core never saves delays.
- `modules/gui/macosx/library/VLCLibraryDataTypes.m:1850-1881` — the macOS interface stores audio delay,
  subtitle track and subtitle delay **per media** in those ML states: a VideoLAN precedent for per-media delay
  persistence from the UI.
- `include/vlc_media_library.h:129-139` (`vlc_ml_show_t`) and `:249-271` (`vlc_ml_group_t`) — the ML has
  shows (TVDB-style) and user-visible **video groups**; `modules/gui/qt/medialibrary/mlvideogroupsmodel.hpp:30`
  is the Qt model of groups. A "show" can be keyed by group id (library media) or by folder (non-library).
- `modules/gui/qt/player/player_controller.hpp:162-172` — track list models and delay properties a preset
  applies through.

Settings / variables involved: `sub-language` (`src/libvlc-module.c:1870`), `audio-language` (`:1867`),
`sub-track` (`:1864`), `audio-track` (`:1861`), `restore-playback-states` (`:1932`), plus the PKT-3021
`freetype-*`/`sub-text-scale` set and `subsdelay-factor`.
Mock boundary: `PlayerPresetStore` tested with a temp `QSettings` file (or a stubbed ML per §5.1) and a fake
"apply target" interface instead of the real `PlayerController`; no media playback.

### Overlaps & contradictions
- **PKT-3017 §5.4** — the same persistence ruling (R5) ("remember for future episodes").
- **PKT-3021** exposes `snapshot()/apply()` for appearance; **PKT-3019** delay; **PKT-3023** audio defaults and
  equalizer preset. This store aggregates them; it owns no UI of theirs.
- **PKT-3042** (profiles carrying layout and settings) is a different, global notion; names must not collide
  ("preset" here, "profile" there).
- `restore-playback-states` overlaps per-media; a preset is per-show. Precedence (§5.3).

## 2b. Upstream state
- Global defaults for subtitle/audio **language** exist and are in simple preferences already; default track by
  index exists only in advanced.
- Per-media memory of tracks/rate/zoom/AR exists in core (off by default), not of delays or subtitle style.
- No per-show or named preset anywhere.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PlayerPresetStore::save` | empty store | `save("The Wand", snapshot)` | `presets()` lists it; reload from storage gives the same snapshot | `test_player_preset_store.cpp::testSaveAndReload` |
| 2 | `bind(showKey, name)` | preset saved | bind group id 42 / folder `/v/The Wand` | `presetFor(key)` returns it | `test_player_preset_store.cpp::testBindToShow` |
| 3 | `applyFor(item)` | item in a bound folder, preset with `subLanguage "eng"`, `subDelayMs -2000`, `textScale 120` | applied to a fake target | target receives language, delay and scale; untouched fields untouched | `test_player_preset_store.cpp::testApplyOnlySetFields` |
| 4 | `applyFor(item)` | item with no binding | applied | nothing applied, `applied == false` | `test_player_preset_store.cpp::testNoBindingNoChange` |
| 5 | precedence | preset binds delay −2 s, ML per-media state has delay −1.5 s | apply | per-media wins (most specific), per §5.3 | `test_player_preset_store.cpp::testPerMediaBeatsPreset` |
| 6 | `remove/rename` | two presets | rename A → C, remove B | bindings follow the rename; B's bindings dropped | `test_player_preset_store.cpp::testRenameAndRemove` |
| 7 | Defaults section | Subtitles tab | set Subtitle Language "English" | `sub-language` = `en` (written only on "Save as default" per PKT-3021 §5.4) | MANUAL — `pkt3022-defaults.png` |
| 8 | preset dropdown | two episodes of one folder | save preset on E01, open E02 | E02 starts with the preset applied; OSD "Preset The Wand applied" | MANUAL — `pkt3022-preset-applied.webm` |

## 4. Fixtures & mocks
- Pattern: `modules/gui/qt/tests/test_navigation_model.cpp` + `modules/gui/qt/meson.build:1342-1360`.
- Storage per §5.1: `QSettings` in a `QTemporaryDir` (option a), or the ML stub of
  `modules/gui/qt/tests/vlc_stub_modules.cpp:122-140` extended to accept `VLC_ML_MEDIA_SET_PLAYBACK_STATE` (option b).
- The apply target is an interface (`PresetTarget`) implemented by `PlayerController` glue in production and by a
  recording fake in tests — no `PlayerController` in the test binary.

## 5. Design decisions & open questions
1. **RULED — storage.** RULED 2026-10-14 (R5): preset bodies live in the Qt settings file (group `Presets/`), keyed by the media-library group/media when the video is indexed and by the parent folder otherwise; works with and without the library. (a) Qt settings (`vlc-qt-interface.conf`, group `Presets/`), works without the media
   library, not shared with other interfaces; (b) media library (`APP_SPECIFIC` state on each media of a group,
   or a new ML table for presets), shared and survives reinstall, needs ML and possibly a libmedialibrary change.
   Recommend (a) for the preset bodies + (b)-style keys when the media is in a group. 
2. **RULED — what is a "show".** RULED 2026-10-14 (R5): a "show" is the media-library group/media when the video is indexed and the parent folder otherwise. ML video group (library) or the parent folder (no library). Matching by
   ML `vlc_ml_show_t` depends on metadata the library may not have; groups are user-visible and editable.
3. **RULED — precedence.** RULED 2026-10-14 (R0): precedence per-media > preset > global; turning a preset on implies saving per-media delays. Per-media restored state (`restore-playback-states`) vs per-show preset vs global
   default. Proposed: per-media > preset > global; and turning a preset on implies saving per-media delays
   (macOS precedent, `VLCLibraryDataTypes.m:1850-1881`).
4. NOTE — the Defaults rows write `sub-language`/`audio-language` (language codes, robust across files) rather
   than `sub-track` (index, differs per file); the "Subtitle Track 1" row of the frame is offered as
   "Track number" only inside a preset.
5. NOTE — moving the language defaults next to the tracks answers 08:12–08:30 without touching the preferences
   tree (that is PKT-3039's job).
6. **Split (T3):** **3022a** Defaults section (languages, `restore-playback-states` switch) — rows 7, 4 h,
   `upstream_ruling: not-needed`; **3022b** `PlayerPresetStore` — rows 1–6, 12 h, rulings R5/R0 of §5.1–5.3;
   **3022c** preset dropdown + auto-apply on item change — row 8, 10 h.

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
- [ ] **Step 0: Split** — write 3022a/b/c from §5.6; 3022b/c are unblocked: §5.1–5.3 are ruled.
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: branch `master-pkt-3022-per-show-presets` (suffix per split) from merged PKT-3021/3023; build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2; verify whether `restore-playback-states` restores anything for a media not in the library (expect: no, `src/player/medialib.c:47-55`).
- [ ] **Step 3 (3022a)** — `player/qml/TrackDefaultsSection.qml` (Subtitle/Audio language with a language picker, "Remember settings per video" switch bound to `restore-playback-states`); MANUAL row 7; commit `qml: add track defaults to the settings sidebar`.
- [ ] **Step 4 (3022b): Row 1 watched failing** — `tests/test_player_preset_store.cpp` against an empty `player/player_preset_store.{hpp,cpp}` registered in `meson.build` → FAIL; rows 2–6; commit `qt: add PlayerPresetStore`.
- [ ] **Step 5 (3022c)** — `player/qml/PresetSelector.qml` at the bottom of the Subtitles/Audio tabs (save, rename, delete, bind to this show); apply on `Player` item change; MANUAL row 8; commit `qml: save and apply per-show player presets`.
- [ ] **Step 6: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 7: MRs** — MRs per split to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
