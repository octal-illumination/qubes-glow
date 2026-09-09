# SESSION_STATE — 2026-09-10T01:50:00Z
## Current Objective
Root-cause LL-019 residue on v3.4 (Build #10, kwin 36048): user reports gold glow penetrating the ACTIVE front window in front of kitty, with intensity/thickness GROWING the longer it sits.
## Discovered Facts
- Live dom0: kwin_x11 PID 36048, session chenpan, XAUTHORITY=/tmp/xauth_PCByVw, DISPLAY=:0, screen 1366x768.
- Bridge runs as chenpan (no su); password dialog per dom0() call; batch everything per call.
- kitty (0x4c01004/79695876) at (189,94,902,469); offset konsole (0x4c00291/79692433) at (159,81,902,469) BEHIND kitty; pi konsoles fullscreen (0,23,1366,725); firefox (0x4c004b5/79692981) at (439,108,902,469); plasmashell panel y748.
- kwinrc has NO [Effect-kittyglow] section → all defaults from glowconfig.h (gold 255,215,0 @153/77 alpha).
- isKittyWindow = windowClass().toLower().contains("kitty"); only one kitty window.
- Capture loop: dom0 PIL ImageGrab(xdisplay=":0") → base64 over bridge → numpy+PIL strict mask (r-g>18, r-b>60, g-b>30, r>80); inactive-tuned mask (r-g>6) for 30% halo; wallpaper is olive (g>=r) — must use strict.
- STATIC STATES ALL CORRECT: kitty-top full halo hugging frame; konsole-top full suppression; firefox-over-kitty → over_firefox=39≈0, exposed segments paint.
- TRANSIENTS CAPTURED: d02 (post windowActivate(firefox), +300ms) and d16 (post mouseup) show unclipped ring over firefox: +2111/+2174 strict px (right ring col x~1160 y96-512; bottom y544-576); re-clip within 350ms.
- Interpretation: transient unclipped halo paints at interaction boundaries; with static front window, no repaint erases them → alpha accumulates per kitty repaint (cursor blink cadence) → growing thickness/intensity (user's exact complaint).
- xdotool titlebar drag FAILED to move firefox (pointer ended 935,378; window stayed 439,108) — interactive-move repro needs retry (maybe wmphys/key modifier or ydelays).
- Effect has NO isUserMove/moved-hook; occluders rebuilt per paint from stackingOrder(); kittyIdx<0 → draws UNCLIPPED (suspect path).
## File Changes
- None to code. New captures: /tmp/kgprobe/{kg_p1,p2,p3,p4,p5,user,a1,b1,c1,c2}.png, burst/kg_d01..16.png.
## Decisions & Rationale
- Strict gold mask over loose: wallpaper olive passes loose (r-b>40,g-b>25) → false washes; strict (r-g>18) separates gold hue from olive.
- Verified stacking immediately before each grab: xdotool windowactivate raise timing made unverified stacks uninterpretable.
## Active Blockers
- Root cause of transient unclipped paints unidentified (candidates: kittyIdx<0 during transitions; stacking async during activation/move-end; occluder set empty in the event→paint window).
## Pending Work
1. Blink/repaint-cadence burst in state C: kitty unfocused, static firefox on top; force repeated windowActivated hook fires (re-activate firefox every ~400ms ×20) → prove accumulation (monotonic gold growth over firefox).
2. Dump kwin journal around transient; correlate with effect log lines.
3. Root-cause in src/kittyglow.cpp (occludedAboveKitty / paint path / repaint hooks) → propose LL-019b fix; await "implement changes".
4. Log ledgers + changelog + commit per Rules 3/9/10/23 (already partially appended this session).
## Next Agent Handoff
Run the state-C repetition experiment (dom0 batch: activate firefox, loop grabs + windowactivate firefox every 400ms), analyze monotonic gold growth over firefox rect; then read src/kittyglow.cpp lines 148-227 + 252-278 with the transient timing in mind. Ask user nothing yet; artifact is reproducible programmatically.
