---
id: PKT-3035
title: "Hotkeys open in their own dialog from a menu entry"
section: "E. Settings"
video_ref: "15:35–16:02"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/dialogs
branch: master-pkt-3035-hotkeys-dialog
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/hotkeys/hotkeys_model.hpp          # new
  - modules/gui/qt/dialogs/hotkeys/hotkeys_model.cpp          # new
  - modules/gui/qt/dialogs/hotkeys/hotkeys_dialog.hpp         # new
  - modules/gui/qt/dialogs/hotkeys/hotkeys_dialog.cpp         # new
  - modules/gui/qt/dialogs/preferences/preferences_widgets.cpp
  - modules/gui/qt/dialogs/preferences/preferences_widgets.hpp
  - modules/gui/qt/dialogs/preferences/simple_preferences.cpp
  - modules/gui/qt/dialogs/dialogs_provider.hpp
  - modules/gui/qt/dialogs/dialogs_provider.cpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/tests/test_hotkeys_model.cpp               # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 12.0
priority: P2
depends_on: [PKT-3013]
upstream_ruling: not-needed
target: upstream
mr_title: "qt: add a standalone hotkeys dialog reachable from the Tools menu"
status: ready
---

# PKT-3035 — Hotkeys open in their own dialog from a menu entry

## 1. Scope & intent
Today the only way to see or change a keyboard shortcut is Tools → Preferences → the sixth icon of the simple
view ("Hotkeys"), or the "Hotkeys settings" node of the All view. At 15:35–16:02 the designer argues that most
people use shortcuts but never dig into preferences, so the hotkey table gets its own dialog, a fresh look and a
direct entry ("Keyboard Shortcuts") in the main menu (frame s018, bottom-centre: the menu lists *Keyboard
Shortcuts* between *Open Recent* and *Customize Player*).

After this packet:
- A `HotkeysDialog` (Qt widgets, like every other preferences dialog in master) shows the action / hotkey /
  global-hotkey table with search, the two mouse-wheel options and, on Windows, `qt-disable-volume-keys`; Save
  writes the config file, Cancel discards.
- Tools menu gets "&Keyboard Shortcuts…" above "&Preferences"; PKT-3013's single menu button inherits it because
  that button pops `ToolsMenu` (see §2).
- The Hotkeys panel of the simple preferences keeps working (no removal here, §5.1) but is built on the same
  extracted `HotkeysModel`, so the two places cannot drift.
- The table logic leaves `KeySelectorControl` (a `QTreeWidget` filled inline) for a `QAbstractTableModel`
  (`HotkeysModel`) that is unit-testable and that PKT-3043 reuses to print the live bindings on the onboarding page.

One MR, three commits:
1. `qt: extract the hotkey table into HotkeysModel` — model + QtTest, `KeySelectorControl` becomes a view on it.
2. `qt: add a standalone hotkeys dialog` — `HotkeysDialog`, `DialogsProvider::hotkeysDialog()`, visibility property.
3. `qt: add a Keyboard Shortcuts entry to the Tools menu`.

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`. Step 2 re-reads every range.
- `modules/gui/qt/dialogs/preferences/simple_preferences.hpp:52-62` — the simple categories enum; `SPrefsHotkeys`
  is the sixth panel, `SPrefsDefaultCat` is Interface.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:257-264` (`SPrefsCatList` ctor) — the six/seven icon
  buttons; `:262` adds the Hotkeys button with `HOTKEYS_TITLE`.
- `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:985-1020` (`SPrefsPanel` ctor, `case SPrefsHotkeys`)
  — builds a `KeySelectorControl`, then `IntegerListConfigControl` for `hotkeys-y-wheel-mode` and
  `hotkeys-x-wheel-mode`, and on `_WIN32` a `BoolConfigControl` for `qt-disable-volume-keys`. This is the exact
  content the new dialog hosts.
- `modules/gui/qt/dialogs/preferences/preferences_widgets.hpp:416-475` (`KeySelectorControl`, `KeyItemAttr`) —
  `ColumnIndex {ACTION_COL, HOTKEY_COL, GLOBAL_HOTKEY_COL, ANY_COL}`, `find_conflict()` static, `reassign_key`,
  `reset_all`, `unset`.
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1289-1328` (`KeySelectorControl` ctor) — search
  field, "in" combo, `QTreeWidget` with three columns; `:1342-1351` `buildAppHotkeysList` collects `QAction`
  shortcuts of the top-level widget into `existingkeys` (in the QML UI there are no such `QAction`s on the
  dialog's root, so the set is effectively empty — the conflict check against app shortcuts is dead code today).
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1353-1420` (`KeySelectorControl::finish`) — walks
  `module_config_get(module_get_main())`, keeps `CONFIG_ITEM_KEY`, pairs every `key-X` with its `global-key-X`
  in a second pass (`:1397-1412`), sets tooltips from `psz_longtext`.
- `modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1494-1503` (`KeySelectorControl::doApply`) —
  `config_PutPsz` for both columns of every row.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:333-373` (`PrefsDialog::save`) — applies the visible view's
  panels, then `config_SaveConfigFile` and `MainCtx::reloadPrefs`. The new dialog copies this tail.
- `src/misc/actions.c:526-578` (`libvlc_InternalActionsInit`) — the key → action map is built **once** from
  `key-*`/`global-key-*` at libvlc start; the only callbacks registered are on `key-pressed` and
  `global-key-pressed` (`:574-576`). A changed binding therefore takes effect after a restart, in master's
  preferences as in the new dialog (§5.2).
- `modules/gui/qt/dialogs/dialogs_provider.cpp:74-88` (`BASE_DIALOG` X-macro) — generates the `xxxVisible`
  properties/signals; `:290-302` `prefsDialog()` toggles a `QPointer<PrefsDialog>`; `:541-544`
  `pluginDialog()` uses `toggleDialogVisible`. `modules/gui/qt/dialogs/dialogs_provider.hpp:108-123` the
  `Q_PROPERTY(bool …DialogVisible)` list.
- `modules/gui/qt/menus/menus.cpp:241-276` (`VLCMenuBar::ToolsMenu`) — Effects, Track sync, Media info, Codec info,
  VLM, Program guide, Messages, Plugins and extensions, separator, Customize Interface, Preferences (Ctrl+P).
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:283-284` and `:408-409` — the QML menubar and the per-menu popup both
  call `ToolsMenu`, so one entry in `ToolsMenu` reaches the menubar, the hamburger popup and PKT-3013's button.
- `src/libvlc-module.c:2588-2720` — the `add_key("key-…")` block; `:2444-2540` the non-macOS defaults.

Settings / variables involved: every `key-*` / `global-key-*` (`src/libvlc-module.c:2588-2720`),
`hotkeys-y-wheel-mode`, `hotkeys-x-wheel-mode` (`src/libvlc-module.c:2276-2280`, list at `:1324-1325`),
`qt-disable-volume-keys` (`modules/gui/qt/qt.cpp`, `_WIN32` block).
Mock boundary: `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.hpp:46-66`) gives a real libvlc with the
core option set, so `HotkeysModel` reads the real `key-*` items without a GUI.

### Overlaps & contradictions
- **PKT-3013** (one menu button) owns the menu structure and already reserves an `AppMenuEntry::KeyboardShortcuts`
  slot, hidden while `hasHotkeysDialog` is false (its §3 row 5). If PKT-3013 is merged first, this packet flips that
  flag and routes the slot to `DialogsProvider::hotkeysDialog` instead of editing `ToolsMenu`; otherwise it adds the
  `ToolsMenu` entry and PKT-3013 picks it up.
- **PKT-3043** (onboarding) reuses `HotkeysModel` to print live bindings; it depends on this packet.
- **PKT-3036/3039** (preferences tree and category map) may drop the Hotkeys panel from preferences; that removal
  is theirs (§5.1), not this packet's.
- **PKT-3015** (OSD names the hotkey) reads the same key names; no file overlap.

## 2b. Upstream state
Master ships a complete hotkey editor (search, conflict dialog `KeyConflictDialog`, global hotkeys, wheel modes),
but only inside preferences. There is no dialog, menu entry or `DialogsProvider` slot for it, and no model class —
the table is a `QTreeWidget` filled in `finish()`. The "fresh coat of paint" is limited here to the dialog layout
(title, short explanation, search on top, table, wheel options under a separator); the visual tokens come from
PKT-3001 and the dialog stays a Qt widget dialog like the rest of preferences (a QML rewrite is not in scope).

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `HotkeysModel` load | `VLCTestingEnv` initialised | model constructed | `rowCount()` equals the number of `CONFIG_ITEM_KEY` items of the main module whose name does not start with `global-` | `tests/test_hotkeys_model.cpp::testRowsMatchKeyOptions` |
| 2 | `HotkeysModel` pairing | same | read row of `key-play-pause` | `HOTKEY_COL` = `config_GetPsz("key-play-pause")`, `GLOBAL_HOTKEY_COL` = `config_GetPsz("global-key-play-pause")` | `::testGlobalColumnPaired` |
| 3 | `HotkeysModel::setKeys` + `apply` | row of `key-snapshot` | set `"Ctrl+Shift+F12"`, `apply()` | `config_GetPsz("key-snapshot")` returns it; other rows unchanged | `::testApplyWritesConfig` |
| 4 | `HotkeysModel::findConflict` | `key-play-pause` bound to `Space` | ask conflict for `Space` on another row, `HOTKEY_COL` | returns the play-pause row; `GLOBAL_HOTKEY_COL` lookup returns none | `::testConflictPerColumn` |
| 5 | `HotkeysModel::resetAll` | two rows changed | `resetAll(HOTKEY_COL)` | both rows equal their `orig.psz` default | `::testResetAll` |
| 6 | `HotkeysModel` filter | — | filter `"snap"` in `ACTION_COL` through the proxy | only rows whose action text contains it | `::testFilterAction` |
| 7 | `DialogsProvider::hotkeysDialog` / Tools menu | running UI | Tools → Keyboard Shortcuts…, change a key, Save, restart | dialog opens; vlcrc holds the new key; after restart the key works | MANUAL (screenshots of menu and dialog) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75` (QObject test class, `initTestCase`
  building `VLCTestingEnv`, `QTEST_GUILESS_MAIN` at `:369`); meson block to copy:
  `modules/gui/qt/meson.build:1404-1423` (`test_qt_dialog_model`).
- Stubs: `tests/vlc_stub_modules.cpp` (no player needed). The model is GUI-less, so `QTEST_GUILESS_MAIN` holds.
- Each test runs on a fresh `VLCTestingEnv` or restores the defaults (`config_ResetAll` is not needed: reset the
  touched keys in `cleanup()`), and `config_SaveConfigFile` is never called from tests.
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): the Hotkeys icon stays in simple preferences. Keep or drop the Hotkeys icon in simple preferences. This packet keeps it (built on the same
   model) so nothing is lost for users who look there; dropping it belongs to PKT-3036/3039, which redraw the
   preference categories ("Controls" in the designer's map holds keyboard and mouse). 
2. **NOTE** — Changed bindings apply after restart (`src/misc/actions.c:526-578` builds the map once). Making them
   live needs a core change (rebuild `as->map` on a `key-*` variable callback), out of scope; the dialog shows a
   one-line note "Changes take effect after restarting VLC", same behaviour as master's preferences.
3. **NOTE** — `buildAppHotkeysList` (`modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1342-1351`) cannot see QML shortcuts
   (`menus/qml/GlobalShortcuts.qml`), so conflicts with Ctrl+P etc. are not detected today. Not fixed here; the
   model keeps an `existingKeys` hook so a later change can feed it.
4. **NOTE** — Entry label: the designer writes "Keyboard Shortcuts"; master calls the panel "Hotkeys". The entry
   uses "Keyboard Shortcuts" (clearer to users); the dialog title says "Keyboard Shortcuts (Hotkeys)" for
   continuity with docs. Translators get one new string each.
5. **NOTE** — `upstream_ruling: not-needed`: additive dialog and menu entry, nothing removed.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3035-hotkeys-dialog ce4af2f`; run the
      §6 command once → green before any change.
- [ ] **Step 2: Dry-run** — re-read every range of §2; confirm `ToolsMenu` is still the builder PKT-3013 uses (if
      PKT-3013 merged, read its menu file instead and move row 7's entry there).
- [ ] **Step 3: First row watched failing** — create `modules/gui/qt/tests/test_hotkeys_model.cpp` with row 1 and an
      empty `HotkeysModel` stub; register `test_qt_hotkeys_model` in `modules/gui/qt/meson.build` next to
      `test_qt_dialog_model` (sources: the test, `tests/vlc_stub_modules.cpp`, the model; moc the model header);
      run `meson test -C build-meson test_qt_hotkeys_model` → FAIL, then implement loading.
- [ ] **Step 4: Rows 2–6** — one TDD cycle each; move the pairing, conflict, reset and apply logic out of
      `KeySelectorControl` (`modules/gui/qt/dialogs/preferences/preferences_widgets.cpp:1353-1503`) into the model; `KeySelectorControl` keeps its
      widgets and edits through the model (`QTreeView` + `QSortFilterProxyModel`). Commit 1.
- [ ] **Step 5: Dialog** — `dialogs/hotkeys/hotkeys_dialog.{hpp,cpp}` (QVLCDialog) hosting `KeySelectorControl` and the
      two wheel `IntegerListConfigControl`s copied from `modules/gui/qt/dialogs/preferences/simple_preferences.cpp:985-1020`; Save = apply +
      `config_SaveConfigFile` + `reloadPrefs` (as `modules/gui/qt/dialogs/preferences/preferences.cpp:354-373`). Add `X(hotkeysDialog)` to
      `BASE_DIALOG` and the slot/property in `dialogs_provider.{hpp,cpp}`; add sources to `meson.build` moc/sources
      lists and `po/POTFILES.in`. Commit 2.
- [ ] **Step 6: Menu** — `menus.cpp` `ToolsMenu`: `addDPStaticEntry(menu, qtr("&Keyboard Shortcuts..."), ":/menu/preferences.svg",
      &DialogsProvider::hotkeysDialog)` right before Preferences. Row 7 MANUAL on a build run from nzxt (X forwarding
      or VNC), screenshots attached. Commit 3.
- [ ] **Step 7: Whole gate** — §6 command; paste the summary in §6 Evidence.
- [ ] **Step 8: Commit and MR** — subjects as in §1, push to the fork, MR to videolan/vlc master (Draft);
      `status: review`.
