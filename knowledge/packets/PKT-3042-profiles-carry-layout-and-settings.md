---
id: PKT-3042
title: "Named profiles that switch layout and a chosen set of settings together"
section: "E. Customization"
video_ref: "19:28–19:36"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/dialogs/toolbar + modules/gui/qt/maininterface + modules/gui/qt/menus
branch: master-pkt-3042-profiles-carry-layout-and-settings
base_sha: ce4af2f
target_files:
  - modules/gui/qt/util/user_profile_model.hpp                # new
  - modules/gui/qt/util/user_profile_model.cpp                # new
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp
  - modules/gui/qt/maininterface/mainctx.hpp
  - modules/gui/qt/maininterface/mainctx.cpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/menus/custom_menus.cpp
  - modules/gui/qt/tests/test_controlbar_profile_model.cpp   # new
  - modules/gui/qt/tests/test_user_profile_model.cpp         # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 26.0
priority: P3
depends_on: [PKT-3041]
upstream_ruling: needed
target: fork
mr_title: "qt: add user profiles that bundle a layout with interface settings"
status: ready
---

# PKT-3042 — Named profiles that switch layout and a chosen set of settings together

## 1. Scope & intent
At 19:28–19:36 the designer wants to save different layouts under different VLC profiles, "so all your settings
change with every profile". Frame s022 (bottom-left) shows the main menu with **Profile (Movies) ›** opening
*Movies ✓ / Shows / Music / Work / + New*.

Master has profiles, but **only for the control bar layout** (§2b): `ControlbarProfileModel` stores named button
layouts in the Qt settings file. Core settings (vlcrc) have no profile concept; the only alternative is starting VLC
with `--config <file>`, which needs a restart. Switching *all* settings live is not possible without a core redesign
(modules read options when they open). The packet therefore defines a profile as:

- a **name**, the **control bar profile** it selects, and
- a **curated set of options** that are safe to change at runtime and that make sense per use case: colour scheme,
  accent colour (PKT-3038), pinned controls, `qt-minimal-view`, start/stop behaviour (`play-and-pause`…), preferred
  audio/subtitle languages (`audio-language`, `sub-language`), audio normalisation, playback speed default.

Switching a profile writes those options (`config_Put*`), reloads the Qt side (`MainCtx::reloadPrefs`), selects the
control bar profile, and saves. A "+ New" entry snapshots the current values under a name.

T3 — split (§5.5):
1. `qt: cover ControlbarProfileModel save and reload with tests` (3042a).
2. `qt: add UserProfileModel bundling a layout with interface settings` (3042b).
3. `qt: add a Profile submenu to the main menu` (3042c).

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.hpp:28-128` (`ControlbarProfileModel`) — list model of
  `ControlbarProfile*`, `selectedProfile`, `cloneSelectedProfile`, `newProfile`, `deleteSelectedProfile`,
  `generateProfileFromStyle`, `save()`/`reload()`; `Style` enum `:47-55` (Empty, Default, Minimalist, One-liner,
  Simplest, Classic).
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:30-37` — QSettings keys `SelectedProfile`,
  `Profiles` array, `Name`, `Model`, separators `,` `|` `$`; `:39-380` the built-in styles (names `:44`, `:98`, `:174`,
  `:242`, `:320`, `:377`); `:491-508` (`insertDefaults`).
- `modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp:696-750` (`save`) — clears and rewrites group
  `ControlbarProfileModel`; `:752-850` (`reload`) — parses it back, falls back to profile 0.
- `modules/gui/qt/maininterface/mainctx.cpp:220` — the model lives on `MainCtx` with `p_intf->mainSettings`;
  `modules/gui/qt/qt.cpp:973-980` — that `QSettings` is `vlc-qt-interface` (ini on Windows, native elsewhere).
- `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorDialog.qml:59-72`, `:90-184` — profile UI (select, clone, new,
  delete) exists only inside the editor window.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:150-167` — first run selects or inserts a style profile
  (`findModel` → `setSelectedProfile` / `insertProfile`): the pattern this packet's apply step follows.
- `src/libvlc-module.c:2878-2879` — `add_string( "config", NULL, … ) change_volatile()`; `src/config/file.c:55-69`
  (`config_GetConfigFile`) — the vlcrc path comes from `--config` or the user config dir, read once at start.
- `modules/gui/qt/maininterface/mainctx.cpp:511-514` (`reloadPrefs`) → `loadPrefs(true)`: the Qt-side refresh after
  `config_Put*`, used by `PrefsDialog::save` (`modules/gui/qt/dialogs/preferences/preferences.cpp:371-372`).
- `modules/gui/qt/menus/menus.cpp:354` — `CheckableListMenu` over a list model (used for Color Scheme): the widget
  that renders the Profile submenu from `UserProfileModel` with no new menu code
  (`modules/gui/qt/menus/custom_menus.cpp`).
- No test covers `ControlbarProfileModel` today (`modules/gui/qt/tests/` holds base model, ML model, navigation,
  renderer, thread runner, dialog model, FSM).

Settings / variables involved: the curated list (§5.2), each checked with `config_FindConfig` in row 5; QSettings
groups `ControlbarProfileModel` and new `UserProfiles`.
Mock boundary: `VLCTestingEnv` for `config_*`; a `QSettings` on a temporary ini file (`QTemporaryDir`) instead of
`mainSettings`; `MainCtx` is not instantiated — the model takes a `std::function` for "reload prefs" and a pointer to
`ControlbarProfileModel`.

### Overlaps & contradictions
- **PKT-3041** edits layouts in place; a profile references a layout by name, so layouts edited in place are
  captured. Depends on it for the UX, not for code.
- **PKT-3038** adds `qt-accent-color` to what a profile can carry.
- **PKT-3013** decides the menu where "Profile ›" lives; this packet adds it to `ViewMenu` next to Color Scheme.
- **PKT-3022** (per-show subtitle/audio presets) is per-media, not per-user; both may set `sub-language` — a
  per-show preset wins while that show plays. Documented, not coded here.

## 2b. Upstream state
Shipped: named control bar profiles with five built-in styles, clone/new/delete and persistence; selecting a profile
changes the bar live. Not shipped: anything that ties other settings to a profile, and any profile UI outside the
editor window. `--config` gives full-vlcrc profiles at the cost of a restart.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `ControlbarProfileModel::save/reload` | temp `QSettings`, defaults inserted, profile 2 selected, one button moved | `save()`, new model on the same file, `reload()` | same count, names, selected index and control lists | `tests/test_controlbar_profile_model.cpp::testSaveReloadRoundTrip` |
| 2 | same | corrupt `Model` string in the file | `reload()` | profile skipped or emptied, no crash, selection falls back to 0 | `::testReloadCorruptValue` |
| 3 | `UserProfileModel::createFromCurrent` | `VLCTestingEnv`, `sub-language="fr"`, colour scheme Night | create "Movies" | stored values equal current ones; layout name = selected control bar profile | `tests/test_user_profile_model.cpp::testSnapshot` |
| 4 | `UserProfileModel::apply` | two profiles with different `sub-language` and layouts | apply "Music" then "Movies" | `config_GetPsz("sub-language")` follows; control bar selection follows; reload callback called once per apply | `::testApplySwitches` |
| 5 | curated list | — | `config_FindConfig` on every key | all exist; none is `change_volatile`/private | `::testCuratedOptionsExist` |
| 6 | persistence | profiles created | save, new model, load | same profiles and active one | `::testPersistence` |
| 7 | menu | running UI | View → Profile › Music, then + New… | checked item follows; settings and bar change without restart | MANUAL (screenshots) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75` (`VLCTestingEnv`) for rows 3–6; rows 1–2
  need no libvlc — pattern `tests/test_thread_runner.cpp` with `QTemporaryDir` + `QSettings(path, IniFormat)`.
- Meson blocks next to `test_qt_dialog_model` (`modules/gui/qt/meson.build:1404-1423`); `test_qt_controlbar_profile_model`
  compiles `dialogs/toolbar/controlbar_profile*.cpp` and `player/control_list_model.cpp`,
  `player/player_controlbar_model.cpp`.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): a curated runtime-safe set of settings per profile. Scope of "all your settings". Proposal: curated runtime-safe set (above). Alternative: full vlcrc per
   profile via `--config` + restart prompt — complete but a restart per switch, and modules' caches (e.g. plugin
   cache) are per install anyway. 
2. **RULED** RULED 2026-10-14 (R0): the first-draft curated list is accepted; Step 2 verifies each entry applies live. Curated list content (first draft): `qt-accent-color`, colour scheme (QSettings),
   `qt-pin-controls`, `qt-minimal-view`, `play-and-pause`, `audio-language`, `sub-language`, `audio-filter`
   normvol flag, `rate`. Every entry must apply live; Step 2 verifies each one's consumer re-reads it.
3. **NOTE** — Built-in profiles named after the mock-up (Movies, Shows, Music, Work) are **not** created by default:
   one "Default" profile wraps today's state, so nothing changes for users who never open the menu.
4. **NOTE** — Storage in the Qt settings file, group `UserProfiles`, like the control bar profiles; vlcrc stays a
   single file.
5. **NOTE (split)** — 3042a rows 1–2 (T1, ~4 h, useful on its own); 3042b model + rows 3–6 (T2, ~14 h); 3042c menu +
   "New…" dialog + row 7 (T2, ~8 h).

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3042-profiles-carry-layout-and-settings ce4af2f`;
      §6 command once → green. 3042b/c follow ruling §5.1.
- [ ] **Step 2: Dry-run** — re-read §2; for each option of §5.2 find its consumer and confirm it re-reads the value
      at runtime (`var_Inherit*` at use time) — drop any that is read only at open.
- [ ] **Step 3 (3042a): Row 1 watched failing** — `tests/test_controlbar_profile_model.cpp`, meson block; wrong
      expected count → FAIL, then right; row 2. Commit `qt: cover ControlbarProfileModel save and reload with tests`; MR.
- [ ] **Step 4 (3042b): Rows 3–6** — `util/user_profile_model.{hpp,cpp}` (list model: name, layout name, values map,
      `active`); `apply()` = `config_Put*` per value + control bar `setSelectedProfile` + reload callback +
      `config_SaveConfigFile`. One TDD cycle per row. Commit `qt: add UserProfileModel bundling a layout with interface settings`; MR.
- [ ] **Step 5 (3042c)** — `MainCtx` owns the model; `ViewMenu` (`modules/gui/qt/menus/menus.cpp:354`) gets
      `CheckableListMenu(qtr("&Profile"), …)` + "New profile…" (`DialogsProvider::getTextDialog`, as
      `modules/gui/qt/dialogs/toolbar/qml/ToolbarEditorDialog.qml:63-66`). Row 7. Commit `qt: add a Profile submenu to the main menu`; MR.
- [ ] **Step 6: Whole gate** — §6 command per MR; summaries in §6 Evidence; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
