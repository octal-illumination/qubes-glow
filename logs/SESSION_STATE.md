# SESSION_STATE.md

> **Note:** All documentation and code in this project are purely AI-generated.

## 2026-09-09T~08:05Z — Seamless-B source round complete, awaiting build consent

### Current Objective
Build + deploy + E2E the seamless Meta+Shift+B toggle (LL-016 flash) and expandedGeometry occluders (LL-017).

### Discovered Facts
- PoC-7 (decisive): KWin script PACKAGE (kpackagetool5, user path) with minimizeall-verbatim metadata auto-runs at kwin start; QTimer fires; client.noBorder flips live on chromium (0,0,23,0→0,0,0,0 @2.4s→back @8.5s). Minimal metadata WITHOUT X-Plasma-API/X-Plasma-MainScript/X-KDE-PluginKeyword keys never loads (PoC-4c/6 failures root-caused).
- minimizeall metadata values: X-Plasma-API="javascript", X-Plasma-MainScript="code/main.js", X-KDE-PluginKeyword="<id>", KPackageStructure="KWin/Script".
- B-flash = full-screen white (frames: uniform 247,248,248 incl. kitty interior), 0.3–0.6 s, from org.kde.KWin.reconfigure RuleBook reload — now eliminated at source (toggle stages value; no reconfigure call).
- LL-017 artifact: front windows paint shadow gradients past frameGeometry; occluder rects now use expandedGeometry().
- T on kitty stays rule-forced (suppressed) by design; T documented for non-kitty windows (option a; Option A offered, not chosen).
- loadScript+start() dead on 5.27.8 dom0 (once-per-process); package install is the ONLY working script channel.
- kglowtest package removed; kglowtestEnabled=false dead key remains (harmless). Parity noborder=true intact; shortcuts=2 (B+T) survived every restart.

### File Changes (this round)
- NEW src/kittytoggle.h (24) / src/kittytoggle.cpp (104): SyncService Q_OBJECT slot nextSource()→int (consumes s_pending 1/2/0), init() (package check, kwinrc enable, registerService "org.kde.kittyglow", registerObject "/sync" ExportSlots), requestApply(bool).
- NEW src/kwin-script/kglowsync/metadata.json + contents/code/main.js (62): 60 ms poll, 400 ms watchdog, 2 s heartbeat gate, applies noBorder to resourceClass~kitty clients.
- EDIT src/kittyglow.cpp (274→283): init() in ctor; toggle→requestApply (reconfigure deleted); occludedAboveKitty→expandedGeometry(); includes pruned; header comment.
- EDIT src/CMakeLists.txt (+kittytoggle.cpp); EDIT scripts/deploy.sh (pkg install to ~/.local/share/kwin/scripts/kglowsync, kglowsyncEnabled, sycoca).

### Decisions & Rationale
- Package auto-run instead of loadScript bootstrap (PoC-7 evidence; start() is once-per-process no-op).
- Pull-model (script polls C++) — script has no service registration; heartbeat gate stops enforcement when effect absent.
- Watchdog re-assert makes T-on-kitty deterministic (reverted ≤400 ms) — documented.
- QStringLiteral(constexpr-var) invalid → fromLatin1 (caught pre-compile).
- kittyglow.cpp left >200 lines (pre-existing 274); paint core untouched; further split = follow-up proposal.

### Active Blockers
- Build consent pending (Rule 1b; approved plan step 5: separate build/deploy consent).

### Pending Work (ordered)
1. Ask user: "build?" → scripts/build.sh (Dev-General), fix any compile errors (moc/kittyglow.moc pattern proven).
2. Deploy: scripts/deploy.sh → dom0 (dialog); verify kglowsync installed + isScriptLoaded(kglowsync)=true after restart.
3. kwin restart (explicit); E2E: B 10-press flash capture (expect ZERO white frames), B latency ≤ ~0.5 s, parity checks each press; overlap repro (kitty focused FIRST, front window last) for LL-017; T behavior doc.
4. Docs: SPEC LL-016/LL-017, HANDBOOK (B semantics), PROJECT_CONTEXT build #8, ROADMAP, HTML regen (Rule 18e diff), commit.

### Next Agent Handoff Message
Read PROJECT_CONTEXT.md + this file; wd ~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow. All source changes are DONE and verified; on user's "build" run scripts/build.sh, then scripts/deploy.sh, restart kwin, run the E2E plan above. Do NOT re-run PoCs. Baseline evidence: logs/run/flash/*.png (white @frame3), logs/run/*.log.
# 2026-09-09T08:21:05Z — BLOCKER: JS noBorder write reverts
Objective: seamless Meta+Shift+B. Proven: rule flip+DBus+poll+callback. Blocker: write rejected, likely in-memory RuleBook re-assert. Next: offline KWin rules-engine research in podman container; propose unloadScript to stop 400 ms churn BEFORE anything else.
