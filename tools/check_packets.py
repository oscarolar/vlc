#!/usr/bin/env python3
"""Mechanical checks for the VLC redesign work packets (knowledge/packets).

Strict reference resolution for `path:N[-M]` tokens inside backticks: the path must be
repo-root-relative and name an existing file in ~/src/vlc (no basename guessing, no
suffix matching, no "…/" truncation). A path listed in the packet's target_files that
does not exist yet is a "new file", not a failure. Anything else is a failure.
Also checks the frontmatter key `target` (upstream|fork), consistent with
upstream_ruling (not-needed -> upstream, needed -> fork).
"""
import os
import re
import subprocess
import sys

PKT_DIR = os.path.expanduser("~/src/vlc-redesign/knowledge/packets")
VLC_DIR = os.path.expanduser("~/src/vlc")
BASE_SHA = "ce4af2f"
QT_ROOT = "modules/gui/qt"
EXPECTED_KEYS = [
    "id", "title", "section", "video_ref", "pipeline", "kind", "track",
    "component", "branch", "base_sha", "target_files", "estimate_h",
    "priority", "depends_on", "upstream_ruling", "target", "mr_title", "status",
]
# Filled by executors after the fact; allowed but not required.
OPTIONAL_KEYS = ["actual_h", "actual_h_bugfix"]
REQUIRED_HEADINGS = ["## 1.", "## 2.", "## 2b.", "## 3.", "## 4.", "## 5.", "## 6.", "## 7."]
KNOWN_EXT = (
    ".c", ".h", ".cpp", ".hpp", ".cc", ".qml", ".py", ".build", ".in", ".cmake",
    ".txt", ".ui", ".js", ".json", ".md", ".sh", ".m4", ".am", ".ac", ".mk",
    ".rc", ".xml", ".ts", ".svg", ".desktop", ".po", ".pot", ".yml", ".yaml",
)
REF_RE = re.compile(r"`([^`\s]+?):(\d+)(?:-(\d+))?`")
ELLIPSIS = "…/"

failures = {}
new_files = {}
resolved_by_name = {}
total_refs = 0
total_packets = 0
estimates = {}
estimate_total = 0.0


def fail(pkt, msg):
    failures.setdefault(pkt, []).append(msg)


def parse_scalar(raw):
    raw = raw.strip()
    if raw[:1] in ("'", '"'):
        q = raw[0]
        end = raw.find(q, 1)
        return raw[1:end] if end != -1 else raw[1:]
    return raw.split(" #", 1)[0].strip()


def parse_frontmatter(lines):
    fm, lists = {}, {}
    current_list = None
    for line in lines:
        m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):(.*)$", line)
        if m:
            key, rest = m.group(1), m.group(2)
            fm[key] = rest.strip()
            current_list = key if rest.strip() == "" else None
            if current_list:
                lists[key] = []
            continue
        if current_list and re.match(r"^\s+-\s+", line):
            lists[current_list].append(re.sub(r"^\s+-\s+", "", line).strip())
    return fm, lists


def split_frontmatter(text):
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return None, lines
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            return lines[1:i], lines[i + 1:]
    return None, lines


def normalize_dep(dep):
    dep = dep.strip().strip("'\"")
    m = re.match(r"^(?:PKT-)?(30\d\d)$", dep)
    return "PKT-" + m.group(1) if m else dep


def resolve_ref(clean, component, tracked):
    """Strict: return ("exact", path, []) when `clean` is a repo-root-relative file, else ("broken", None, [])."""
    if os.path.isfile(os.path.join(VLC_DIR, clean)):
        return "exact", clean, []
    return "broken", None, []


def main():
    global total_refs, total_packets, estimate_total
    head = subprocess.run(["git", "-C", VLC_DIR, "rev-parse", "--short", "HEAD"],
                          capture_output=True, text=True).stdout.strip()
    dirty = subprocess.run(["git", "-C", VLC_DIR, "status", "--porcelain"],
                           capture_output=True, text=True).stdout.strip()
    tracked = [t for t in subprocess.run(["git", "-C", VLC_DIR, "ls-files"],
                                         capture_output=True, text=True).stdout.split("\n") if t]
    notes = []
    if head != BASE_SHA:
        notes.append(f"HEAD is {head}, expected {BASE_SHA}")
    if dirty:
        notes.append("worktree dirty (line counts read from working tree)")

    files = sorted(f for f in os.listdir(PKT_DIR) if re.match(r"^PKT-30\d\d-.+\.md$", f))
    known_ids = {"PKT-" + f[4:8] for f in files}

    readme_rows = set()
    with open(os.path.join(PKT_DIR, "README.md"), encoding="utf-8") as fh:
        for line in fh:
            m = re.match(r"^\|\s*(PKT-30\d\d)\s*\|", line)
            if m:
                readme_rows.add(m.group(1))

    for f in files:
        m = re.match(r"^PKT-(30\d\d)-(.+)\.md$", f)
        num, slug = m.group(1), m.group(2)
        pid = "PKT-" + num
        total_packets += 1
        with open(os.path.join(PKT_DIR, f), encoding="utf-8") as fh:
            text = fh.read()

        fm_lines, body_lines = split_frontmatter(text)
        if fm_lines is None:
            fail(pid, "no frontmatter block at top of file")
            continue
        fm, lists = parse_frontmatter(fm_lines)

        missing = [k for k in EXPECTED_KEYS if k not in fm]
        extra = [k for k in fm if k not in EXPECTED_KEYS + OPTIONAL_KEYS]
        if missing:
            fail(pid, f"missing keys: {', '.join(missing)}")
        if extra:
            fail(pid, f"extra keys: {', '.join(extra)}")

        if "id" in fm and parse_scalar(fm["id"]) != pid:
            fail(pid, f"id {parse_scalar(fm['id'])!r} != filename prefix {pid}")
        branch_expected = f"master-pkt-{num}-{slug}"
        if "branch" in fm and parse_scalar(fm["branch"]) != branch_expected:
            fail(pid, f"branch {parse_scalar(fm['branch'])!r} != {branch_expected!r}")
        if "base_sha" in fm and parse_scalar(fm["base_sha"]) != BASE_SHA:
            fail(pid, f"base_sha {parse_scalar(fm['base_sha'])!r} != {BASE_SHA!r}")
        track = parse_scalar(fm.get("track", ""))
        if track not in ("T1", "T2", "T3"):
            fail(pid, f"track {track!r} not in T1/T2/T3")
        prio = parse_scalar(fm.get("priority", ""))
        if prio not in ("P1", "P2", "P3"):
            fail(pid, f"priority {prio!r} not in P1/P2/P3")
        ruling = parse_scalar(fm.get("upstream_ruling", ""))
        if ruling not in ("needed", "not-needed"):
            fail(pid, f"upstream_ruling {ruling!r} not in needed/not-needed")
        target = parse_scalar(fm.get("target", ""))
        if target not in ("upstream", "fork"):
            fail(pid, f"target {target!r} not in upstream/fork")
        elif ruling in ("needed", "not-needed") and target != ("fork" if ruling == "needed" else "upstream"):
            fail(pid, f"target {target!r} inconsistent with upstream_ruling {ruling!r}")
        if "mr_title" in fm:
            mr = parse_scalar(fm["mr_title"])
            if len(mr) > 72:
                fail(pid, f"mr_title is {len(mr)} chars (> 72)")
        if "depends_on" in fm:
            raw = fm["depends_on"]
            deps = [d for d in raw.strip("[]").split(",") if d.strip()] \
                if raw.startswith("[") else lists.get("depends_on", [])
            for d in deps:
                if normalize_dep(d) not in known_ids:
                    fail(pid, f"depends_on {d.strip()!r} does not exist")
        try:
            est = float(parse_scalar(fm.get("estimate_h", "")))
            estimates[track] = estimates.get(track, 0.0) + est
            estimate_total += est
        except ValueError:
            fail(pid, f"estimate_h {fm.get('estimate_h')!r} is not a number")

        targets = [t.strip().lstrip("./") for t in lists.get("target_files", []) if t.strip()]
        component = parse_scalar(fm.get("component", ""))

        for h in REQUIRED_HEADINGS:
            if not any(line.startswith(h) for line in body_lines):
                fail(pid, f"heading {h!r} missing")

        for ref_path, n_s, m_s in REF_RE.findall(text):
            if not ("/" in ref_path or ref_path.endswith(KNOWN_EXT)):
                continue
            total_refs += 1
            clean = ref_path
            ref_txt = f"`{ref_path}:{n_s}" + (f"-{m_s}" if m_s else "") + "`"
            if not clean or clean.startswith(".") or "/" not in clean and clean not in targets:
                fail(pid, f"not repo-root-relative {ref_txt}")
                continue
            status, repo_path, cands = resolve_ref(clean, component, tracked)
            if status == "broken":
                if clean in targets:
                    new_files.setdefault(pid, []).append(clean)
                else:
                    fail(pid, f"broken path {ref_txt}: no such file at the repo root")
                continue
            if status == "ambiguous":
                fail(pid, f"ambiguous path {ref_txt}: {len(cands)} matches, e.g. {cands[0]}")
                continue
            with open(os.path.join(VLC_DIR, repo_path), encoding="utf-8", errors="replace") as src:
                nlines = len(src.read().splitlines())
            if int(n_s) > nlines:
                fail(pid, f"out-of-range {ref_txt}: {repo_path} has {nlines} lines")
            if m_s:
                if int(m_s) > nlines:
                    fail(pid, f"out-of-range {ref_txt}: {repo_path} has {nlines} lines")
                if int(m_s) < int(n_s):
                    fail(pid, f"reversed range {ref_txt}")

        if pid not in readme_rows:
            fail(pid, "no row in README.md table")

    for row in sorted(readme_rows - known_ids):
        fail(row, "README.md has a row but no packet file")

    print(f"VLC HEAD: {head} (expected {BASE_SHA})" + (f" -- {'; '.join(notes)}" if notes else ""))
    print(f"Packets checked: {total_packets} | README rows: {len(readme_rows)}")
    print(f"References checked: {total_refs} (strict: repo-root-relative only)")
    print(f"New files (in target_files, not in repo yet): {sum(len(v) for v in new_files.values())}")
    print(f"Failures: {sum(len(v) for v in failures.values())} in {len(failures)} packets")
    for pid in sorted(failures):
        print(f"{pid}:")
        for msg in failures[pid]:
            print(f"  - {msg}")
    print("Estimates (h):")
    for t in sorted(estimates):
        print(f"  {t}: {estimates[t]:.1f}")
    print(f"  TOTAL: {estimate_total:.1f}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
