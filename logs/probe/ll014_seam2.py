# Note: This code is purely AI-generated.
# LL-014 seam probe, differential pass: A = konsole3-active/kitty-inactive
# (partial occlusion, existing grab), B = kitty minimized (no ring),
# C = kitty active (full ring, active alpha, on top). Metric: gold-ness of
# A-B inside the C-B ring mask, seam strip vs free left strip.
import subprocess as sp, time, os
from PIL import Image, ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def xdo(*a):
    return sp.run(["xdotool"] + list(a), capture_output=True, text=True).stdout.strip()

A = np.array(Image.open("/tmp/ll014_seam.png").convert("RGB")).astype(int)

kid = xdo("search", "--class", "kitty").splitlines()[0]
xdo("windowminimize", kid); time.sleep(1.2)
B = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
xdo("windowactivate", "--sync", kid); time.sleep(1.5)
C = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)

def goldness(d):
    r, g, b = d[:, :, 0], d[:, :, 1], d[:, :, 2]
    return ((r - b > 15) & (r - g > 6) & (r > 40))

ring_mask = goldness(C - B)
seam = (slice(250, 450), slice(1031, 1060))
left = (slice(250, 450), slice(301, 333))
top  = (slice(166, 200),  slice(500, 900))
print("ring_mask total:", int(ring_mask[100:600, 200:1200].sum()))
print("SEAM  A-ring px:", int((ring_mask[seam] & goldness(A - B)[seam]).sum()), "/ mask", int(ring_mask[seam].sum()))
print("LEFT  A-ring px:", int((ring_mask[left] & goldness(A - B)[left]).sum()), "/ mask", int(ring_mask[left].sum()))
print("TOP   A-ring px:", int((ring_mask[top]  & goldness(A - B)[top]).sum()),  "/ mask", int(ring_mask[top].sum()))

for name, img in (("A", A), ("B", B), ("C", C)):
    crop = img[120:600, 250:1120].astype(np.uint8)
    Image.fromarray(crop).save(f"/tmp/ll014_{name}_crop.png")
print("crops saved")
