<!-- HTML sibling: regenerate ONLY via bash ~/Projects/scripts/generate-docs-html.sh QubesOS/UI-Enhancements/Kwin/kitty-glow/ROADMAP.md — Rule 18c left-aligned CSS. Never hand-roll pandoc. -->
# kitty-glow — Roadmap

> **Note:** All documentation and code in this project are purely AI-generated.

## Phase 1 — Build & Install the Effect  ✅ DONE
- [x] Write `KittyGlowEffect` (C++20) with 8-layer alpha halo.
- [x] Compile in Fedora-37 / KWin 5.27.8 container; clean build, zero warnings.
- [x] Deploy `.so` + `metadata.json` into dom0 plugin paths (perms `644`).
- [x] Enable via `kwinrc` `[Plugins] kittyglowEnabled=true`.

## Phase 2 — Activate  ⏳ PENDING (user approval)
- [ ] Restart KWin (`kwin_x11 --replace`) so the plugin list is re-read.
- [ ] Verify KWin stays alive + no `kittyglow` load error in journalctl.

## Phase 3 — Visual Verify & Tune  ⏳ BLOCKED on Phase 2
- [ ] Open a kitty window → confirm yellow halo; fullscreen → no halo.
- [ ] If halo is offset, fix projection handling in `paintQuad`.
- [ ] Tune `margin` / `layers` / color / alpha to taste.

## Phase 4 — Enhancements  🔜 PLANNED
- [ ] Per-window toggle hotkey (e.g. Meta+Shift+G) to enable/disable the glow.
- [ ] Intensity control (config UI or kwinrc key) for halo strength.
- [ ] Optional "border glow" variant vs. full halo.

## Phase 5 — Packaging  🔜 OPTIONAL
- [ ] Consider a repeatable RPM/spec or a one-shot installer script wrapping
      `build.sh` + `deploy.sh` for clean reinstalls after KWin upgrades.

## Notes
Phases are gated: Phase 3 cannot be validated until Phase 2 (activation) is
approved and performed. See HANDBOOK.md for the exact commands per phase.
