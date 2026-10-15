---
id: PKT-3011
title: "An audio button lists audio tracks and the output device, with a refresh"
section: "B. Tracks & menus"
video_ref: "04:35–04:43"
pipeline: CI
kind: model
track: T2
component: modules/gui/qt/player, modules/gui/qt/util
branch: master-pkt-3011-audio-button-output-device
base_sha: ce4af2f
target_files:
  - modules/gui/qt/util/audio_device_model.hpp
  - modules/gui/qt/util/audio_device_model.cpp
  - modules/gui/qt/tests/test_audio_device_model.cpp                       # new
  - modules/gui/qt/tests/vlc_stub_modules.cpp
  - modules/gui/qt/tests/vlc_stub_modules.hpp
  - modules/gui/qt/player/control_list_model.hpp
  - modules/gui/qt/player/qml/PlayerControlbarControls.qml
  - modules/gui/qt/player/qml/controlbarcontrols/AudioButton.qml          # new
  - modules/gui/qt/player/qml/TracksTabPage.qml
  - modules/gui/qt/util/qml/TrackTabsModel.qml
  - modules/gui/qt/tests/tst_TrackTabs.qml
  - modules/gui/qt/dialogs/toolbar/controlbar_profile_model.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 14.0
priority: P1
depends_on: [PKT-3010]
upstream_ruling: needed
target: fork
mr_title: "qml: add an audio button with audio tracks and output devices"
status: ready
---

# PKT-3011 — An audio button lists audio tracks and the output device, with a refresh

## 1. Scope & intent
**Before.** Changing the output device (speakers → headphones) from the Qt interface means right-click →
Audio → Audio Device, or the global "⋯" menu → Audio → Audio Device. The control bar has no audio entry point;
`LangButton` lists audio tracks only. The device list is read **once**, when `PlayerController` is built: a
headset plugged in later never appears until VLC restarts.

**After (04:35–04:43, sheet `s006` second frame).** An **audio button** (waveform icon) sits immediately left of
the subtitle button of PKT-3010. It opens the same tabbed popup with tabs "Audio | Subtitles", Audio selected:

- section "Embedded Tracks": "None", "Track 1 - English" (accent check on the selected one);
- section "Output Device" with a **refresh** icon on the right of the title, then one row per device
  ("Speakers", "Headphones"), the current one checked; picking a row switches the device immediately;
- footer "Show Preferences" (as in PKT-3010).

The refresh icon is not decoration: master has no way to be told about hot-plugged devices (§2), so the model
gains `refresh()`, and the popup also calls it every time it opens.

One MR, three commits:
1. `qt: let the audio device model refresh its device list` — model + QtTest + stub aout.
2. `qml: add an audio tab with output devices to the track popup` — `AudioButton`, Audio tab, QML rows.
3. `qt: put the audio button in the default video layout`.

## 2. Dry-run findings
Read on nzxt at `~/src/vlc`, `base_sha` `ce4af2f`.

- `modules/gui/qt/util/audio_device_model.cpp:42-54` (`AudioDeviceModel::AudioDeviceModel`) — adds a player aout
  listener (`:46-49`), then `vlc_player_aout_Hold` and **one** `aout_DevicesList(m_aout, &m_ids, &m_names)`
  (`:51-53`). Nothing ever calls `aout_DevicesList` again; if no aout exists at construction the model stays empty.
- `modules/gui/qt/util/audio_device_model.cpp:26-40` — the only listener callback is `on_device_changed`
  (`player_aout_cbs`, `:36-40`), which updates the checked row; there is no hot-plug callback.
- `modules/gui/qt/util/audio_device_model.cpp:87-103` (`data`) — `DisplayRole` = name, `CheckStateRole` = id equals
  `m_current` (or the empty "default" id when `m_current` is empty). `:105-122` (`setData`) — a `true`
  `CheckStateRole` calls `aout_DeviceSet(m_aout, m_ids[row])`. `:124-147` (`updateCurrent`) — emits
  `dataChanged` for the old and new rows. `:149-155` (`roleNames`) — `display`, `checked`: the **same role names
  as `TrackListModel`**, so the `CheckedDelegate` of `modules/gui/qt/player/qml/TracksListPage.qml:223-251` works on it unchanged.
- `modules/gui/qt/util/audio_device_model.hpp:32-61` — no `Q_INVOKABLE`, no `count`; members `m_inputs`,
  `m_names`, `m_ids`, `m_aout`.
- `src/audio_output/output.c:123-165` (`aout_HotplugNotify`) — updates the aout's private device list under
  `owner->dev.lock` and notifies nobody. `include/vlc_player.h:2411-2418` — `vlc_player_aout_cbs` has
  `on_device_changed` but no "device list changed" event. Hence polling (`refresh()`) is the only Qt-side option
  without a core change (§5.2).
- `modules/gui/qt/player/player_controller.hpp:216` — `Q_PROPERTY(AudioDeviceModel* audioDevices … CONSTANT)`;
  `modules/gui/qt/player/player_controller.cpp:1211` builds it (`m_audioDeviceList(m_player)`), `:2249` its getter.
- `modules/gui/qt/menus/menus.cpp:412-431` (`VLCMenuBar::AudioMenu`) — "Audio &Track" and "&Audio Device"
  (`:417`) `CheckableListMenu`s on the same two models; they benefit from `refresh()` too.
- `modules/control/hotkeys.c:610-680` — `ACTIONID_AUDIODEVICE_CYCLE` re-lists devices on every press
  (`aout_DevicesList` at `:622`) and shows OSD "Audio device: %s" (`:674`): the core hotkey path already polls.
- `modules/gui/qt/tests/vlc_stub_modules.cpp:149-162` — stub modules (interface, renderer discovery, medialibrary);
  `:186-188` libvlc args `--aout=adummy`. `modules/audio_output/adummy.c` reports no device, so the test needs
  its own stub aout submodule that calls `aout_HotplugReport` (`include/vlc_aout.h:347`).
- `modules/gui/qt/meson.build:1363-1382` (`test_qt_renderer_manager`) — the QtTest registration to copy.
- `modules/gui/qt/player/control_list_model.hpp:85` `RESERVED_0X2B` (`static_assert` `:114`) — free slot for
  `AUDIO_BUTTON`.

Settings / variables involved: `aout` (`src/libvlc-module.c:1651`), the aout `device` variable
(`src/audio_output/output.c:118-121`). No new option.
Mock boundary: `VLCTestingEnv` (`tests/vlc_stub_modules.hpp`) with a new stub `audio output` submodule whose
device list the test controls; the QML rows use `ListModel`s.

### Overlaps & contradictions
- **PKT-3010** creates `TracksTabPage`, `TrackTabsModel` and `tst_TrackTabs.qml`; this packet only adds the Audio
  tab and its rows. Must merge after 3010.
- **PKT-3023** (Audio tab of the sidebar: default track, delay, equalizer) — different surface; it may reuse
  `audioDevices` but does not touch the model.
- **PKT-3003** owns the default layout; this packet inserts `AUDIO_BUTTON` left of `SUBTITLE_BUTTON` only.
- **PKT-3012** also edits `TrackTabsModel`/`tst_TrackTabs.qml` (Video tab). Same files, separate rows.

## 2b. Upstream state
Master already ships the device list and switching (right-click → Audio → Audio Device; the `AudioDeviceModel`
behind it) and the hotkey cycle with OSD. The video's "you have to get out of VLC" is only true of the control
bar. What is missing: an entry point on the bar, the device list next to the audio tracks, and a list that
follows hot-plug (a genuine master defect: snapshot at construction, `modules/gui/qt/util/audio_device_model.cpp:51-53`).

## 3. Per-test plan
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `AudioDeviceModel` | stub aout reports devices `spk`/"Speakers" | model constructed | `rowCount()==1`, row 0 `display`=="Speakers" | `test_audio_device_model.cpp::testInitialList` |
| 2 | `AudioDeviceModel::refresh` | model built with 1 device; stub then hot-plugs `hp`/"Headphones" | `refresh()` | `modelReset` emitted once, `rowCount()==2`, row 1 "Headphones" | `::testRefreshAddsHotpluggedDevice` |
| 3 | `AudioDeviceModel::refresh` | 2 devices; stub removes `hp` | `refresh()` | `rowCount()==1`; no dangling `m_ids` (ASan build in CI) | `::testRefreshRemovesDevice` |
| 4 | `AudioDeviceModel::refresh` | current device `hp` | `refresh()` | `hp` row still `checked` after the reset | `::testRefreshKeepsCurrent` |
| 5 | `AudioDeviceModel` | no aout at construction | `refresh()` once an aout exists | the list is filled (today: stays empty forever) | `::testRefreshAcquiresAout` |
| 6 | `AudioDeviceModel::setData` | 2 devices | `setData(index(1), true, CheckStateRole)` | stub's `device_select` received `hp` | `::testSelectDevice` |
| 7 | `TrackTabsModel` | audio 2 rows, subtitle 1, `requested: "audio"` | built | tabs `["audio","subtitle"]`, current `"audio"` | `tst_TrackTabs.qml::test_audio_requested` |
| 8 | Audio tab | headset plugged while VLC runs | open the popup / press refresh | headset listed; selecting it moves the sound | MANUAL — screenshot `pkt3011-audio-popup.png` |

## 4. Fixtures & mocks
- Pattern to copy: `modules/gui/qt/tests/test_renderer_manager_model.cpp:29-86` (`TestClass` with a
  `VLCTestingEnv`, `init`/`cleanup` per test) and its meson block `modules/gui/qt/meson.build:1363-1382`.
- Stubs: add to `modules/gui/qt/tests/vlc_stub_modules.cpp:149-162` an `add_submodule() set_capability("audio output", 0)
  add_shortcut("aouttest")` whose `Open` sets `aout->device_select` and reports the devices held in
  `VLCTestingEnv` (new fields `std::vector<std::pair<std::string,std::string>> aoutDevices` and an
  `audio_output_t* aout` pointer so the test can call `aout_HotplugReport` later); switch `--aout=adummy` to
  `--aout=aouttest` only for this test (new `VLCTestingEnv::extraArgs`, so the other Qt tests keep `adummy`).
- The player comes from the stub interface: `vlc_playlist_GetPlayer(vlc_intf_GetMainPlaylist(env.intf->intf))`.
- Sample media: none for rows 1–7; any audio file for row 8.

## 5. Design decisions & open questions
1. **RULED** RULED 2026-10-14 (R0): a second (audio) track button on the bar is accepted, same answer as PKT-3010 §5.1. A second track button (audio) on the bar, beside the subtitle one, contradicts the single
   `LangButton` grouping of VLC 4 (as in PKT-3010 §5.1). Same ruling covers both packets.
2. **RULED** RULED 2026-10-14 (R0): Qt-only `refresh()` on popup open plus a refresh icon; no core API change in this packet. Polling vs. core event. `refresh()` on popup open + a refresh icon is Qt-only. The clean fix is a
   new `on_devices_changed` in `vlc_player_aout_cbs` fired from `aout_HotplugNotify`; that is a core API change
   and is **out of scope** here (follow-up packet if VideoLAN prefers it; then the refresh icon can go).
3. NOTE — `refresh()` uses `beginResetModel`/`endResetModel`; `CheckableListMenu` (`modules/gui/qt/menus/menus.cpp:417`) listens to
   resets, so the classic menu also picks up new devices (checked in row 8).
4. NOTE — the stub aout changes `vlc_stub_modules.cpp`, shared by every Qt QtTest: the new submodule has priority 0
   and is only selected by `--aout=aouttest`, so the other tests are unaffected (Step 8 runs the whole qt suite).

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
- [ ] **Step 1: Work clone** — on nzxt, `cd ~/src/vlc`, branch `master-pkt-3011-audio-button-output-device` from
      the merge of PKT-3010; §6 command → green.
- [ ] **Step 2: Dry-run** — re-read §2; confirm the player owns an aout right after `libvlc_InternalPlay` in the
      test env (else row 5 is the first row, not the last); confirm `CheckableListMenu` handles `modelReset`.
- [ ] **Step 3: Stub + row 1 watched failing** — add the `aouttest` submodule and `VLCTestingEnv` fields; create
      `tests/test_audio_device_model.cpp` with row 1 expecting 2 rows → FAIL; then the real expectation → PASS.
      Register `test_qt_audio_device_model` in `meson.build` after `:1382` with sources
      `tests/vlc_stub_modules.cpp`, `tests/test_audio_device_model.cpp`, `util/audio_device_model.cpp`
      (+ moc of the test and `util/audio_device_model.hpp`).
- [ ] **Step 4: Rows 2–6** — add `Q_INVOKABLE void refresh()` and `Q_PROPERTY(int count …)` to
      `AudioDeviceModel`; refresh re-holds the aout when `m_aout` is null, frees and re-reads `m_ids`/`m_names`
      inside a model reset. One TDD cycle per row. Commit 1 `qt: let the audio device model refresh its device list`.
- [ ] **Step 5: Row 7** — Audio tab in `TrackTabsModel`, new test function in `tst_TrackTabs.qml`.
- [ ] **Step 6: UI** — `AUDIO_BUTTON` takes `RESERVED_0X2B` (`modules/gui/qt/player/control_list_model.hpp:85`, keep the
      `static_assert` on 0x2B updated); `controlbarcontrols/AudioButton.qml` (copy of `SubtitleButton.qml`,
      `requested: "audio"`); in `TracksTabPage.qml` the Audio tab = audio tracks list + "Output Device" section on
      `Player.audioDevices` with a refresh `IconToolButton` (`VLCIcons.ic_fluent_arrow_sync_24_regular`,
      `modules/gui/qt/style/vlcicons.hpp:214`) and `Player.audioDevices.refresh()` in the popup's `onOpened`. meson + POTFILES.
      Row 8 by hand. Commit 2 `qml: add an audio tab with output devices to the track popup`.
- [ ] **Step 7: Default layout** — `AUDIO_BUTTON` left of `SUBTITLE_BUTTON` in `controlbar_profile_model.cpp`.
      Commit 3 `qt: put the audio button in the default video layout`.
- [ ] **Step 8: Whole gate** — §6 command (whole `qt` suite, §5.4); summary into §6 Evidence.
- [ ] **Step 9: MR** — PR to the fork github.com/oscarolar/vlc (base `master`), §5.1–5.2 listed as recorded rulings; `status: review`.
