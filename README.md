# kitty-glow — KWin Yellow-Halo Effect for kitty

> **Note:** All documentation and code in this project are purely AI-generated.

A custom **KWin (KDE Plasma 5.27) compositor effect** that paints a soft yellow
halo around every `kitty` terminal window. Built as a Qt/KWin plugin and
deployed into dom0 (where the X11 compositor runs under Qubes OS).

## Why
kitty is the primary terminal in this environment; a subtle yellow glow makes its
windows instantly distinguishable from other VM windows on the shared desktop.

## Layout
```
kitty-glow/
├── src/                     # effect source (canonical)
│   ├── kittyglow.cpp        # KWin::Effect subclass + GL halo drawing
│   ├── CMakeLists.txt       # builds the plugin (C++20)
│   └── kittyglow.json       # effect metadata (embedded into the .so)
├── container/
│   └── setup-build-container.sh   # Fedora-37 / KWin 5.27.8 build env
├── scripts/
│   ├── build.sh             # compile inside the container -> dist/
│   └── deploy.sh            # copy into dom0 + enable in kwinrc
├── dist/                    # build output (gitignored)
├── docs/                    # extra notes / research
├── logs/                    # CHANGELOG, audit, action history, etc.
├── SPECIFICATION.md         # problem, decisions, conformance, quality gates
├── ARCHITECTURE.md          # design, data flow, GL drawing
├── HANDBOOK.md              # build / deploy / activate / tune
├── PROJECT_CONTEXT.md       # state snapshot
└── ROADMAP.md               # phases
```

## Quick start
```bash
scripts/build.sh        # -> dist/kittyglow.so
scripts/deploy.sh       # -> dom0 plugin paths + kwinrc enable
# then restart KWin (needs explicit approval):
#   kwin_x11 --replace
```

See [HANDBOOK.md](HANDBOOK.md) for the full workflow and tuning knobs.
