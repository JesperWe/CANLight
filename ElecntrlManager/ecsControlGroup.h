#ifndef ecsControlGroup_H
#define ecsControlGroup_H

#include <QtGui>
#include "ecsManager.h"

class ecsControlGroupModel;
class ecsAction;
class ecsEvent;

class ecsControlGroup :  public QGraphicsItem
{

public:

	enum { Type = UserType + ecsManager::ControlGroup };
	int type() const { return Type; };

	enum itemTypes_e {
		Unknown,
		Controller,
		Activity,
		Appliance,
		noItemTypes
	};

	//--------------------------------------------------------------------------------------
	// Constructors

	ecsControlGroup() : QGraphicsItem(0) {
		id = 0;
		description = "N/A";
		itemType = ecsControlGroup::Controller;
		longestChildWidth = ecsManager::GroupChildMinimumWidth;
		offset = 0;
	}

	ecsControlGroup( int newId );

	//--------------------------------------------------------------------------------------

	static bool compareIdsAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareIdsDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareDscrAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareDscrDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 );

	void recalcBoxSize();
	QRectF boundingRect() const { return selectBox; };
	QString displayText();
	QGraphicsSimpleTextItem* appendLinkedAppliance( ecsControlGroup* appliance );

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QPoint anchorIn();
	QPoint anchorOut();
	void recalcLinkPositions();
	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);
	void zap();
	QVariant typeIcon() const;

	//--------------------------------------------------------------------------------------

	int             id;
	QString    description;
	int             itemType;
	float          offset;
	float          longestChildWidth;

	QRectF boxSize, selectBox;
	QList<QGraphicsSimpleTextItem*> links; // data[0] is pointer to appliance item. data[1] is ctrlFunction.
	QList<ecsEvent*> events;

};

#endif // ecsControlGroup_H
