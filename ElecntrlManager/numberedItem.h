#ifndef NUMBEREDITEM_H
#define NUMBEREDITEM_H

#include <QtGui>
#include "ecsManager.h"

class NumberedItemModel;
class ecsAction;
class ecsEvent;

class NumberedItem :  public QObject, public QGraphicsItem
{
	Q_OBJECT

public:

	enum { Type = UserType + 1 };
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

	NumberedItem() : QGraphicsItem(0) {
		id = 0;
		description = "N/A";
		itemType = NumberedItem::Controller;
		longestChildWidth = ecsManager::GroupChildMinimumWidth;
		offset = 0;
	}

	NumberedItem( int newId );

	//--------------------------------------------------------------------------------------

	static bool compareIdsAsc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareIdsDesc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareDscrAsc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareDscrDesc( const NumberedItem* a1, const NumberedItem* a2 );

	void recalcBoxSize();
	QRectF boundingRect() const { return selectBox; };
	QString displayText();
	QGraphicsSimpleTextItem* appendLinkedAppliance( NumberedItem* appliance );

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	QPoint anchorIn();
	QPoint anchorOut();

	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);

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

signals:
	void modified();

};

#endif // NUMBEREDITEM_H
