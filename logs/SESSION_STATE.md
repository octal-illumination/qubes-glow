# Session State (pre-compaction brain dump)

## Timestamp
2026-09-06T23:40:28Z

## Current Objective
Scaffold the kitty-glow KWin-effect project under
`~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow` and return to prompt
(no KWin restart performed).

## Discovered Facts
- KWin in dom0: 5.27.8 (Fedora 37). Built-in effects are statically linked into
  `libkwin.so`; custom effects are separate `.so` files in `kwin/effects/`.
- Build container: `dom0-replica-fed37` (Fedora 37, kwin-devel 5.27.8, KF5 5.108).
  Live container currently mounts legacy `/home/user/kitty-glow:/src`.
- Effect `.so` sha: `89e8513b9d282aacd9763fa3fd20cedf89b377f16f63ee18e7fade04bde228d4`.
- KWin reads the plugin list only at startup → activation needs `kwin_x11 --replace`.

## File Changes (this session)
- Created: project tree; `src/*` (imported); `container/setup-build-container.sh`;
  `scripts/{build,deploy}.sh`; `README.md`; SPEC/ARCH/HANDBOOK/PROJECT_CONTEXT/ROADMAP
  `.md`; `logs/*` ledger set.
- Unchanged from prior session: `dist/` build artifacts, dom0 deployment, kwinrc enable.

## Decisions & Rationale
- Project path: `QubesOS/UI-Enhancements/Kwin/kitty-glow` (kitty-glow = effect name).
- Transfer to dom0 via `qvm-run` base64 pipe (qvm-copy refused).
- Deferred KWin restart (requires explicit user consent).

## Active Blockers
- None blocking scaffolding. Activation is blocked on user approval of KWin restart.

## Pending Work
1. (User approval) Restart KWin to activate the effect.
2. Visual verification of the halo; tune params (`margin`/`layers`/color/alpha) if needed.
3. Optional: re-point build container mount to project src (auto-handled on first `build.sh`).

## Next Agent Handoff
- To activate: run the KWin restart command in HANDBOOK.md §5 (needs explicit user
  go-ahead). Then open a kitty window and confirm the yellow halo. If absent, check
  `journalctl _COMM=kwin_x11` for kittyglow errors and review `paintQuad` projection.
