
QT       += core gui widgets
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += printsupport
}

TARGET = uTmax
TEMPLATE = app
RC_FILE = icon.rc

#CONFIG+= static
#CONFIG+= console
QT += serialport

CONFIG(debug, debug|release) {
    OBJECTS_DIR = debug_obj
    MOC_DIR = debug_obj
    DESTDIR = debug
}

CONFIG(release, debug|release) {
    OBJECTS_DIR = release_obj
    MOC_DIR = release_obj
    DESTDIR = release

}

# uTmax6: GSL is linked from the system on every platform.
#   Linux / Raspberry Pi OS:  sudo apt install libgsl-dev
#   Windows (MSYS2 UCRT64):   pacman -S mingw-w64-ucrt-x86_64-gsl
# Nick Barton's original lines pointed at copies of GSL on his own PC;
# they are kept in git history (first commit).
LIBS += -lgsl -lgslcblas

RESOURCES +=

SOURCES += main.cpp\
    cal_wizard.cpp \
        mainwindow.cpp \
    cal_dialog.cpp \
    debug_dialog.cpp \
    options_dialog.cpp \
    qcustomplot.cpp \
    datasavedialog.cpp \
    dr_optimize.cpp \
    plotTabWidget.cpp

HEADERS  += mainwindow.h \
    cal_dialog.h \
    cal_wizard.h \
    debug_dialog.h \
    options_dialog.h \
    qcustomplot.h \
    datasavedialog.h \
    typedefs.h \
    dr_optimize.h \
    plotTabWidget.h

FORMS    +=  cal_dialog.ui \
    cal_wizard.ui \
    debug_dialog.ui \
    options_dialog.ui \
    datasavedialog.ui

#unix {
#        FORMS    += mainwindow_l.ui
#}
#macx {
#        FORMS    += mainwindow_osx.ui
#}
#win32 {
        FORMS    += mainwindow.ui
#}

#DISTFILES += \
#    Style.qss
