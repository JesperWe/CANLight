QT += svg \
	xml
TARGET = ElecntrlManager
TEMPLATE = app
SOURCES += ecsGraphicsView.cpp \
	ecsTableView.cpp \
	main.cpp \
	mainwindow.cpp \
	systemdescription.cpp \
	ecsControlGroup.cpp \
	ecsControlGroupModel.cpp \
	ecsEvent.cpp \
	ecsAction.cpp \
	ecsControlGroupGraphic.cpp \
	ecsManagerApp.cpp \
	ecsCANUSB.cpp
HEADERS += ecsGraphicsView.h \
	ecsTableView.h \
	mainwindow.h \
	systemdescription.h \
	ecsControlGroup.h \
	ecsControlGroupModel.h \
	ecsEvent.h \
	ecsAction.h \
	ecsManager.h \
	ecsControlGroupGraphic.h \
	ecsManagerApp.h \
	CANUSB/include/lawicel_can.h \
	CANUSB/include/Ftd2xx.h \
	CANUSB/include/lawicel_can.h \
	ecsCANUSB.h
FORMS += mainwindow.ui \
	about.ui
RESOURCES += resources.qrc
INCLUDEPATH += CANUSB/include
win32:LIBS += CANUSB/libs/canusbdrv.lib
