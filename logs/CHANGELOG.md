# CHANGELOG

## 2026-09-06T23:50:19Z — Re-pointed build container to project src
- Ran `container/setup-build-container.sh`: committed live container to image
  `dom0-replica-fed37-img` (preserves the KWin 5.27.8 toolchain), then recreated
  `dom0-replica-fed37` mounting `src/` at `/src`.
- Project now fully self-consistent: `scripts/build.sh` compiles the canonical
  source directly. Legacy `/home/user/kitty-glow` no longer used by the build.

## 2026-09-06T23:40:28Z — Project created; effect imported & documented
- Created `~/Projects/QubesOS/UI-Enhancements/Kwin/kitty-glow/` with full
  AGENTS.md-compliant structure (`src/`, `container/`, `scripts/`, `docs/`, `logs/`).
- Imported canonical source: `src/kittyglow.cpp`, `src/CMakeLists.txt`,
  `src/kittyglow.json` (checksums match the prior working copy).
- Added `container/setup-build-container.sh` (Fedora-37 / KWin 5.27.8) and
  `scripts/build.sh` + `scripts/deploy.sh`.
- Authored `SPECIFICATION.md`, `ARCHITECTURE.md`, `HANDBOOK.md`,
  `PROJECT_CONTEXT.md`, `ROADMAP.md` (each with Rule 18 HTML pointer) + `README.md`.
- Carried forward from prior session: `kittyglow.so` (sha `89e8513b…`, 47 600 B)
  already deployed to dom0 plugin paths and enabled in kwinrc; KWin activation
  (restart) deferred per user.
- See SPECIFICATION.md §8 (Lessons Learned) and HANDBOOK.md for build/deploy/activate.

## 2026-09-06T23:52:44+00:00 — Removed legacy working directory
- Deleted `/home/user/kitty-glow/` (redundant pre-project working copy). Project
  `src/` is now the sole canonical source. `kitty-glow-out/` (build artifacts) kept.
- Container mount and all project files unaffected.

## 2026-09-06T23:53:14+00:00 — Removed redundant build-artifact dir
- Deleted `/home/user/kitty-glow-out/` (copy of compiled `kittyglow.so` + `kittyglow.json`).
  Canonical artifacts live in `dist/`; build container keeps its own `/src/build`.
- No project file references the removed dir.
