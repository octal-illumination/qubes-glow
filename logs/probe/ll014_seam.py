# Note: This code is purely AI-generated.
# LL-017s seamless probe (build #14): partial occlusion — offset konsole over
# part of kitty's ring. Expect: loose gold PRESENT in the seam strip just
# outside the occluder's frame edge (was a wallpaper gap under #13), ~0 inside
# the occluder's opaque frame, partial ring visible overall.
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def xdo(*a):
    return sp.run(["xdotool"] + list(a), capture_output=True, text=True).stdout.strip()

ids = [i for i in xdo("search", "--onlyvisible", "--class", "konsole").splitlines() if i]
def geo(i):
    out = xdo("getwindowgeometry", "--shell", i)
    g = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return int(g["X"]), int(g["Y"]), int(g["WIDTH"]), int(g["HEIGHT"])

target = None
for i in ids:
    x, y, w, h = geo(i)
    if (x, y, w, h) == (120, 104, 910, 472):
        target = i
print("konsole3 id:", target)
for i in ids:
    if i != target:
        xdo("windowminimize", i)
time.sleep(0.5)
xdo("windowactivate", "--sync", target)
time.sleep(1.5)

a = np.array(ImageGrab.grab(xdisplay=":0"))
r, g, b = a[:, :, 0].astype(int), a[:, :, 1].astype(int), a[:, :, 2].astype(int)
strict = (r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80)
loose = (r - b > 35) & (r > 120)

seam = loose[250:450, 1031:1060]     # just RIGHT of konsole3 frame (x ends 1030)
frame_int = loose[300:400, 700:900]  # inside konsole3 opaque area
far = loose[600:700, 1200:1300]      # far field baseline
print("SEAM_STRIP loose:", int(seam.sum()), "/6000")
print("FRAME_INTERIOR loose:", int(frame_int.sum()), "/20000")
print("FAR_FIELD loose:", int(far.sum()), "/10000")
ys, xs = np.nonzero(loose[100:600, 200:1200])
print("RING_ZONE loose total:", int(loose[100:600, 200:1200].sum()))
ImageGrab.grab(xdisplay=":0").save("/tmp/ll014_seam.png")
