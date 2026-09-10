# Note: This code is purely AI-generated.
# LL-020 verification burst for kittyglow build #10 (scissor-test fix).
# Runs in dom0. Cycles activation between firefox and a konsole (both sit
# above the kitty window), grabs the screen each cycle, and counts strict /
# loose gold pixels inside each occluder rect. Success = ~0 gold over ALL
# occluders across the burst (no accumulation). Failure = gold hundreds+
# growing per cycle (the pre-fix leak signature).
import subprocess, time, os, sys, json
from PIL import ImageGrab
import numpy as np

ENV = dict(os.environ, XAUTHORITY="/tmp/xauth_PCByVw", DISPLAY=":0")

def run(args):
    return subprocess.run(args, capture_output=True, text=True, env=ENV).stdout.strip()

def geo(wid):
    out = run(["xdotool", "getwindowgeometry", "--shell", str(wid)])
    d = {}
    for line in out.splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            d[k] = int(v)
    return (d["X"], d["Y"], d["WIDTH"], d["HEIGHT"])

def find(cls):
    return [w for w in run(["xdotool", "search", "--class", cls]).split() if w]

def activate(wid):
    subprocess.run(["xdotool", "windowactivate", "--sync", str(wid)],
                   capture_output=True, env=ENV)

W, H = map(int, run(["xdotool", "getdisplaygeometry"]).split())
kitty_ids, kon_ids, ff_ids = find("kitty"), find("konsole"), find("firefox")
kg = geo(kitty_ids[0])
rects = {"kitty_self": kg}
for i, w in enumerate(kon_ids):
    rects["konsole%d" % i] = geo(w)
for i, w in enumerate(ff_ids):
    rects["ff%d" % i] = geo(w)
rects["panel"] = (0, H - 20, W, 20)
ff_id = ff_ids[0]
kon0 = kon_ids[0]

def mask_counts(arr):
    r = arr[:, :, 0].astype(int)
    g = arr[:, :, 1].astype(int)
    b = arr[:, :, 2].astype(int)
    strict = (r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80)
    loose = (r - g > 6) & (r - b > 40) & (g - b > 20) & (r > 60)
    out = {}
    for name, (x, y, w, h) in rects.items():
        sx, sy = max(x, 0), max(y, 0)
        out[name] = [int(strict[sy:y + h, sx:x + w].sum()),
                     int(loose[sy:y + h, sx:x + w].sum())]
    return out

print("SCREEN %dx%d" % (W, H))
print("KITTY %s %s" % (kitty_ids[0], kg))
print("RECTS " + json.dumps(rects))
frames = []
cyc = [ff_id, kon0]
results = []
for i in range(24):
    activate(cyc[i % 2])
    time.sleep(0.06)
    arr = np.array(ImageGrab.grab(xdisplay=":0"))
    c = mask_counts(arr)
    results.append({"i": i, "c": c})
    if i % 6 == 0:
        fn = "/tmp/kg_v35_f%d.png" % i
        ImageGrab.grab(xdisplay=":0").save(fn)
        frames.append(fn)
    time.sleep(0.30)
for i in range(5):
    time.sleep(0.5)
    arr = np.array(ImageGrab.grab(xdisplay=":0"))
    results.append({"i": "idle%d" % i, "c": mask_counts(arr)})
print(json.dumps(results))
print("FRAMES " + " ".join(frames))
