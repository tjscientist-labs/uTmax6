#include <QtWidgets/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile stylesheet("Style.qss");
    stylesheet.open(QFile::ReadOnly);
    QString setSheet = QLatin1String(stylesheet.readAll());
    a.setStyleSheet(setSheet);

    MainWindow w;
    w.show();

    // uTmax6: "--test-sweep <file>" runs one sweep for automated tests (test_hook.cpp)
    int t = a.arguments().indexOf("--test-sweep");
    if (t > 0 && t + 1 < a.arguments().size())
        w.startTestHook(a.arguments().at(t + 1));
    
    return a.exec();
}
