#ifndef ECSACTION_H
#define ECSACTION_H

#include <QGraphicsItem>

class ecsAction : public QGraphicsItem
{
public:
	static const int size = 50;
	static const int polygon[4][2];

	ecsAction();
	ecsAction( int t ) { actionType = t; setAcceptDrops(true); };
	ecsAction( int itemIndex, int t ) { cGroupSource = itemIndex; actionType = t; setAcceptDrops(true); };

	enum { Type = UserType + 3 };
	int type1() const { return Type; }

	QRectF boundingRect() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	QPoint anchorIn();
	QPoint anchorOut();
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);

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

};

#endif // ECSACTION_H
