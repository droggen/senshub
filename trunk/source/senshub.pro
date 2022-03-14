# -------------------------------------------------
# Project created by QtCreator 2009-11-02T21:46:10
# -------------------------------------------------
QT       += core gui
QT       += serialport network widgets xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SensHub
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    cio.cpp \
    Reader/BaseReader.cpp \
    serialio/serialio.cpp \
    Reader/SimSensReader.cpp \
    Reader/FBReader.cpp \
    Reader/FrameParser3.cpp \
    Reader/Base.cpp \
    precisetimer.cpp \
    TableDisplayWidget.cpp \
    helper.cpp \
    Reader/RCBuffer.cpp \
    Reader/Writer.cpp \
    Reader/Merger.cpp \
    deviceform.cpp \
    Reader/LabelReader.cpp \
    offline.cpp \
    Reader/TSBuffer.cpp \
    helpdialog.cpp \
    Reader/Hub.cpp
HEADERS += mainwindow.h \
    cio.h \
    Reader/BaseReader.h \
    serialio/serialio.h \
    Reader/SimSensReader.h \
    Reader/FBReader.h \
    Reader/FrameParser3.h \
    Reader/Base.h \
    precisetimer.h \
    TableDisplayWidget.h \
    helper.h \
    Reader/RCBuffer.h \
    Reader/Writer.h \
    Reader/Merger.h \
    deviceform.h \
    Reader/LabelReader.h \
    offline.h \
    Reader/TSBuffer.h \
    helpdialog.h \
    Reader/Hub.h
FORMS += mainwindow.ui \
    deviceform.ui \
    helpdialog.ui
DEFINES += DEVELMODE
win32:DEFINES += WIN32
RESOURCES += resources.qrc
OTHER_FILES += \ 
    senshub.rc

# icon
win32: RC_FILE = senshub.rc
