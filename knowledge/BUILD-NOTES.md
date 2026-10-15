# Build and implementation notes (learned 2026-10-14, PKT-3019/3032/3037)

Read before implementing any packet.

## Environment
- Work in a worktree per packet: `git -C ~/src/vlc worktree add ~/src/vlc-wt/pkt-NNNN -b <branch> ce4af2f`.
- Build in the CI image, mounting ~/src at the same path (the worktree's .git pointer is absolute):
  docker run --rm --user $(id -u):$(id -g) -v $HOME/src:$HOME/src -w $HOME/src/vlc-wt/pkt-NNNN registry.videolan.org/vlc-debian-unstable:20260611225331 bash -c 'meson setup build-meson -Dextra_checks=true && ninja -j8 -C build-meson && meson test -C build-meson --suite qt'
- A fresh full build takes a few minutes with -j8. Baseline: 7/7 qt tests OK on ce4af2f.
- nzxt has no global git identity. Commit with `git -c user.name=OscarOlar -c user.email=oscar@vauxoo.com` (do not change the shared clone config).
- No AI attribution in commits or PRs (no Co-Authored-By, no "Generated with").

## Shell pitfalls (nzxt login shell is zsh)
- Quote globs inside `ssh nzxt '...'` (e.g. `--include='*.c'`).
- Do not nest heredocs inside `ssh nzxt '...'`. Use `ssh nzxt 'cd <wt> && python3 -' <<'EOF2' ... EOF2` or pipe a script file.
- To compare one file against master with uncommitted edits, use `git show <sha>:<file> > <file>`, never `git checkout <sha> -- <file>` on a dirty tree.

## Tests
- `--suite src` has a known failure in this image: `test_src_player_lifecycle` SIGSEGVs on master too (likely no PulseAudio in the container). Ignore it in gates, but report it.
- The QML test runner only loads VLC.Util. For QML elsewhere, use a MANUAL row plus a qmllint before/after diff:
  `/usr/lib/qt6/bin/qmllint` (the `qmllint` on PATH is Qt5). Its output is noisy (~200-300 warnings without VLC qmltypes), so compare counts and lines before/after.
- QtTests that include `dialogs_provider.hpp` need the generated `ui_open*.h`: use the `ui_files` key in `modules/gui/qt/tests` test definitions (added on branch master-pkt-3032-playlists-create-button-bugfix).
- The stub medialibrary in `vlc_stub_modules.cpp` rejects every control query. Use the `mlControl` hook (same branch) for ML-driven model tests.
- `meson test` rebuilds before running.

## QML review traps
- Inside a `Connections` handler, an unqualified property write (e.g. `value = ...`) does not target the parent item. Always qualify (`spinBox.value = ...`).

## Packets
- `check_packets.py` treats any backticked text with a slash as a repo path. Do not backtick image names or URLs in §6.
- Frontmatter accepts `actual_h` and `actual_h_bugfix` (OPTIONAL_KEYS).
- Measured so far: PKT-3037 full 0.3 h (est. 4 h); PKT-3019 bugfix 0.3 h; PKT-3032 bugfix 0.2 h.

## Added after PKT-3001
- Inside `docker run ... bash -c "..."` the container HOME is `/`, so `~` expands to `/`. Use host-expanded `$HOME` paths.
- `check_packets.py` exits 1 on failures, but piping it (`| tail`) hides that from `&&`. Run it unpiped, read the result, then commit.
- qmllint before/after: run it before editing; normalise positions when diffing: `sed -E "s/:[0-9]+:[0-9]+:/:L:C:/"`.
- A `ColorContext` that sets any state property (`enabled`, `pressed`, ...) stops inheriting hover/press state from its parent context.
- An unqualified `implicitHeight` inside a `Rectangle` is that Rectangle's own (often 0), not the parent's.
- The Figma community file returns HTTP 403 without a browser session; use the packet's proposed values and record it.
- Measured: PKT-3001 0.2 h (est. 12 h).

## Added after PKT-3027
- Smoke-run the UI in the CI image to catch QML load errors: `QT_QPA_PLATFORM=offscreen timeout 25 ./build-meson/bin/vlc-static -I qt --aout dummy -vv 2>&1 | grep -i qml`. Without `--aout dummy` it crashes in PulseAudio; non-static `bin/vlc` fails ("process_pool"). The image has no medialibrary module, so library pages are unreachable.
- Put QML widget logic that needs tests in `Helpers.qml` (VLC.Util).
- QtTests linking `navigation_history.cpp` pull in the network model; branch master-pkt-3027 injects the node validator (util/navigation_history_node).
- `check_packets.py` rejects backticked paths of new files not listed in `target_files`; write them as plain text.
- Never edit sources while a build is running in the same worktree.
- Measured: PKT-3027 0.4 h.
