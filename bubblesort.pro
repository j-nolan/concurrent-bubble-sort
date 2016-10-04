#-------------------------------------------------
#
# Project created by QtCreator 2015-04-24T16:51:08
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = bubblesort
CONFIG   += console
CONFIG   += c++11
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    sorttester.cpp

HEADERS += \
    bubblesort.h \
    isort.h \
    bubblesortthreaded.h \
    sorttester.h \
    bthread.h
