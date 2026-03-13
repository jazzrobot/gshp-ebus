# Documentation

This directory holds the human-readable design and operating notes for the GSHP eBUS tap project.

The project is being developed in deliberate stages:

1. safe passive capture from the live bus
2. validation and decoding of useful signals
3. Home Assistant integration
4. anything more ambitious only after the earlier phases are stable

## Start here

- [architecture/overview.md](architecture/overview.md) - overall scope, architecture, phases, and safety boundaries
- [architecture/mvp-plan.md](architecture/mvp-plan.md) - detailed MVP work plan
- [setup/live-ebus-install-checklist.md](setup/live-ebus-install-checklist.md) - live install and rollback checklist
- [debugging/common-failures.md](debugging/common-failures.md) - first troubleshooting notes
- [decisions/README.md](decisions/README.md) - architecture decision records

## Documentation principles

- Write down safety constraints, not just happy-path behaviour.
- Keep facts separate from hypotheses.
- Treat anything that touches the live heating system as install-grade work, even in prototype form.
- If a lesson is painful once, capture it here so it is not re-learned later.
