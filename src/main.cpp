#include <QtWidgets/QApplication>
#include "mainwindow.h"
#include "app_paths.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // uTmax6: make ~/uTmax_files and give it the built-in tube library if it has
    // none (3.01 asked for data.csv in a file dialog and never saved cal.txt).
    // The style sheet is looked for there first (3.01: the current folder only).
    if (!ensureUtmaxDir())
        qWarning() << "Cannot create" << utmaxDir() << "- settings will not be saved";
    else if (!installDefaultFile(":/data.csv", "data.csv"))
        qWarning() << "Cannot copy the tube library into" << utmaxDir();
    QString sheetPath = findStyleSheet();
    if (!sheetPath.isEmpty()) {
        QFile stylesheet(sheetPath);
        if (stylesheet.open(QFile::ReadOnly))
            a.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    }

    MainWindow w;
    w.show();

    // uTmax6: "--test-sweep <file>" runs one sweep for automated tests (test_hook.cpp)
    int t = a.arguments().indexOf("--test-sweep");
    if (t > 0 && t + 1 < a.arguments().size())
        w.startTestHook(a.arguments().at(t + 1));
    
    return a.exec();
}
