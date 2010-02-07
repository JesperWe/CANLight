#include <QIcon>

#include "ecsManager.h"
#include "numberedItem.h"
#include "numberedItemModel.h"
#include "mainwindow.h"
#include "ecsEvent.h"

NumberedItem::NumberedItem( int newId ) : QGraphicsItem(0) {
	id = newId;
	setFlag( QGraphicsItem::ItemIsSelectable, true );
	longestChildWidth = ecsManager::GroupChildMinimumWidth;
	qDebug() << "Create " << newId;
}

bool NumberedItem::compareIdsAsc( const NumberedItem* a1, const NumberedItem* a2 ) { return a1->id < a2->id; }

bool NumberedItem::compareIdsDesc( const NumberedItem* a1, const NumberedItem* a2 ) { return a1->id > a2->id; }

bool NumberedItem::compareDscrAsc( const NumberedItem* a1, const NumberedItem* a2 ) { return a1->description < a2->description; }

bool NumberedItem::compareDscrDesc( const NumberedItem* a1, const NumberedItem* a2 ) { return a1->description > a2->description; }

QVariant NumberedItem::typeIcon() const {
	switch( itemType ) {
	case NumberedItem::Controller: { return QIcon(":/graphics/finger.svg"); }
	case NumberedItem::Activity: { return QIcon(":/graphics/flash.svg"); }
	case NumberedItem::Appliance: { return QIcon(":/graphics/appliance.svg"); }
	}
	return QVariant();
}

//---------------------------------------------------------------------------

float NumberedItem::calculateHeight()
{
	float myHeight = 0;

	if( itemType == NumberedItem::Appliance ) return myHeight;

	myHeight = 55 + links.count()*16;

	// For controllers, try height based on number of events, use it if it is larger.

	if( itemType == NumberedItem::Controller ) {
		float myHeight2 = 40 + events.count()*60;
		if( myHeight2 > myHeight ) {
			myHeight = myHeight2;
		}
	}

	return myHeight;
}
//------------------------------------------------------------------------------------

QPoint NumberedItem::anchorIn() {
		return QPoint( rect.x(), rect.y() + rect.height()/2 );
}

//------------------------------------------------------------------------------------

QPoint NumberedItem::anchorOut() {
		return QPoint( rect.x() + rect.width(), rect.y() + rect.height()/2 );
}

//------------------------------------------------------------------------------------

void NumberedItem::recalcBoundingRect()
{
	qreal penWidth = qApp->property( "cGroupPen" ).value<QPen>().width();
	float boxWidth = longestChildWidth + 10;

	float rw = boxWidth + penWidth;
	float rh = 10 + penWidth + ( links.count() * ecsManager::ApplianceLineSpacing );
	float rx = -rw / 2;
	float ry = -rh / 2;
	rect.setRect( rx, ry, rw, rh );

	qDebug() << "   boundingRect " << id << ": " << rect.x() << "," << rect.y() << "  " << rect.width() << "," << rect.height();
}

//------------------------------------------------------------------------------------

void NumberedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
					   QWidget *widget)
{
	recalcBoundingRect();

	if( isSelected() ) {
		painter->setPen( Qt::NoPen );
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
		painter->drawRect( rect.x()-5, rect.y()-25, rect.width()+10, rect.height()+30 );
	}

	painter->setBrush( qApp->property( "cGroupBrush" ).value<QBrush>() );
	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
	painter->setFont( qApp->property( "headerFont" ).value<QFont>() );

	painter->drawRoundedRect(rect.x(), rect.y(), rect.width(), rect.height(), 6, 6);
	painter->drawText( rect.x(), rect.y()-6, QString::number(id)   + " - " + description );

	QPixmap* icon = new QPixmap(":/graphics/button.svg");

	float buttonSize = ecsManager::CtrlButtonSize;

	foreach( QGraphicsItem* link, childItems() ) {
		QPointF pos = link->pos();
		pos.setX( pos.x() + link->boundingRect().width() + 10 );

		int func = link->data(1).toInt();

		if( func != ecsEvent::Unknown ) {
			painter->setFont( qApp->property( "buttonFont" ).value<QFont>() );

			switch( func ) {
			case ecsEvent::Key0: {
					painter->drawPixmap( pos.x(), pos.y(), buttonSize, buttonSize, *icon );
					painter->drawText( pos.x(), pos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "1", 0 );
					break; }
			case ecsEvent::Key1: {
					painter->drawPixmap( pos.x(), pos.y(), buttonSize, buttonSize, *icon );
					painter->drawText( pos.x(), pos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "2", 0 );
					break; }
			case ecsEvent::Key2: {
					painter->drawPixmap( pos.x(), pos.y(), buttonSize, buttonSize, *icon );
					painter->drawText( pos.x(), pos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "3", 0 );
					break; }
			case ecsEvent::AnalogSignal: {
					QPixmap* sym = new QPixmap(":/graphics/signal.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonSize, buttonSize, *sym );
					break; }
			case ecsEvent::ChangeNotifiation: {
					QPixmap* sym = new QPixmap(":/graphics/connections.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonSize, buttonSize, *sym );
					break; }
			}
		}
	}

	// Now draw the output line to our events.

	foreach( ecsEvent* event, events ) {
		painter->drawLine( anchorOut().x(), anchorOut().y(), event->anchorIn().x(), event->anchorIn().y() );
	}
}

//------------------------------------------------------------------------------------

void NumberedItem::dragEnterEvent( QGraphicsSceneDragDropEvent *event ) {
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-appliance-id"));
}

//------------------------------------------------------------------------------------

void NumberedItem::dropEvent( QGraphicsSceneDragDropEvent *event ) {
	NumberedItemModel* appsModel;
	NumberedItem* appliance;
	QGraphicsSimpleTextItem* link;
	float y_pos;

	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-appliance-id") ) return;

	QString idString(data->data("x-application/ecs-appliance-id"));
	int applianceId = idString.toInt() ;

	appsModel = ((MainWindow*)(qApp->activeWindow()))->applianceModel;
	appliance = appsModel->findItem( applianceId );

	prepareGeometryChange();

	link = new QGraphicsSimpleTextItem( QString::number( appliance->id ) + " - " + appliance->description );
	link->setData( 0, QVariant::fromValue( (void*) appliance ) );
	link->setData( 1, ecsEvent::Key0 );
	link->setParentItem(this);
	link->setZValue(2);
	link->setFont( qApp->property( "contentFont" ).value<QFont>() );
	link->setFlag(QGraphicsItem::ItemIsSelectable, true);

	links.append( link );

	if( childrenBoundingRect().width() > ecsManager::GroupChildMinimumWidth ) {
		longestChildWidth = childrenBoundingRect().width();
	} else {
		longestChildWidth = ecsManager::GroupChildMinimumWidth;
	}
	longestChildWidth += ecsManager::CtrlFunctionIconWidth;
	recalcBoundingRect();

	y_pos = 0;
	foreach ( QGraphicsItem* child, childItems() ) {
		child->setPos( rect.x() + 6, rect.y() + 4 + y_pos );
		y_pos += ecsManager::ApplianceLineSpacing;
	}
}
