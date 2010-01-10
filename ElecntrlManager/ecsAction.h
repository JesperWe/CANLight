#ifndef ECSACTION_H
#define ECSACTION_H

#include <QGraphicsItem>

class ecsAction : public QGraphicsItem
{
public:
    static const int size = 50;
    static const int polygon[4][2];

    ecsAction();
    ecsAction( int t ) { actionType = t; };
    ecsAction( int itemIndex, int t ) { cGroupIndex = itemIndex; actionType = t; };

    enum { Type = UserType + 3 };
    int type1() const { return Type; }

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPoint anchorIn();

    enum actionType_e {
        None,
        SwitchON,
        SwitchOFF,
        ToggleOnOff,
        FadeStart,
        FadeStop,
        ChangeColor,
        noActionTypes
    };


    int actionType;
    int cGroupIndex;

};

#endif // ECSACTION_H
