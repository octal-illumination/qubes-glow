# Note: This code is purely AI-generated.
# Build #16 verification: G/B shortcut split + LL-026 exclusions, with
# journal + pixel + geometry evidence. Sends super+shift+g / super+shift+b.
import subprocess as sp, time, os, re
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

def journal(pat, mins=2):
    out = sp.run(["journalctl", "_COMM=kwin_x11", "--since", f"-{mins}min",
                  "--no-pager"], capture_output=True, text=True).stdout
    return [l.split("dom0 ")[-1][:130] for l in out.splitlines()
            if re.search(pat, l)]

def gold_total():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    m = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))
    corner = int(m[0:60, 0:60].sum())
    return int(m.sum()), corner

def kitty_geom():
    kid = run("xdotool", "search", "--onlyvisible", "--class", "kitty").splitlines()[0]
    out = run("xdotool", "getwindowgeometry", "--shell", kid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    return int(d["HEIGHT"])

print("=== shortcuts ===")
si = run("qdbus", "org.kde.kglobalaccel", "/component/kwin",
         "org.kde.kwin.kglobalaccel.allShortcutInfos")
for l in si.splitlines():
    if re.search(r"(?i)glow|border", l):
        print("  " + l[:120])

print("=== kglowsync bootstrap (journal) ===")
for l in journal(r"kglowsync\(js\)")[:6]:
    print("  " + l)

total, corner = gold_total()
print(f"=== baseline gold: total={total} corner(0,0,60x60)={corner} "
      f"{'PASS' if corner < 60 else 'FAIL (ghost ring?)'} ===")

kg0 = kitty_geom()
run("xdotool", "key", "--clearmodifiers", "super+shift+g")
time.sleep(1.2)
total_off, _ = gold_total()
print(f"=== after G (glow off): total={total_off} "
      f"{'PASS' if total_off < total // 4 else 'FAIL'} ===")

run("xdotool", "key", "--clearmodifiers", "super+shift+g")
time.sleep(1.2)
total_on, corner2 = gold_total()
print(f"=== after G (glow on): total={total_on} corner={corner2} "
      f"{'PASS' if total_on > total // 2 else 'FAIL'} ===")

run("xdotool", "key", "--clearmodifiers", "super+shift+b")
time.sleep(1.5)
kg1 = kitty_geom()
print(f"=== after B (borders on): kitty frame {kg0} -> {kg1} px tall "
      f"{'PASS' if kg1 > kg0 + 15 else 'FAIL'} ===")

run("xdotool", "key", "--clearmodifiers", "super+shift+b")
time.sleep(1.5)
kg2 = kitty_geom()
print(f"=== after B (borders off): kitty frame {kg2} px tall "
      f"{'PASS' if abs(kg2 - kg0) <= 6 else 'FAIL'} ===")

print("=== toggle journal lines ===")
for l in journal(r"toggle:"):
    print("  " + l)
