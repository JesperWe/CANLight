QT += svg \
    xml
TARGET = ElecntrlManager
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    systemdescription.cpp \
    ecmgraphicsview.cpp \
    numberedItem.cpp \
    ecstableview.cpp \
    numberedItemModel.cpp \
    cGroupItem.cpp \
    ecsEvent.cpp
HEADERS += mainwindow.h \
    systemdescription.h \
    ecmgraphicsview.h \
    numberedItem.h \
    ecstableview.h \
    numberedItemModel.h \
    cGroupItem.h \
    ecsEvent.h
FORMS += mainwindow.ui \
    about.ui
RESOURCES += resources.qrc
