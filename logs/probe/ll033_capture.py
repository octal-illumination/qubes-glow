#!/usr/bin/env python3
# Note: This code is purely AI-generated.
# LL-033 capture: human-in-the-loop surface identification for the menu and
# notification glow artifacts. Re-censuses every 300 ms while the operator
# opens the surface, printing managed/override-redirect state plus an ASCII
# rendering of the popup neighbourhood. READ-ONLY: no config writes, no
# process control, no compositor interaction.
import os, re, sys, time, subprocess as sp

os.environ.setdefault("DISPLAY", ":0")
_xa = sp.run("bash -lc 'ls -t /tmp/xauth_* 2>/dev/null | head -1'", shell=True,
             capture_output=True, text=True).stdout.strip()
if _xa:
    os.environ["XAUTHORITY"] = _xa
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
print("[env] XAUTHORITY=%s" % os.environ.get("XAUTHORITY"), flush=True)

import numpy as np                                     # noqa: E402
from PIL import ImageGrab                              # noqa: E402


def sh(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout


def grab():
    return np.asarray(ImageGrab.grab().convert("RGB")).astype(np.int16)


def managed_dec():
    """Decimal ids in _NET_CLIENT_LIST -- the CLI equivalent of
    Window::isClient(), i.e. the inverse of EffectWindow::isManaged()."""
    out = set()
    for tok in sh("xprop", "-root", "_NET_CLIENT_LIST").replace(",", " ").split():
        if tok.startswith("0x"):
            out.add(str(int(tok, 16)))
    return out


def visible():
    return [w for w in sh("xdotool", "search", "--onlyvisible", "--class", ".").split()
            if w]


def geo(wid):
    d = dict(l.split("=", 1) for l in
             sh("xdotool", "getwindowgeometry", "--shell", wid).splitlines() if "=" in l)
    try:
        return (int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"]))
    except (KeyError, ValueError):
        return (-1, -1, 0, 0)


def describe(wid, mgset):
    cls = sh("xdotool", "getwindowclassname", wid).strip()
    xi = sh("xwininfo", "-id", wid)
    ovr = re.search(r"Override Redirect State:\s*(\w+)", xi)
    mp = re.search(r"Map State:\s*(\w+)", xi)
    typ = sh("xprop", "-id", wid, "_NET_WM_WINDOW_TYPE").split("=")[-1].strip()
    state = sh("xprop", "-id", wid, "_NET_WM_STATE").split("=")[-1].strip()
    opac = sh("xprop", "-id", wid, "_NET_WM_WINDOW_OPACITY").split("=")[-1].strip()
    qubes = [l for l in sh("xprop", "-id", wid).splitlines() if "_QUBES_" in l]
    return dict(
        cls=cls,
        managed=(wid in mgset),
        override=(ovr.group(1) if ovr else "?"),
        mapstate=(mp.group(1) if mp else "?"),
        type=(typ or "<none>"),
        state=(state or "<none>"),
        opacity=(opac or "<unset -> opacity()==1.0>"),
        qubes=[q[:120] for q in qubes[:3]],
    )


def ascii_view(arr, cols=76, tag=""):
    h, w = arr.shape[:2]
    if w == 0 or h == 0:
        print("      [%s] empty" % tag, flush=True)
        return
    rows = max(1, int(cols * h / float(w) / 2.1))
    ys = np.linspace(0, h, rows + 1).astype(int)
    xs = np.linspace(0, w, cols + 1).astype(int)
    ramp = " .:-=+*#%@"
    print("      [%s] ASCII %dx%d -> %dx%d" % (tag, w, h, cols, rows), flush=True)
    for i in range(rows):
        line = ""
        for j in range(cols):
            blk = arr[ys[i]:ys[i + 1], xs[j]:xs[j + 1]]
            v = float(blk.mean()) if blk.size else 0.0
            line += ramp[min(len(ramp) - 1, int(v / 256.0 * len(ramp)))]
        print("      |" + line + "|", flush=True)


def watch(tag, seconds, base_img, base_vis, base_mg):
    """Poll for a newly appeared surface; report the first one in detail."""
    print("  watching for %d s ..." % seconds, flush=True)
    t0, hit = time.time(), False
    while time.time() - t0 < seconds:
        mgset = managed_dec()
        cur_vis = set(visible())
        new = cur_vis - base_vis
        newmg = mgset - base_mg
        if new or newmg:
            print("\n  *** SURFACE DETECTED at t=%.1fs ***" % (time.time() - t0), flush=True)
            for wid in sorted(new | newmg):
                d = describe(wid, mgset)
                g = geo(wid)
                print("      id=%s class=%s geom=%s" % (wid, d["cls"], g), flush=True)
                print("      MANAGED=%s override_redirect=%s map=%s"
                      % (d["managed"], d["override"], d["mapstate"]), flush=True)
                print("      type=%s" % d["type"], flush=True)
                print("      state=%s" % d["state"], flush=True)
                print("      opacity=%s" % d["opacity"], flush=True)
                for q in d["qubes"]:
                    print("      qubes-prop: %s" % q, flush=True)
                if g[2] > 4 and g[3] > 4:
                    cur = grab()
                    ch = np.abs(base_img - cur).sum(axis=2) > 24
                    print("      changed px vs baseline=%d" % int(ch.sum()), flush=True)
                    pad = 60
                    x0, y0 = max(0, g[0] - pad), max(0, g[1] - pad)
                    x1 = min(cur.shape[1], g[0] + g[2] + pad)
                    y1 = min(cur.shape[0], g[1] + g[3] + pad)
                    ascii_view(cur[y0:y1, x0:x1], 76, "%s-luma" % tag)
                    bm = (cur[:, :, 0] > 200) & (cur[:, :, 1] > 200) & (cur[:, :, 2] > 200)
                    ascii_view((bm[y0:y1, x0:x1] * 255).astype(np.int16), 76,
                               "%s-BRIGHT-mask" % tag)
            hit = True
            break
        time.sleep(0.3)
    if not hit:
        print("  [%s] nothing new appeared in %d s" % (tag, seconds), flush=True)
    return hit


def snap_xywh(grid=20):
    """Coarse ASCII map of the whole screen so the operator can locate the
    target window before driving it."""
    img = grab()
    luma = img.mean(axis=2)
    h, w = luma.shape
    ramp = " .:-=+*#%@"
    print("      screen map (%dx%d):" % (w, h), flush=True)
    for i in range(grid):
        line = ""
        for j in range(int(grid * w / float(h) / 2.0)):
            y0, y1 = int(i * h / grid), int((i + 1) * h / grid)
            x0, x1 = int(j * w / (grid * w / float(h) / 2.0)), \
                     int((j + 1) * w / (grid * w / float(h) / 2.0))
            v = float(luma[y0:y1, x0:x1].mean()) if luma[y0:y1, x0:x1].size else 0.0
            line += ramp[min(len(ramp) - 1, int(v / 256.0 * len(ramp)))]
        print("      |" + line + "|", flush=True)


print("--- current desktop: %s" % sh("xprop", "-root", "_NET_CURRENT_DESKTOP").strip(),
      flush=True)
print("--- visible konsole windows:", flush=True)
for wid in sh("xdotool", "search", "--onlyvisible", "--class", "konsole").split():
    print("      id=%s class=%s geom=%s" % (wid, sh("xdotool", "getwindowclassname",
                                                   wid).strip(), geo(wid)), flush=True)
snap_xywh()

if "--map-only" in sys.argv:
    # Sanity mode: prove the capture path works (screen grab + window census)
    # without waiting for an operator. Exits before any interactive phase.
    print("--- map-only mode: capture path OK, exiting before watch ---", flush=True)
    raise SystemExit(0)

base_img = grab()
base_vis = set(visible())
base_mg = managed_dec()
print("--- baseline: %d visible, %d client-list ids ---"
      % (len(base_vis), len(base_mg)), flush=True)

print("\n" + "=" * 74, flush=True)
print(">>> 1. OPEN the Konsole menu-bar drop-down NOW and leave it open <<<", flush=True)
print("=" * 74, flush=True)
watch("menu", 12, base_img, base_vis, base_mg)

print("\n>>> close the menu (Esc), then press Enter here when ready <<<", flush=True)
try:
    input()
except EOFError:
    time.sleep(4)

base_img = grab()
base_vis = set(visible())
base_mg = managed_dec()
print("\n" + "=" * 74, flush=True)
print(">>> 2. TRIGGER the notification that shows the glow (40 s) <<<", flush=True)
print("=" * 74, flush=True)
watch("notif", 40, base_img, base_vis, base_mg)

print("\ndone -- paste this output into the session.", flush=True)