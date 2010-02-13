#ifndef ECSACTION_H
#define ECSACTION_H

#include <QtGui>

#include "ecsManager.h"
#include "ecsControlGroupGraphic.h"
#include "ecsEvent.h"

class ecsAction : public QGraphicsItem
{
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
	QPointF dragToPos;

public:
	static const int polygon[4][2];

	ecsAction() : QGraphicsItem(0)  {
		setAcceptDrops(true);
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		actionType = ecsManager::NoAction;
	};
	ecsAction( int t ) : QGraphicsItem(0) {
		setAcceptDrops(true);
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		actionType = t;
	};

	enum { Type = UserType + ecsManager::Action };
	int type() const { return Type; };

	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	int actionType;
	int cGroupSource;
	int eventIndex;

	QRectF boundingRect() const;
	QPointF anchorIn();
	QPointF anchorOut();
	void zap();

	QList<ecsControlGroupGraphic*> targetGroups;

};

#endif // ECSACTION_H
