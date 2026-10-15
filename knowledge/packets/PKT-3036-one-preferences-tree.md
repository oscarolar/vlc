---
id: PKT-3036
title: "One preferences page per category: basic options first, advanced below, info icons"
section: "E. Settings"
video_ref: "16:02–17:35, 18:47–18:55"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/dialogs/preferences
branch: master-pkt-3036-one-preferences-tree
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/preferences/preferences.cpp
  - modules/gui/qt/dialogs/preferences/preferences.hpp
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
  - modules/gui/qt/dialogs/preferences/simple_preferences.hpp
  - modules/gui/qt/dialogs/preferences/complete_preferences.cpp
  - modules/gui/qt/dialogs/preferences/preferences_widgets.cpp
  - modules/gui/qt/dialogs/preferences/preferences_widgets.hpp
  - modules/gui/qt/dialogs/preferences/prefs_merge.hpp        # new
  - modules/gui/qt/dialogs/preferences/prefs_merge.cpp        # new
  - modules/gui/qt/qt.cpp
  - modules/gui/qt/tests/test_prefs_merge.cpp                 # new
  - modules/gui/qt/tests/test_config_control_help.cpp         # new
  - modules/gui/qt/meson.build
estimate_h: 30.0
priority: P2
depends_on: []
upstream_ruling: needed
target: fork
mr_title: "qt: show advanced options under each simple preferences panel"
status: review
actual_h: 0.5
---

# PKT-3036 — One preferences page per category: basic options first, advanced below, info icons

## 1. Scope & intent
Today the preferences dialog has three separate views chosen by radio buttons at the bottom: **Simple** (six or
seven icon panels built from `.ui` files), **All** (the category → subcategory → plugin tree) and **Expert** (a flat
table). A setting missing from a Simple panel means switching to All, finding the same category again in a tree that
is organised differently, and scanning every option. At 16:02–16:31 the designer asks for one path: open a category,
read its basic settings, scroll, and the advanced settings of that same category follow. At 16:57–17:03 and
18:47–18:55 the long explanation of every option, today a hover tooltip, becomes an info icon next to the option
(frames s020/s021: every row of the redesigned pages carries a small ⓘ after its label, and each page ends with an
"Advanced Options ˅" expander).

After this packet:
- Each Simple panel ends with a collapsed **"Advanced options"** group listing the core (and, for Interface, the Qt
  module) options of the subcategories that panel stands for, minus the ones the panel already shows; same
  widgets as the All view (`ConfigControl::createControl`), same sections (`CONFIG_SECTION` → `QGroupBox`).
- Save applies both the basic and the advanced controls of every panel that was opened.
- Every `ConfigControl` whose option has a `psz_longtext` gets an info button after its label; clicking or
  keyboard-activating it shows the text (`QWhatsThis`/popup), the tooltip stays for mouse users.
- The All and Expert views stay reachable (§5.1); the default view does not change in this packet.

The category list itself (13 categories) is PKT-3039; this packet works with today's six/seven panels.

T3 — split into three MRs (§5.5):
1. `qt: preferences: add an info button to config controls` (3036a).
2. `qt: preferences: list a panel's remaining options under Advanced options` (3036b).
3. `qt: preferences: apply advanced options from simple panels on save` (3036b, same MR as 2).
4. `qt: preferences: fold the view selector` (3036c, per the §5.1 ruling).

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/dialogs/preferences/preferences.hpp:42-101` (`PrefsDialog`) — `enum { SIMPLE, ADVANCED, EXPERT }`,
  three lazily built stacks, radio buttons `simple`, `all`, `expert`.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:65-185` (`PrefsDialog` ctor) — the "Show settings" group box
  with the three radios (`:69-84`), the Save/Cancel/Reset box, the stacked views; `:170-175` picks the start view
  from `qt-initial-prefs-view`.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:333-352` (`PrefsDialog::save`) — **applies only the view that
  is checked and visible**: a change made in All and then a switch back to Simple before Save is silently dropped.
  Merging the views must apply everything that was built.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:295-304` (`changeSimplePanel`) — panels are created on first
  visit (`new SPrefsPanel(p_intf, …, number)`).
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:283-349` (`configGeneric`, `configGenericNoUi`,
  `configGenericFile`, `configBool`) — every basic option is bound by name through `config_FindConfig` and the
  resulting `ConfigControl` is appended to `controls`; that list is the set of names a panel already shows.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:351-380` (`SPrefsPanel` ctor head) and the per-panel
  cases (`:383` Video, `:445` Audio, `:603` Input/Codecs, `:697` Interface, `:933` Subtitles, `:985` Hotkeys,
  `:1024` Media Library) — each panel is a `.ui` form (`sprefs_*.ui`) plus hand-made bindings.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:1189-1260` (`SPrefsPanel::apply`) — applies `controls`
  plus panel-specific extras (style, audio, ML folders).
- `modules/gui/qt/dialogs/preferences/complete_preferences.cpp:47-180` (`PrefsTree` ctor) — the All tree: core
  subcats from the main module's `CONFIG_SUBCATEGORY` markers, then one node per plugin under its first subcat.
- `modules/gui/qt/dialogs/preferences/complete_preferences.cpp:525-660` (`AdvPrefsPanel` ctor) — walks the config
  items from the subcat marker to the next one, opens a `QGroupBox` per `CONFIG_SECTION` (`:609-620`), one
  control per item through `ConfigControl::createControl` (`:632-641`), one hotkey control only (`:625-630`).
  This loop is what the Advanced group reuses (extracted to `prefs_merge.cpp`).
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:72-86` (`formatTooltip`) and the `finish()` of every
  control (`:257`, `:297`, `:365`, `:516`, `:635`, `:764`, `:900`, `:944`, `:1023`, `:1092`, `:1141`, `:1235`,
  `:1279`) — each one copies `psz_longtext` into `setToolTip` of label and widget; no other surface shows it.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:147-161`, `:321-330` (`changeExpertDesc`) — the Expert view
  already prints title and long text of the selected row under the table: the only place where the help is not
  hover-only.
- `include/vlc_configuration.h:68-90` (`struct module_config_t`) — **no "basic/advanced" flag exists**: the split is
  entirely the hand-picked content of the `.ui` panels. Nothing in core needs to change for this packet.
- `src/libvlc-module.c:1799-1848` — example of a mapped subcat: `SUBCAT_VIDEO_SUBPIC` holds `spu`, `osd`, the
  "Subtitles" section with `sub-autodetect-fuzzy` (`:1819`) …; the Subtitles panel binds only `osd`, `spu`,
  `video-title-*`, `subsdec-encoding`, `sub-language`, `freetype-*`, `sub-margin`, `secondary-sub-*`
  (`modules/gui/qt/dialogs/preferences/simple_preferences.cpp:939-979`), so the rest is the Advanced list.

Settings / variables involved: `qt-initial-prefs-view` (`modules/gui/qt/qt.cpp:434-436`, values 0/1/2 =
Simple/All/Expert, texts at `:146-147`); no new option.
Mock boundary: `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.hpp:46-66`) for the real option set; widget rows need a
`QApplication` (`QTEST_MAIN` with `QT_QPA_PLATFORM=offscreen`, as `modules/gui/qt/tests/qml_test.cpp:43-44` does).

### Overlaps & contradictions
- **PKT-3039** redraws the category list (13 categories) and decides which subcats feed which page. This packet
  keeps the mapping in one table (`prefs_merge.cpp`, `panelSubcats()`), which PKT-3039 rewrites — the merge
  mechanics do not change.
- **PKT-3037** adds `sub-autodetect-fuzzy` to the Subtitles panel; once it lands, the option moves from that
  panel's Advanced list to its basic part automatically (the list is computed, not hard-coded).
- **PKT-3038** adds the accent colour control to the Interface panel; same automatic effect for `qt-accent-color`.
- **PKT-3040** moves the plugin list into the advanced area; plugin nodes are not part of the Advanced group here.
- **PKT-3035** keeps the Hotkeys panel; its Advanced list is empty by construction (all `key-*` are in the
  hotkey table).

## 2b. Upstream state
Master already has: three views, search in All and Expert, "Only show current" and "Modified only" filters, and the
Expert view's inline long text. It does **not** have: any link from a Simple panel to its advanced options, help
other than tooltips, or a Save that covers more than one view. VLC 4 kept the Simple/All split deliberately and added
Expert in 4.0 (`qt-advanced-pref` obsoleted at `modules/gui/qt/qt.cpp:433`), so folding the selector is a design change (§5.1).

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PrefsMerge::remainingItems` | `VLCTestingEnv`; subcat `SUBCAT_VIDEO_SUBPIC`; shown = names bound by the Subtitles panel | compute | contains `sub-autodetect-fuzzy`, `sub-autodetect-path`; contains none of the shown names | `tests/test_prefs_merge.cpp::testSubtitlesRemaining` |
| 2 | same | any mapped subcat | compute with shown = ∅ | equals the `CONFIG_ITEM` list of that subcat in config order (same as `AdvPrefsPanel` walks) | `::testEmptyShownIsWholeSubcat` |
| 3 | same | subcat with `CONFIG_SECTION` markers | compute | sections with no remaining item are dropped; order kept | `::testEmptySectionsDropped` |
| 4 | `PrefsMerge::panelSubcats` | every `SPrefs*` panel | read table | each panel maps to ≥1 subcat or module, no subcat appears in two panels, `SPrefsHotkeys` maps to `SUBCAT_INTERFACE_HOTKEYS` | `::testPanelMappingDisjoint` |
| 5 | `SPrefsPanel` Advanced group | Subtitles panel built offscreen | toggle a remaining bool (e.g. `sub-autodetect-file`), `apply()` | `config_GetInt` returns the new value | `::testAdvancedControlApplied` |
| 6 | `PrefsDialog::save` | change in an Advanced group, change in All view | Save | both written (no view-dependent drop) | `::testSaveAppliesAllBuiltViews` |
| 7 | `ConfigControl` help button | `BoolConfigControl` on an item with `psz_longtext` | build into a grid layout | a `QToolButton` with accessible name "More information" exists after the label, its tooltip/what's-this holds the text; none for items without longtext | `tests/test_config_control_help.cpp::testInfoButton` |
| 8 | Visual | running UI | open Subtitles, expand Advanced, click ⓘ | page matches §1; keyboard reaches every ⓘ | MANUAL (screenshots) |

Rows 5–7 are widget tests; Step 2 confirms they link with the preferences sources only. If the link pulls the whole
module, rows 5–6 become MANUAL and the logic they cover is moved into `PrefsMerge` so rows 1–4 still guard it.

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75` (`VLCTestingEnv` in `initTestCase`),
  meson block `modules/gui/qt/meson.build:1404-1423`.
- Widget rows: `QTEST_MAIN` plus `qputenv("QT_QPA_PLATFORM", "offscreen")` before the app, like
  `modules/gui/qt/tests/qml_test.cpp:43-44`.
- Tests never call `config_SaveConfigFile`; touched options are restored in `cleanup()`.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R3): fold the Simple/All/Expert selector into one path (basic, then advanced per category) with a "Show all settings (advanced tree)" button at the bottom; `qt-initial-prefs-view` keeps working; 3036c is unblocked. Fold the Simple/All/Expert selector. The designer wants a single path; VideoLAN keeps three views
   on purpose (power users, `qt-initial-prefs-view`). Proposal: keep All and Expert behind a single "Show all
   settings (advanced tree)" button at the bottom and drop the radio group; `qt-initial-prefs-view` keeps working.
   3036c is unblocked by R3; 3036a/b are additive and independent.
2. **RULED** RULED 2026-10-14 (R0): the `panelSubcats()` table is accepted as proposed (PKT-3039 later replaces it). Which subcats feed which panel (`panelSubcats()`): proposal Interface → `SUBCAT_INTERFACE_GENERAL`
   + `SUBCAT_INTERFACE_MAIN` restricted to module `qt`; Audio → `SUBCAT_AUDIO_GENERAL`; Video →
   `SUBCAT_VIDEO_GENERAL`; Subtitles → `SUBCAT_VIDEO_SUBPIC`; Input/Codecs → `SUBCAT_INPUT_GENERAL`; Hotkeys →
   `SUBCAT_INTERFACE_HOTKEYS`; Media Library → module `medialibrary`. Plugin subcats (filters, outputs, demuxers)
   stay in the All tree. PKT-3039 replaces this table.
3. **NOTE** — Long lists: `SUBCAT_INPUT_GENERAL` alone runs `src/libvlc-module.c:1848-2045`; the Advanced group is
   collapsed by default and searchable through the existing All/Expert search, not a new one.
4. **NOTE** — Info button vs tooltip: tooltip kept (mouse users), button added (keyboard, touch, discoverability).
   One helper `attachHelp(label, widget, longtext)` replaces the 13 copies in `finish()`.
5. **NOTE (split)** — 3036a info button (T2, ~8 h, rows 7–8), 3036b Advanced group + save (T2, ~16 h, rows 1–6),
   3036c selector fold (T1, ~4 h, MANUAL, per ruling R3). Each is one MR; a/b are independent.

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
Executed 2026-10-15 on nzxt, worktree ~/src/vlc-wt/pkt-3036 from ce4af2f, CI image registry.videolan.org/vlc-debian-unstable:20260611225331, meson setup build-meson -Dextra_checks=true, ninja -j8. Logs: ~/src/logs/pkt-3036-*.log.

Split as §5.5 (one PR each, fork only):
- 3036a, branch master-pkt-3036a-info-button: dcd6f42 qt: preferences: add an info button to config controls. PR https://github.com/oscarolar/vlc/pull/6
- 3036b, branch master-pkt-3036b-advanced-options (from ce4af2f, independent of 3036a): bf3d5d5 qt: preferences: list a panel's remaining options under Advanced options; fb9f34b qt: preferences: apply advanced options from simple panels on save. PR https://github.com/oscarolar/vlc/pull/7
- 3036c, branch master-pkt-3036c-fold-view-selector (stacked on 3036b): 1df40ad qt: preferences: fold the view selector. PR https://github.com/oscarolar/vlc/pull/8
- Packet branch master-pkt-3036-one-preferences-tree = all four (dcd6f42, 25409ad, c39a710, 2af459f).

Step 2 dry-run: preferences_widgets.cpp links alone in a QtTest (plus searchlineedit, customwidgets, animators, vlchotkeyconverter, qt_dirs), and so do complete_preferences.cpp and expert_model.cpp. SPrefsPanel and PrefsDialog pull MainCtx, DialogsProvider and the media library, so row 5 tests AdvancedOptionsSection (the Advanced group the panel embeds), row 6 tests AdvancedOptionsSection + AdvPrefsPanel + ExpertPrefsTableModel applied in the save order, and the PrefsDialog::save wiring itself is MANUAL.

Tests (QtTest, offscreen):
- test_qt_config_control_help (row 7): testInfoButton, testInfoButtonAfterLabel, testInfoButtonOnExistingForm, testNoInfoButtonWithoutLongtext, testInfoButtonFollowsVisibility. Red: 4 failed ('info' returned FALSE). Green: 7/7 functions.
- test_qt_prefs_merge (rows 1-6): testSubtitlesRemaining, testEmptyShownIsWholeSubcat, testEmptySectionsDropped, testPanelMappingDisjoint, testHotkeysAdvancedIsJumpSizes, testAdvancedControlApplied, testSaveAppliesAllBuiltViews, testExpertSubmitsEditedOnly. Red against a skeleton prefs_merge: 8 failed (expert row failed on the stale write). Green: 10/10 functions.
- Gate --suite qt: baseline ce4af2f 7/7 OK; 3036a 8/8; 3036b 8/8; 3036c (b+c) 8/8; full packet branch 9/9 OK.
- -Dextra_checks=true: no warning in dialogs/preferences or the new tests (remaining warnings are pre-existing: extended_panels, open_panels, menus).
- No QML touched, so no qmllint diff.
- Independent code review (read-only) found: hotkey table always applied (fixed: KeySelectorControl::value() + markClean), empty row left when a label is hidden (fixed: the row follows the label). Dirty flags are not reset after save: not an issue, the dialog is WA_DeleteOnClose.

Deviations:
- Hotkeys Advanced list is not empty: SUBCAT_INTERFACE_HOTKEYS also holds the four jump sizes, which the panel does not show; they are listed and tested.
- Save applying every built view needed write-only-what-changed semantics to avoid stale overwrites: ConfigControl::markClean/isModified, and an edited flag in the Expert table (expert_model.hpp/.cpp, expert_view.cpp, outside the target list). Options written by panel code (caching presets, volume, fullscreen screen, intf, freetype opacity) are excluded from every Advanced list (handledByPanels).
- The panel enum moved from simple_preferences.hpp to prefs_merge.hpp so tests need no generated ui headers.
- qt.cpp unchanged: qt-initial-prefs-view values and texts still fit (Simple/Advanced/Expert).
- Makefile.am lists prefs_merge (sources + moc) but not the new tests (meson only).
- PKT-3037: git merge-tree against master-pkt-3037-subtitle-fuzziness-in-words is clean; sub-autodetect-fuzzy then leaves the Subtitles Advanced list by itself.

MANUAL (pending, need a desktop): row 8 screenshots (Subtitles panel with Advanced collapsed and expanded, info button popup, keyboard Tab through info buttons); PrefsDialog::save with a change in an Advanced group and one in the All view, reopen; 3036c bottom bar in Simple and All views, Table view, --qt-initial-prefs-view=1 and 2.

## 7. Agent brief
- [x] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3036-one-preferences-tree ce4af2f`; §6
      command once → green.
- [x] **Step 2: Dry-run** — re-read §2; try linking a minimal widget test against `preferences_widgets.cpp`
      (+ `widgets/native/searchlineedit.cpp`) and record in §5 whether rows 5–7 stay QtTest or become MANUAL.
- [x] **Step 3 (3036a): Row 7 watched failing** — `tests/test_config_control_help.cpp`, register
      `test_qt_config_control_help` in `meson.build`; add `attachHelp()` in `preferences_widgets.cpp`, call it from
      each `finish()`; row 8 screenshots. Commit `qt: preferences: add an info button to config controls`, MR.
- [x] **Step 4 (3036b): Rows 1–4** — `prefs_merge.{hpp,cpp}`: `panelSubcats()` table and `remainingItems(module,
      subcat, shownNames)`, extracted from `AdvPrefsPanel` (`modules/gui/qt/dialogs/preferences/complete_preferences.cpp:597-641`) so the All view
      calls the same walker; `tests/test_prefs_merge.cpp` first, red then green, one row at a time.
- [x] **Step 5 (3036b): Rows 5–6** — in `SPrefsPanel` ctor, after the panel form, build a collapsed
      `QGroupBox(qtr("Advanced options"))` from `remainingItems`, append its controls to `controls`; make
      `PrefsDialog::save` apply every built view (`modules/gui/qt/dialogs/preferences/preferences.cpp:333-352`). Commits
      `qt: preferences: list a panel's remaining options under Advanced options` and
      `qt: preferences: apply advanced options from simple panels on save`, MR.
- [x] **Step 6 (3036c)** — per ruling R3 (§5.1): replace the radio group by the "Show all settings" button;
      MANUAL screenshots; commit `qt: preferences: fold the view selector`.
- [x] **Step 7: Whole gate** — §6 command per MR; summaries in §6 Evidence.
- [x] **Step 8: MRs** — PR to the fork github.com/oscarolar/vlc (base `master`), 3036a first; `status: review`.
