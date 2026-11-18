
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

# macx unix win32
win32:INCLUDEPATH += "C:/Users/nick/Downloads/mingw-w64-x86_64-gsl-2.7.1-1-any.pkg/mingw64/include"

#LIBS+="C:/Users/nick/Downloads/mingw-w64-x86_64-gsl-2.7.1-1-any.pkg/mingw64/lib/libgsl.dll.a"
#LIBS+="C:/Users/nick/Downloads/mingw-w64-x86_64-gsl-2.7.1-1-any.pkg/mingw64/lib/libgslcblas.dll.a"

win32:LIBS+= release/libgsl-27.dll
win32:LIBS+= release/libgslcblas-0.dll

#win32:LIBS+= -Lrelease -llibgsl-27.dll
#win32:LIBS+= -Lrelease -llibgslcblas-0.dll

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
