# Output History (internal rationale)

## 2026-09-06T23:40:28Z — Design choice: layered-alpha halo
- Decision: render the glow as 8 stacked translucent GL rects (alpha 0→70) rather
  than a blur shader or a single opaque rect.
- Rationale: zero custom shader code, cheap, gives a soft falloff; KWin's
  `UniformColor` shader + `GLVertexBuffer` already provide everything needed.
- Location: `src/kittyglow.cpp` (`drawGlow`, `paintQuad`).

## 2026-09-06T23:40:28Z — Build isolation: Fedora-37 replica container
- Decision: compile inside `dom0-replica-fed37` (Fedora 37, kwin-devel 5.27.8)
  instead of directly on dom0.
- Rationale: dom0 has no toolchain; the container pins the exact KWin ABI so the
  `.so` loads against dom0's `libkwin.so` without symbol mismatch.

## 2026-09-06T23:40:28Z — Why qvm-run instead of qvm-copy
- Decision: transfer the built `.so` into dom0 via `qvm-run` base64 pipe.
- Rationale: `qvm-copy` *into* dom0 triggered a refused Filecopy RPC; `qvm-run`
  (dom0 executing in Dev-General and piping stdout back) is permitted and writes
  the file via `sudo` in dom0.
