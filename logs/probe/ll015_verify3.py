# Note: This code is purely AI-generated.
# Build #15 verification v3: kglowsync revival via reconfigure, then
# clean-layout ring checks + exclusions. pi-konsole never touched.
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

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

def xdo(*a):
    return run("xdotool", *a)

# PHASE A: revive kglowsync (script re-reads main.js on reconfigure)
run("qdbus", "org.kde.KWin", "/KWin", "reconfigure")
time.sleep(2.5)

kid = xdo("search", "--onlyvisible", "--class", "kitty").splitlines()[0]

# PHASE B: minimize konsoles except the pi terminal, kitty active
vis = xdo("search", "--onlyvisible", "--class", ".").splitlines()
minimized = 0
for wid in vis:
    if wid == kid:
        continue
    name = xdo("getwindowname", wid)
    if "pi" in name.lower() and "konsole" in name.lower():
        continue
    xdo("windowminimize", wid); minimized += 1
time.sleep(0.8)
xdo("windowactivate", "--sync", kid); time.sleep(1.2)
m = grab(); kg = geo_of(kid)
kb = band_count(m, kg)
print("PHASE-B minimized:", minimized, "| kitty band:", kb, "frame:", kg,
      "PASS" if kb > 800 else "FAIL")

# PHASE C: notification exclusion
n_before = set(xdo("search", "--class", "plasmashell").splitlines())
sp.run(["dbus-send", "--session", "--print-reply", "--dest=org.freedesktop.Notifications",
        "/org/freedesktop/Notifications", "org.freedesktop.Notifications.Notify",
        "string:glowtest", "uint32:0", "string:", "string:Glow eligibility",
        "string:no halo for notifications", "array:string:", "dict:string:",
        "int32:8000"], capture_output=True)
time.sleep(1.8)
new = [w for w in set(xdo("search", "--class", "plasmashell").splitlines()) - n_before if w]
m = grab()
if new:
    nb = band_count(m, geo_of(new[0]))
    print("PHASE-C notification band:", nb, "geo:", geo_of(new[0]),
          "PASS" if nb < 200 else "FAIL")
else:
    print("PHASE-C notification window not found: SKIP")

# PHASE D: dialog exclusion
p = sp.Popen(["kdialog", "--title", "glowtest", "--msgbox", "eligibility test"],
             stdout=sp.DEVNULL, stderr=sp.DEVNULL)
time.sleep(1.8)
ids = xdo("search", "--onlyvisible", "--name", "glowtest").splitlines()
m = grab()
if ids:
    db = band_count(m, geo_of(ids[0]))
    print("PHASE-D dialog band:", db, "PASS" if db < 200 else "FAIL")
    xdo("windowclose", ids[0])
else:
    print("PHASE-D dialog not found: SKIP")
p.terminate()
