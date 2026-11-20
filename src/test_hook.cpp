/* test_hook.cpp — run one sweep from a settings file and report what happened.
 *
 * Part of uTmax6, a fork of uTmax by Nick Barton. GNU GPL v3 or later.
 *
 * For automated tests only. Started with
 *
 *     uTmax --test-sweep sweep.txt
 *
 * it does what a person would do: waits until uTmax has opened the port and
 * is idle, picks the tube, fills in the sweep fields, presses Start once,
 * lets uTmax warm the heater for HtrSecs seconds and run the sweep, writes a
 * JSON report and quits. It changes
 * nothing in how a sweep runs: every command still comes from the normal
 * state machine in mainwindow.cpp.
 *
 * Settings file, one key=value per line ('#' starts a comment):
 *
 *     out=/path/report.json   where to write the report (required)
 *     tube=12AX7/ECC83        tube library entry, as shown in the tube list
 *     VaStart= VaEnd= VaSteps= VsStart= VsEnd= VsSteps= VgStart= VgEnd= VgSteps=
 *     Vh=12.6                 heater voltage
 *     HtrSecs=1               heater warm-up before the sweep, seconds (uTmax: 60)
 *     Delay=0                 Options > Delay, in seconds (non-zero = hold path)
 *     timeout=120             give up after this many seconds
 *     StopAfter=2             press Stop once this many points are stored
 *
 * Keys left out keep whatever uTmax would have shown. The report holds
 * "result" (done, never started, timeout, no port), the last negative-rail and
 * supply readings ("Vn", "Vsu", volts), the application style sheet in use
 * ("appStyleSheet"), every status-bar message
 * and dialog in order (dialogs are recorded and closed, so an error box cannot
 * hang a test), and every measured point from the data store.
 *
 * Used by tests/test_sweeps.py; described in docs/DEVELOPER.md.
 */
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStatusBar>
#include <QTimer>

static const int HOOK_TICK_MS = 100;

void MainWindow::startTestHook(const QString &file)
{
    testHook.cfg.clear();
    QFile f(file);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine());
            int hash = line.indexOf('#');
            if (hash >= 0) line = line.left(hash);
            line = line.trimmed();
            int eq = line.indexOf('=');
            if (eq <= 0) continue;
            testHook.cfg[line.left(eq).trimmed()] = line.mid(eq + 1).trimmed();
        }
    }
    if (!testHook.cfg.contains("out")) {
        qWarning() << "test hook: no 'out=' in" << file;
        QTimer::singleShot(0, qApp, &QCoreApplication::quit);
        return;
    }
    connect(ui->statusBar, &QStatusBar::messageChanged, this, [this](const QString &m) {
        if (!m.isEmpty()) testHookNote("status", m);
    });
    testHook.timer = new QTimer(this);
    connect(testHook.timer, &QTimer::timeout, this, [this]() { testHookTick(); });
    testHook.timer->start(HOOK_TICK_MS);
}

void MainWindow::testHookNote(const QString &kind, const QString &text)
{
    if (!testHook.notes.isEmpty() && testHook.notes.last().first == kind
            && testHook.notes.last().second == text)
        return;                                     // skip repeats of the same message
    testHook.notes.append(qMakePair(kind, text));
}

void MainWindow::testHookTick()
{
    testHook.ticks++;
    int limitTicks = testHook.cfg.value("timeout", "120").toInt() * 1000 / HOOK_TICK_MS;

    // An error box would wait for a click that never comes: record it and close it.
    if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
        testHookNote("dialog", box->windowTitle() + ": " + box->text());
        box->done(QMessageBox::Ok);
    }
    if (testHook.ticks > limitTicks) {
        testHookFinish("timeout");
        return;
    }

    switch (testHook.phase) {
    case 0: {                                       // wait for the port, then idle for 1 s
        bool ready = portInUse->isOpen() && status == Idle && rxLen == 0;
        testHook.idleTicks = ready ? testHook.idleTicks + 1 : 0;
        if (testHook.idleTicks < 1000 / HOOK_TICK_MS) {
            if (testHook.ticks > 30000 / HOOK_TICK_MS && !portInUse->isOpen())
                testHookFinish("no port");
            return;
        }
        const QMap<QString, QString> &c = testHook.cfg;
        if (c.contains("tube")) {
            int i = ui->TubeSelector->findText(c["tube"]);
            if (i < 0) { testHookFinish("tube not found: " + c["tube"]); return; }
            ui->TubeSelector->setCurrentIndex(i);
        }
        const QList<QPair<QString, QLineEdit *>> fields = {
            {"VaStart", ui->VaStart}, {"VaEnd", ui->VaEnd}, {"VaSteps", ui->VaSteps},
            {"VsStart", ui->VsStart}, {"VsEnd", ui->VsEnd}, {"VsSteps", ui->VsSteps},
            {"VgStart", ui->VgStart}, {"VgEnd", ui->VgEnd}, {"VgSteps", ui->VgSteps},
            {"Vh", ui->VhVal}, {"HtrSecs", ui->HtrSecs}};
        for (const auto &fld : fields)
            if (c.contains(fld.first)) fld.second->setText(c[fld.first]);
        if (c.contains("Delay")) options.Delay = c["Delay"].toInt();
        ui->checkQuickTest->setChecked(false);
        ui->checkAutoNumber->setChecked(false);     // no files written by the sweep itself
        testHookNote("hook", "start");
        testHook.phase = 1;
        testHook.idleTicks = 0;
        // Press Start from the event loop, not from inside this tick: Qt does not
        // re-enter a timer whose handler is still running, so an error box opened
        // by Start could not be closed by the next tick.
        QTimer::singleShot(0, this, [this]() { on_Start_clicked(); });
        return;
    }
    case 1:                                         // sweep running: wait for idle again
        if (testHook.cfg.contains("StopAfter") && !testHook.stopPressed
                && dataStore->length() >= testHook.cfg["StopAfter"].toInt()) {
            testHook.stopPressed = true;
            testHookNote("hook", "stop");
            on_Stop_clicked();
        }
        if (status != Idle) testHook.seenBusy = true;
        else if (testHook.seenBusy && rxLen == 0) testHookFinish("done");
        else if (!testHook.seenBusy && ++testHook.idleTicks > 5000 / HOOK_TICK_MS)
            testHookFinish("never started");
        return;
    default:
        return;
    }
}

void MainWindow::testHookFinish(const QString &why)
{
    testHook.phase = 2;
    testHook.timer->stop();

    QJsonArray notes;
    for (const auto &n : testHook.notes)
        notes.append(QJsonObject{{"kind", n.first}, {"text", n.second}});
    QJsonArray points;
    for (const results_t &r : *dataStore)
        points.append(QJsonObject{{"Va", r.Va}, {"Vs", r.Vs}, {"Vg", r.Vg}, {"Vf", r.Vf},
                                  {"Ia_mA", r.Ia}, {"Is_mA", r.Is}, {"ignore", int(r.ignore)}});
    QJsonObject report{{"result", why}, {"uTracer6", options.uTracer6},
                       {"VaMax", calData.VaMax},
                       {"Vn", adc_real.Vn}, {"Vsu", adc_real.Vsu},
                       {"appStyleSheet", qApp->styleSheet()}, {"notes", notes}, {"points", points}};
    QFile out(testHook.cfg.value("out"));
    if (out.open(QIODevice::WriteOnly))
        out.write(QJsonDocument(report).toJson());
    else
        qWarning() << "test hook: cannot write" << out.fileName();
    QTimer::singleShot(500, qApp, &QCoreApplication::quit);
}
