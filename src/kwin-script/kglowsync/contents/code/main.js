// kglowsync — kittyglow borderless live-sync poller (KWin scripting).
// Note: This code is purely AI-generated.
//
// State source: the kittyglow effect persists the desired borderless state
// in ~/.config/kittyglowrc (kwinrulesrc no longer participates — a loaded
// forcing rule overrides scripting noBorder writes, which is the
// write-revert fight diagnosed live on 2026-09-09).
//
// Behaviour:
//   * on load: getCurrentState() via callDBus restores the persisted state
//     and applies it to all existing eligible app windows (covers kwin
//     restarts);
//   * polls nextSource() every 60 ms for staged global commands from the
//     C++ effect (1 = borderless, 2 = bordered, 0 = nothing) and
//     nextWindowOp() for per-window commands (1 = flip borders on the
//     focused window; build #18 — Meta+Shift+B is per-window now, the
//     global sweep lives on Meta+Shift+Alt+B);
//   * per-window overrides (windowId -> noBorder) shield the user's
//     per-window choices from the 400 ms sweep; runtime-only — a restart
//     restores the launch default (borderless everywhere, glow on).
//   * applies desired to every eligible app window via Client.noBorder —
//     event-driven on clientAdded, plus a 400 ms sweep as safety net;
//   * heartbeat: if the effect stops replying for >2 s (plugin unloaded),
//     the sweep pauses and windows keep their last state.
//
// print() is dropped by journald on this build; JS diagnostics route
// through the effect's scriptLog slot (one-way callDBus, no reply).
//
// Keep byte-identical with the copy installed by deploy.sh under
// ~/.local/share/kwin/scripts/kglowsync/contents/code/main.js.

var SERVICE = "org.kde.kittyglow";
var PATH = "/sync";
var IFACE = "org.kde.kittyglow";

function nowMs() {
    return new Date().getTime();
}

// LL-026 (2026-09-11): qubes-gui strips _NET_WM_WINDOW_TYPE from every
// VM-proxied window, so type flags alone never exclude chrome. Mirror the
// C++ predicate (glowtargets.h): WM_CLASS survives the proxy — exclude
// plasma surfaces (panels, popups, start menu + submenus), the Qubes
// tray-widget ghosts (Qui-*, the 16x16 "ghost square" sources pinned at
// (0,0)), legacy tray embeds and krunner; check the scripting type flags
// defensively for dom0-native windows.
var CHROME_CLASSES = /plasmashell|xembedsniproxy|krunner/i;

function isBorderlessTarget(c) {
    if (!c || c.deleted || c.desktopWindow || c.dock) return false;
    // Size guard (LL-026 mirror): icons/ghosts (sub-48 px) are not windows.
    var fg = c.frameGeometry;
    if (fg && (fg.width < 48 || fg.height < 48)) return false;
    var cls = String(c.resourceClass == null ? "" : c.resourceClass);
    if (!cls) return false;
    var full = String(c.resourceName == null ? "" : c.resourceName) + ":" + cls;
    if (CHROME_CLASSES.test(full)) return false;
    if (cls.toLowerCase().indexOf("qui-") === 0) return false;
    var flags = ["dialog", "splash", "tooltip", "notification",
                 "onScreenDisplay", "popupMenu", "comboBox", "dropdownMenu",
                 "utility", "menu", "dockMenu"];
    for (var i = 0; i < flags.length; i++) {
        try { if (c[flags[i]]) return false; } catch (e) {}
    }
    return true;
}

function appWindows() {
    var list = workspace.clientList ? workspace.clientList()
             : (workspace.windowList ? workspace.windowList() : []);
    var out = [];
    for (var i = 0; i < list.length; i++) {
        if (isBorderlessTarget(list[i])) out.push(list[i]);
    }
    return out;
}

function slog(msg) {
    // Service may not own its bus name yet (kwin --replace overlap: the old
    // instance holds org.kde.kittyglow until it exits). Drop diagnostics
    // rather than letting callDBus throw and kill the whole script.
    try { callDBus(SERVICE, PATH, IFACE, "scriptLog", String(msg)); }
    catch (e) {}
}

// Returns a started QTimer(ms, fn) or null — never throws. Parentless
// construction first: the script global object emits "Could not convert
// argument 0" as a parent on this kwin build.
function makeTimer(ms, fn) {
    var t = null;
    var builders = [
        function() { return new QTimer(); },
        function() { return new QTimer(null); },
        function() { return new QTimer(this); }
    ];
    for (var i = 0; i < builders.length; i++) {
        try {
            t = builders[i]();
            if (t !== null && t !== undefined) break;
            t = null;
        } catch (e) { t = null; }
    }
    if (t === null) return null;
    t.interval = ms;
    t.timeout.connect(fn);
    t.start();
    return t;
}

var desired = null;      // last commanded noBorder value (true/false)
var lastReplyMs = 0;     // heartbeat of the effect's service

// Per-window border overrides (build #18): windowId (string) -> bool.
// Written by the focused-window flip (Meta+Shift+B); consulted by
// applyDesired BEFORE the global default so the 400 ms safety-net sweep
// protects the user's per-window choice instead of clobbering it. Keyed by
// windowId (plain string keys — no object references, nothing to prune;
// stale ids of closed windows are harmless garbage). Runtime-only by
// design: kwin restart restores the launch default (borderless).
var overrides = {};

// Flip borders on the script's OWN focused window (the C++ effect stages
// the command without a window id — workspace.activeClient is the same
// window the user pressed the key on, 60 ms earlier). The new state is
// recorded in overrides so the sweep keeps it.
function focusedBorderFlip(origin) {
    var c = workspace.activeClient ? workspace.activeClient : null;
    if (!c || !isBorderlessTarget(c)) {
        slog(origin + ": no eligible focused window ("
             + (c ? String(c.resourceClass) : "none") + ")");
        return;
    }
    var before = c.noBorder;
    var next = !before;
    c.noBorder = next;
    overrides[String(c.windowId)] = next;
    slog(origin + ": win=" + String(c.windowId) + " ("
         + String(c.resourceClass) + ") noBorder " + before + " -> " + next);
}

// Applies the effective noBorder value to every eligible app window; logs
// each actual write. Effective value per window = per-window override if
// one exists, else the global default. With no forcing rule these writes
// stick, so a steady state produces no output.
function applyDesired(origin) {
    if (desired === null) return;
    var ws = appWindows();
    for (var i = 0; i < ws.length; i++) {
        var id = String(ws[i].windowId);
        var want = (id in overrides) ? overrides[id] : desired;
        var before = ws[i].noBorder;
        if (before !== want) {
            ws[i].noBorder = want;
            slog(origin + ": win=" + id + " noBorder "
                 + before + " -> " + want
                 + (id in overrides ? " (per-window)" : ""));
        }
    }
}

slog("script alive, app windows=" + appWindows().length);

// Bootstrap with retry: at script load the C++ effect's service may not
// own its bus name yet (see slog) — the old kwin instance holds it until it
// exits after --replace. Retry every 500 ms up to 60 times (30 s) instead
// of dying at load ("Could not initialize scripted effect", 2026-09-10).
var bootTries = 0;
var booted = false;
function bootstrap() {
    bootTries++;
    try {
        callDBus(SERVICE, PATH, IFACE, "getCurrentState", function(st) {
            if (booted) return;            // idempotent: first reply wins
            booted = true;
            lastReplyMs = nowMs();
            desired = (Number(st) === 1);
            slog("bootstrap: persisted state=" + st + " -> desired=" + desired);
            applyDesired("bootstrap");
        });
    } catch (e) { /* service may not own its name yet */ }
}
bootstrap();

// LL-025 hardening part 2 (2026-09-11): a bootstrap reply can be silently
// LOST when the request lands on the dying kwin instance during --replace
// overlap — no throw, no callback, and the script hangs with desired=null
// forever (seen live on build #16's first restart). Re-arm every 5 s until
// the first reply lands (bounded by bootTries).
var bootWatch = makeTimer(5000, function() {
    if (!booted && bootTries < 60) bootstrap();
    if (booted && bootWatch) bootWatch.stop();
});

// Live toggle channel: consume staged commands from the C++ effect.
var poll = makeTimer(60, function() {
    try {
        callDBus(SERVICE, PATH, IFACE, "nextSource", function(src) {
            lastReplyMs = nowMs();
            var v = Number(src);
            if (v === 1) desired = true;
            else if (v === 2) desired = false;
        });
        // Per-window command channel (build #18): 1 = flip borders on the
        // focused window. Heartbeat shared with nextSource — the effect
        // answers both, so either reply proves the service is alive.
        callDBus(SERVICE, PATH, IFACE, "nextWindowOp", function(op) {
            lastReplyMs = nowMs();
            var v = Number(op);
            if (v === 1) focusedBorderFlip("focused-op");
        });
    } catch (e) { /* transient service gap: heartbeat pauses the sweep */ }
});
if (poll === null) slog("FATAL poll timer not constructible");

// Safety-net sweep (also covers windows that predate the script or whose
// class was not yet set at clientAdded time).
var watch = makeTimer(400, function() {
    if (desired === null || nowMs() - lastReplyMs >= 2000) return;
    applyDesired("sweep");
});
if (watch === null) slog("FATAL watch timer not constructible");

// Spawn coverage: borderless state applies the moment an eligible window maps.
if (workspace.clientAdded) {
    workspace.clientAdded.connect(function(c) {
        if (desired !== null && isBorderlessTarget(c)) {
            c.noBorder = desired;
            slog("clientAdded: applied desired=" + desired);
        }
    });
} else {
    slog("workspace.clientAdded unavailable — spawn coverage via sweep only");
}
