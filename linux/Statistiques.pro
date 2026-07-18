QT += widgets
CONFIG += c++17
CONFIG -= app_bundle

TARGET = Statistiques
TEMPLATE = app

unix:!macx {
    CONFIG += linux
    QMAKE_CXXFLAGS += -D_GNU_SOURCE
    LIBS += -lm
}

SOURCES += \
    ../Statistiques.cpp \
    ../statsengine.cpp \
    ../chartwidget.cpp

HEADERS += \
    ../statsengine.h \
    ../chartwidget.h

INCLUDEPATH += ../includes
