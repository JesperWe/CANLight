#ifndef ECSACTION_H
#define ECSACTION_H

#include <QtGui>
#include "numberedItem.h"
#include "ecsEvent.h"

class ecsAction : public QGraphicsItem
{
public:
	static const int size = 50;
	static const int polygon[4][2];

	ecsAction() : QGraphicsItem(0)  {
		setAcceptDrops(true);
		actionType = ecsAction::None;
	};
	ecsAction( int t ) : QGraphicsItem(0) {
		setAcceptDrops(true);
		actionType = t;
	};

	enum { Type = UserType + 3 };
	int type() const { return Type; };

	void drawOutputTo( QPoint to, QGraphicsScene* scene );
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

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
	int cGroupSource;
	int eventIndex;

	QRectF boundingRect() const;
	QPoint anchorIn();
	QPoint anchorOut();

	QList<NumberedItem*> targetGroups;

};

#endif // ECSACTION_H
