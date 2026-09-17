#!/usr/bin/env python3
# Note: This code is purely AI-generated.
# LL-033 scan: instant snapshot of every visible window's managed/override/
# type/opacity/_QUBES_ state plus an ASCII locator map. NO waiting, NO drive.
# Run with a one-word label: python3 /tmp/ll033scan.py <label>
# The caller compares against the known steady-state census to spot the newly
# opened menu / notification surface. READ-ONLY.
import os, re, sys, subprocess as sp
import numpy as np                  # noqa: E402
from PIL import ImageGrab           # noqa: E402

os.environ.setdefault("DISPLAY", ":0")
_xa = sp.run("bash -lc 'ls -t /tmp/xauth_* 2>/dev/null | head -1'", shell=True,
             capture_output=True, text=True).stdout.strip()
if _xa:
    os.environ["XAUTHORITY"] = _xa
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
LABEL = sys.argv[1] if len(sys.argv) > 1 else "scan"


def sh(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout


def managed_dec():
    return {str(int(t, 16)) for t in
            sh("xprop", "-root", "_NET_CLIENT_LIST").replace(",", " ").split()
            if t.startswith("0x")}


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


def full(wid, mgset):
    cls = sh("xdotool", "getwindowclassname", wid).strip()
    xi = sh("xwininfo", "-id", wid)
    ovr = re.search(r"Override Redirect State:\s*(\w+)", xi)
    ty = sh("xprop", "-id", wid, "_NET_WM_WINDOW_TYPE").split("=")[-1].strip()
    st = sh("xprop", "-id", wid, "_NET_WM_STATE").split("=")[-1].strip()
    op = sh("xprop", "-id", wid, "_NET_WM_WINDOW_OPACITY").split("=")[-1].strip()
    qb = [l.strip()[:90] for l in sh("xprop", "-id", wid).splitlines() if "_QUBES_" in l]
    return dict(cls=cls, mg=(wid in mgset),
                ovr=(ovr.group(1) if ovr else "?"), type=(ty or "<none>"),
                state=(st or "<none>"), opacity=(op or "<1.0=no attr>"), qubes=qb[:2])


img = np.asarray(ImageGrab.grab().convert("RGB")).astype(np.int16)
mgset = managed_dec()
vis = [w for w in visible()]

print("==== LL033SCAN[%s]  desktop:%s  visible=%d  managed=%d ===="
      % (LABEL, sh("xprop", "-root", "_NET_CURRENT_DESKTOP").strip(), len(vis),
         len(mgset)), flush=True)
for wid in sorted(vis):
    d = full(wid, mgset)
    g = geo(wid)
    print(" %-4s %-10s %-26s %s cls=%-24s type=%-34s st=%-24s op=%s  qubes=%s"
          % ("MG" if d["mg"] else "UNM", wid, str(g), d["ovr"],
             d["cls"][:24], d["type"][:34], d["state"][:24], d["opacity"],
             d["qubes"][0] if d["qubes"] else ""), flush=True)

# ASCII brightness map so an outline shape is visible in text (no image).
luma = img.mean(axis=2).astype(float)
h, w = luma.shape
ramp = " .:-=+*#%@"
print("    map:", flush=True)
for i in range(22):
    line = ""
    for j in range(int(22 * w / float(h) / 2.0)):
        y0, y1 = int(i * h / 22), int((i + 1) * h / 22)
        x0, x1 = int(j * w / (22 * w / float(h) / 2.0)), \
                 int((j + 1) * w / (22 * w / float(h) / 2.0))
        v = float(luma[y0:y1, x0:x1].mean()) if luma[y0:y1, x0:x1].size else 0.0
        line += ramp[min(len(ramp) - 1, int(v / 256.0 * len(ramp)))]
    print("    |" + line + "|", flush=True)
print("==== END[%s] ====" % LABEL, flush=True)