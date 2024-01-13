QT += quick
QT       += network

SOURCES += \
        ClientManager.cpp \
        main.cpp \
        ../Common/verify.pb.cc

resources.files = main.qml 
resources.prefix = /$${TARGET}
RESOURCES += resources

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ClientManager.h \
    ../Common/verify.pb.h
    
INCLUDEPATH += ../Common/3rd/include
LIBS += -L../Common/3rd/lib -lprotobuf \
        -L../Common/output/ -lCommon

