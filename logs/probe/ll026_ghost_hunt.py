# Note: This code is purely AI-generated.
# Ghost-square hunt: find every gold ring component on screen, derive each
# culprit window's frame (ring bbox inset by the 22 px margin), cross-match
# against the FULL X tree and KWin supportInformation.
import subprocess as sp, time, os, re
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

# connected components (4-neighbour flood fill)
lab = np.zeros((H, W), dtype=int)
comps = []
cur = 0
for y in range(H):
    for x in range(W):
        if mask[y, x] and lab[y, x] == 0:
            cur += 1
            stack = [(y, x)]
            lab[y, x] = cur
            pix = []
            while stack:
                cy, cx = stack.pop()
                pix.append((cy, cx))
                for ny, nx in ((cy-1,cx),(cy+1,cx),(cy,cx-1),(cy,cx+1)):
                    if 0 <= ny < H and 0 <= nx < W and mask[ny, nx] and lab[ny, nx] == 0:
                        lab[ny, nx] = cur
                        stack.append((ny, nx))
            if len(pix) >= 30:
                ys = [p[0] for p in pix]; xs = [p[1] for p in pix]
                comps.append((len(pix), min(xs), min(ys), max(xs), max(ys)))
comps.sort(reverse=True)
print(f"screen {W}x{H}, {len(comps)} gold components (px, bbox x0,y0-x1,y1):")
for n, x0, y0, x1, y1 in comps[:15]:
    w, h = x1 - x0 + 1, y1 - y0 + 1
    ring_w = (w - (w - 44)) if w > 44 else w  # frame guess: inset 22 each side
    fw, fh = max(w - 44, 1), max(h - 44, 1)
    print(f"  {n:6d} px  bbox=({x0},{y0})-({x1},{y1}) {w}x{h}  -> frame? ({x0+22},{y0+22}) {fw}x{fh}")

MARGIN = 22
print("\n--- candidate frames (bbox inset by margin) vs X tree ---")
tree = run("xwininfo", "-root", "-tree")
open("/tmp/xwininfo_tree.txt", "w").write(tree)
cand = []
for n, x0, y0, x1, y1 in comps[:15]:
    fx, fy, fw, fh = x0 + MARGIN, y0 + MARGIN, x1 - x0 + 1 - 2 * MARGIN, y1 - y0 + 1 - 2 * MARGIN
    if fw < 4 or fh < 4:
        continue
    cand.append((n, fx, fy, fw, fh))
    # find tree lines whose geometry is within 6 px of the frame
    hits = []
    for line in tree.splitlines():
        m = re.search(r"(\d+)x(\d+)([+-]\d+)([+-]\d+)\s*$", line)
        if not m:
            continue
        tw, th, tx, ty = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
        if abs(tx - fx) <= 6 and abs(ty - fy) <= 6 and abs(tw - fw) <= 6 and abs(th - fh) <= 6:
            hits.append(line.strip()[:150])
    print(f"  ring {n}: frame ({fx},{fy}) {fw}x{fh} -> {len(hits)} tree hit(s)")
    for hl in hits[:4]:
        print("      " + hl)

print("\n--- KWin supportInformation blocks overlapping candidate frames ---")
si = run("qdbus", "org.kde.KWin", "/KWin", "supportInformation")
open("/tmp/kwin_supportinfo.txt", "w").write(si)
blocks, cur_blk = [], []
for line in si.splitlines():
    if line.startswith(("Client ", "Unmanaged ", "Deleted ")):
        if cur_blk: blocks.append(cur_blk)
        cur_blk = [line]
    elif cur_blk:
        cur_blk.append(line)
if cur_blk: blocks.append(cur_blk)
for n, fx, fy, fw, fh in cand:
    print(f"  ring {n}: frame ({fx},{fy}) {fw}x{fh}")
    shown = 0
    for blk in blocks:
        gm = re.search(r"[Gg]eometry\s*:\s*(-?\d+),\s*(-?\d+)\s+(\d+)x(\d+)", "\n".join(blk))
        if not gm:
            continue
        gx, gy, gw, gh = map(int, gm.groups())
        if gx < fx + fw + MARGIN and gx + gw > fx - MARGIN and gy < fy + fh + MARGIN and gy + gh > fy - MARGIN:
            info = " | ".join(l.strip() for l in blk if re.match(
                r"^\s*(Client|Unmanaged|Class|Geometry|ClientSize|Opacity|Modal|ShellSurface|ResourceName|Window Role)",
                l, re.I))[:220]
            print("      " + info)
            shown += 1
            if shown >= 5:
                break
    if shown == 0:
        print("      (no KWin block overlaps)")
