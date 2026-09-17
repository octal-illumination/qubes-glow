# Qubes Glow

> **Note:** All documentation and code in this project are purely AI-generated.

VM-label-aware window glow and border controls for **Qubes OS**, implemented
as a **KWin 5.27.8 / Qt5 / C++20 effect** running in dom0 on X11.

## Features

- Soft SDF halos around eligible application windows, not just kitty.
- Halo hue follows each VM's Qubes label; dom0 windows use configured colors.
- Focused-window glow and border controls, plus persisted global masters.
- Managed-window eligibility excludes override-redirect menus and popups.
- Occlusion clipping and animation tracking for seamless window movement.

Window-type hints lost through the Qubes GUI proxy impose limitations;
see [HANDBOOK.md](HANDBOOK.md) for exclusions and operational details.

## Names and compatibility

- Display name: **Qubes Glow**.
- Repository/directory: **`qubes-glow`** (formerly `kitty-glow`).
- Existing runtime names remain: `kittyglow.so`, `kittyglowrc`, `kglowsync`,
  DBus interfaces, configuration keys and shortcut registration IDs.
- This branding change does not migrate settings or change effect behavior.
- The installed display name changes only after an approved build/deployment.

## Project map

```text
qubes-glow/
├── src/         C++ effect, metadata, supporting KWin script
├── container/   Fedora 37 / KWin 5.27.8 build environment
├── scripts/     build, deployment, rollback, regression checks
├── dist/        build artifacts (legacy basenames retained)
├── docs/        extended notes and research
└── logs/        ledgers and runtime evidence
```

## Development

See [HANDBOOK.md](HANDBOOK.md) for build, deployment and shortcuts;
[SPECIFICATION.md](SPECIFICATION.md) for requirements and compatibility rules;
[ARCHITECTURE.md](ARCHITECTURE.md) for design;
[PROJECT_CONTEXT.md](PROJECT_CONTEXT.md) for current state;
[ROADMAP.md](ROADMAP.md) for pending work.

**Rename prerequisite:** the existing build container retains its old bind
source. The next approved build must recreate it via the project's setup
script; see HANDBOOK Section 2. No old-path symlink is used.
