# Note: This code is purely AI-generated.
# Diagnose: kglowsync load state + corner/tray blob ownership (halo vs icon).
import subprocess as sp, time, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    r = sp.run(list(a), capture_output=True, text=True)
    return (r.stdout + r.stderr).strip()

print("=== kglowsync package ===")
print("main.js:", run("ls", "-la",
      "/home/chenpan/.local/share/kwin/scripts/kglowsync/contents/code/main.js").splitlines()[-1][:100])
print("kglowsyncEnabled:", run("kreadconfig5", "--file", "kwinrc",
      "--group", "Plugins", "--key", "kglowsyncEnabled"))
print("isScriptLoaded:", run("qdbus", "org.kde.KWin", "/Scripting",
      "org.kde.kwin.Scripting.isScriptLoaded", "kglowsync"))
print("loadedScripts:", run("qdbus", "org.kde.KWin", "/Scripting",
      "org.kde.kwin.Scripting.loadedScripts")[:200])
print("getCurrentState:", run("qdbus", "org.kde.kittyglow", "/sync",
      "org.kde.kittyglow.getCurrentState"))
print("effectLoaded:", run("qdbus", "org.kde.KWin", "/Effects",
      "org.kde.kwin.Effects.isEffectLoaded", "kittyglow"))

def gold_regions():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    m = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))
    return int(m[0:60, 0:60].sum()), int(m[735:768, 990:1070].sum())

c_on, tray_on = gold_regions()
sp.run(["xdotool", "key", "--clearmodifiers", "super+shift+g"], capture_output=True)
time.sleep(1.3)
c_off, tray_off = gold_regions()
sp.run(["xdotool", "key", "--clearmodifiers", "super+shift+g"], capture_output=True)
time.sleep(1.3)
print(f"=== corner gold: glow ON={c_on} OFF={c_off} tray=ON:{tray_on} OFF:{tray_off} ===")
print("corner persists with effect off -> icon content, not halo"
      if c_off > c_on * 0.5 else "corner is effect halo")
print("tray persists with effect off -> icon content, not halo"
      if tray_off > tray_on * 0.5 else "tray region had halo")
