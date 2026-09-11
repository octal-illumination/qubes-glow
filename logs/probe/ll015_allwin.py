# Note: This code is purely AI-generated.
# Build #15 verification: all-windows glow + exclusions + Meta+Shift+B toggle.
# Band = strict gold in the 4 strips hugging a window's frame (interior text
# excluded). Expect: bands > 0 for kitty/konsole, ~0 for notification/dialog,
# all bands drop to noise while the glow is toggled off, restore when on.
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def xdo(*a):
    return sp.run(["xdotool"] + list(a), capture_output=True, text=True).stdout.strip()

def grab():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    return ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))

def band_count(mask, geo, ext=40, inset=4):
    x, y, w, h = geo
    x0, y0 = max(x - ext, 0), max(y - ext, 0)
    x1, y1 = min(x + w + ext, mask.shape[1]), min(y + h + ext, mask.shape[0])
    region = mask[y0:y1, x0:x1].copy()
    ix0, iy0 = x - inset - x0, y - inset - y0
    ix1, iy1 = x + w + inset - x0, y + h + inset - y0
    region[max(iy0,0):iy1, max(ix0,0):ix1] = False   # drop interior
    return int(region.sum())

def geo_of(wid):
    out = xdo("getwindowgeometry", "--shell", wid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"])

kid = xdo("search", "--onlyvisible", "--class", "kitty").splitlines()[0]
kg = geo_of(kid)

# STEP 1: kitty ring (active)
xdo("windowactivate", "--sync", kid); time.sleep(1.2)
m = grab(); k_band = band_count(m, kg)
print("STEP1 kitty band:", k_band, "PASS" if k_band > 800 else "FAIL")

# STEP 2: toggle OFF (Meta+Shift+B), expect bands -> noise
xdo("key", "--clearmodifiers", "meta+shift+b"); time.sleep(1.2)
m = grab(); k_off = band_count(m, kg)
print("STEP2 toggle-off kitty band:", k_off, "PASS" if k_off < 300 else "FAIL")

# STEP 3: toggle ON again
xdo("key", "--clearmodifiers", "meta+shift+b"); time.sleep(1.2)
m = grab(); k_on2 = band_count(m, kg)
print("STEP3 toggle-on kitty band:", k_on2, "PASS" if k_on2 > 800 else "FAIL")

# STEP 4: notification exclusion (dbus Notify, 3s timeout)
n_before = set(xdo("search", "--onlyvisible", "--class", "plasmashell").splitlines())
sp.run(["dbus-send", "--session", "--print-reply", "--dest=org.freedesktop.Notifications",
        "/org/freedesktop/Notifications", "org.freedesktop.Notifications.Notify",
        "string:glowtest", "uint32:0", "string:", "string:Glow eligibility",
        "string:notifications must have NO halo", "array:string:", "dict:string:",
        "int32:6000"], capture_output=True)
time.sleep(1.8)
n_after = set(xdo("search", "--onlyvisible", "--class", "plasmashell").splitlines())
new = [w for w in n_after - n_before if w]
m = grab()
if new:
    ng = geo_of(new[0])
    n_band = band_count(m, ng)
    print("STEP4 notification band:", n_band, "(win", new[0], ng, ")",
          "PASS" if n_band < 200 else "FAIL")
else:
    print("STEP4 notification window not found — checking any plasmashell band: SKIP")

# STEP 5: dialog exclusion (kdialog)
dlg = None
if sp.run(["which", "kdialog"], capture_output=True).returncode == 0:
    p = sp.Popen(["kdialog", "--title", "glowtest", "--msgbox", "eligibility test"],
                 stdout=sp.DEVNULL, stderr=sp.DEVNULL)
    time.sleep(1.8)
    ids = xdo("search", "--onlyvisible", "--name", "glowtest").splitlines()
    m = grab()
    if ids:
        dg = geo_of(ids[0]); d_band = band_count(m, dg)
        print("STEP5 dialog band:", d_band, "(win", ids[0], dg, ")",
              "PASS" if d_band < 200 else "FAIL")
        xdo("windowclose", ids[0])
    else:
        print("STEP5 dialog window not found: SKIP")
    p.terminate()
else:
    print("STEP5 kdialog unavailable: SKIP")
