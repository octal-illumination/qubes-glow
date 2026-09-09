#include <kwineffects.h>
#include <QPluginLoader>
#include <cstdio>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    fprintf(stderr, "STEP loading /src/build/kittyglow.so\n");
    QPluginLoader loader("/src/build/kittyglow.so");
    QObject *inst = loader.instance();
    fprintf(stderr, "instance=%p err=%s\n", (void*)inst, loader.errorString().toUtf8().constData());
    KWin::EffectPluginFactory *f = qobject_cast<KWin::EffectPluginFactory*>(inst);
    fprintf(stderr, "factory=%p\n", (void*)f);
    if (f) {
        fprintf(stderr, "isSupported=%d\n", f->isSupported());
        KWin::Effect *e = f->createEffect();
        fprintf(stderr, "createEffect=%p\n", (void*)e);
    }
    fprintf(stderr, "DONE\n");
    return 0;
}
