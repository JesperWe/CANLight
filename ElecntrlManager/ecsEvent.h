#ifndef ECSEVENT_H
#define ECSEVENT_H

#include <QtGui>

#define iconDim 30

class ecsEvent : public QGraphicsItem {

public:
    enum { Type = UserType + 2 };
    int type() const { return Type; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPoint anchorIn();

    enum eventTypes_e {
        SingleClick,
        DoubleClick,
        PressHold,
        Release,
        noEventTypes
    };

    ecsEvent();
    ecsEvent( int t ) { eventType = t; };

    int eventType;

private:
    QRectF rect;

};

#endif // ECSEVENT_H
