# Note: This code is purely AI-generated.
# LL-020 positive control: minimize the konsoles covering kitty's ring,
# grab the screen, count halo gold, then restore everything. Definitive
# test of whether the halo RENDER PATH still draws (scissor fix or bust).
import subprocess, time, os
from PIL import ImageGrab
import numpy as np

ENV = dict(os.environ, XAUTHORITY="/tmp/xauth_PCByVw", DISPLAY=":0")

def run(args):
    return subprocess.run(args, capture_output=True, text=True, env=ENV).stdout.strip()

KITTY = 79698807
ring = (303, 168, 751, 382)  # kitty (333,198,691,322) +/- 30px

def counts(img):
    a = np.array(img)
    r, g, b = a[:, :, 0].astype(int), a[:, :, 1].astype(int), a[:, :, 2].astype(int)
    x, y, w, h = ring
    s = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))[y:y + h, x:x + w]
    l = ((r - g > 6) & (r - b > 40) & (g - b > 20) & (r > 60))[y:y + h, x:x + w]
    return int(s.sum()), int(l.sum())

kon = [w for w in run(["xdotool", "search", "--class", "konsole"]).split()]
print("KONSOLES", len(kon))
for w in kon:
    subprocess.run(["xdotool", "windowminimize", w], capture_output=True, env=ENV)
time.sleep(1.2)
img = ImageGrab.grab(xdisplay=":0")
img.save("/tmp/kg_positive.png")
print("MINIMIZED_COUNTS strict,loose =", counts(img))
# raise kitty too (above desktop remnants) and regrab
subprocess.run(["xdotool", "windowraise", str(KITTY)], capture_output=True, env=ENV)
time.sleep(0.6)
img2 = ImageGrab.grab(xdisplay=":0")
img2.save("/tmp/kg_positive2.png")
print("RAISED_COUNTS strict,loose =", counts(img2))
for w in kon:
    subprocess.run(["xdotool", "windowactivate", w], capture_output=True, env=ENV)
print("RESTORED")
