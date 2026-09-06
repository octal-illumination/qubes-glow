#!/usr/bin/env bash
# Note: This code is purely AI-generated.
#
# Recreates the Fedora-37 build container used to compile the KWin 5.27.8
# effect. The live container `dom0-replica-fed37` was originally built this way.
#
# Strategy:
#   1. Capture a reusable image `dom0-replica-fed37-img` that contains the
#      installed toolchain (kwin-devel + KF5 + Qt5 + libepoxy + cmake + gcc).
#      This is what makes builds reproducible without re-installing packages.
#   2. (Re)create the container mounting this project's src/ at /src.
#
# Fedora 37 is EOL, so a from-scratch build pulls from the Fedora vault archive.
set -euo pipefail

IMG=dom0-replica-fed37-img
CTR=dom0-replica-fed37
SRC="$(cd "$(dirname "$0")/../src" && pwd)"
ARCHIVE=https://archives.fedoraproject.org/pub/archive/fedora/linux/releases/37/Everything/x86_64/os

# ---------------------------------------------------------------------------
# 1. Image
# ---------------------------------------------------------------------------
if podman image exists "$IMG"; then
  echo "Image $IMG already present."
elif podman container exists "$CTR"; then
  echo "Committing live container -> $IMG (captures installed toolchain)"
  podman commit "$CTR" "$IMG"
else
  echo "Building fresh image from Fedora 37 archive (EOL release)..."
  podman run --name "$CTR" fedora:37 bash -c "
    dnf install -y --nogpgcheck \
      --setopt=reposdir=/dev/null \
      --setopt=baseurl=$ARCHIVE \
      cmake gcc-c++ extra-cmake-modules \
      kwin-devel kf5-kcoreaddons-devel kf5-kwindowsystem-devel \
      kf5-kconfig-devel kf5-kservice-devel kf5-kpackage-devel \
      kf5-kdeclarative-devel libepoxy-devel qt5-qtbase-devel \
      qt5-qtdeclarative-devel
  "
  podman commit "$CTR" "$IMG"
  podman rm -f "$CTR"
fi

# ---------------------------------------------------------------------------
# 2. Container (mounts project src at /src so builds use the canonical source)
# ---------------------------------------------------------------------------
podman rm -f "$CTR" 2>/dev/null || true
podman run -d --name "$CTR" -v "$SRC:/src" "$IMG" sleep infinity
echo "Container $CTR ready (src -> /src). Build with scripts/build.sh"
