# Audit-CHANGELOG

## 2026-09-06T23:40:28Z — dom0 deployment audit (carried from prior session)
- Effect plugin deployed into dom0 privileged paths:
  - `/usr/lib64/qt5/plugins/kwin/effects/kittyglow.so` (root:root, mode 644)
  - `/usr/share/kwin/effects/kittyglow/metadata.json` (root:root, mode 644)
- Transfer method: `qvm-run` (dom0 → Dev-General) base64 pipe, written via `sudo`.
  Rationale: `qvm-copy` Filecopy RPC *into* dom0 was refused; `qvm-run` exec is allowed.
- Permission hardening: files must be world-readable (644). A `600` mode silently
  prevents KWin (running as the user) from loading the plugin — caught and fixed
  (Lesson LL-004).
- Activation (`kwin_x11 --replace`) remains unexecuted — requires explicit approval.
- No new attack surface introduced: the effect only draws GL geometry; it does not
  handle input, network, or read cross-VM data.
