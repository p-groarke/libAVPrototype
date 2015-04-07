#-------------------------------------------------
#
# Project created by QtCreator 2015-02-22T18:37:25
#
#-------------------------------------------------

QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

LIBS += -stdlib=libc++
macx:QMAKE_CXXFLAGS += -stdlib=libc++
macx:QMAKE_LFLAGS += -stdlib=libc++

TARGET = lv2_proto
TEMPLATE = app

INCLUDEPATH += /opt/local/include/ \
        /usr/local/include/ \
        /usr/include/

DEPENDPATH += /opt/local/lib/ \
        /usr/local/lib/ \
        /usr/include/

SOURCES += main.cpp\
        mainwindow.cpp \
    audioplayer.cpp

HEADERS  += mainwindow.h \
    audioplayer.h \
    globals.h

FORMS    += mainwindow.ui

win32:CONFIG(release, debug|release) {
# TODO: Build on windows #
} else:win32:CONFIG(debug, debug|release) {
# TODO: Build on windows #
} else:unix {
        LIBS += /usr/local/lib/libavformat.a \
                /usr/local/lib/libavutil.a \
                /usr/local/lib/libavcodec.a \
                /usr/local/lib/libavresample.a \
                -L/usr/local/lib/ -lao \
                -lz -lm -lbz2
}

