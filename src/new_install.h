/* new_install.h — first-start settings, and themes that work from any folder.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * The cal.txt that uTmax 3.01 created on a new machine said:
 *
 *   COM=COM1                     a Windows port name; on Linux nothing opened
 *   Style=<program folder>/style.qss
 *                                a file that doesn't exist (the themes are in a
 *                                Themes folder, and the name is wrong)
 *
 * and its themes find their images with paths such as
 * url(./Themes/img/dark/check.png), relative to the folder uTmax was started
 * from, so they only looked right when it was started from that folder.
 *
 * uTmax6 picks the first USB serial port it can see, installs the themes that
 * are built into the program (utmax6.qrc) in ~/uTmax_files/Themes, starts on
 * "Utracer Dark", and resolves a theme's ./ paths against the folder that holds
 * its Themes folder. VaMax stays 300 V on a new install, on purpose: the user
 * raises it, for a uTracer6, once the hardware is checked.
 *
 * Tested in tests/test_new_install.py.
 */
#ifndef NEW_INSTALL_H
#define NEW_INSTALL_H

#include "app_paths.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QSerialPortInfo>
#include <QStringList>

constexpr const char *DEFAULT_THEME = "Utracer Dark.qss";

// The serial port to put in a new cal.txt: the first USB serial adapter seen
// (/dev/ttyUSB*, /dev/ttyACM*, a Mac's cu.usbserial*/cu.usbmodem*, any COM
// port on Windows), else the usual first adapter for the system.
inline QString defaultSerialPort()
{
    for (const QSerialPortInfo &p : QSerialPortInfo::availablePorts()) {
#ifdef Q_OS_WIN
        return p.portName();
#else
        const QString n = p.portName();
        if (n.startsWith("ttyUSB") || n.startsWith("ttyACM")
                || n.startsWith("cu.usbserial") || n.startsWith("cu.usbmodem"))
            return p.systemLocation();
#endif
    }
#if defined(Q_OS_WIN)
    return QStringLiteral("COM1");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("/dev/cu.usbserial");
#else
    return QStringLiteral("/dev/ttyUSB0");
#endif
}

// Copy every file under a resource folder (":/Themes") into ~/uTmax_files/<dir>,
// keeping the layout, without replacing a file that is already there.
inline bool installDefaultTree(const QString &resourceDir, const QString &dir)
{
    bool ok = true;
    QDirIterator it(resourceDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString res = it.next();
        const QString rel = dir + res.mid(resourceDir.length());   // "Themes/img/dark/x.png"
        ok = QDir().mkpath(QFileInfo(utmaxDir() + rel).absolutePath()) && ok;
        ok = installDefaultFile(res, rel) && ok;
    }
    return ok;
}

// The theme a new cal.txt starts with.
inline QString defaultThemePath()
{
    return utmaxDir() + "Themes/" + DEFAULT_THEME;
}

// A theme refers to its images as url(./Themes/img/...), meaning "next to the
// Themes folder". If the theme file is itself in a folder called Themes, make
// those paths absolute; otherwise leave the sheet as it is (3.01's behaviour).
inline QString resolveThemeUrls(const QString &sheet, const QString &themePath)
{
    QDir dir = QFileInfo(themePath).absoluteDir();
    if (dir.dirName() != "Themes" || !dir.cdUp()) return sheet;
    QString out = sheet;
    out.replace("url(./", "url(" + dir.absolutePath() + "/");
    return out;
}

#endif // NEW_INSTALL_H
