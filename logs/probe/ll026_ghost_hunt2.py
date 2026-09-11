# Note: This code is purely AI-generated.
# Ghost hunt v2: supportInformation inventory + component attribution.
import subprocess as sp, re, os
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout

a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
mask = (r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80)
H, W = mask.shape

# same components as v1 (compact, cached findings)
spots = [(452, 0, 0, 40, 44), (198, 915, 124, 930, 139), (112, 1036, 6, 1047, 17),
         (97, 16, 183, 27, 196), (87, 435, 43, 448, 51)]

# window inventory via xdotool (managed, incl. plasmashell this time)
wins = []
for wid in [w for w in run("xdotool", "search", "--class", ".").splitlines() if w]:
    out = run("xdotool", "getwindowgeometry", "--shell", wid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    try:
        wins.append((wid, run("xdotool", "getwindowclassname", wid),
                     int(d["X"]), int(d["Y"]), int(d["WIDTH"]), int(d["HEIGHT"])))
    except (KeyError, ValueError):
        pass

def contains(cx, cy):
    return [w for w in wins if w[2] <= cx <= w[2] + w[4] and w[3] <= cy <= w[3] + w[5]]

print("--- gold components attributed ---")
for n, x0, y0, x1, y1 in spots:
    cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
    hosts = contains(cx, cy)
    ringlike = n > 300
    kind = "RING?" if ringlike else "icon/text blob"
    host_s = ", ".join(f"{w[1]}({w[2]},{w[3]},{w[4]}x{w[5]})" for w in hosts) or "NO WINDOW (desktop/unmanaged)"
    print(f"  {kind}: {n}px bbox({x0},{y0})-({x1},{y1}) inside: {host_s}")

print("\n--- KWin supportInformation: small windows + top-strip windows ---")
si = run("qdbus", "org.kde.KWin", "/KWin", "supportInformation")
blocks, cur = [], []
for line in si.splitlines():
    if line.startswith(("Client ", "Unmanaged ", "Deleted ")):
        if cur: blocks.append(cur)
        cur = [line]
    elif cur:
        cur.append(line)
if cur: blocks.append(cur)
print(f"({len(blocks)} KWin window blocks)")
for blk in blocks:
    txt = "\n".join(blk)
    gm = re.search(r"[Gg]eometry\s*:\s*(-?\d+),\s*(-?\d+)\s+(-?\d+)x(-?\d+)", txt)
    cm = re.search(r"[Cc]lass\s*:\s*(\S+)", txt)
    if not gm:
        continue
    gx, gy, gw, gh = map(int, gm.groups())
    cls = cm.group(1) if cm else "?"
    interesting = (gw <= 80 or gh <= 80) or (gy <= 140 and gh <= 200) or "Qui" in cls or "plasma" in cls.lower()
    if interesting:
        role = re.search(r"[Ww]indow [Rr]ole\s*:\s*(\S+)", txt)
        role = role.group(1) if role else "-"
        kind = blk[0].split()[0]
        print(f"  {kind:9s} {cls:28s} ({gx},{gy}) {gw}x{gh} role={role}")
