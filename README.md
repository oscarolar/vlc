# VLC redesign work packets

Planning for a VLC 4 Qt/QML interface redesign, based on Juxtopposed's video "I Redesigned VLC After 25 Years" (https://www.youtube.com/watch?v=ERFeOgqk324).

- `knowledge/packets/`: one work packet per feature (index in `knowledge/packets/README.md`).
- `knowledge/DECISIONS.md`: owner rulings that apply across packets.
- `knowledge/BUILD-NOTES.md`: build, test and shell notes for implementing a packet.
- `tools/check_packets.py`: validates packet frontmatter and code references (`python3 -I tools/check_packets.py`).

This branch is unrelated to the VLC history; code changes live in `master-pkt-*` branches and their PRs.
