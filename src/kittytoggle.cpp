// kittytoggle.cpp — DBus pull-service + kglowsync script-package plumbing.
// Note: This code is purely AI-generated.
//
// Single responsibility: connect the C++ effect to the KWin-scripting side
// of the seamless Meta+Shift+B toggle (LL-016). The effect persists the new
// value in kittyglowrc (kittyglowstate.cpp) and stages it here; the
// kglowsync KWin script polls nextSource() every 60 ms and writes
// Client.noBorder directly — live, restart-free, and without the
// org.kde.KWin.reconfigure() round-trip that reloaded the whole RuleBook and
// flashed every screen white.
#include "kittytoggle.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QFileInfo>
#include <QStandardPaths>
#include <KConfigGroup>
#include <KSharedConfig>
#include <QDebug>

#include "kittyglowstate.h"

namespace {

constexpr auto SERVICE_NAME = "org.kde.kittyglow";
constexpr auto OBJECT_PATH = "/sync";

// 0 = nothing pending, 1 = noBorder true (borderless), 2 = noBorder false.
// Written from the effect's toggle path, consumed by nextSource() via the
// DBus dispatch — both run in kwin's main thread, so no locking is needed.
int s_pending = 0;

// Per-window command slot (build #18): 0 = nothing pending, 1 = flip
// borders on the script's focused window. Same single-threaded staging
// contract as s_pending; last command wins (manual key presses only).
int s_pendingOp = 0;

class SyncService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kittyglow")
public:
    explicit SyncService(QObject *parent = nullptr) : QObject(parent) {}

public Q_SLOTS:
    // Called by the kglowsync KWin script (callDBus) every 60 ms. Returns and
    // consumes the staged value so a delayed reply can never re-apply a stale
    // toggle. 0 tells the script nothing was commanded.
    int nextSource() {
        const int v = s_pending;
        s_pending = 0;
        // Log only real command consumption — the poll flood (17 Hz) is
        // already proven alive; the journal does not need it repeated.
        if (v != 0)
            qWarning() << "kittyglow: nextSource consumed staged ->" << v;
        return v;
    }

    // Bootstrap for a freshly started kglowsync: the persisted state as
    // true/false so the script restores the last known state after a kwin
    // restart without waiting for a toggle. Read-only, never consumed.
    Q_SCRIPTABLE bool getCurrentState() {
        return KittyGlowState::loadNoBorder();
    }

    // Per-window command channel (build #18). Polled by the script alongside
    // nextSource(); returns and consumes the staged op (0 = nothing). The
    // script resolves the focused window itself, so no window id crosses
    // the bus.
    int nextWindowOp() {
        const int v = s_pendingOp;
        s_pendingOp = 0;
        if (v != 0)
            qWarning() << "kittyglow: nextWindowOp consumed staged ->" << v;
        return v;
    }

    // Diagnostic bridge: KWin scripts' print() is swallowed on this build
    // (debug-level dropped by journald), so the JS side reports through the
    // same session-bus service it already polls. One-way, no reply needed.
    // Audit L8: flatten newlines — a multi-line msg would inject fake
    // journal lines (cosmetic hygiene; the bus is same-user trust anyway).
    Q_SCRIPTABLE void scriptLog(const QString &msg) {
        QString flat = msg;
        flat.replace(QLatin1Char('\n'), QLatin1Char(' '));
        flat.replace(QLatin1Char('\r'), QLatin1Char(' '));
        qWarning().noquote() << "kglowsync(js):" << flat;
    }
};

// The kglowsync package must live in the desktop user's KPackage path; kwin
// (dom0, user chenpan) scans ~/.local/share/kwin/scripts at startup. Without
// it the staged value is never applied — deploy.sh installs it.
bool scriptPackagePresent() {
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/kwin/scripts/kglowsync");
    return QFileInfo::exists(base + QStringLiteral("/metadata.json"))
        && QFileInfo::exists(base + QStringLiteral("/contents/code/main.js"));
}

// Idempotent: keep the script enabled in kwinrc [Plugins] so a kwin restart
// always auto-runs it (PoC-7: [Plugins] <id>Enabled=true + restart = loaded).
void enablePluginEntry() {
    auto cfg = KSharedConfig::openConfig(QStringLiteral("kwinrc"));
    KConfigGroup plugins(cfg, QStringLiteral("Plugins"));
    if (!plugins.readEntry("kglowsyncEnabled", false)) {
        plugins.writeEntry("kglowsyncEnabled", true);
        cfg->sync();
    }
}

}  // namespace

namespace KittyToggle {

void init() {
    static bool done = false;
    if (done) return;
    done = true;

    if (!scriptPackagePresent()) {
        qWarning() << "kittyglow: kglowsync script package missing in"
                   << QStandardPaths::writableLocation(
                          QStandardPaths::GenericDataLocation)
                   << "/kwin/scripts — run scripts/deploy.sh; Meta+Shift+B "
                      "stays restart-based until it is installed";
    }
    enablePluginEntry();

    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.registerObject(QString::fromLatin1(OBJECT_PATH),
                       new SyncService(QCoreApplication::instance()),
                       QDBusConnection::ExportScriptableSlots
                           | QDBusConnection::ExportNonScriptableSlots);
    if (!bus.registerService(QString::fromLatin1(SERVICE_NAME))) {
        qWarning() << "kittyglow: could not own" << QString::fromLatin1(SERVICE_NAME)
                   << "- another effect instance may be running";
    }
}

void requestApply(bool noBorder) {
    s_pending = noBorder ? 1 : 2;
}

void requestWindowOp(int op) {
    s_pendingOp = op;
}

}  // namespace KittyToggle

#include "kittytoggle.moc"
