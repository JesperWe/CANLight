#ifndef ECSEVENT_H
#define ECSEVENT_H

#include <QtGui>
#include "ecsAction.h"
#include "numberedItem.h"

class ecsEvent : public QGraphicsItem {

public:
	ecsEvent() {};
	ecsEvent( int t ) { eventType = t; };
	ecsEvent( int itemId, int t ) {
		cGroupId = itemId;
		eventType = t;
		this->setFlag(QGraphicsItem::ItemIsSelectable, true);
	};

	enum { Type = UserType + 2 };
	int type() const { return Type; };

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPoint anchorIn();
	QPoint anchorOut();

	enum eventTypes_e {
		None,
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

	int eventType;
	ecsAction* eventAction;
	int cGroupId;

};

#endif // ECSEVENT_H
