QT += svg \
    xml
TARGET = ElecntrlManager
TEMPLATE = app
SOURCES += ecsGraphicsView.cpp \
    ecsTableView.cpp \
    main.cpp \
    mainwindow.cpp \
    systemdescription.cpp \
    numberedItem.cpp \
    numberedItemModel.cpp \
    ecsEvent.cpp \
    ecsAction.cpp
HEADERS += ecsGraphicsView.h \
    ecsTableView.h \
    mainwindow.h \
    systemdescription.h \
    numberedItem.h \
    numberedItemModel.h \
    ecsEvent.h \
    ecsAction.h \
    ecsManager.h
FORMS += mainwindow.ui \
    about.ui
RESOURCES += resources.qrc
