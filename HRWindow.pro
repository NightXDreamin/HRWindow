QT       += core gui network
QT       += core5compat


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    casemanager.cpp \
    dashboardmanager.cpp \
    jobmanager.cpp \
    loginwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    productmanager.cpp

HEADERS += \
    casemanager.h \
    dashboardmanager.h \
    datastructures.h \
    jobmanager.h \
    loginwindow.h \
    mainwindow.h \
    productmanager.h

TRANSLATIONS += \
    HRWindow_zh_CN.ts
CONFIG += lrelease
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

FORMS += \
    casemanager.ui \
    dashboardmanager.ui \
    jobmanager.ui \
    loginwindow.ui \
    mainwindow.ui \
    productmanager.ui
