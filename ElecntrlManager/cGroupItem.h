#ifndef CGROUPITEM_H
#define CGROUPITEM_H

#include <QtGui>
#include "numberedItemModel.h"

#define lineSpacing 16

class cGroupItem : public QGraphicsItem {

public:
    enum { Type = UserType + 1 };
    int type() const { return Type; };

    cGroupItem( NumberedItemModel* m, int i );

    QRectF boundingRect() const { return rect; };
    void addApplianceTexts();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
    void dropEvent(QGraphicsSceneDragDropEvent *event);

    QPoint anchorOut();

    float maxChildWidth;
    int itemIndex;
    QRectF rect;

private:
    void recalcBoundingRect();
    NumberedItemModel* cGroupModel;

};

#endif // CGROUPITEM_H
