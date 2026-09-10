# Note: This code is purely AI-generated.
# LL-014 seam probe v4: konsole3 (500x300 at 120,104) raised ABOVE active
# kitty. Top ring band (y 166-198) crosses the occluder's right frame edge
# (x=620): measure gold OUTSIDE (seam continuity), INSIDE (translucent
# show-through), and FREE (reference). Plus visual crop.
import subprocess as sp, time, os
from PIL import ImageGrab, Image
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def xdo(*a):
    return sp.run(["xdotool"] + list(a), capture_output=True, text=True).stdout.strip()

kid = xdo("search", "--class", "kitty").splitlines()[0]
xdo("windowactivate", "--sync", kid)   # kitty active
sp.run(["xdotool", "windowraise", "79696295"])  # konsole3 above, no focus steal
time.sleep(2.0)                        # cursor blink repaints

a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
gold = ((r - b > 15) & (r - g > 6) & (r > 40))
seam_out = int(gold[166:198, 622:652].sum())   # right of occluder edge
seam_in  = int(gold[166:198, 588:618].sum())   # left of edge, under konsole
free     = int(gold[166:198, 800:1000].sum())  # unoccluded top band
print("SEAM_OUT:", seam_out, "/960")
print("SEAM_IN :", seam_in, "/960")
print("FREE    :", free, "/6400")
ImageGrab.grab(xdisplay=":0").save("/tmp/ll014_seam4.png")
crop = a[130:430, 150:750].astype(np.uint8)
Image.fromarray(crop).save("/tmp/ll014_seam4_crop.png")
