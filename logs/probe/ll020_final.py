# Note: This code is purely AI-generated.
# LL-020 final verification: the pre-fix failure signature was gold
# PERSISTING over occluders after kitty is re-buried (accumulating over
# minutes). Protocol: 3x [activate fullscreen konsole -> grab (BURIED),
# activate kitty -> grab (RAISED)]. PASS if every BURIED grab shows ~0
# strict gold in the ring region and no growth across cycles.
import subprocess, time, os, json
from PIL import ImageGrab
import numpy as np

ENV = dict(os.environ, XAUTHORITY="/tmp/xauth_PCByVw", DISPLAY=":0")

def run(args):
    return subprocess.run(args, capture_output=True, text=True, env=ENV).stdout.strip()

def geo(wid):
    d = {}
    for line in run(["xdotool", "getwindowgeometry", "--shell", str(wid)]).splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            d[k] = int(v)
    return (d["X"], d["Y"], d["WIDTH"], d["HEIGHT"])

def activate(wid):
    subprocess.run(["xdotool", "windowactivate", "--sync", str(wid)],
                   capture_output=True, env=ENV)

def gold(arr, rect):
    x, y, w, h = rect
    r = arr[y:y + h, x:x + w, 0].astype(int)
    g = arr[y:y + h, x:x + w, 1].astype(int)
    b = arr[y:y + h, x:x + w, 2].astype(int)
    strict = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80)).sum()
    loose = ((r - g > 6) & (r - b > 40) & (g - b > 20) & (r > 60)).sum()
    return [int(strict), int(loose)]

def kitty_geo(retries=8):
    for _ in range(retries):
        ids = run(["xdotool", "search", "--onlyvisible", "--class", "kitty"]).split()
        for wid in ids:
            g = geo(wid)
            if g[2] > 0 and g[3] > 0:
                return g
        time.sleep(1.0)
    raise SystemExit("no visible kitty window with valid geometry")

kitty = kitty_geo()
M = 30  # halo margin
ring = (max(kitty[0] - M, 0), max(kitty[1] - M, 0),
        kitty[2] + 2 * M, kitty[3] + 2 * M)
bury_id = [w for w in run(["xdotool", "search", "--class", "konsole"]).split()
           if geo(w)[2] >= 1300][0]  # a fullscreen konsole covering the ring

print("KITTY %s RING %s BURY %s" % (kitty, ring, bury_id))
out = []
for i in range(3):
    activate(bury_id); time.sleep(0.6)
    a1 = np.array(ImageGrab.grab(xdisplay=":0"))
    out.append({"ph": "buried%d" % i, "ring": gold(a1, ring)})
    act = run(["xdotool", "search", "--onlyvisible", "--class", "kitty"]).split()
    if act:
        activate(act[0])
    time.sleep(0.6)
    a2 = np.array(ImageGrab.grab(xdisplay=":0"))
    out.append({"ph": "raised%d" % i, "ring": gold(a2, ring)})
print(json.dumps(out))
