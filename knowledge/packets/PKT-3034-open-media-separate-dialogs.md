---
id: PKT-3034
title: "Each Media-menu entry opens its own single-purpose open dialog instead of the tabbed media factory"
section: "D. Open media"
video_ref: "12:28–13:11"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/dialogs/open
branch: master-pkt-3034-open-media-separate-dialogs
base_sha: ce4af2f
target_files:
  - modules/gui/qt/dialogs/open/open.hpp
  - modules/gui/qt/dialogs/open/open.cpp
  - modules/gui/qt/dialogs/open/open_mode.hpp
  - modules/gui/qt/dialogs/dialogs_provider.cpp
  - modules/gui/qt/menus/menus.cpp
  - modules/gui/qt/tests/test_open_mode.cpp
  - modules/gui/qt/meson.build
estimate_h: 14.0
priority: P2
depends_on: [PKT-3013]
upstream_ruling: needed
target: fork
mr_title: "qt: open a single-purpose dialog for each Media menu entry"
status: ready
---

# PKT-3034 — Each Media-menu entry opens its own single-purpose open dialog instead of the tabbed media factory

## 1. Scope & intent
At 12:28–13:11 the "add network stream" button opens the classic Open Media dialog, which the designer calls "an
all-in-one media factory": four tabs (File, Disc, Network, Capture Device) and a Play button whose drop-down offers
Enqueue / Play / Stream / Convert (frame `s015` 12:36–12:48). Her points: enqueue here is redundant with the queue;
stream and convert "are extremely important and should be separate, not hidden"; the Media menu **already** has an
entry per source (Open File, Open Directory, Network Stream, Capture Device, Clipboard, Recent, Convert, Stream), so
"each of them can be a dialog like this one" (frame `s015` 12:54–13:06: one small dialog per entry; the reorganised
menu Open File(s), Open Folder(s), Open Disc, Open Network Stream, Open Capture Device, Open from Clipboard, Convert,
Stream, Open Recent…).

After this packet:
1. Media ▸ Open Disc / Open Network Stream / Open Capture Device each open a dialog **without the tab bar**, titled
   after the source, with one primary button (Play) and Cancel; Enqueue/Stream/Convert are no longer in a drop-down.
2. Media ▸ Convert and Media ▸ Stream keep the file/disc/network/capture choice (they need a source) but their
   primary button says Convert / Stream and the drop-down is gone.
3. The queue's "Advanced Open…" and VLM's `SELECT` mode keep today's tabbed dialog (nothing is removed).

One MR, three commits:
1. `qt: describe open dialog modes in a testable table`
2. `qt: open dialog single-source mode without tabs or action drop-down`
3. `qt: route Media menu entries to the single-source open dialogs`

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`.
- `modules/gui/qt/menus/menus.cpp:174-215` (`VLCMenuBar::FileMenu`) — Open File (`simpleOpenDialog`, a plain
  `QFileDialog`, `:181-182`), Open Multiple Files (`openFileDialog` → tabbed dialog, `:183-184`), Open Directory
  (`PLOpenDir`, `:185-186`), Open Disc / Network / Capture (`:187-191`), Open Location from clipboard
  (`openUrlDialog`, `:193-194`), Open Recent (`:196-206`), Save playlist, Convert / Save (`:210-211`), Stream (`:212-213`).
- `modules/gui/qt/dialogs/dialogs_provider.cpp:631-662` (`openDialog(tab)`, `openDiscDialog`, `openNetDialog`,
  `openCaptureDialog`, `PLAppendDialog`) — every source entry calls `m_openDialog->showTab(tab, OPEN_AND_PLAY)` on the
  **same** tabbed dialog; `:958-967` Stream/Convert call `showTab(OPEN_FILE_TAB, OPEN_AND_STREAM | OPEN_AND_SAVE)`.
- `modules/gui/qt/dialogs/dialogs_provider.cpp:723-747` (`openUrlDialog`) + `modules/gui/qt/dialogs/open/openurl.cpp:41-80`
  (`OpenUrlDialog` ctor) — already a single-purpose URL dialog (Play / Enqueue / Cancel, clipboard prefill): the
  "dialog like this one" exists for the clipboard entry.
- `modules/gui/qt/dialogs/open/open.cpp:43-101` (`OpenDialog::OpenDialog`) — inserts the four panels into
  `ui.Tab` (`:58-66`), builds the Play drop-down Enqueue/Play/Stream/Convert with Alt+E/P/S/O (`:84-94`).
- `modules/gui/qt/dialogs/open/open.cpp:170-199` (`setMenuAction`) — maps `ActionFlag` to the Play button text;
  `:226-238` (`showTab`) selects the tab and shows; `:342-366` (`enqueue`) collects MRLs from **all** panels
  (`:352-353`), so hiding the tab bar must also stop other panels from contributing.
- `modules/gui/qt/dialogs/open/open.hpp:46-60` — `OpenTab` and `ActionFlag` enums.
- `modules/gui/qt/dialogs/vlm/vlm.cpp:349` — `setActionFlag(OpenDialog::SELECT)`: a second user of the tabbed mode.
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:1131-1135` — the queue context menu "Advanced Open…" →
  `PLAppendDialog()`; keeps the tabbed dialog.

Settings / variables involved: none (window geometry saved per role by `QVLCDialog`).
Mock boundary: the mode table (`open_mode.hpp`: tab × action → title, primary label, tab bar visible, drop-down
visible, contributing panels) is header-only and tested without widgets; the dialog itself is MANUAL.

### Overlaps & contradictions
- **PKT-3013** replaces the duplicated top menu with one menu button; it owns the menu **layout and order**
  (frame `s015`), this packet owns what each entry opens. 3013 merges first; if it renames entries, Step 2 follows it.
- **PKT-3031** uses the network single-source dialog as its "Add network stream" target.
- **PKT-3017** (add-subtitles dialog) is a separate dialog; the slave/subtitle field of the advanced panel
  (`modules/gui/qt/dialogs/open/open.cpp:68-71`) stays where it is.
- **PKT-3033**'s Recent tab overlaps Media ▸ Open Recent (`modules/gui/qt/menus/menus.cpp:196-206`); not touched here.

## 2b. Upstream state
Master still ships the VLC 3 tabbed `OpenDialog` unchanged in substance; the Media menu already has one entry per
source but they all land on the tabbed dialog. Open File and Open Location from clipboard are already single-purpose.
The packet narrows to: tab-less single-source presentation, a fixed primary action, and routing.

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `openModeFor(OPEN_NETWORK_TAB, OPEN_AND_PLAY, SingleSource)` | — | lookup | title "Open Network Stream", primary "Play", tabs hidden, drop-down hidden, panels {network} | `tests/test_open_mode.cpp::testNetworkSingle` |
| 2 | `openModeFor(OPEN_DISC_TAB, …)` / capture | — | lookup | same shape, own title, panels {disc} / {capture} | `test_open_mode.cpp::testDiscAndCaptureSingle` |
| 3 | `openModeFor(OPEN_FILE_TAB, OPEN_AND_SAVE, SourceChoice)` | — | lookup | title "Convert", primary "Convert / Save", tabs visible, drop-down hidden | `test_open_mode.cpp::testConvertKeepsTabs` |
| 4 | `openModeFor(any, SELECT, Tabbed)` and `PLAppendDialog` | — | lookup | today's behaviour: tabs + drop-down visible | `test_open_mode.cpp::testLegacyTabbedUnchanged` |
| 5 | `enqueue()` panel set | single-source network | accept with a stale file selected earlier | only the network MRL is appended | MANUAL `pkt3034-network-only.png` (+ log of the queue) |
| 6 | Media ▸ Open Network Stream | — | screenshot | small dialog, no tabs, Play/Cancel | MANUAL `pkt3034-network.png` |
| 7 | Media ▸ Stream | — | screenshot | source tabs, primary "Stream", no drop-down | MANUAL `pkt3034-stream.png` |

## 4. Fixtures & mocks
- Pattern to copy: `tests/test_thread_runner.cpp` meson block (`modules/gui/qt/meson.build:1383-1400`, no libvlc link) — the mode
  table needs only Qt Core and `open.hpp`'s enums (moved to `open_mode.hpp` so the test does not pull widgets).
- Stubs: none.
- Sample media: MANUAL rows use any `http://` stream URL and a local file.

## 5. Design decisions & open questions
1. **RULED — removing the action drop-down** RULED 2026-10-14 (R0): the action drop-down is removed from the source dialogs; Enqueue is reached through the queue's "Advanced Open..." and Stream/Convert become their own menu entries. from the source dialogs drops Enqueue/Stream/Convert from that path
   (Alt+E/P/S/O shortcuts, `modules/gui/qt/dialogs/open/open.cpp:84-92`). They stay reachable: Enqueue via the queue's "Advanced Open…", Stream and
   Convert as their own menu entries. Long-time users rely on Alt+E.
2. NOTE — "Open Multiple Files" keeps the tabbed file panel (it is the only place with the file list and the
   subtitle/slave option); the video's menu calls it "Open File(s)" — wording is PKT-3013's.
3. NOTE — `enqueue()` iterating all panels (`modules/gui/qt/dialogs/open/open.cpp:352-353`) is correct for the tabbed dialog (only the current
   panel has a MRL after `signalCurrent`) but must be restricted to the mode's panel set in single-source mode; row 5.
4. NOTE — one shared `m_openDialog` instance is kept (window-role geometry); a mode switch reconfigures it on
   `showTab`, so two single-source dialogs cannot be open at once — same as today.

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
<Filled by the executor.>

## 7. Agent brief
- [ ] **Step 1: Work clone** — on `nzxt`, `cd ~/src/vlc && git switch -c master-pkt-3034-open-media-separate-dialogs` on top of merged PKT-3013; §6 build → green.
- [ ] **Step 2: Dry-run** — re-read §2, especially `modules/gui/qt/menus/menus.cpp:174-215` as left by 3013.
- [ ] **Step 3: First row watched failing** — `tests/test_open_mode.cpp` + `vlc_tests` block; row 1 against an empty table → FAIL.
- [ ] **Step 4: Mode table** — `dialogs/open/open_mode.hpp` (enums moved from `modules/gui/qt/dialogs/open/open.hpp:46-60`, `constexpr` table); rows 1–4 green. Commit `qt: describe open dialog modes in a testable table`.
- [ ] **Step 5: Dialog** — `OpenDialog::showTab(tab, action, presentation)`: `ui.Tab->tabBar()->setVisible`, `playButton->setMenu(nullptr|menu)`, title, restrict `enqueue()` panels; screenshots rows 5–7. Commit `qt: open dialog single-source mode without tabs or action drop-down`.
- [ ] **Step 6: Routing** — `modules/gui/qt/dialogs/dialogs_provider.cpp:644-655` and `:958-967` pass the presentation; `PLAppendDialog` and VLM untouched. Commit `qt: route Media menu entries to the single-source open dialogs`.
- [ ] **Step 7: Whole gate** — §6 command; Evidence.
- [ ] **Step 8: Commit and MR** — Draft MR; `status: review`.
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
