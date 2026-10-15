---
id: PKT-3000
title: "<short title: the behaviour the user gets>"
section: "<area of the redesign, e.g. Player control bar>"
video_ref: "<mm:ss–mm:ss of the Juxtopposed video where the change is shown>"
pipeline: CI              # CI (meson test, debian-meson job) | MANUAL (visual check only, no automatable row)
kind: ui                  # ui (QML only) | model (C++ model/controller + QML) | core (libvlc / module outside modules/gui/qt)
track: T2                 # T1 small (≤ 4 h) · T2 medium (≤ 16 h) · T3 large (> 16 h, split before starting)
component: <qt subtree, e.g. modules/gui/qt/player>
branch: master-pkt-3000-<slug>
base_sha: ce4af2f         # VLC master sha at which §2 ranges were read
target_files:
  - <path>
estimate_h: 0.0
priority: P2              # P1 core of the redesign · P2 default · P3 nice to have
depends_on: []            # packet ids that must be merged first
upstream_ruling: needed   # needed (changes a VideoLAN design decision) | not-needed (bug-level / additive behind a setting)
target: fork               # upstream (MR to videolan/vlc) | fork (our redesign fork) — ruling R1 in ../DECISIONS.md
mr_title: "qml: <what this MR does, ≤ 72 chars>"
status: planned           # planned | ready | in_progress | review | done | blocked
---

# PKT-3000 — <title>

## 1. Scope & intent
<What the user sees before, what they see after, and why the redesign asks for it (cite the video timestamp).
One MR, N commits, each with its VLC-style subject (`qml: …`, `qt: …`, `<module>: …`).>

## 2. Dry-run findings
Read at `base_sha`. Every range is re-read in §7 Step 2 against the work clone's HEAD.
- `<path>:<start>-<end>` (`<component / function>`) — <what it does today, in one clause>
- `<path>:<line>` — <the property, binding or setting that the change hinges on>

Settings / variables involved: `<vlc option name>` (`src/libvlc-module.c:<line>` or module), or "none".
Mock boundary: `<what a test stubs, e.g. PlayerController in a QML TestCase, vlc_stub_modules.cpp>`, or "none".

### Overlaps & contradictions
<Other packets of this plan touching the same files, upstream work already heading elsewhere (e.g. the VLC 4
redesign already moving the same control), and how this packet resolves it. "None detected" written out.>

## 2b. Upstream state
<What VLC master already ships for this feature (partial or complete), with paths. When the video claims
"VLC can't do X" and master already can, say so here and narrow the packet.>

## 3. Per-test plan
Rows are hypotheses read at `base_sha`; Step 2 confirms or rewrites them. Behaviour in a C++ model gets a
QtTest row (`modules/gui/qt/tests/test_*.cpp`); behaviour in QML gets a `TestCase` row registered in
`qml_tests` (`modules/gui/qt/meson.build`); purely visual outcomes are a MANUAL row with the screenshot to attach.
| # | Target | Given | When | Then | Test |
|---|---|---|---|---|---|
| 1 | <component or method> | <state> | <action> | <observable outcome> | `<file>::<test name>` |

## 4. Fixtures & mocks
- Pattern to copy: `<existing test file:line>` (<what it builds>).
- Stubs: <stubbed modules / fake media / fake player>.
- Sample media: <files under test/samples or generated in the test>, or "none".

## 5. Design decisions & open questions
<Numbered. Each one marked DECISION (needs a ruling from the owner before `ready`) or NOTE. Once the owner rules (see ../DECISIONS.md), the label becomes RULED and the item starts with one sentence `RULED <date> (Rn): <what is decided>`; the original reasoning stays below it.>

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
- [ ] **Step 1: Work clone** — on nzxt, in `~/src/vlc`: `git fetch origin && git switch -c <branch> <base_sha>`; build once with the §6 command → green before any change. Nothing runs on the Mac.
- [ ] **Step 2: Dry-run** — re-read every range of §2 on the branch; a moved range updates §2 and `base_sha`; a row of §3 the code contradicts is rewritten and the contradiction noted in §5.
- [ ] **Step 3: First row watched failing** — write the first test of §3 with a wrong expectation → FAIL, then the real one.
- [ ] **Step 4..N: One TDD cycle per row of §3** — product change, row green, nothing else red.
- [ ] **Step N+1: Whole gate** — the §6 command; paste the summary in §6 Evidence.
- [ ] **Step N+2: Commit and MR** — commits with VLC subjects (`qml: …`), push to the fork, MR as Draft to the `target` of the frontmatter (upstream: "MR to videolan/vlc master (Draft)"; fork: "MR to the fork github.com/oscarolar/vlc (base `master`)"); `status: review`.
