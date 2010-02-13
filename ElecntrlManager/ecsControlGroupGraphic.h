#ifndef ecsControlGroupProxy_H
#define ecsControlGroupProxy_H

class ecsControlGroupModel;
class ecsControlGroupGraphic;
class ecsControlGroup;
class ecsAction;
class ecsEvent;

#include <QtGui>
#include "ecsManager.h"
#include "ecsControlGroup.h"

class ecsControlGroupGraphic :  public QGraphicsItem
{

public:

	enum { Type = UserType + ecsManager::ControlGroupProxy };
	int type() const { return Type; };

	//--------------------------------------------------------------------------------------
	// Constructors

	ecsControlGroupGraphic() : QGraphicsItem(0) {
		longestChildWidth = ecsManager::GroupChildMinimumWidth;
		offset = 0;
		setVisible( true );
		setAcceptDrops(true);
		setFlag( QGraphicsItem::ItemIsSelectable );
	}

	ecsControlGroupGraphic( ecsControlGroup* mySrcGroup ) : QGraphicsItem(0) {
		srcGroup = mySrcGroup;
		longestChildWidth = ecsManager::GroupChildMinimumWidth;
		offset = 0;
		setVisible( true );
		setAcceptDrops(true);
		setFlag( QGraphicsItem::ItemIsSelectable );
	}

	//--------------------------------------------------------------------------------------

	void recalcBoxSize();
	QRectF boundingRect() const { return selectBox; };

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QPoint anchorIn();
	QPoint anchorOut();
	void recalcLinkPositions();
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void updateLinkTexts();

	//--------------------------------------------------------------------------------------

	float          offset;
	float          longestChildWidth;

	QRectF boxSize, selectBox;
	QHash<int, QGraphicsSimpleTextItem*> linkTexts;

	ecsControlGroup* srcGroup;
};

#endif // ecsControlGroupProxy_H
