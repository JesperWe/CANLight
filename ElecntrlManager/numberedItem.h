#ifndef NUMBEREDITEM_H
#define NUMBEREDITEM_H

#include <QtGui>

class NumberedItemModel;
class ecsAction;
class ecsEvent;

class NumberedItem : public QGraphicsItem
{

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
	}

	NumberedItem( NumberedItemModel* m, int i );

	//--------------------------------------------------------------------------------------

	static bool compareIdsAsc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareIdsDesc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareDscrAsc( const NumberedItem* a1, const NumberedItem* a2 );
	static bool compareDscrDesc( const NumberedItem* a1, const NumberedItem* a2 );

	float calculateHeight();
	void recalcBoundingRect();
	QRectF boundingRect() const { return rect; };
	void addApplianceTexts();

	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

	void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	void dropEvent(QGraphicsSceneDragDropEvent *event);

	QPoint anchorOut();
	QPoint anchorIn();

	QVariant typeIcon() const;

	//--------------------------------------------------------------------------------------

	int             id;
	QString    description;
	int             itemType;
	float          offset;
	float          longestChildWidth;

	QRectF rect;

	QList<NumberedItem*> links;
	QList<int> ctrlFunctions;

	QList<ecsEvent*> events;
};

#endif // NUMBEREDITEM_H
