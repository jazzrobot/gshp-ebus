# Repository Conventions

This file documents the structural rules for the repository so that the layout
stays consistent as the project grows.

---

## Hardware — vertical by prototype

Hardware is organised into one folder per prototype stage, not by file type.

```
hardware/
  proto-v1/         ← breadboard POC
  proto-v2/         ← isolated DC-DC stripboard or PCB
  proto-v3/         ← future transmit-capable design
  mechanical/       ← shared across all protos (enclosure, mounting)
```

### Rules

**Each proto folder is self-contained.**
Everything needed to understand, build, and validate that prototype lives inside
its folder. You should never need to open another proto folder to build the current one.

**Every proto folder has the same structure:**

```
proto-vN/
  README.md           ← what it is, status, when to move on
  front-end.md        ← circuit design and schematic
  bom.csv             ← the only BOM for this prototype
  build-notes.md      ← session notes, bring-up results
```

Proto v2 and later may add subfolders (e.g. `kicad/`, `gerbers/`) but the
above four files are always present.

**The BOM lives in exactly one place.**
`bom.csv` in the proto folder is the authoritative component list.
The `front-end.md` links to `./bom.csv` — it does not reproduce the table.
No other file in the repo duplicates or summarises the BOM.

**Cross-references use relative paths.**
Links between proto folders, docs, and firmware use relative paths
(e.g. `../proto-v2/front-end.md`) so the repo can be moved or cloned without
breaking navigation.

**`mechanical/` is the only shared hardware folder.**
It covers enclosure, mounting, and cable management that applies across protos.
Do not create other shared folders inside `hardware/` — if something is
proto-specific, it goes in the proto folder.

**Do not put files directly in `hardware/`** other than `README.md`.

**Proto folders are archivable.**
When a prototype is superseded, its folder becomes a complete historical record.
Do not modify a previous proto folder when working on a later one.

---

## Docs — horizontal by topic

`docs/` is organised by type, not by phase. This is the opposite of hardware
and is intentional — documents that span multiple phases (architecture, decisions,
setup) benefit from being grouped by topic.

```
docs/
  architecture/     ← overview, MVP plan, roadmap
  decisions/        ← ADR-style decision records
  setup/            ← install checklists, environment setup
  debugging/        ← failure modes and troubleshooting
```

### Rules

**Architecture docs describe the full project scope**, not just the current phase.
Update them as the project evolves rather than creating phase-specific copies.

**Decision records are append-only.**
Add new records for new decisions. Do not edit existing records to reflect
changed thinking — instead add a new record that supersedes the old one.

**`docs/repo-structure.md` is generated.**
Do not edit it by hand. Run `tools/scripts/generate-repo-structure.sh` after
any structural change to the repository.

---

## Firmware — vertical by board, then by stage

```
firmware/
  esp32-s3-poe/
    current/        ← the version on the device
    dev/            ← work in progress
```

When a second board type is added, it gets its own subfolder alongside
`esp32-s3-poe/`.

---

## Logs — horizontal by capture type

```
logs/
  bench-tests/      ← bring-up notes, bench PSU sweeps
  bus-captures/     ← raw eBUS capture files and summaries
```

Log files are named with a date prefix: `YYYY-MM-DD-description.txt`.

---

## General rules

- **One canonical source per piece of information.** If the same fact appears
  in two files, one of them will drift. Choose one and link to it from the other.

- **READMEs are navigation, not documentation.** A `README.md` tells you what
  is in the directory and where to go next. The actual content lives in named
  files within the directory.

- **Do not create new top-level directories** without updating `hardware/README.md`
  or the root `README.md` as appropriate, and regenerating `docs/repo-structure.md`.

---

## Changing these conventions

These rules exist to prevent drift, not to block improvement.

If you spot an opportunity to improve the structure — a pattern that isn't working,
a rule that conflicts with a real need, or a better way to organise something —
**raise it for discussion rather than acting on it unilaterally.**

The process is:
1. Describe the problem or opportunity
2. Propose the change and its trade-offs
3. Get explicit agreement before making structural changes

Changes to this file and to the repository structure require the project owner's
approval. Do not refactor the layout, rename conventions, or introduce new
organisational patterns without that discussion first.
