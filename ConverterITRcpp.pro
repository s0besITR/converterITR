QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_LFLAGS_RELEASE += -static -static-libstdc++ -static-libgcc
INCLUDEPATH += $$PWD/include
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    3rd/pugixml.cpp \
    acs_csv/elesy_csv.cpp \
    acs_xml/elesy_xml.cpp \
    acs_xml/iec104slave.cpp \
    acs_xml/mbstcpslave.cpp \
    excel/excel.cpp \
    forms/aboutwindow.cpp \
    forms/dialog_regulchannels.cpp \
    forms/mainwindow.cpp \
    forms/settings_dialog.cpp \
    io_csv/io_csv.cpp \
    main.cpp \
    mk_logic/mk_logic.cpp \
    other/deny_req.cpp \
    other/hmi_actions.cpp

HEADERS += \
    acs_csv/elesy_csv.h \
    acs_xml/elesy_xml.h \
    acs_xml/iec104slave.h \
    acs_xml/mbstcpslave.h \
    builddatetime.h \
    excel/excel.h \
    forms/aboutwindow.h \
    forms/dialog_regulchannels.h \
    forms/mainwindow.h \
    forms/settings_dialog.h \
    io_csv/io_csv.h \
    mk_logic/mk_logic.h \
    other/deny_req.h \
    other/hmi_actions.h


FORMS += \
    forms/aboutwindow.ui \
    forms/dialog_regulchannels.ui \
    forms/mainwindow.ui \
    forms/settings_dialog.ui


LIBS += -L$$PWD/lib -lxlnt


# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:RC_ICONS += resources/icon.ico
TARGET = ITR_Configurator

VERSION = 8.12.2022
QMAKE_TARGET_PRODUCT = "ITR Configurator"
QMAKE_TARGET_COPYRIGHT = "Sobetskiy Alexander"

RESOURCES += \
    resources/resources.qrc

WIN_PWD=$$replace(PWD, /, \\)

# Для генерации времени и даты сборки
buildtimeTarget.target = builddatetime.h
buildtimeTarget.depends = FORCE
buildtimeTarget.commands = copy /b $$WIN_PWD\builddatetime.h+,,$$WIN_PWD\builddatetime.h
PRE_TARGETDEPS += builddatetime.h
QMAKE_EXTRA_TARGETS += buildtimeTarget
