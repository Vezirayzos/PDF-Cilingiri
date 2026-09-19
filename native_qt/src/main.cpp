#include <QApplication>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("PDF Çilingiri");
    app.setApplicationDisplayName("PDF Çilingiri - Evrak & Belge Stüdyosu");
    app.setOrganizationName("Vezir");

    QIcon appIcon("app.ico");
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    MainWindow window;
    window.show();

    return app.exec();
}
