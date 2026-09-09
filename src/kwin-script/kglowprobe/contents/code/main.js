// kglowprobe — one-shot occlusion diagnostic for the glow penetration
// artifact (temporary; deploy → capture → uninstall in a single dom0 batch).
// Note: This code is purely AI-generated.
//
// Observer only: dumps kitty + front-window geometry through the effect's
// scriptLog slot (print() is dropped by journald on this build), then makes
// the first non-maximized konsole active so it stacks above the fake-kitty
// window (konsole --name kitty). Writes no noBorder, closes nothing.
//
// Timer idiom copied verbatim from kglowsync (parentless QTimer works on
// this kwin build; keep byte-identical with any deployed copy).

var SERVICE = "org.kde.kittyglow";
var PATH = "/sync";
var IFACE = "org.kde.kittyglow";

function slog(msg) {
    callDBus(SERVICE, PATH, IFACE, "scriptLog", String(msg));
}

function dump(tag, c) {
    if (!c) { slog("KITTYGLOWPROBE " + tag + " null"); return; }
    slog("KITTYGLOWPROBE " + tag + " id=" + String(c.windowId)
        + " cls=" + String(c.resourceClass)
        + " frame=" + String(c.frameGeometry)
        + " expanded=" + String(c.expandedGeometry)
        + " op=" + String(c.opacity)
        + " min=" + String(c.minimized));
}

function pick() {
    var kl = null, fr = null;
    var list = workspace.clientList ? workspace.clientList()
             : (workspace.windowList ? workspace.windowList() : []);
    for (var i = 0; i < list.length; i++) {
        var c = list[i];
        var cls = String(c.resourceClass).toLowerCase();
        if (!kl && cls.indexOf('kitty') >= 0) kl = c;
        if (!fr && cls.indexOf('konsole') >= 0 && !c.minimized
            && c.maximized !== true) fr = c;
    }
    if (!fr) {
        for (var j = 0; j < list.length; j++) {
            var c2 = list[j];
            if (!fr && String(c2.resourceClass).toLowerCase().indexOf('konsole') >= 0
                && !c2.minimized) fr = c2;
        }
    }
    return [kl, fr];
}

var pair = pick();
dump("T0-KITTY", pair[0]);
dump("T0-FRONT", pair[1]);
if (pair[1]) {
    try {
        workspace.activeClient = pair[1];
        slog("KITTYGLOWPROBE RAISED id=" + String(pair[1].windowId));
    } catch (e) {
        slog("KITTYGLOWPROBE RAISE-FAILED " + e);
    }
}

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

var done = makeTimer(2500, function() {
    var p = pick();
    dump("T1-KITTY", p[0]);
    dump("T1-FRONT", p[1]);
    slog("KITTYGLOWPROBE DONE");
});
if (done === null) slog("KITTYGLOWPROBE FATAL timer not constructible");
