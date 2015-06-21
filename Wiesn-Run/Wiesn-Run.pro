TEMPLATE = app
CONFIG += console
CONFIG += c++11
# Felix: QT += gui declarative  # geht bei mir nicht (Rupi)

QT += core gui
QT += widgets

#QT += testlib

SOURCES += src/main.cpp \
    src/game.cpp \
    src/gameobject.cpp \
    src/movingobject.cpp \
    src/shoot.cpp \
    src/player.cpp \
    src/enemy.cpp \
    src/input.cpp \
    src/audio.cpp \
    src/audiocontrol.cpp \
    src/renderbackground.cpp \
    src/renderplayer.cpp \
    src/renderobstacle.cpp \
    src/renderguielement.cpp \
    src/renderenemy.cpp \
    src/renderattack.cpp \
    src/menu.cpp \
    src/powerup.cpp

HEADERS += \
    src/game.h \
    src/gameobject.h \
    src/movingobject.h \
    src/shoot.h \
    src/player.h \
    src/enemy.h \
    src/input.h \
    src/audio.h \
    src/audiocontrol.h \
    src/renderbackground.h \
    src/renderplayer.h \
    src/renderobstacle.h \
    src/renderguielement.h \
    src/renderenemy.h \
    src/renderattack.h \
    src/definitions.h \
    src/menu.h \
    src/powerup.h

RESOURCES += \
    src/ressources.qrc
