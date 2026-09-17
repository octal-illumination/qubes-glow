#!/usr/bin/env python3
# Note: This code is purely AI-generated.
# LL-033 probe v7: DECISIVE. Live halo is WHITE on dom0 (GlowColor=255,255,255),
# not gold — all earlier gold masks measured wallpaper. Detects the Konsole
# popup by (a) new X windows + Override-Redirect state, (b) _NET_CLIENT_LIST
# membership, (c) bright-ring analysis around the changed region, and renders
# the neighbourhood as ASCII so it is readable without images. READ-ONLY.
import os, re, time, subprocess as sp

os.environ.setdefault("DISPLAY", ":0")
_xa = sp.run("bash -lc 'ls -t /tmp/xauth_* 2>/dev/null | head -1'", shell=True,
             capture_output=True, text=True).stdout.strip()
if _xa:
    os.environ["XAUTHORITY"] = _xa
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
print("[env] XAUTHORITY=%s" % os.environ.get("XAUTHORITY"), flush=True)

import numpy as np                                    # noqa: E402
from PIL import ImageGrab                             # noqa: E402


def sh(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout


def grab():
    return np.asarray(ImageGrab.grab().convert("RGB")).astype(np.int16)


def managed_dec():
    out = set()
    for prop in ("_NET_CLIENT_LIST", "_NET_CLIENT_LIST_STACKING"):
        for tok in sh("xprop", "-root", prop).replace(",", " ").split():
            if tok.startswith("0x"):
                out.add(str(int(tok, 16)))
    return out


def tree_ids():
    out = {}
    for line in sh("xwininfo", "-root", "-tree").splitlines():
        m = re.match(r"\s+(0x[0-9a-f]+)\s+(.*)$", line)
        if m:
            out[m.group(1)] = m.group(2).strip()
    return out


def geo(wid):
    d = dict(l.split("=", 1) for l in
             sh("xdotool", "getwindowgeometry", "--shell", wid).splitlines() if "=" in l)
    try:
        return (int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"]))
    except (KeyError, ValueError):
        return (-1, -1, 0, 0)


def ascii_view(arr, cols=72, tag=""):
    """Downsample luminance to ASCII so the layout is human-readable."""
    h, w = arr.shape[:2]
    rows = max(1, int(cols * h / max(1, w) / 2.1))
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


def bright_mask(a, thr=200):
    return (a[:, :, 0] > thr) & (a[:, :, 1] > thr) & (a[:, :, 2] > thr)


def ring_stats(mask, rect, pad=22):
    x0, y0, x1, y1 = rect
    gx0, gy0 = max(0, x0 - pad), max(0, y0 - pad)
    gx1, gy1 = min(mask.shape[1], x1 + pad), min(mask.shape[0], y1 + pad)
    inner = mask[max(0, y0):y1 + 1, max(0, x0):x1 + 1]
    ring = mask[gy0:gy1, gx0:gx1].copy()
    iy, ix = max(0, y0 - gy0), max(0, x0 - gx0)
    ring[iy:iy + (y1 - y0 + 1), ix:ix + (x1 - x0 + 1)] = False
    return int(inner.sum()), int(ring.sum())


base = grab()
kw = None
for wid in sh("xdotool", "search", "--onlyvisible", "--class", "^konsole$").split():
    if geo(wid)[2] > 300:
        kw = wid
        break
kg = geo(kw) if kw else None
print("--- konsole=%s geom=%s" % (kw, kg), flush=True)
if not kw:
    raise SystemExit("no konsole")

mg0 = managed_dec()
print("--- konsole in _NET_CLIENT_LIST: %s" % (kw in mg0), flush=True)
print("--- baseline: %d client-list ids, %d tree ids" % (len(mg0), len(tree_ids())),
      flush=True)

sp.run(["xdotool", "windowactivate", "--sync", kw], capture_output=True)
time.sleep(0.7)

attempts = [("click-1", ["xdotool", "mousemove", str(kg[0] + 25), str(kg[1] + 15),
                         "click", "1"]),
            ("click-2", ["xdotool", "mousemove", str(kg[0] + 25), str(kg[1] + 22),
                         "click", "1"]),
            ("alt-f", ["xdotool", "key", "--clearmodifiers", "alt+f"]),
            ("F10", ["xdotool", "key", "--clearmodifiers", "F10"])]

for tag, cmd in attempts:
    print("\n=== ATTEMPT %s : %s ===" % (tag, " ".join(cmd)), flush=True)
    base_tree = tree_ids()
    base_mg = managed_dec()
    sp.run(cmd, capture_output=True)
    time.sleep(1.0)
    cur = grab()
    d = np.abs(base - cur).sum(axis=2) > 24
    new_tree = {k: v for k, v in tree_ids().items() if k not in base_tree}
    new_mg = managed_dec() - base_mg
    print("  changed px=%d  new tree windows=%d  new client-list ids=%d"
          % (int(d.sum()), len(new_tree), len(new_mg)), flush=True)
    for hexid, rest in list(new_tree.items())[:6]:
        dec = str(int(hexid, 16))
        xi = sh("xwininfo", "-id", hexid)
        ovr = re.search(r"Override Redirect State:\s*(\w+)", xi)
        gm = re.search(r"(\d+x\d+\+\-?\d+\+\-?\d+)", rest)
        mstate = re.search(r"Map State:\s*(\w+)", xi)
        print("    %s dec=%s geom=%s override_redirect=%s map=%s managed=%s"
              % (hexid, dec, gm.group(1) if gm else "?", ovr.group(1) if ovr else "?",
                 mstate.group(1) if mstate else "?", dec in managed_dec()), flush=True)
    if d.sum() > 500:
        ys, xs = np.nonzero(d)
        # ignore full-screen changes (desktop switch)
        if (xs.max() - xs.min()) < 1200:
            rect = (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max()))
            bm = bright_mask(cur)
            inr, ring = ring_stats(bm, rect)
            print("  CHANGED-REGION rect=%s  bright(>200) in-rect=%d ring(+22px)=%d"
                  % (rect, inr, ring), flush=True)
            x0, y0, x1, y1 = rect
            pad = 55
            crop = cur[max(0, y0 - pad):y1 + pad, max(0, x0 - pad):x1 + pad]
            ascii_view(crop, 72, "%s-brightness" % tag)
            ascii_view((bm[max(0, y0 - pad):y1 + pad,
                           max(0, x0 - pad):x1 + pad] * 255).astype(np.int16),
                       72, "%s-WHITE-MASK" % tag)
    sp.run(["xdotool", "key", "--clearmodifiers", "Escape"], capture_output=True)
    time.sleep(0.6)
print("done", flush=True)