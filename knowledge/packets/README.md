# VLC redesign — work packets

Rulings: ../DECISIONS.md (2026-10-14). Fork repo: https://github.com/oscarolar/vlc (remote `fork` in nzxt:~/src/vlc).

Source: Juxtopposed, "I Redesigned VLC After 25 Years" (https://www.youtube.com/watch?v=ERFeOgqk324),
Figma: https://www.figma.com/community/file/1691465999134370980.
Target code: VLC master `ce4af2f` (Qt 6 / QML interface, `modules/gui/qt`), clone at `nzxt:~/src/vlc`.
Packet format: `_TEMPLATE.md` (adapted from nhomar/irc-second-plan `knowledge/packets/_TEMPLATE.md`).

| ID | Area | Packet | Video | Depends on | Target |
|---|---|---|---|---|---|
| PKT-3001 | A. Visual system | Design tokens: orange accent, neutral greys, softer corners | 00:36–01:32 | — | fork |
| PKT-3002 | A. Visual system | Two icon sets: two-tone for menus, monochrome everywhere else | 00:59–01:14 | 3001 | fork |
| PKT-3003 | A. Control bar | Default layout: play left, prev/next around it, fullscreen/settings right | 01:45–02:23 | 3001 | fork |
| PKT-3004 | A. Control bar | "Playlist" toggle becomes the Queue | 02:23–02:55 | 3003 | fork |
| PKT-3005 | A. Control bar | Volume slider as a full cone | 02:55–03:10 | 3001 | fork |
| PKT-3006 | A. Control bar | Thick seek bar at the bottom edge, one thin control row | 03:10–03:24 | 3003 | fork |
| PKT-3007 | A. Control bar | Thumbnail preview while seeking | 03:24–03:30 | 3006 | fork |
| PKT-3008 | A. Chrome | Top bar kept in fullscreen; both bars reveal on hover near the edge | 03:30–03:54 | — | fork |
| PKT-3009 | A. Chrome | Fullscreen and picture-in-picture buttons in the top bar | 03:54–04:04 | 3008 | fork |
| PKT-3010 | B. Tracks & menus | Subtitle button opens the subtitle track list | 04:06–04:35 | 3003 | fork |
| PKT-3011 | B. Tracks & menus | Audio button: audio tracks plus audio output device | 04:35–04:43 | 3010 | fork |
| PKT-3012 | B. Tracks & menus | Video tracks tab (no bar button) | 04:43–04:55 | 3010 | upstream |
| PKT-3013 | B. Tracks & menus | One menu button replaces the duplicated top menu bar | 04:55–05:13 | 3003 | fork |
| PKT-3014 | B. Tracks & menus | Context menu cleanup and "Hide interface" entry | 05:13–05:30 | 3013 | fork |
| PKT-3015 | B. Feedback | OSD names the hotkey that triggered the action | 15:18–15:32 | — | fork |
| PKT-3016 | B. Feedback | Resume prompt next to the central controls | 13:58–14:10 | 3003 | fork |
| PKT-3017 | C. Subtitles | Add-subtitles dialog: file / folder / playlist scope, local or OpenSubtitles | 05:39–06:24 | 3010 | fork |
| PKT-3018 | C. Sidebar | Player settings sidebar with four tabs (replaces the extended panel) | 06:30–07:16 | 3010 | fork |
| PKT-3019 | C. Subtitles | Subtitle delay / speed / duration as sliders | 07:16–07:22 | 3018 | upstream |
| PKT-3020 | C. Subtitles | Subtitle timeline for visual sync | 06:54–07:03 | 3018 | fork |
| PKT-3021 | C. Subtitles | Subtitle appearance live, sizes relative or in pixels | 07:22–07:46 | 3018 | fork |
| PKT-3022 | C. Subtitles | Per-show presets with default subtitle/audio track or language | 07:46–08:41 | 3021, 3023 | fork |
| PKT-3023 | C. Audio | Audio tab: default track, delay, equalizer with friendly presets | 08:41–08:56 | 3018 | upstream |
| PKT-3024 | C. Video | Video tab: filters with previews, flip/rotate/zoom/aspect | 08:56–09:07 | 3018 | fork |
| PKT-3025 | C. Playback | Playback tab: speed, reverse, sleep timer, timeline/waveform/metadata toggles | 09:07–09:18 | 3018, 3020 | fork |
| PKT-3026 | C. Bookmarks | Bookmarks and chapters in the sidebar, one-key bookmark | 14:52–15:18 | 3018 | fork |
| PKT-3027 | D. Library | Library sections cleanup and clickable address bar | 09:24–10:28 | — | fork |
| PKT-3028 | D. Library | Visible sort / view / search, "unwatched first" sort | 10:28–10:57 | 3027 | upstream |
| PKT-3029 | D. Library | Show-name entry point and compact library beside the video, drag to queue | 11:00–11:58 | 3004, 3027 | fork |
| PKT-3030 | D. Library | Music playback view: large artwork, waveform seek bar | 11:58–12:22 | 3006 | fork |
| PKT-3031 | D. Library | Network streams: add by URL, remote list becomes a playlist | 12:22–13:21 | 3034 | fork |
| PKT-3032 | D. Library | Playlists: create from a button, "add to playlist" everywhere | 13:21–13:31 | 3027 | upstream |
| PKT-3033 | D. Library | Sidebar pins, recent files and a home page | 13:31–13:56 | 3027 | fork |
| PKT-3034 | D. Open media | The open-media factory split into separate dialogs | 12:28–13:11 | 3013 | fork |
| PKT-3035 | E. Settings | Hotkeys in their own dialog with a menu entry | 15:35–16:02 | 3013 | upstream |
| PKT-3036 | E. Settings | One preferences tree: basic then advanced per category, info icons | 16:02–17:35, 18:47–18:55 | — | fork |
| PKT-3037 | E. Settings | Subtitle autodetect fuzziness in words, not numbers | 17:04–17:35 | — | upstream |
| PKT-3038 | E. Settings | Theme (light/dark) and accent colour in General | 17:39–17:50 | 3001, 3036 | upstream |
| PKT-3039 | E. Settings | New category map (13 categories) | 17:35–18:27 | 3036 | fork |
| PKT-3040 | E. Settings | Extensions (add-ons) separated from internal plugins | 18:27–18:47 | 3036 | fork |
| PKT-3041 | E. Customization | In-place drag-and-drop layout editor | 18:58–19:28 | 3003 | fork |
| PKT-3042 | E. Customization | Profiles that carry layout and settings | 19:28–19:36 | 3041 | fork |
| PKT-3043 | E. Onboarding | First-run screen: theme choice and basic shortcuts | 19:36–19:54 | 3038, 3035 | upstream |
