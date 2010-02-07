#include <QIcon>

#include "ecsManager.h"
#include "numberedItem.h"
#include "numberedItemModel.h"
#include "mainwindow.h"
#include "ecsEvent.h"

NumberedItem::NumberedItem( NumberedItemModel* m, int i ) {
	id = i;
	setAcceptDrops(true);
	setFlag( QGraphicsItem::ItemIsSelectable, true );
	longestChildWidth = ecsManager::GroupChildMinimumWidth;
	qDebug() << "Create " << i;
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
		return QPoint(
								rect.x()  + this->pos().x(),
								rect.y() + rect.height()/2 + this->pos().y()
				   );
}

//------------------------------------------------------------------------------------

QPoint NumberedItem::anchorOut() {
		return QPoint(
								rect.x() + rect.width() + this->pos().x(),
								rect.y() + rect.height()/2 + this->pos().y()
				   );
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

void NumberedItem::addApplianceTexts() {
	QGraphicsSimpleTextItem* txtItem;
	int noChildren, noAppliances;
	QString app;
	float y_pos;

	noChildren = childItems().count();
	noAppliances = 0;

	foreach( NumberedItem* appliance, links ) {
		noAppliances++;
		app = QString::number( appliance->id ) + " - " + appliance->description;

		if( noChildren > noAppliances ) {
			qDebug() << "   Reusing appTextItem " + QString::number(noAppliances);
			txtItem = (QGraphicsSimpleTextItem*)(childItems()[noAppliances-1]);
		}
		else {
			qDebug() << "   Creating appTextItem " + QString::number(noAppliances);
			txtItem = new QGraphicsSimpleTextItem();
			txtItem->setParentItem(this);
			txtItem->setZValue(2);
			txtItem->setFont( qApp->property( "contentFont" ).value<QFont>() );
			txtItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
		}

		txtItem->setText(app);
		txtItem->setData( 0, QVariant::fromValue( (void *) appliance ) );   // Store a pointer to the source appliance object.
		txtItem->setData( 1, noAppliances-1 );   // And our index.
	}

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

//------------------------------------------------------------------------------------

void NumberedItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
					   QWidget *widget)
{
	qDebug() << "NumberedItem Paint " << QString::number(id) << " : " << this->isSelected();

	addApplianceTexts();

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

	float buttonDim = 17;

	for( int i=0; i<childItems().count(); i++ ) {
		QPointF pos = ((QGraphicsSimpleTextItem*)(childItems()[i]))->pos();
		pos.setX( pos.x() + ((QGraphicsSimpleTextItem*)(childItems()[i]))->boundingRect().width() + 10 );

		if( ctrlFunctions.count() <= i ) continue;

		int func = ctrlFunctions[i];
		if( func != ecsEvent::Unknown ) {
			painter->setFont( qApp->property( "buttonFont" ).value<QFont>() );

			switch( func ) {
			case ecsEvent::Key0: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "1", 0 );
					break; }
			case ecsEvent::Key1: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "2", 0 );
					break; }
			case ecsEvent::Key2: {
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *icon );
					painter->drawText( pos.x(), pos.y(), buttonDim, buttonDim, Qt::AlignHCenter|Qt::AlignVCenter, "3", 0 );
					break; }
			case ecsEvent::AnalogSignal: {
					QPixmap* sym = new QPixmap(":/graphics/signal.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *sym );
					break; }
			case ecsEvent::ChangeNotifiation: {
					QPixmap* sym = new QPixmap(":/graphics/connections.svg");
					painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );
					painter->drawPixmap( pos.x(), pos.y(), buttonDim, buttonDim, *sym );
					break; }
			}
		}
	}
}

//------------------------------------------------------------------------------------

void NumberedItem::dragEnterEvent( QGraphicsSceneDragDropEvent *event ) {
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-appliance-id"));
}

//------------------------------------------------------------------------------------

void NumberedItem::dropEvent( QGraphicsSceneDragDropEvent *event ) {
	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-appliance-id") ) return;

	QString idString(data->data("x-application/ecs-appliance-id"));
	int applianceId = idString.toInt() ;
	NumberedItemModel* apps = ((MainWindow*)(qApp->activeWindow()))->applianceModel;
	NumberedItem* app = apps->findItem( applianceId );

	this->prepareGeometryChange();
	this->links.append( app );
	this->ctrlFunctions.append( ecsEvent::Key0 );
	recalcBoundingRect();
	((MainWindow*)qApp->activeWindow())->updateScene();
}
