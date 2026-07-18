QT += widgets
CONFIG += c++17
CONFIG -= app_bundle

TARGET = Statistiques
TEMPLATE = app

SOURCES += \
    ../Statistiques.cpp \
    ../statsengine.cpp \
    ../chartwidget.cpp

HEADERS += \
    ../statsengine.h \
    ../chartwidget.h

INCLUDEPATH += ../includes
