#include "mainwindow.h"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    // Qt sets the locale in the QApplication constructor, but libmpv requires
    // the LC_NUMERIC category to be set to "C", so change it back.
    setlocale(LC_NUMERIC, "C");
    QString arg;
    if (argc > 1) {
        arg = QString::fromUtf8(argv[1]);
    }
    MainWindow w(arg);
    w.show();
    return a.exec();
}
