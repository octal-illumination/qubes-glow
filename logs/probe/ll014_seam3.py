# Note: This code is purely AI-generated.
# LL-014 seam probe v3: small occluder (konsole3 resized 500x300 at 120,104),
# kitty half visible -> its cursor-blink repaints redraw the halo. Metrics:
# seam strip just right of the occluder frame, free-ring reference right of
# kitty, occluder interior (expect 0), plus visual crop.
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def xdo(*a):
    return sp.run(["xdotool"] + list(a), capture_output=True, text=True).stdout.strip()

kid = xdo("search", "--class", "kitty").splitlines()[0]
k3 = "79696295"
xdo("windowactivate", "--sync", kid)          # kitty active + on top
time.sleep(0.4)
xdo("windowsize", k3, "500", "300")           # shrink occluder; kitty stays active
time.sleep(2.5)                               # let cursor blink drive repaints

a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
gold = ((r - b > 15) & (r - g > 6) & (r > 40))
seam = int(gold[200:400, 625:655].sum())      # just right of konsole3 frame (x ends 620)
freer = int(gold[200:400, 1000:1056].sum())   # unoccluded ring right side
inner = int(gold[150:350, 300:500].sum())     # inside occluder (expect ~0)
bottom = int(gold[450:560, 700:1000].sum())   # ring below occluder
print("SEAM(625-655):", seam, "/1500")
print("FREE_RING(1000-1056):", freer, "/3360")
print("OCCLUDER_INNER:", inner, "/10000")
print("RING_BOTTOM:", bottom, "/3300")
ImageGrab.grab(xdisplay=":0").save("/tmp/ll014_seam3.png")
crop = a[120:600, 250:1120].astype(np.uint8)
from PIL import Image as I
I.fromarray(crop).save("/tmp/ll014_seam3_crop.png")
