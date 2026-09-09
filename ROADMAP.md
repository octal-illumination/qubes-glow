<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/ROADMAP.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Roadmap

> **Note:** All documentation and code in this project are purely AI-generated.

## Phase 1 — Build & Install the Effect  ✅ DONE
- [x] Write `KittyGlowEffect` (C++20) with 8-layer alpha halo.
- [x] Compile in Fedora-37 / KWin 5.27.8 container; clean build, zero warnings.
- [x] Deploy `.so` + `metadata.json` into dom0 plugin paths (perms `644`).
- [x] Enable via `kwinrc` `[Plugins] kittyglowEnabled=true`.

## Phase 2 — Activate  ✅ DONE
- [x] Restart KWin (`kwin_x11 --replace`) so the plugin list is re-read.
- [x] Verify KWin stays alive + no `kittyglow` load error in journalctl.

## Phase 3 — Visual Verify & Tune  ✅ DONE
- [x] Open a kitty window → confirm yellow halo; fullscreen → no halo.
- [x] Halo projection fixed/verified (SDF shader, v3).
- [x] Tuned `margin` / `layers` / color / alpha (kwinrc `[Effect-kittyglow]`).

## Phase 4 — Enhancements  🔜 PLANNED
- [ ] Per-window toggle hotkey (e.g. Meta+Shift+G) to enable/disable the glow.
- [ ] Intensity control (config UI or kwinrc key) for halo strength.
- [ ] Optional "border glow" variant vs. full halo.

## Phase 5 — Packaging  🔜 OPTIONAL
- [ ] Consider a repeatable RPM/spec or a one-shot installer script wrapping

## Phase 6 — Shortcut Reliability (v3.4)  ✅ DONE (2026-09-09)
- [x] Heal Meta+Shift+B/T lost to the kwin-before-kglobalaccel boot race
      (kwin restart AFTER the daemon — SPEC LL-011).
- [x] Normalize kwinrulesrc to a single ACTIVE canonical rule group
      ([1] vs [kitty-borderless] duplication — LL-007 family).
- [x] Step C (rebuild, 2026-09-09): persistent state moved OUT of kwinrulesrc
      (loaded forcing rule overrides scripting writes — LL-016). State store
      `kittyglowstate.cpp/.h` in `kittyglowrc`; `kittyborderrule.cpp/.h`
      retired; kglowsync gains getCurrentState bootstrap + clientAdded.
- [x] Build #7 zero-warning, deployed (badf8df7…), live-reloaded; E2E
      keypress verified; group-rename immunity proven (rename → toggles
      still work → restore). Recovery re-validated by a real kwin crash
      (auto-restart re-registered both shortcuts).
      `build.sh` + `deploy.sh` for clean reinstalls after KWin upgrades.

## Notes
Phases are gated: Phase 3 cannot be validated until Phase 2 (activation) is
approved and performed. See HANDBOOK.md for the exact commands per phase.
