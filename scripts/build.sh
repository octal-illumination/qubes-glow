#!/usr/bin/env bash
# Note: This code is purely AI-generated.
#
# Builds kittyglow.so inside the dom0-replica-fed37 container (Fedora 37 /
# KWin 5.27.8) and copies the artifacts into ../dist.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="$ROOT/src"
DIST="$ROOT/dist"
CTR=dom0-replica-fed37
mkdir -p "$DIST"

# Ensure the container exists and mounts the project src at /src.
MOUNTED="$(podman inspect --format '{{ range .Mounts }}{{ .Source }}{{ end }}' "$CTR" 2>/dev/null || true)"
if [[ "$MOUNTED" != *"$SRC"* ]]; then
  echo "Container not mounted to project src; running setup..."
  "$ROOT/container/setup-build-container.sh"
fi
[ "$(podman inspect -f '{{.State.Running}}' "$CTR" 2>/dev/null)" = "true" ] || podman start "$CTR"

# Compile (C++20; KWin 5.27 kwineffects.h uses std::span).
podman exec "$CTR" bash -c \
  "cd /src && rm -rf /tmp/b && cmake -B /tmp/b -DCMAKE_BUILD_TYPE=Release >/tmp/b_cmake.log 2>&1 && cmake --build /tmp/b -j\$(nproc) >/tmp/b_make.log 2>&1"

podman cp "$CTR:/tmp/b/kittyglow.so" "$DIST/"
podman cp "$CTR:/src/kittyglow.json" "$DIST/"
echo "Built:"; sha256sum "$DIST/kittyglow.so" "$DIST/kittyglow.json"
