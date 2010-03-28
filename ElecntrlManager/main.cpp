#include "elecntrlmanager.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ElecntrlManager w;
    w.show();
    return a.exec();
}
