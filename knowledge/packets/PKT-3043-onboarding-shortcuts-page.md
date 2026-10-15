---
id: PKT-3043
title: "First-run screen teaches the basic shortcuts, after the theme choice"
section: "E. Onboarding"
video_ref: "19:36–19:54"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/dialogs/firstrun
branch: master-pkt-3043-onboarding-shortcuts-page
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp
  - modules/gui/qt/dialogs/firstrun/firstrunwizard.hpp
  - modules/gui/qt/dialogs/firstrun/firstrunwizard.ui
  - modules/gui/qt/dialogs/firstrun/onboarding_shortcuts_model.hpp   # new
  - modules/gui/qt/dialogs/firstrun/onboarding_shortcuts_model.cpp   # new
  - modules/gui/qt/tests/test_onboarding_shortcuts.cpp               # new
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 12.0
priority: P2
depends_on: [PKT-3038, PKT-3035]
upstream_ruling: not-needed
target: upstream
mr_title: "qt: show the basic keyboard shortcuts in the first-run wizard"
status: ready
---

# PKT-3043 — First-run screen teaches the basic shortcuts, after the theme choice

## 1. Scope & intent
At 19:36–19:54 the designer cites a viewer poll (1,439 answers, frame s022 bottom): about half of users want to keep
customization minimal, so an onboarding screen with theme options and a page introducing the basic shortcuts is "a
simple solution". Frame s023 (top-left) shows the shortcut page: title "Learn the basic keyboard shortcuts.", two
columns of twelve actions with key caps (Play/Pause, Toggle fullscreen, Seek backward/forward, Increase/decrease
volume, Previous/next item, Show/hide queue, Previous/next frame, Show/hide file browser, Cycle subtitle track, Take
a snapshot, Cycle audio track, Record), a footer "Find the full list in the menu." and *Back* / *Finish*.

**Master already has an onboarding wizard** (§2b) with privacy, media-library folders, a colour scheme page
(System/Day/Night with previews) and a layout page (modern/classic). This packet adds what is missing:
- a **Shortcuts** page as the last page, listing ~12 basic actions with **the user's actual bindings**, read from the
  `key-*` options (not the designer's keys: VLC's defaults differ per platform, §2);
- a "See all shortcuts…" link opening PKT-3035's hotkeys dialog ("Find the full list in the menu");
- the accent swatch row of PKT-3038 on the colour scheme page.

One MR, three commits:
1. `qt: add OnboardingShortcutsModel listing the basic actions and their keys` — model + QtTest.
2. `qt: add a keyboard shortcuts page to the first-run wizard`.
3. `qt: offer the accent colour on the first-run colour page`.

## 2. Dry-run findings
Read at `ce4af2f` on `nzxt:~/src/vlc`.
- `modules/gui/qt/maininterface/mainctx.cpp:267-272` — the wizard opens (queued) when `qt-privacy-ask` is true;
  `modules/gui/qt/qt.cpp:442-443` `qt-privacy-ask` is `change_private`; `modules/gui/qt/dialogs/dialogs_provider.cpp:305-310`
  (`firstRunDialog`) shows it non-modal-transient.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.hpp:36` — `enum { WELCOME_PAGE, FOLDER_PAGE, COLOR_SCHEME_PAGE,
  LAYOUT_PAGE }`; `:37` `enum { MODERN, CLASSIC }`.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.ui:28`, `:110`, `:154`, `:373` — the four `QWizardPage`s.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:34-129` (ctor) — privacy text, colour scheme buttons and
  preview images (`:66-77`), layout buttons (`:79-100`), no Cancel (`:103`), ML folder model (`:106-117`).
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:135-180` (`finish`) — writes `metadata-network-access`,
  clears `qt-privacy-ask`, sets the colour scheme (`:142`), `qt-menubar`/`qt-titlebar`/`qt-pin-controls`
  (`:145-148`), selects or inserts the Default/Classic control bar profile (`:150-167`), saves.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:263-279` (`nextId`) — Welcome → Folder (if ML enabled) →
  Colour scheme → Layout → end. The new page goes after Layout.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:286-307` (`initializePage`) — fills scheme labels from
  `ColorSchemeModel::getSchemes()`; the shortcut page gets its `initializePage` branch.
- `modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:316-339` (`reject`) — defaults on Esc/close.
- `src/libvlc-module.c:2588-2714` — `add_key` for the actions on the page: `key-play-pause` `:2592`,
  `key-toggle-fullscreen` `:2588`, `key-jump-extrashort`/`key-jump+extrashort` `:2610-2613`, `key-vol-up`/`-down`
  `:2650-2651`, `key-prev`/`key-next` `:2604-2605`, `key-frame-previous` `:2628`/`key-frame-next` `:2626`,
  `key-subtitle-track` `:2682`, `key-audio-track` `:2675`, `key-snapshot` `:2713`, `key-record` `:2714`.
- `src/libvlc-module.c:2444-2540` — non-macOS defaults: volume is `Ctrl+Up`/`Ctrl+Down` (`:2481-2488`), next/previous are
  `n`/`p` (`:2455-2461`), seek is `Shift+Left/Right` for extra-short (`:2466-2467`), frame previous `Shift+e`
  (`:2493`); outside Windows several bindings carry a second key after a tab (`"Space\tMedia Play Pause"`, `:2459`).
  macOS has its own set (`:2317-2431`). The mock-up's keys (`←/→`, `↑/↓`, `<`/`>`, `Q`, `L`) are therefore not VLC's
  and must not be hard-coded.
- No default binding exists for "show/hide queue" or "show/hide file browser" as `key-*` actions; the queue toggle is
  a QML shortcut (`modules/gui/qt/menus/qml/GlobalShortcuts.qml`). Those two rows are dropped or replaced (§5.2).

Settings / variables involved: the `key-*` options above; `qt-privacy-ask` (`modules/gui/qt/qt.cpp:442`);
`qt-accent-color` via PKT-3038.
Mock boundary: `VLCTestingEnv` (`modules/gui/qt/tests/vlc_stub_modules.hpp:46-66`) for the real key options; the wizard itself is
not instantiated in tests (it needs `MainCtx`); its page order becomes a static function so it is testable.

### Overlaps & contradictions
- **PKT-3035** provides `HotkeysModel` and the hotkeys dialog; the shortcuts model reads bindings through it and the
  link opens it. Depends on it.
- **PKT-3038** provides the accent swatch widget and "Light/Dark" labels used on the colour page. Depends on it.
- **PKT-3015** (OSD names the hotkey) formats key names; both use `vlc_keycode2str`-style strings from config, so
  display stays consistent; share the formatter if PKT-3015 adds one.
- **PKT-3013** (one menu button): the footer says where the full list lives; text follows PKT-3013's entry name.

## 2b. Upstream state
Shipped in master: first-run wizard (Qt widgets `QWizard`) with privacy/metadata consent, media-library folders,
colour scheme with previews, layout choice applied to menubar/titlebar/pinned controls and control bar profile.
Missing: any mention of keyboard shortcuts, and accent colour. The "theme options" half of the video's request is
already done.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `OnboardingShortcutsModel` rows | `VLCTestingEnv` | construct | the fixed list of basic actions, in page order, each with a translated label and its `key-*` name(s) | `tests/test_onboarding_shortcuts.cpp::testRowsAndOrder` |
| 2 | key text | defaults | read Play/Pause row | equals the first tab-separated binding of `config_GetPsz("key-play-pause")` | `::testShowsFirstBinding` |
| 3 | paired actions | defaults | read Volume row | shows `key-vol-up` and `key-vol-down` values joined by " / " | `::testPairedKeys` |
| 4 | user rebinding | `config_PutPsz("key-snapshot", "Ctrl+F12")` | rebuild | Snapshot row shows `Ctrl+F12` | `::testFollowsUserBinding` |
| 5 | unbound action | `config_PutPsz("key-record", "")` | rebuild | row hidden (not shown with an empty cap) | `::testUnboundRowHidden` |
| 6 | page order | static `FirstRunWizard::nextPage(current, mlEnabled)` | walk from WELCOME with ML on and off | W→F→C→L→S→end and W→C→L→S→end | `::testPageOrder` |
| 7 | wizard | fresh profile (`--reset-config` or `qt-privacy-ask=1`) | go through the wizard | shortcuts page lists live keys; "See all shortcuts…" opens the hotkeys dialog; Finish saves as before | MANUAL (screenshots) |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_vlc_dialog_model.cpp:28-75` (`VLCTestingEnv`) and meson block
  `modules/gui/qt/meson.build:1404-1423`; GUI-less → `QTEST_GUILESS_MAIN`. Row 6 calls a static function, so
  `firstrunwizard.cpp` is not linked; `nextPage` lives in `onboarding_shortcuts_model.cpp` or a small header.
- Touched `key-*` options restored in `cleanup()`; no `config_SaveConfigFile` in tests.
- Sample media: none.

## 5. Design decisions & open questions
1. **NOTE** — Live bindings, not the mock-up's keys (§2: defaults differ per platform and users may have changed them).
2. **RULED** RULED 2026-10-14 (R0): replace the two rows with "Show/hide interface" (`key-intf-show`) and "Leave fullscreen" (`key-leave-fullscreen`). Rows without a `key-*` action in VLC ("Show/hide queue", "Show/hide file browser"): proposal
   replace with "Show/hide interface" (`key-intf-show`, `:2706`) and "Leave fullscreen" (`key-leave-fullscreen`,
   `:2590`). Adding new core actions for queue/browser toggles is out of scope.
3. **NOTE** — Page placement: last, after Layout; Back works; reject() (Esc) still applies defaults without the page.
4. **NOTE** — The wizard stays Qt widgets (`QWizard`); a QML onboarding is a separate effort not asked by the video.
5. **NOTE** — `upstream_ruling: not-needed`: one more informative page, nothing removed.

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
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`, branch `master-pkt-3043-onboarding-shortcuts-page` from the tip
      containing PKT-3035 and PKT-3038 (else `ce4af2f`, stubbing their pieces); §6 command once → green.
- [ ] **Step 2: Dry-run** — re-read §2; list the final rows with ruling §5.2.
- [ ] **Step 3: Row 1 watched failing** — `tests/test_onboarding_shortcuts.cpp`, `test_qt_onboarding_shortcuts` in
      `meson.build`; empty model → FAIL; implement.
- [ ] **Step 4: Rows 2–6** — one cycle each; move `nextId`'s switch (`modules/gui/qt/dialogs/firstrun/firstrunwizard.cpp:263-279`) into static
      `nextPage()` and call it from `nextId`. Commit `qt: add OnboardingShortcutsModel listing the basic actions and their keys`.
- [ ] **Step 5: Page** — `SHORTCUTS_PAGE` in `modules/gui/qt/dialogs/firstrun/firstrunwizard.hpp:36`, a `QWizardPage` in `firstrunwizard.ui` after
      `layoutPage` (`:373`) with a two-column grid filled in `initializePage`, key caps as styled `QLabel`s, and a
      "See all shortcuts…" button → `DialogsProvider::hotkeysDialog()` (PKT-3035). Row 7. Commit
      `qt: add a keyboard shortcuts page to the first-run wizard`.
- [ ] **Step 6: Accent** — PKT-3038's swatch row on `colorSchemePage` (`modules/gui/qt/dialogs/firstrun/firstrunwizard.ui:154`), written in `finish()`
      next to `:142`. Commit `qt: offer the accent colour on the first-run colour page`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MR** — MR to videolan/vlc master (Draft); `status: review`.
