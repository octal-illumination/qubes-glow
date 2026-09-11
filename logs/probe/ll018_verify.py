# Note: This code is purely AI-generated.
# Build #18 verification: per-window B/G (focused) + global Alt-masters.
# Discriminators: journal lines name the scope; gold counts confirm the
# render path; journal SILENCE after a focused flip proves sweep protection.
import subprocess as sp, time, os, re
from PIL import ImageGrab
import numpy as np

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

def journal(pat, mins=4):
    out = sp.run(["journalctl", "_COMM=kwin_x11", "--since", f"-{mins}min",
                  "--no-pager"], capture_output=True, text=True).stdout
    return [l.split("dom0 ")[-1][:132] for l in out.splitlines()
            if re.search(pat, l)]

def gold():
    a = np.array(ImageGrab.grab(xdisplay=":0")).astype(int)
    r, g, b = a[:, :, 0], a[:, :, 1], a[:, :, 2]
    m = ((r - g > 18) & (r - b > 60) & (g - b > 30) & (r > 80))
    return int(m.sum())

def key(k):
    run("xdotool", "key", "--clearmodifiers", k)
    time.sleep(1.4)

kid = run("xdotool", "search", "--onlyvisible", "--class", "kitty").splitlines()[0]
run("xdotool", "windowactivate", "--sync", kid)
time.sleep(0.8)

print("=== 1. FOCUSED B on kitty (borderless flip; others stay bordered) ===")
g0 = gold()
key("super+shift+b")
g1 = gold()
print(f"  gold {g0} -> {g1} (kitty halo unaffected: {'PASS' if abs(g1-g0) < 80 else 'CHECK'})")

print("=== 2. Sweep-protection soak (2.5 s — overrides must shield kitty) ===")
time.sleep(2.5)
reverts = journal(r"sweep:.*kitty", 1)
print(f"  sweep reverts on kitty: {len(reverts)} (PASS if 0)")
for l in journal(r"focused-op", 2): print("  " + l)

print("=== 3. FOCUSED G on kitty (ring drops; scope=focused in journal) ===")
g2 = gold()
key("super+shift+g")
g3 = gold()
key("super+shift+g")
g4 = gold()
print(f"  on={g2} off={g3} back-on={g4} (PASS if g3 << g2, g4 ~= g2)")
for l in journal(r"toggle: glow \(focused", 2): print("  " + l)

print("=== 4. GLOBAL Alt+G (master off/on; scope=global in journal) ===")
key("super+shift+alt+g")
g5 = gold()
key("super+shift+alt+g")
g6 = gold()
print(f"  master-off={g5} master-on={g6} (PASS if g5 ~= 0, g6 ~= g2)")
for l in journal(r"toggle: glow \(global", 2): print("  " + l)

print("=== 5. GLOBAL Alt+B (borderless sweep; fixes persisted default) ===")
key("super+shift+alt+b")
time.sleep(1.0)
for l in journal(r"toggle: borderless \(global|sweep: win=", 1)[:6]: print("  " + l)
g7 = gold()
print(f"  final gold={g7} (halos visible with borderless default: PASS if > 0)")
