---
id: PKT-3040
title: "Add-ons and extensions apart from VLC's internal plugins"
section: "E. Settings"
video_ref: "18:27–18:47"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/dialogs/plugins + modules/gui/qt/dialogs/preferences
branch: master-pkt-3040-addons-apart-from-plugins
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/plugins/plugins.cpp
  - modules/gui/qt/dialogs/plugins/plugins.hpp
  - modules/gui/qt/dialogs/plugins/plugin_list_model.hpp      # new
  - modules/gui/qt/dialogs/plugins/plugin_list_model.cpp      # new
  - modules/gui/qt/dialogs/preferences/preferences.cpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/tests/test_plugin_list_model.cpp          # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 10.0
priority: P3
depends_on: [PKT-3036]
upstream_ruling: needed
target: fork
mr_title: "qt: separate installable add-ons from the internal plugin list"
status: ready
---

# PKT-3040 — Add-ons and extensions apart from VLC's internal plugins

## 1. Scope & intent
Tools → "Plugins and extensions" opens one window with three tabs: **Addons Manager** (installable skins, playlist
parsers, service discoveries, interfaces, art fetchers, extensions from addons.videolan.org), **Active Extensions**
(loaded Lua extensions, e.g. VLsub) and **Plugins** (every compiled module with its capability and score — frame s021
bottom-centre: "Shared memory framebuffer — access_demux — 0", "addons.videolan.org — addons finder — 100" …). At
18:27–18:47 the designer separates the two ideas: add-ons/extensions are things a user installs; plugins are VLC's
internal functions that users are "not really supposed to edit here", so they belong with the advanced settings.
Frame s021 (top-centre) shows the target "Extensions & Scripts" page: list of extensions, search, *Find more
extensions*, *Add*, *Remove*, and a *Load extensions on startup* toggle.

After this packet:
- The window becomes **"Add-ons and Extensions"** with two tabs (Add-ons, Extensions); the *Plugins* tab leaves it.
- The internal plugin list is reachable from the advanced side of preferences: a read-only "Internal plugins" page in
  the All view (top-level node after "Advanced"), backed by a `PluginListModel` with the same columns and search.
- The Extensions tab carries the `qt-autoload-extensions` toggle (today only in the All view).
- Menu label follows: Tools → "Add-ons and E&xtensions…".

One MR, three commits:
1. `qt: extract the module list into PluginListModel` — model + QtTest; `PluginTab` becomes a view on it.
2. `qt: preferences: list internal plugins in the advanced view`.
3. `qt: keep add-ons and extensions only in the add-ons dialog`.

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/dialogs/plugins/plugins.cpp:79-100` (`PluginDialog` ctor) — window title "Plugins and extensions",
  three tabs `AddonsTab`, `ExtensionTab`, `PluginTab`, a Close button, geometry key `PluginsDialog`.
- `modules/gui/qt/dialogs/plugins/plugins.cpp:112-152` (`PluginTab` ctor) — `QTreeWidget` Name/Capability/Score,
  header state in QSettings `Plugins/Header-State`, search field; `:154-175` (`FillTree`) iterates `module_list_get`,
  skips modules without capability in non-DEBUG builds (`:168-170`); `:177-188` (`search`) matches columns 0 and 1.
- `modules/gui/qt/dialogs/plugins/plugins.cpp:222-272` (`ExtensionTab` ctor) — `QListView` over
  `ExtensionListModel` (`ExtensionsManager`), "More information…" and "Reload extensions" buttons.
- `modules/gui/qt/dialogs/plugins/plugins.cpp:308-480` (`AddonsTab` ctor) — category buttons `:360-381` (All, Skins,
  Playlist parsers, Service Discovery, Interfaces, Art and meta fetchers, Extensions), "Only installed" check
  (`:402`), "Find more addons online" (`:423`). `:360-361` gives the "All" button the tooltip "Interface Settings" — a
  copy-paste slip fixed in commit 3.
- `modules/gui/qt/dialogs/plugins/plugins.hpp:61-155` — `PluginDialog`, `PluginTab`, `ExtensionTab`, `AddonsTab`.
- `modules/gui/qt/menus/menus.cpp:266-267` (`ToolsMenu`) — "Plu&gins and extensions" → `DialogsProvider::pluginDialog`
  (`modules/gui/qt/dialogs/dialogs_provider.cpp:541-544`).
- `modules/gui/qt/menus/menus.cpp:361-362`, `:377-390` (`ExtensionsMenu`) — the View menu lists activated extensions;
  `:384` honours `qt-autoload-extensions`.
- `modules/gui/qt/qt.cpp:222-223`, `:448-449` — `qt-autoload-extensions` ("Load extensions on startup").
- `modules/gui/qt/dialogs/preferences/complete_preferences.cpp:155-178` (`PrefsTree` ctor tail) — top-level nodes
  are forced into core category order; an extra "Internal plugins" node is appended after them.
- `modules/gui/qt/dialogs/preferences/preferences.cpp:306-318` (`changeAdvPanel`) — lazily builds the panel of the
  selected node; the new node returns a panel hosting the plugin list view instead of an `AdvPrefsPanel`.

Settings / variables involved: `qt-autoload-extensions` (`modules/gui/qt/qt.cpp:448`); QSettings
`Plugins/Header-State`.
Mock boundary: `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.hpp:46-66`) — the module bank is real, so
`PluginListModel` lists the actual modules (stubs included).

### Overlaps & contradictions
- **PKT-3036** owns the advanced side of preferences; the "Internal plugins" node lands in whatever advanced view it
  leaves (All view today). Depends on it.
- **PKT-3039** reserves the **Extensions** category page; once it lands, that page embeds the Extensions tab widget
  and the autoload toggle, and the dialog stays the full add-ons manager. No code conflict if this merges first.
- **PKT-3035/3013** also edit `ToolsMenu`; the label change is one line — rebase whichever lands second.

## 2b. Upstream state
Master already separates the three things into tabs, so nothing is "mashed" at the data level; what the video points
at is that internal modules sit in the same user-facing window as installable content. The Addons manager itself
(categories, install, online repository) is complete and kept as-is. Moving the plugin list out of a Tools-menu
window into preferences is a VideoLAN UX choice → `upstream_ruling: needed`.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `PluginListModel` | `VLCTestingEnv` | construct | one row per module of `module_list_get` with a non-empty capability; columns Name/Capability/Score | `tests/test_plugin_list_model.cpp::testRowsMatchModuleBank` |
| 2 | same | — | read the row of a known stub module | capability and score equal `module_get_capability` / `module_get_score` | `::testColumnsFromModule` |
| 3 | proxy filter | — | filter `"demux"` | only rows whose name or capability contains it (same semantics as `PluginTab::search`, `modules/gui/qt/dialogs/plugins/plugins.cpp:177-188`) | `::testSearchNameOrCapability` |
| 4 | sort | — | sort by Score descending | numeric order (not lexical), ties by name — mirrors `PluginTreeItem::operator<` (`modules/gui/qt/dialogs/plugins/plugins.cpp:206-220`) | `::testNumericScoreSort` |
| 5 | dialog / preferences | running UI | Tools → Add-ons and Extensions; Preferences → All → Internal plugins | dialog has two tabs; plugin list in preferences with search; autoload toggle on Extensions tab saved to vlcrc | MANUAL (screenshots) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75`, meson block
  `modules/gui/qt/meson.build:1404-1423`; GUI-less → `QTEST_GUILESS_MAIN`.
- Stubs: `tests/vlc_stub_modules.cpp` (defines static stub modules with capabilities used as known rows in row 2).
- Sample media: none.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): the plugin list moves from the Tools window into preferences (advanced). Move the plugin list out of the Tools window into preferences (advanced). Alternative if refused:
   keep three tabs but rename the window "Add-ons and Extensions" and put *Plugins* last behind an "Advanced" label.
2. **NOTE** — Vocabulary: the designer says "Extensions" for the page; VLC's "Extensions" are Lua scripts, a subset of
   add-ons. Labels used: window "Add-ons and Extensions", tabs "Add-ons" and "Extensions" — no user-visible term
   changes meaning.
3. **NOTE** — "Add / Remove" buttons of the mock-up: installing a local Lua file is not supported by the Addons
   manager today (install comes from repositories or the `addons local storage` module); not added here.
4. **NOTE** — The "All → Interface Settings" tooltip slip (`modules/gui/qt/dialogs/plugins/plugins.cpp:360-361`) is fixed in commit 3.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, `git switch -c master-pkt-3040-addons-apart-from-plugins ce4af2f`
      (rebase on PKT-3036 once merged); §6 command once → green. Ruling §5.1 (R0) is recorded.
- [ ] **Step 2: Dry-run** — re-read §2; list two stub modules from `tests/vlc_stub_modules.cpp` to use in row 2.
- [ ] **Step 3: Row 1 watched failing** — `tests/test_plugin_list_model.cpp`, `test_qt_plugin_list_model` in
      `meson.build`; empty model → FAIL; implement from `FillTree` (`modules/gui/qt/dialogs/plugins/plugins.cpp:154-175`).
- [ ] **Step 4: Rows 2–4** — one cycle each; `PluginTab` switches to `QTreeView` + `QSortFilterProxyModel`.
      Commit `qt: extract the module list into PluginListModel`.
- [ ] **Step 5: Preferences node** — append "Internal plugins" after the category nodes in `PrefsTree`
      (`modules/gui/qt/dialogs/preferences/complete_preferences.cpp:155-178`) and serve its panel in `changeAdvPanel` (`modules/gui/qt/dialogs/preferences/preferences.cpp:306-318`).
      Commit `qt: preferences: list internal plugins in the advanced view`.
- [ ] **Step 6: Dialog** — drop the Plugins tab (`modules/gui/qt/dialogs/plugins/plugins.cpp:90-91`), retitle, add the autoload `BoolConfigControl`
      to `ExtensionTab`, fix the tooltip, menu label in `modules/gui/qt/menus/menus.cpp:266`; row 5. Commit
      `qt: keep add-ons and extensions only in the add-ons dialog`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MR** — PR to the fork github.com/oscarolar/vlc (base `master`); `status: review`.
