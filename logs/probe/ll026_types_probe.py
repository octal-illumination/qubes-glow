# Note: This code is purely AI-generated.
# Probe: window types/classes for exclusion design + top-right mystery square
# + kicker popup type. Read-only except xdotool key Meta.
import subprocess as sp, time, os

os.environ.setdefault("DISPLAY", ":0")
os.environ["XAUTHORITY"] = "/tmp/xauth_PCByVw"
os.environ.setdefault("DBUS_SESSION_BUS_ADDRESS", "unix:path=/run/user/1000/bus")
os.environ.setdefault("XDG_RUNTIME_DIR", "/run/user/1000")

def run(*a):
    return sp.run(list(a), capture_output=True, text=True).stdout.strip()

def dump(tag):
    ids = [w for w in run("xdotool", "search", "--onlyvisible", "--class", ".").splitlines() if w]
    print(f"--- {tag} ({len(ids)} windows) ---")
    for wid in ids:
        c = run("xdotool", "getwindowclassname", wid)
        n = run("xdotool", "getwindowname", wid)[:40]
        out = run("xdotool", "getwindowgeometry", "--shell", wid)
        d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
        g = (d.get("X"), d.get("Y"), d.get("WIDTH"), d.get("HEIGHT"))
        wt = run("xprop", "-id", wid, "_NET_WM_WINDOW_TYPE")
        wt = wt.replace("_NET_WM_WINDOW_TYPE(ATOM) = ", "").replace("_NET_WM_WINDOW_TYPE_", "").strip()
        tf = run("xprop", "-id", wid, "WM_TRANSIENT_FOR")
        tf = "T" if "= window id" in tf else "-"
        if c == "plasmashell":
            continue
        print(f"  {wid} {c:16s} {str(g):24s} {wt:14s} {tf} {n}")

dump("BASELINE")
tr = []
for wid in [w for w in run("xdotool", "search", "--class", ".").splitlines() if w]:
    out = run("xdotool", "getwindowgeometry", "--shell", wid)
    d = dict(l.split("=", 1) for l in out.splitlines() if "=" in l)
    try:
        x, y = int(d["X"]), int(d["Y"])
        w, h = int(d["WIDTH"]), int(d["HEIGHT"])
    except (KeyError, ValueError):
        continue
    if x + w > 1150 and y < 120 and w <= 80 and h <= 80:
        c = run("xdotool", "getwindowclassname", wid)
        tr.append((wid, c, (x, y, w, h)))
print("--- TOP-RIGHT small windows (incl. unmanaged/hidden) ---")
for wid, c, g in tr:
    wt = run("xprop", "-id", wid, "_NET_WM_WINDOW_TYPE").replace(
        "_NET_WM_WINDOW_TYPE(ATOM) = ", "").strip()
    st = run("xprop", "-id", wid, "WM_STATE").splitlines()[:1]
    print(f"  {wid} {c:16s} {g} type={wt} state={st}")

sp.run(["xdotool", "key", "--clearmodifiers", "super"], capture_output=True)
time.sleep(1.6)
dump("KICKER-OPEN")
sp.run(["xdotool", "key", "--clearmodifiers", "Escape"], capture_output=True)
