#include "mainwindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDir>
#include <QFileInfo>
#include <QVariant>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    bool isNew = false;
    QStringList files;
    QVariantList var;
    QStringList arguments(a.arguments());
    arguments.takeFirst();

    for (const auto& arg : arguments) {
        if (arg == "-h" || arg == "--help") {
            qInfo() << "Usage: fastplayer [option] [file(s)]";
            qInfo() << "";
            qInfo() << "Options:";
            qInfo() << "-h, --help\tShow this message";
            qInfo() << "-n, --new\tOpens a new instance";
            qInfo() << "";
            qInfo() << "When opening files without '--new', files are added to the running instance.";
            qInfo() << "If no file is provided, always opens a new instance.";
            return 0;
        }
        if (arg == "-n" || arg == "--new") {
            isNew = true;
            continue;
        }
        QFileInfo info(arg);
        if (info.exists()) {
            if (info.isDir()) {
                for (const auto& entry : info.dir().entryInfoList()) {
                    if (!entry.isDir()) {
                        files << entry.absoluteFilePath();
                    }
                }
            }
            else {
                files << info.absoluteFilePath();
            }
        }
    }
    // if (argc > 1) {
    //     arg = QString::fromUtf8(argv[1]);
    // }
    QDBusInterface iface(SERVICE_NAME, "/", SERVICE_NAME, QDBusConnection::sessionBus());
    if (!isNew && files.count() > 0 && iface.isValid()) {
        qInfo() << "Sending via D-Bus:" << files;
        // send files via dbus
        QVariantList var;
        var += files;
        iface.callWithArgumentList(QDBus::CallMode::Block, "loadFiles", var);

        return 0;
    }

    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    setlocale(LC_NUMERIC, "C");
    MainWindow w;
    if (files.count() > 0) {
        w.loadFiles(files);
    }
    w.show();

    return a.exec();
}
