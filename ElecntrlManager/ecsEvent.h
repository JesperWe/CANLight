#ifndef ECSEVENT_H
#define ECSEVENT_H

#include <QtGui>
#include "cGroupItem.h"

#define iconDim 30

class ecsEvent : public QGraphicsItem {

public:
    enum { Type = UserType + 2 };
    int type() const { return Type; };

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPoint anchorIn();
    QPoint anchorOut();

    enum eventTypes_e {
        SingleClick,
        DoubleClick,
        PressHold,
        Release,
        noEventTypes
    };

    enum eventSources_e {
        Unknown,
        Key0,
        Key1,
        Key2,
        AnalogSignal,
        ChangeNotifiation,
        noEventSources
    };

    ecsEvent();
    ecsEvent( int t ) { eventType = t; };
    ecsEvent( int itemIndex, int t ) { cGroupIndex = itemIndex; eventType = t; };

    int eventType;
    int eventAction;
    int cGroupIndex;
    int eventIndex;

};

#endif // ECSEVENT_H
