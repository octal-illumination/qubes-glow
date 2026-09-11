# Note: This code is purely AI-generated.
# Build #17 verification: ghost square gone, G/B toggles live, watchdog-fed
# kglowsync consuming borderless commands.
import subprocess as sp, time, os, re
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

def journal(pat, mins=3):
    out = sp.run(["journalctl", "_COMM=kwin_x11", "--since", f"-{mins}min",
                  "--no-pager"], capture_output=True, text=True).stdout
    return [l.split("dom0 ")[-1][:130] for l in out.splitlines()
            if re.search(pat, l)]

def gold():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    m = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))
    return int(m.sum()), int(m[0:60, 0:60].sum())

def kitty_h():
    kid = run("xdotool", "search", "--onlyvisible", "--class", "kitty").splitlines()[0]
    out = run("xdotool", "getwindowgeometry", "--shell", kid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return int(d["HEIGHT"])

time.sleep(3)  # let the script's watchdog bootstrap land
print("=== load + bootstrap (journal) ===")
for l in journal(r"Successfully loaded plugin effect:  \"kittyglow\"|script alive|bootstrap:|nextSource consumed"):
    print("  " + l)

total, corner = gold()
print(f"=== glow ON: total={total} corner={corner} "
      f"{'PASS (ghost gone)' if corner < 60 else 'FAIL'} ===")

run("xdotool", "key", "--clearmodifiers", "super+shift+g"); time.sleep(1.2)
off_total, off_corner = gold()
print(f"=== G off: total={off_total} corner={off_corner} "
      f"{'PASS' if off_total < total // 3 else 'FAIL'} ===")
run("xdotool", "key", "--clearmodifiers", "super+shift+g"); time.sleep(1.2)
on_total, on_corner = gold()
print(f"=== G on: total={on_total} corner={on_corner} "
      f"{'PASS' if on_total > total // 2 else 'FAIL'} ===")

kg0 = kitty_h()
run("xdotool", "key", "--clearmodifiers", "super+shift+b"); time.sleep(2.0)
kg1 = kitty_h()
print(f"=== B borders-on: kitty {kg0} -> {kg1} px "
      f"{'PASS' if kg1 > kg0 + 15 else 'FAIL'} ===")
run("xdotool", "key", "--clearmodifiers", "super+shift+b"); time.sleep(2.0)
kg2 = kitty_h()
print(f"=== B borders-off: kitty {kg2} px "
      f"{'PASS' if abs(kg2 - kg0) <= 6 else 'FAIL'} ===")

print("=== toggle + consumption journal ===")
for l in journal(r"toggle:|nextSource consumed|noBorder"):
    print("  " + l)
