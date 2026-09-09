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
//     and applies it to all existing kitty windows (covers kwin restarts);
//   * polls nextSource() every 60 ms for staged toggle commands from the
//     C++ effect (1 = borderless, 2 = bordered, 0 = nothing);
//   * applies desired to every kitty window via Client.noBorder —
//     event-driven on clientAdded, plus a 400 ms sweep as safety net;
//   * heartbeat: if the effect stops replying for >2 s (plugin unloaded),
//     the sweep pauses and kitty keeps its last state.
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

function isKitty(c) {
    return !c.deleted && !c.desktopWindow && !c.dock
        && String(c.resourceClass).toLowerCase().indexOf('kitty') >= 0;
}

function kittyWindows() {
    var list = workspace.clientList ? workspace.clientList()
             : (workspace.windowList ? workspace.windowList() : []);
    var out = [];
    for (var i = 0; i < list.length; i++) {
        if (isKitty(list[i])) out.push(list[i]);
    }
    return out;
}

function slog(msg) {
    callDBus(SERVICE, PATH, IFACE, "scriptLog", String(msg));
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

// Applies desired to every kitty window; logs each actual write. With no
// forcing rule these writes stick, so a steady state produces no output.
function applyDesired(origin) {
    if (desired === null) return;
    var ws = kittyWindows();
    for (var i = 0; i < ws.length; i++) {
        var before = ws[i].noBorder;
        if (before !== desired) {
            ws[i].noBorder = desired;
            slog(origin + ": win=" + String(ws[i].windowId) + " noBorder "
                 + before + " -> " + desired + " (now=" + ws[i].noBorder + ")");
        }
    }
}

slog("script alive, kitty windows=" + kittyWindows().length);

// Bootstrap: restore the persisted state after kwin/script restarts. The
// reply doubles as the first service heartbeat.
callDBus(SERVICE, PATH, IFACE, "getCurrentState", function(st) {
    lastReplyMs = nowMs();
    desired = (Number(st) === 1);
    slog("bootstrap: persisted state=" + st + " -> desired=" + desired);
    applyDesired("bootstrap");
});

// Live toggle channel: consume staged commands from the C++ effect.
var poll = makeTimer(60, function() {
    callDBus(SERVICE, PATH, IFACE, "nextSource", function(src) {
        lastReplyMs = nowMs();
        var v = Number(src);
        if (v === 1) desired = true;
        else if (v === 2) desired = false;
    });
});
if (poll === null) slog("FATAL poll timer not constructible");

// Safety-net sweep (also covers windows that predate the script or whose
// class was not yet set at clientAdded time).
var watch = makeTimer(400, function() {
    if (desired === null || nowMs() - lastReplyMs >= 2000) return;
    applyDesired("sweep");
});
if (watch === null) slog("FATAL watch timer not constructible");

// Spawn coverage: borderless state applies the moment a kitty window maps.
if (workspace.clientAdded) {
    workspace.clientAdded.connect(function(c) {
        if (desired !== null && isKitty(c)) {
            c.noBorder = desired;
            slog("clientAdded: applied desired=" + desired);
        }
    });
} else {
    slog("workspace.clientAdded unavailable — spawn coverage via sweep only");
}
