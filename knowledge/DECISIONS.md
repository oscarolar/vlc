# Owner rulings — VLC redesign packets

Ruled by the owner (Oscar Alcalá) on 2026-10-14. Every `DECISION` item in a packet's §5 is now either one of
the explicit rulings below or, when none applies, **R0** (the packet's own proposal/recommendation is accepted).

| # | Scope | Ruling |
|---|---|---|
| R0 | every DECISION not listed below | The proposal or recommendation written in the packet is accepted as is. |
| R1 | destination of all packets | **Mixed.** Packets with `upstream_ruling: not-needed` (bug fixes, additive changes) go as MRs to `videolan/vlc` master. Packets with `upstream_ruling: needed` (redesign choices that contradict VideoLAN) are implemented on our own fork (https://github.com/oscarolar/vlc), branch per packet, without waiting for VideoLAN. A fork packet may still be offered upstream later; that is not a blocker. |
| R2 | PKT-3039 category map | **Path B**: Qt presentation map, core taxonomy untouched (split 3039a/b/c as written). |
| R3 | PKT-3036 Simple/All/Expert selector | **Fold** into a single path (basic, then advanced per category) with a "Show all settings (advanced tree)" button at the bottom; `qt-initial-prefs-view` keeps working. |
| R4 | PKT-3017 §5.3 OpenSubtitles | **Per-user credentials**: the dialog asks the user for their own OpenSubtitles account and API key on first use (REST API `api.opensubtitles.com`) and stores them in the system keystore (`vlc_keystore`). No key ships in the binary. 3017c is unblocked. |
| R5 | PKT-3017 §5.4 and PKT-3022 §5.1–5.2 persistence | **Mixed**: preset bodies and subtitle↔episode assignments live in the Qt settings file; the key is the media-library group/media when the video is indexed, the parent folder otherwise. Works with and without the library. |
| R6 | PKT-3027 §5.1 sidebar sections (and PKT-3031 §5.1) | **Merge** Browse and Discover as children of "Network streams"; local folders reachable from the address line's home and "Add folder". |
| R7 | PKT-3007 §5.1 seek thumbnails | **Local files only**, on by default, software decoding, one request in flight; never for network/remote media. |
