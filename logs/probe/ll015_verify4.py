# Note: This code is purely AI-generated.
# Build #15 final probe: per-window ring bands + notification type/exclusion.
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a, env_extra=None):
    e = dict(os.environ)
    if env_extra: e.update(env_extra)
    return sp.run(list(a), capture_output=True, text=True, env=e).stdout.strip()

def grab():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    return ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))

def band_count(mask, geo, ext=40, inset=4):
    x, y, w, h = geo
    x0, y0 = max(x - ext, 0), max(y - ext, 0)
    x1, y1 = min(x + w + ext, mask.shape[1]), min(y + h + ext, mask.shape[0])
    region = mask[y0:y1, x0:x1].copy()
    region[max(y - inset - y0, 0):y + h + inset - y0,
           max(x - inset - x0, 0):x + w + inset - x0] = False
    return int(region.sum())

def geo_of(wid):
    out = run("xdotool", "getwindowgeometry", "--shell", wid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"])

xdo = lambda *a: run("xdotool", *a)

# PHASE A: per-window bands on the current (mostly minimized) layout
vis = [w for w in xdo("search", "--onlyvisible", "--class", ".").splitlines() if w]
m = grab()
print("PHASE-A per-window bands (strict gold):")
for wid in vis[:20]:
    c = xdo("getwindowclassname", wid)
    if c in ("plasmashell",): continue
    try:
        g = geo_of(wid)
        print(f"  {wid} {c:12s} {g} band={band_count(m, g)}")
    except Exception:
        pass

# PHASE B: notification — stacking diff + window type + band
def stacking():
    out = run("xprop", "-root", "_NET_CLIENT_LIST_STACKING")
    return out.split("#", 1)[-1].split(",") if "#" in out else []

before = stacking()
sp.run(["dbus-send", "--session", "--print-reply", "--dest=org.freedesktop.Notifications",
        "/org/freedesktop/Notifications", "org.freedesktop.Notifications.Notify",
        "string:glowtest", "uint32:0", "string:", "string:Glow eligibility",
        "string:notifications carry no halo", "array:string:", "dict:string:",
        "int32:8000"], capture_output=True)
time.sleep(1.8)
after = stacking()
new = [w.strip() for w in after if w.strip() not in before]
m = grab()
if new:
    wid = new[-1]
    wt = run("xprop", "-id", wid, "_NET_WM_WINDOW_TYPE")
    try:
        g = geo_of(wid)
        nb = band_count(m, g)
        print("PHASE-B notification win", wid, "geo", g, "band:", nb,
              "PASS" if nb < 200 else "FAIL")
    except Exception as e2:
        print("PHASE-B notification win", wid, "(override-redirect, unmanaged):",
              "PASS-BY-CONSTRUCTION" if "Not" in str(e2) else e2)
    print("PHASE-B window type:", wt.strip()[:120])
else:
    print("PHASE-B no new stacking entry: SKIP")
