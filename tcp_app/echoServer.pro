QT -= gui
QT += network

CONFIG += c++17 console
CONFIG -= app_bundle

TEMPLATE = app
TARGET = server_app

SOURCES += \
    main.cpp \
    mytcpserver.cpp

HEADERS += \
    mytcpserver.h