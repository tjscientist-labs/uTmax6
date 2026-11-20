/* app_paths.h — where uTmax keeps its files, and making sure they exist.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * Everything lives in ~/uTmax_files/: the calibration file (cal.txt), the tube
 * library (data.csv), the application style sheet (Style.qss) and, when switched
 * on, the debug logs.
 *
 * uTmax 3.01 never created that folder. On a new machine it opened a file dialog
 * asking for data.csv, cal.txt was never written (every save failed silently),
 * and the style sheet and logs were read and written in whatever folder the
 * program happened to be started from. Dean Jenkins' fork checked for the folder.
 *
 * uTmax6 creates the folder at start-up, puts a copy of the tube library that is
 * built into the program there if there is none (it is never overwritten), and
 * looks for Style.qss in the folder first. Tested in tests/test_app_paths.py.
 */
#ifndef APP_PATHS_H
#define APP_PATHS_H

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>

// ~/uTmax_files/, with the trailing slash.
inline QString utmaxDir()
{
    return QDir::homePath() + "/uTmax_files/";
}

// Create ~/uTmax_files if it is missing. False if it can't be created.
inline bool ensureUtmaxDir()
{
    return QDir().mkpath(utmaxDir());
}

// Copy a file built into the program (a Qt resource, ":/data.csv") into the
// folder under `name`, unless a file of that name is already there. The copy is
// made writable (resource files are read-only). True if the file is there.
inline bool installDefaultFile(const QString &resource, const QString &name)
{
    const QString dest = utmaxDir() + name;
    if (QFile::exists(dest)) return true;
    if (!QFile::copy(resource, dest)) return false;
    return QFile::setPermissions(dest, QFile::ReadOwner | QFile::WriteOwner
                                       | QFile::ReadGroup | QFile::ReadOther);
}

// The application style sheet: ~/uTmax_files/Style.qss, else Style.qss next to
// the program, else Style.qss in the current folder (where 3.01 looked). Empty if
// there is none.
inline QString findStyleSheet()
{
    const QStringList places = {utmaxDir() + "Style.qss",
                                QCoreApplication::applicationDirPath() + "/Style.qss",
                                QStringLiteral("Style.qss")};
    for (const QString &p : places)
        if (QFile::exists(p)) return p;
    return QString();
}

#endif // APP_PATHS_H
