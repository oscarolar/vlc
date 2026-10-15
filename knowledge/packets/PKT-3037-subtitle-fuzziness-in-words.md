---
id: PKT-3037
title: "Subtitle autodetection fuzziness is a list of words, not a number"
section: "E. Settings"
video_ref: "17:04–17:35"
pipeline: CI
kind: core
track: T1
component: src (libvlc-module.c) + modules/gui/qt/dialogs/preferences
branch: master-pkt-3037-subtitle-fuzziness-in-words
base_sha: ce4af2f
target_files:
  - src/libvlc-module.c
  - test/src/config/sub_autodetect_fuzzy.c                   # new
  - test/src/meson.build
  - modules/gui/qt/dialogs/preferences/sprefs_subtitles.ui
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
estimate_h: 4.0
actual_h: 0.3
priority: P2
depends_on: []
upstream_ruling: not-needed
target: upstream
mr_title: "core: name the subtitle autodetection fuzziness levels"
status: review
---

# PKT-3037 — Subtitle autodetection fuzziness is a list of words, not a number

## 1. Scope & intent
"Subtitle autodetection fuzziness" is a bare integer spin box in the All view (frame s020: value `3`, and a hover
tooltip listing `0 = no subtitles autodetected … 4 = subtitle file matching the movie name exactly`). At 17:04–17:35
the designer calls it the worst setting of all: the meaning of each number is only in a tooltip. The redesign shows
a drop-down "File name matching" in Subtitles → Loading Subtitles with **Off / Any title / Contains title / Title with
extra characters / Exact title match** (frame s020 centre and s021 top-centre).

The fix belongs in core, not in Qt: the option has no value list, so every interface (Qt All view, Expert view,
macOS, `vlc -H`) shows a number. Giving it `change_integer_list` with translatable texts makes Qt's generic factory
build a combo box by itself (`IntegerListConfigControl`, §2) and the help output print the names. The Qt simple
Subtitles panel then exposes it as a combo, so most users never need the All view for it.

One MR, two commits:
1. `core: name the subtitle autodetection fuzziness levels` — value list + texts + shorter longtext, core test.
2. `qt: show subtitle file name matching in the simple subtitles panel`.

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `src/libvlc-module.c:865-873` (`SUB_FUZZY_TEXT`, `SUB_FUZZY_LONGTEXT`) — the numbers are explained only in the
  long text.
- `src/libvlc-module.c:1819-1820` — `add_integer( "sub-autodetect-fuzzy", 3, … )`: no `change_integer_list`, no
  `change_integer_range`; any integer is accepted. Siblings `sub-autodetect-file` `:1817-1818` and
  `sub-autodetect-path` `:1826-1827`. The subcat is `SUBCAT_VIDEO_SUBPIC` (`:1799`), section "Subtitles" (`:1812`).
- `src/libvlc-module.c:422-423`, `:1841` — `pi_sub_align_values` / `ppsz_sub_align_descriptions` used with
  `change_integer_list`: the exact pattern to follow, in the same section.
- `include/vlc_plugin.h:528-532` (`change_integer_list`) — sets `list_count`, `list.i`, `list_text`.
- `src/input/subtitles.c:221-225` (`subtitles_Detect`) — `0` disables; `:288-318` assigns a priority per file:
  exact name → `SLAVE_PRIORITY_MATCH_ALL`, name followed only by blanks after a prefix → `MATCH_RIGHT`, name followed
  by more characters → `MATCH_LEFT`, no name in the main dir → `MATCH_NONE`; `:322` keeps files with
  `prio >= fuzzy`. `include/vlc_input_item.h:156-160` gives `NONE=1, RIGHT=2, LEFT=3, ALL=4, USER=5`.
- `src/input/item.c:1753-1756` and `:1603`, `:1631` — the directory-browse path (`vlc_readdir_helper`) uses the same
  threshold.
- `test/libvlc/slaves.c:149-195` — existing behaviour test for values 1–4 (labels there: 2 = "full, left, and
  right match", 3 = "full and left match", 4 = "full match"). The semantics are already covered; this packet adds no
  behaviour, only names.
- `modules/misc/medialibrary/fs/directory.cpp:207` — the media library forces `sub-autodetect-fuzzy=2` per item;
  unaffected (value unchanged).
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:119-123` (`ConfigControl::createControl`) — an
  `CONFIG_ITEM_INTEGER` with `list_count` becomes `IntegerListConfigControl` (combo), otherwise
  `IntegerRangeConfigControl` (spin box). The All view changes with zero Qt code.
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1023-1060` (`IntegerListConfigControl::finish`) — fills
  the combo from `list.i` / `list_text` (translated with `qfut`) and selects the current value.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:933-980` (`case SPrefsSubtitles`) — does not bind
  `sub-autodetect-fuzzy` or `sub-autodetect-file`; `modules/gui/qt/dialogs/preferences/sprefs_subtitles.ui:96-121` is the grid holding
  `subLangLabel`, `encoding`, `preferredLanguage` where a "File name matching" row fits.

Settings / variables involved: `sub-autodetect-fuzzy` (`src/libvlc-module.c:1819`), `sub-autodetect-file`
(`:1817`).
Mock boundary: none for the core row (real libvlc, real module bank); the Qt row is MANUAL.

### Overlaps & contradictions
- **PKT-3036** computes each simple panel's Advanced list; after this packet the option leaves the Subtitles
  Advanced list because the panel binds it. No code conflict.
- **PKT-3021/3022** (subtitle appearance, per-show presets) touch the same panel file; rebase order free, the hunks
  are distinct (`.ui` grid row vs. appearance widgets).
- **PKT-3017** (add-subtitles dialog) is about manual loading; no overlap with autodetection.

## 2b. Upstream state
Nothing names these levels in master; the value list does not exist anywhere (Qt, macOS, help). This is a missing
`change_integer_list`, bug-level, hence `upstream_ruling: not-needed`.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `sub-autodetect-fuzzy` config item | `libvlc_new` with `test_defaults_args` | `config_FindConfig("sub-autodetect-fuzzy")` | `list_count == 5`, `list.i == {0,1,2,3,4}`, every `list_text[i]` non-NULL and non-empty | `test/src/config/sub_autodetect_fuzzy.c::test_fuzzy_has_named_levels` |
| 2 | same | same | read `orig.i` | still `3` (default unchanged) | `::test_fuzzy_default_unchanged` |
| 3 | behaviour | existing | `meson test test_libvlc_slaves` | still green (values 1–4 unchanged) | `test/libvlc/slaves.c` (existing) |
| 4 | Qt All view and simple Subtitles panel | running UI | open Subtitles; open All → Video → Subtitles / OSD | a combo with the five names in both places, current = "Title with extra characters" | MANUAL (two screenshots) |

## 4. Fixtures & mocks
- Pattern to copy: `test/src/config/chain.c` (assert-style C test, `#include "../../libvlc/test.h"`) and its meson
  block `test/src/meson.build:1-6`; add `'link_with' : [libvlc, libvlccore]` because the module bank is needed
  (`libvlc_new` after `test_init()`, `test/libvlc/test.h:51-80`).
- Stubs: none. Sample media: none (behaviour stays covered by `test/libvlc/slaves.c` and its samples).

## 5. Design decisions & open questions
1. **NOTE** — Wording. The designer's labels, mapped to the code's real meaning: 0 "Off", 1 "Any subtitle file",
   2 "Contains the file name", 3 "File name with extra characters" (default), 4 "Exact file name match". "Title"
   (designer) is replaced by "file name": matching is on file names, not metadata titles — a "title" label would
   mislead. Texts are `N_()` and land in `po/vlc.pot` through the existing extraction.
2. **NOTE** — Values > 4 are accepted today and disable detection silently (nothing reaches priority 5 except user
   slaves). Adding `change_integer_range(0, 4)` would reject old configs with odd values at load; not done — the list
   alone is enough for the UIs, and the command line keeps accepting integers.
3. **NOTE** — `SUB_FUZZY_LONGTEXT` drops the "0 = … 4 = …" enumeration (now redundant) and keeps one sentence.
   Translators lose one long string and gain five short ones.
4. **NOTE** — Overlap with PKT-3036 handled automatically (§2 overlaps).

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --suite src --print-errorlogs && meson test -C build-meson test_libvlc_slaves --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `src/` or `modules/gui/qt`.
- Strings added are marked for translation (`N_` / `qtr`); no new file in `po/POTFILES.in` is needed for `src/`.
- Keyboard navigation and accessibility work on the new combo (label buddy set).

### Evidence
Executed 2026-10-15 on `nzxt`, worktree `~/src/vlc-wt/pkt-3037` (branch `master-pkt-3037-subtitle-fuzziness-in-words`
from `ce4af2f`), the CI image of §6, `meson setup build-meson
-Dextra_checks=true`, `ninja -j8`. Logs: `~/src/logs/pkt-3037-*.log`.

- Commits: `ab10a81` core: name the subtitle autodetection fuzziness levels; `3c8e008` qt: show subtitle file name
  matching in the simple subtitles panel.
- PR: https://github.com/oscarolar/vlc/pull/2 (fork, base `master`).
- Row 1/2 red (`pkt-3037-test-red.log`): `meson test -C build-meson test_src_config_sub_autodetect_fuzzy` → FAIL,
  `Assertion 'cfg->list_count == ARRAY_SIZE(expected)' failed` (SIGABRT).
- Row 1/2/3 green (`pkt-3037-test-green-core.log`): `meson test -C build-meson test_src_config_sub_autodetect_fuzzy
  test_libvlc_slaves` → Ok 2, Fail 0.
- Whole gate (`pkt-3037-gate.log`): `meson test -C build-meson --suite qt --suite src --print-errorlogs` → Ok 51,
  Fail 1 (all 7 qt tests OK); `meson test -C build-meson test_libvlc_slaves` → Ok 1. The failure is
  `test_src_player_lifecycle` (SIGSEGV), pre-existing: it also fails with master's `src/libvlc-module.c`
  (`pkt-3037-lifecycle-master.log`). It calls `libvlc_new(0, NULL)` and loads the real PulseAudio output in the
  headless container, so it is unrelated to this packet.
- Warnings: none in `src/libvlc-module.c`, `simple_preferences.cpp`, `sprefs_subtitles.ui` or the new test.
- Help output: `vlc -H` now prints `--sub-autodetect-fuzzy {0 (Off), 1 (Any subtitle file), 2 (Contains the file
  name), 3 (File name with extra characters), 4 (Exact file name match)}`.
- Row 4 (MANUAL): screenshots still pending. There is no display in the build environment, so they must be taken on a
  desktop build and attached to the PR.
- Deviations: the §6 command mounts `~/src/vlc` at `/vlc`; it was run from the worktree with `~/src` mounted at the same
  path instead, so that the worktree `.git` pointer resolves. Labels follow §5.1 (2 vs 3 confirmed in
  `src/input/subtitles.c:288-322`: 2 accepts any file whose name contains the movie name, 3 requires the movie name followed by extra characters or an exact match).

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3037-subtitle-fuzziness-in-words ce4af2f`;
      §6 command once → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; confirm the meaning of 2 vs 3 in `src/input/subtitles.c:288-318`
      before fixing the labels of §5.1.
- [ ] **Step 3: Row 1 watched failing** — `test/src/config/sub_autodetect_fuzzy.c`, block
      `test_src_config_sub_autodetect_fuzzy` in `test/src/meson.build`; run it → FAIL (`list_count == 0`).
- [ ] **Step 4: Fix** — in `src/libvlc-module.c` add `static const int pi_sub_fuzzy_values[] = {0,1,2,3,4};` and the
      five `N_()` texts next to `pi_sub_align_values` (`:422`), then `change_integer_list(…)` under `:1819-1820`;
      shorten `SUB_FUZZY_LONGTEXT`. Rows 1–3 green. Commit `core: name the subtitle autodetection fuzziness levels`.
- [ ] **Step 5: Qt** — add a label + `QComboBox` row "File name matching" in `sprefs_subtitles.ui` (grid at `:96-121`)
      and `configGeneric<IntegerListConfigControl>("sub-autodetect-fuzzy", …)` in `simple_preferences.cpp` case
      `SPrefsSubtitles`; row 4 screenshots. Commit `qt: show subtitle file name matching in the simple subtitles panel`.
- [ ] **Step 6: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 7: MR** — MR to videolan/vlc master (Draft); `status: review`.
