#ifndef ECSEVENT_H
#define ECSEVENT_H

#include <QtGui>
#include "ecsAction.h"
#include "ecsControlGroupGraphic.h"

class ecsEvent : public QGraphicsItem {

public:
	ecsEvent() : QGraphicsItem(0) {
		eventAction = NULL;
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		qDebug() << "Create Event (default)";
	};

	ecsEvent( int t ) : QGraphicsItem(0) {
		eventAction = NULL;
		eventType = t;
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		qDebug() << "Create Event type=" << t;
	};

	ecsEvent( int itemId, int t ) : QGraphicsItem(0) {
		eventAction = NULL;
		eventType = t;
		cGroupId = itemId;
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		qDebug() << "Create Event id=" << itemId << " type=" << t;
	};

	enum { Type = UserType + ecsManager::Event };
	int type() const { return Type; };

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPointF anchorIn();
	QPointF anchorOut();
	void zap();

	int eventType;
	ecsAction* eventAction;
	int cGroupId;

};

#endif // ECSEVENT_H
