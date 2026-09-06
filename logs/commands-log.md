# Commands Log

## 2026-09-06T23:50:19Z — Re-point build container
- Command: `bash container/setup-build-container.sh`
- Reason: address item #2 — make the build container mount the project's `src/`
  and capture the toolchain into image `dom0-replica-fed37-img` so deps persist.

## 2026-09-06T23:40:28Z — Scaffold project tree + import source
- Command: `mkdir -p .../kitty-glow/{src,container,scripts,docs/research,logs/{build,output,error,run},dist}` then `cp /home/user/kitty-glow/{kittyglow.cpp,CMakeLists.txt,kittyglow.json} .../src/`
- Reason: create AGENTS.md-compliant project structure and import canonical source.

## (prior session) Build effect in container
- Command: `podman exec dom0-replica-fed37 bash -c 'cd /src && rm -rf /tmp/build && cmake -B /tmp/build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/build -j$(nproc)'`
- Reason: compile the KWin effect against the KWin 5.27.8 ABI.

## (prior session) Deploy to dom0
- Command: `dom0 "sudo bash -lc '... qvm-run -p Dev-General base64 -w0 <so> | base64 -d > /usr/lib64/qt5/plugins/kwin/effects/kittyglow.so; ...; chmod 644 ...'"`
- Reason: install plugin into dom0 where the compositor runs.

## (prior session) Enable plugin
- Command: `dom0 "kwriteconfig5 --file kwinrc --group Plugins --key kittyglowEnabled true"`
- Reason: register the effect so KWin loads it on next (re)start.

## 2026-09-06T23:52:44+00:00 — Remove legacy kitty-glow dir
- Command: `rm -rf /home/user/kitty-glow`
- Reason: clean up redundant pre-project working copy (user-approved; kitty-glow-out retained).

## 2026-09-06T23:53:14+00:00 — Remove kitty-glow-out
- Command: `rm -rf /home/user/kitty-glow-out`
- Reason: user-approved cleanup of redundant build-artifact copy.
