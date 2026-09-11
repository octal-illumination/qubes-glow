# Note: This code is purely AI-generated.
# Build #15 diagnosis: is ANY ring drawing? Is the shortcut registered?
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
strict = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))
loose = ((r - b > 35) & (r > 120))
ys, xs = np.nonzero(strict)
print("GLOBAL strict:", int(strict.sum()), "loose:", int(loose.sum()))
if len(xs):
    print("strict bbox:", (int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())))
# kitty ring zone bbox from the journal geometry: frame unknown; check the
# known kitty window's surroundings with the loose mask too
kid = run("xdotool", "search", "--onlyvisible", "--class", "kitty").splitlines()[0]
out = run("xdotool", "getwindowgeometry", "--shell", kid)
d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
x, y, w, h = int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"])
print("kitty frame:", (x, y, w, h))
ex = 40
zone = loose[max(y-ex,0):y+h+ex, max(x-ex,0):x+w+ex]
inner = loose[y+4:y+h-4, x+4:x+w-4]
print("kitty zone loose:", int(zone.sum()), "| interior loose:", int(inner.sum()),
      "| band loose:", int(zone.sum()) - int(inner.sum()))
print("kglobalaccelrc [Shortcuts][kittyglow]:")
print(run("kreadconfig5", "--file", "kglobalaccelrc", "--group", "Shortcuts",
          "--group", "kittyglow", "--key", "Toggle Kitty Borderless"))
print("kittyglowrc [General]: glowEnabled=",
      run("kreadconfig5", "--file", "kittyglowrc", "--group", "General",
          "--key", "glowEnabled"),
      "| noBorder=", run("kreadconfig5", "--file", "kittyglowrc",
                         "--group", "General", "--key", "noBorder"))
print("kwinrc kittyglowEnabled=", run("kreadconfig5", "--file", "kwinrc",
      "--group", "Plugins", "--key", "kittyglowEnabled"))
