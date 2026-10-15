---
id: PKT-3017
title: "Add subtitles to this file, this folder or this playlist, from disk or OpenSubtitles"
section: "C. Subtitles"
video_ref: "05:39–06:24"
pipeline: CI
kind: model
track: T3
component: modules/gui/qt/player
branch: master-pkt-3017-add-subtitles-dialog
base_sha: ce4af2f
target_files:
  - src/input/subtitles.c
  - test/libvlc/slaves.c
  - test/samples/slaves/
  - modules/gui/qt/player/subtitle_match_model.cpp
  - modules/gui/qt/player/subtitle_match_model.hpp
  - modules/gui/qt/player/qml/AddSubtitlesDialog.qml
  - modules/gui/qt/player/qml/TracksListPage.qml
  - modules/gui/qt/tests/test_subtitle_match_model.cpp
  - modules/gui/qt/meson.build
  - po/POTFILES.in
estimate_h: 30.0
priority: P2
depends_on: [PKT-3010]
upstream_ruling: needed
target: fork
mr_title: "qml: add subtitles to a file, folder or playlist in one dialog"
status: ready
---

# PKT-3017 — Add subtitles to this file, this folder or this playlist, from disk or OpenSubtitles

## 1. Scope & intent
05:39–05:58: the designer downloads a zip of subtitles for a show, extracts it next to the episodes, and the
names do not match (`The_Wand_S01E04.mp4` vs `thewand_s01e04.srt`, frame `s007` ≈05:45), so VLC does not pick
them up and every episode needs a manual drag. 06:00–06:22: the subtitle section gets an **Add** action that
opens an **Add Subtitles** dialog. Frames `s007`/`s008` (≈06:05–06:20) show it: a modal panel over the
blurred video with a close ×, a first row of three controls — scope dropdown **To This File / To This Folder /
To This Playlist**, source dropdown **From This Computer / From OpenSubtitles**, a search field prefilled
"The Wand S01" and a language dropdown "English" — then a two-column list pairing each video
(`The_Wand_S01E01.mp4`) with a subtitle file (`The_Wand_S01E01.srt`) joined by a dotted leader, and
*Cancel* / *Save* (orange) at the bottom. Matches are automatic; any row can be re-pointed by hand; the
association is remembered for later episodes.

This is **T3** and is split before starting (§5.6). The whole intent, in three deliverables:
1. **Core**: autodetect matches on an **episode key** (`S01E04`, `1x04`) when names differ only in separators and
   case — the video's exact failure, fixed for everyone with no UI at all.
2. **Qt**: a `SubtitleMatchModel` (C++) that lists the videos of the scope, proposes a subtitle per video
   (same key rules), accepts manual overrides, and applies them; and the QML dialog over it.
3. **OpenSubtitles** as a source — unblocked by R4 (per-user credentials, §5.3).

Commits (one MR per split packet): `input: match subtitles by episode number`,
`qt: add SubtitleMatchModel`, `qml: add the add-subtitles dialog`, later
`qml: search OpenSubtitles from the add-subtitles dialog`.

## 2. Dry-run findings
Read at `base_sha` on `nzxt:~/src/vlc`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `src/input/subtitles.c:59-111` (`filename_trim_inplace`) — lowercases and collapses every run of
  non-alphanumerics to one space: `The_Wand_S01E04` → `the wand s01e04`, `thewand_s01e04` →
  `thewand s01e04`. The two never compare equal.
- `src/input/subtitles.c:220-340` (`subtitles_Detect`) — per candidate in the video's directory and in
  `sub-autodetect-path` subdirs: exact trimmed name → `SLAVE_PRIORITY_MATCH_ALL`, video name followed by only
  whitespace → `MATCH_RIGHT`, video name inside → `MATCH_LEFT`, anything else in the same dir →
  `MATCH_NONE` (`:292-318`); kept only when `i_prio >= i_fuzzy` (`:322`). With the default fuzzy `3` the
  video's files land at `MATCH_NONE` (1) and are dropped; with fuzzy `1` **every** `.srt` of the folder is
  attached to every episode — neither is what the user wants.
- `include/vlc_input_item.h:156-160` — `SLAVE_PRIORITY_MATCH_NONE = 1 … MATCH_ALL, USER`.
- `include/vlc_input_item.h:273-281` — `input_item_slave_New`, `input_item_AddSlave`: the API to attach a
  subtitle to an item **before** it plays (what "this playlist" needs for items not yet playing).
- `test/libvlc/slaves.c:128-205` — the existing test: four sample slaves under `test/samples/slaves/`
  (`test.aac`, `test.rt.srt`, `lt-test.srt`, `nomatch.srt`) checked at fuzzy 1–4 and with
  `--no-sub-autodetect-file`; registered `test/libvlc/meson.build:141-147` (`test_libvlc_slaves`, suite
  `libvlc`).
- `modules/gui/qt/player/qml/TracksListPage.qml:256-273` — the subtitle column's menu: `Open` →
  `DialogsProvider.loadSubtitlesFile()`, `Synchronize` → subtitle page, `Download` → `Player.openVLsub()`.
- `modules/gui/qt/menus/qml_menu_wrapper.cpp:743-760` (`QmlSubtitleMenu::beforePopup`) — builds those three
  actions.
- `modules/gui/qt/dialogs/dialogs_provider.cpp:970-1001` (`loadMediaFile`, `loadSubtitlesFile`) — a file picker
  opened in the current item's folder; each picked URL goes to `THEMIM->AddAssociatedMedia(SPU_ES, …)`
  (`modules/gui/qt/player/player_controller.cpp:2229-2234` → `vlc_player_AddAssociatedMedia`): **current item only**, session only.
- `modules/gui/qt/player/player_controller.cpp:1926-1935` (`openVLsub`) — loads the Lua extensions and opens
  VLSub.
- `share/lua/extensions/VLSub.lua:359-369` (`descriptor`, "VLsub 0.11.1") and `:1040-1050` (`openSub.conf`) —
  talks to `https://api.opensubtitles.org/xml-rpc` with user agent `VLSub 0.11.1`; `:1663-1669` (`add_sub`) ends
  in `vlc.player.add_subtitle(path, true)`, again current item only.

Settings / variables involved: `sub-autodetect-file` (`src/libvlc-module.c:1817`), `sub-autodetect-fuzzy`
(`:1819`), `sub-autodetect-path` (`:1826`), `sub-file` (`:1815`), `sub-language` (`:1870`, for the dialog's
language default).
Mock boundary: core rows use real sample files and libvlc (as `slaves.c` does). Qt rows build a temporary
directory of empty `.mp4`/`.srt` files (`QTemporaryDir`) and a `vlc_playlist` from `VLCTestingEnv`; no network.
OpenSubtitles rows (split 3017c) mock the HTTP client.

### Overlaps & contradictions
- **PKT-3010** owns the subtitle track list and its header button; this packet replaces the `Open` action of
  `modules/gui/qt/player/qml/TracksListPage.qml:256-273` with "Add…" opening the dialog. Merge after 3010.
- **PKT-3037** (autodetect fuzziness in words) edits the *meaning* of `sub-autodetect-fuzzy` levels in
  preferences; the episode-key priority added here must be named in that packet's wording (§5.2).
- **PKT-3022** (per-show presets) may store "the subtitle choice for this show"; the persistence ruling of §5.4 (R5)
  must be the same storage.
- **PKT-3034** (open-media dialogs split) touches `dialogs_provider.cpp`; this packet does not edit it.

## 2b. Upstream state
- Autodetect exists and is good for exact or prefixed names; it fails exactly in the video's case (different
  separators/case around the show name). The episode key is the missing rule.
- Manual add exists for the **current** item only (file picker, drag-and-drop, VLSub). There is no folder or
  playlist scope and nothing is remembered between sessions.
- OpenSubtitles exists through **VLSub**, a Lua extension dialog on the legacy XML-RPC endpoint.

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | `subtitles_Detect` episode key | `The_Wand_S01E04.mp4` + `thewand_s01e04.srt` + `thewand_s01e05.srt` in one dir, fuzzy 3 | the media is parsed | exactly one slave, `thewand_s01e04.srt`, priority ≥ 3 | `test/libvlc/slaves.c` — new block "episode key" |
| 2 | same | `Show.1x04.mkv` + `show 1x04.srt` | parsed | matched (`1x04` ≡ `S01E04`) | `test/libvlc/slaves.c` — same block |
| 3 | same | `The_Wand_S01E04.mp4` + `other_S01E04.srt` (different show words) | parsed at fuzzy 3 | **not** matched: the key alone is not enough, the show words must overlap | `test/libvlc/slaves.c` — same block |
| 4 | existing rows | samples of `test/libvlc/slaves.c:131-143` | fuzzy 1–4 | unchanged results (no regression of the four levels) | `test/libvlc/slaves.c` (existing) |
| 5 | `SubtitleMatchModel::setScope(Folder)` | temp dir with 3 videos + 3 subs of the video's naming | `refresh()` | 3 rows, each video paired with its episode's subtitle | `test_subtitle_match_model.cpp::testFolderPairsByEpisode` |
| 6 | same | one video without a subtitle | `refresh()` | that row's subtitle is empty and `matchState == Missing` | `test_subtitle_match_model.cpp::testMissingIsReported` |
| 7 | `setSubtitle(row, url)` | row 2 auto-matched | user picks another file | row 2 holds the override, `matchState == Manual` | `test_subtitle_match_model.cpp::testManualOverride` |
| 8 | `setScope(Playlist)` | playlist of 3 items (2 not started) | `apply()` | each item's `input_item_t` has a slave with `SLAVE_PRIORITY_USER` for its subtitle | `test_subtitle_match_model.cpp::testApplyToPlaylistAddsSlaves` |
| 9 | `setScope(File)` | current item playing | `apply()` | `vlc_player_AddAssociatedMedia(SPU_ES, …, select=true)` path taken (one track added) | `test_subtitle_match_model.cpp::testApplyToCurrentFile` |
| 10 | `AddSubtitlesDialog.qml` | a show folder playing | Subtitles → Add… | dialog as in frame `s007`: scope, source, search, language, paired list, Cancel/Save | MANUAL — `pkt3017-dialog.png` |

## 4. Fixtures & mocks
- Core pattern: `test/libvlc/slaves.c:128-205`; new samples under `test/samples/slaves/episodes/` (empty files
  are enough — the test uses `--codec none`, `test/libvlc/slaves.c:149-152`), registered through the existing
  `SLAVES_DIR` define; no new test binary.
- Qt pattern: `modules/gui/qt/tests/test_ml_model.cpp` + its `vlc_tests` entry `modules/gui/qt/meson.build:1306-1340`
  (`playlist/playlist_controller.cpp` and `tests/vlc_stub_modules.cpp` already compiled in there), with
  `QTemporaryDir` for the files.
- Stubs: none for 3017a/b; 3017c needs a fake HTTP layer (`QNetworkAccessManager` subclass) and a fake `vlc_keystore` for the stored credentials — no live network
  in CI.

## 5. Design decisions & open questions
1. **RULED — core change in `subtitles_Detect`.** RULED 2026-10-14 (R0): the episode-key rule in `subtitles_Detect` is accepted as proposed (priority `MATCH_LEFT`). Matching on an episode key is a behaviour change of a
   VideoLAN core heuristic (`src/input/subtitles.c`). It needs upstream review; the rule proposed: extract
   `s(\d+)e(\d+)` or `(\d+)x(\d+)` from both trimmed names; equal keys **and** at least one shared alphanumeric
   word or one name being a prefix of the other once spaces are removed (`thewand` vs `the wand`) → priority
   `MATCH_LEFT` (3), so the default fuzzy level accepts it.
2. NOTE — PKT-3037 must describe the new rule in its human wording of the fuzzy levels.
3. **RULED — OpenSubtitles.** RULED 2026-10-14 (R4): per-user OpenSubtitles credentials: the dialog asks for the user's own account and API key on first use (REST API `api.opensubtitles.com`) and stores them in the system keystore (`vlc_keystore`); no key ships in the binary; 3017c is unblocked. VLSub uses `api.opensubtitles.org/xml-rpc` (`VLSub.lua:1044`), the legacy API
   OpenSubtitles has been retiring in favour of the REST API at `api.opensubtitles.com`, which requires a
   registered **API key** per application and user login for downloads, with rate limits. Shipping a key in VLC,
   or asking each user for one, was the legal question (terms of service, privacy of the hash lookup), answered by the per-user option. The dialog's "From OpenSubtitles" entry is the source of split 3017c; VLSub stays as it is.
4. **RULED — "saved for all future episodes".** RULED 2026-10-14 (R5): preset bodies and subtitle-episode assignments live in the Qt settings file, keyed by the media-library group/media when the video is indexed and by the parent folder otherwise; this supersedes the (c)/(a) recommendation below. Options: (a) apply to playlist items only (session);
   (b) remember a folder→subtitle-folder mapping in Qt settings; (c) store per media in the media library
   (`vlc_ml_media_set_playback_state`, `VLC_ML_PLAYBACK_STATE_APP_SPECIFIC`, `include/vlc_media_library.h:677-696`);
   (d) copy/rename the subtitle files next to the videos. (d) writes into the user's folders and is refused here;
   recommend (c) for library media and (a) otherwise. Same answer must serve PKT-3022.
5. NOTE — "To This Folder" lists the video files of the current item's directory (non-recursive), "To This
   Playlist" the play queue items; both through the model, so the QML stays thin.
6. **Split (T3, > 16 h):** **3017a** core episode-key match + rows 1–4 (6 h, `upstream_ruling: needed`);
   **3017b** `SubtitleMatchModel` + dialog, local source, rows 5–10 (16 h); **3017c** OpenSubtitles source
   (8 h, unblocked by R4). 3017a is independent and useful alone; 3017b reuses its key function through a small
   shared helper or a Qt-side copy (Step 2 decides — no core header for it exists today).

## 6. Definition of Done
```bash
# all on nzxt, inside the CI image (never on the Mac)
docker --context nzxt run --rm -v ~/src/vlc:/vlc -w /vlc registry.videolan.org/vlc-debian-unstable:20260611225331 \
  sh -c 'meson setup build-meson -Dextra_checks=true && ninja -C build-meson && meson test -C build-meson --suite qt --suite libvlc --print-errorlogs'
```
- Every row of §3 exists, passes, and was watched failing first; MANUAL rows have their screenshot in the MR.
- `ninja -C build-meson` has no new warning in `modules/gui/qt` or `src/input`.
- Strings added are marked for translation (`qsTr` / `qtr`) and `po/POTFILES.in` is updated when a file is new.
- Keyboard navigation (`Navigation.*`) and accessibility (`Accessible.*`) work on every new control.

### Evidence
<Filled by the executor: build tail, `meson test` summary, screenshots, MR URL.>

## 7. Agent brief
- [ ] **Step 0: Split** — create packets 3017a/b/c from §5.6 before any code; this file becomes the parent.
- [ ] **Step 1: Work clone** — on `nzxt:~/src/vlc`: `git switch -c master-pkt-3017-add-subtitles-dialog ce4af2f` (one branch per split, suffix `-a/-b/-c`); build once with §6 → green.
- [ ] **Step 2: Dry-run** — re-read §2 ranges; run `test_libvlc_slaves` alone and note its output.
- [ ] **Step 3 (3017a): Row 1 watched failing** — add the episode samples and the new block in `test/libvlc/slaves.c` → FAIL (no slave found at fuzzy 3).
- [ ] **Step 4 (3017a): Rows 1–4** — episode-key rule in `src/input/subtitles.c`; commit `input: match subtitles by episode number`.
- [ ] **Step 5 (3017b): Rows 5–9** — `player/subtitle_match_model.{hpp,cpp}`, `tests/test_subtitle_match_model.cpp` registered in `meson.build`; commit `qt: add SubtitleMatchModel`.
- [ ] **Step 6 (3017b): Row 10** — `player/qml/AddSubtitlesDialog.qml`, "Add…" in `TracksListPage.qml`, `po/POTFILES.in`; commit `qml: add the add-subtitles dialog`.
- [ ] **Step 7: Whole gate** — §6 command; summary in §6 Evidence.
- [ ] **Step 8: MRs** — 3017a as its own Draft MR (`input:`), 3017b after it; `status: review`. 3017c follows 3017b (per-user credentials, R4).
  Destination: PR to the fork github.com/oscarolar/vlc (base `master`).
