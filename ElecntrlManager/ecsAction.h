#ifndef ECSACTION_H
#define ECSACTION_H

#include <QGraphicsItem>

class ecsAction : public QGraphicsItem
{
public:
    enum { Type = UserType + 3 };
    int type1() const { return Type; }

    static const int size = 120;
    static const int polygon[4][2];

    ecsAction();
    ecsAction( int t ) { actionType = t; };

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    QPoint anchorIn();

    enum actionsType {
        SwitchON,
        SwitchOFF,
        ToggleOnOff,
        FadeStart,
        FadeStop,
        ChangeColor,
        noEventTypes
    };


    int actionType;

private:
    QRectF rect;
};

#endif // ECSACTION_H
