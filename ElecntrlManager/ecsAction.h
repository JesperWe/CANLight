#ifndef ECSACTION_H
#define ECSACTION_H

#include <QtGui>
#include "ecsControlGroupGraphic.h"
#include "ecsManager.h"
#include "ecsEvent.h"

class ecsAction : public QGraphicsItem
{
public:
	static const int polygon[4][2];

	ecsAction() : QGraphicsItem(0)  {
		setAcceptDrops(true);
		setFlag(QGraphicsItem::ItemIsSelectable, true);
		actionType = ecsAction::None;
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
	QPointF anchorIn();
	QPointF anchorOut();
	void zap();

	QList<ecsControlGroupGraphic*> targetGroups;

};

#endif // ECSACTION_H
