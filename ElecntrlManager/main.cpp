#include <QtGui/QApplication>
#include <QDesktopWidget>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	MainWindow w;

	//QDesktopWidget *desktop = QApplication::desktop();

	//int screenWidth = desktop->width();
	//int screenHeight = desktop->height();

	w.show();

	return a.exec();
}
