QT -= gui
QT       += network

TEMPLATE = lib
CONFIG += staticlib

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    verify.pb.cc \
    networkCodec.cpp

HEADERS += \
    verify.pb.h \
    verify.proto \
    comDefinition.h \
    networkCodec.h

INCLUDEPATH += ./3rd/include
#LIBS += -L./3rd/lib - lprotobuf


DESTDIR = ../Common/output

# Default rules for deployment.
unix {
    target.path = $$[QT_INSTALL_PLUGINS]/generic
}
!isEmpty(target.path): INSTALLS += target

