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
	ecsControlGroupGraphic.cpp
HEADERS += ecsGraphicsView.h \
	ecsTableView.h \
	mainwindow.h \
	systemdescription.h \
	ecsControlGroup.h \
	ecsControlGroupModel.h \
	ecsEvent.h \
	ecsAction.h \
	ecsManager.h \
	ecsControlGroupGraphic.h
FORMS += mainwindow.ui \
	about.ui
RESOURCES += resources.qrc
