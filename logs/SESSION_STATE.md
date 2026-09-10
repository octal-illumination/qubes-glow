# SESSION_STATE — 2026-09-10T10:05:00Z (session: kitty-glow LL-020 verification, agent: build)

## Current Objective
Verify the LL-020 scissor-test fix for kittyglow build #10; FAILED — fix silenced the halo entirely. New fix (CPU quad subdivision) proposed, awaiting user approval.

## Current Objective (1-line)
Replace GL scissor-based halo clipping with CPU quad subdivision; then verify halo renders + occlusion + no leak.

## Discovered Facts (verified this session)
- Build #10 (sha 22636345…) deployed to dom0, kwin restarted PID 41654, OpenGL 4.6 compositing (journal-confirmed both instances).
- `qdbus org.kde.KWin /Effects isEffectLoaded kittyglow` → true (effect methods live on /Effects, NOT /KWin; bridge shells need explicit DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$(id -u)/bus).
- On-disk kwinrc: kittyglowEnabled=true; **[Effect-kittyglow] section absent** → effect runs on defaults (gold 255,215,0, radius 32 — see src/glowconfig.h).
- kittyglowrc [General] noBorder=true (kglowsync applies to fresh kitty: "clientAdded: applied desired=true").
- OLD kitty window (79695876 @ 625,81) vanished from X (~08:20); fresh kitty launched by me: **79698807 (0x4c01b77) @ (333,198) 691×322**, process 241872 in Dev-General. Old process 98424 still alive w/o window (left alone).
- Konsoles = MAXIMIZED (not keep-above): 0x4c001a7 (pi/UI-Enhancements), 0x4c001d4, 0x4c00759, 0x4c0014b, 0x4c0100d, 0x4c011a7 fullscreen [0,23,1366,725]; offset ones 0x4c004b5=firefox, 0x4c00188=chromium, 0x4c002e4=dolphin.
- Plasma desktop window (0x60001d, fullscreen) + panel (0x60002d) exist; desktop window is plasmashell type-normal — candidate occluder hazard (isDesktop() not excluded in occludedAboveKitty loop).
- Desktop wallpaper contains ~18k strict-gold px visible when fullscreen konsoles hidden (explain run1 "C-states" 23,434 — NEVER halo).
- Positive control (ll020_positive.py: minimize 9 konsoles → kitty raised on bare desktop): ring gold = **74/91 px = noise floor → HALO DOES NOT RENDER POST-FIX**.
- Pre-fix binary DID render (leak rings measured 8,949px pre-restart); post-fix nothing renders ⇒ GLVertexBuffer::render(region,mode,true) scissor boxes are degenerate/misplaced in our paint context (Y-flip via GLFramebuffer::currentFramebuffer() height suspect).
- Burst infra quirks: xdotool slow with --sync (~1-4s/cycle → 24-cycle burst needs timeout≥300); windowraise on managed KWin windows gets overridden; python must run -u (block-buffered stdout lost when bridge kills process group); dom0 KDialog password dialog steals focus per bridge call (stacking races).

## File Changes
- src/kittyglow.cpp: scissor lines added earlier this session (build #10) — TO BE REPLACED by subdivision approach.
- logs/probe/ll020_final.py, ll020_v35_burst.py, ll020_positive.py: probe scripts (shipped+run in dom0).
- logs/probe/ll020/crop_f6.png, crop_f12.png: crops (unviewable — no image support this session).
- logs/commands-log.md: chained per command throughout.

## Decisions & Rationale
- Rejected: further scissor debugging (viewport/box introspection rebuilds) — heavy dom0 cycles, uncertain payoff.
- Chosen: CPU subdivision (draw one quad per clip rect, render(GL_TRIANGLES) unclipped) — SDF is fragment-position-based so sub-rects are pixel-identical to full quad; removes ALL scissor dependency; plus isDesktop() exclusion in occluder loop (desktop window can never occlude — kills alternate empty-clip hypothesis).

## Active Blockers
- Halo render broken in build #10 — LL-020 fix must be replaced; awaiting user "implement changes".

## Pending Work
1. Implement subdivision + isDesktop() exclusion in src/kittyglow.cpp (Rule 16 verify, build #11, deploy, restart kwin).
2. Positive control (ll020_positive.py): halo MUST appear (~thousands px) with kitty unoccluded.
3. Occlusion check: halo clipped (~0) with konsole above kitty.
4. Leak burst (v35): no accumulation over occluders.
5. Regression assertion in qubes-dev-tools/open-dom0/regression-checks.sh + LL-020 entry in SPECIFICATION.md (two-part lesson: caller-must-enable AND boxes unusable → subdivide) + HANDBOOK/PROJECT_CONTEXT/ARCHITECTURE refresh + changelogs + commit.

## Next Agent Handoff Message
Read PROJECT_CONTEXT.md + this file. Run `git -C ~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow status` (expect clean tree at LL-020-scissor commit). ASK USER to approve the subdivision patch before touching src/. Do NOT trust /Effects queries without DBUS_SESSION_BUS_ADDRESS exported. Kitty test window id may change — re-derive via xdotool search --onlyvisible --class kitty.
