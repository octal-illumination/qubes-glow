// kglowdump — one-shot KWin script: dump every managed client's class and
// frame geometry through the kittyglow scriptLog channel (journal-visible).
// Note: This code is purely AI-generated.
function slog(m) {
    callDBus("org.kde.kittyglow", "/sync", "org.kde.kittyglow", "scriptLog", String(m));
}
var ws = workspace.clientList ? workspace.clientList()
       : (workspace.windowList ? workspace.windowList() : []);
slog("DUMP BEGIN n=" + ws.length);
for (var i = 0; i < ws.length; i++) {
    var c = ws[i];
    var g = c.frameGeometry;
    slog("DUMP id=" + c.windowId + " cls=[" + c.windowClass + "] deleted=" + c.deleted
         + " frame=" + Math.round(g.x) + "," + Math.round(g.y) + " "
         + Math.round(g.width) + "x" + Math.round(g.height)
         + " noBorder=" + c.noBorder);
}
slog("DUMP END");
