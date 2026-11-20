/* cal_file.h — write a settings file only when its text has changed.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * uTmax 3.01 rewrote cal.txt whenever the serial port opened (every start and
 * every reconnect attempt that succeeded), on every exit, and after plot and
 * pen updates, whether anything had changed or not. On a Raspberry Pi that is a
 * steady stream of needless writes to the SD card, and a power cut in the middle
 * of one could leave cal.txt half written, losing the calibration. Dean Jenkins'
 * fork stopped the start-up rewrite (his commit 391cb4f).
 *
 * writeFileIfChanged() compares the new text with the file on disk and does
 * nothing if they are the same. When they differ it writes through QSaveFile:
 * the text goes to a temporary file in the same folder, which is then renamed
 * over the old one, so the file on disk is always either the old version or the
 * new one, never a mixture.
 *
 * Tested end to end in tests/test_cal_file.py.
 */
#ifndef CAL_FILE_H
#define CAL_FILE_H

#include <QFile>
#include <QSaveFile>
#include <QString>

// True if the file now holds `text` (already did, or was written). False if it
// could not be written; the old file is then left as it was.
inline bool writeFileIfChanged(const QString &path, const QString &text)
{
    const QByteArray bytes = text.toUtf8();
    QFile old(path);
    if (old.open(QIODevice::ReadOnly)) {
        bool same = old.readAll() == bytes;
        old.close();
        if (same) return true;
    }
    QSaveFile out(path);
    if (!out.open(QIODevice::WriteOnly)) return false;
    out.write(bytes);
    return out.commit();
}

#endif // CAL_FILE_H
