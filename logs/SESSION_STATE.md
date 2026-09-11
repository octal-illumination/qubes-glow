# SESSION_STATE.md — kitty-glow
- Timestamp: 2026-09-11T09:22:36+05:30
- Objective: RESOLVED — session handoff from 2M-token session 01a077e8; git
  history reconstructed; build #16 then #17 shipped and verified live.
- Key facts: kwin 60603 running build #17 sha d66b9009; dom0 bridge =
  scripts/dom0-bridge.sh (Edit /root/.opencode/agent/db file, password
  dialog per call); user = chenpan; xauth=/tmp/xauth_PCByVw, display :0;
  kittyglowrc stores noBorder+glowEnabled (retired kwinrulesrc rule —
  verified absent today).
- Build #17: size guard (sub-48px unmanaged Qui-* ghosts, corner 456→0) +
  kglowsync 5s bootstrap watchdog (lost-reply wedge). 17/17 assertions.
- B toggle verified E2E: requestApply → consumed → sweep 14 windows both
  ways. G verified firing. xdotool client-geometry caveat documented.
- Pending: user acceptance (press B / G themselves); optional ROADMAP items.
- Next agent: read PROJECT_CONTEXT.md + SPECIFICATION.md LL-026; git log
  has full history (d697c0f phase1, #16 64f2e194, #17 d66b9009).
