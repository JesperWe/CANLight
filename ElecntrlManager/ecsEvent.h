#ifndef ECSEVENT_H
#define ECSEVENT_H

#include <QtGui>

#define iconDim 30

class ecsEvent : public QGraphicsItem {

public:

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPoint anchorIn();

    enum eventType {
        SingleClick,
        DoubleClick,
        PressHold,
        Release,
        noEventTypes
    };

    ecsEvent();
    ecsEvent( int t ) { type = t; };

    int type;

private:
    QRectF rect;

};

#endif // ECSEVENT_H
