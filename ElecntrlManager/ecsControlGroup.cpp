#include <QIcon>

#include "ecsManager.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupModel.h"
#include "mainwindow.h"
#include "ecsEvent.h"

ecsControlGroup::ecsControlGroup( int newId ) : QGraphicsItem(0) {
	id = newId;
	setFlag( QGraphicsItem::ItemIsSelectable, true );
	longestChildWidth = ecsManager::GroupChildMinimumWidth;
	qDebug() << "Create " << newId;
}

bool ecsControlGroup::compareIdsAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->id < a2->id; }
bool ecsControlGroup::compareIdsDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->id > a2->id; }
bool ecsControlGroup::compareDscrAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->description < a2->description; }
bool ecsControlGroup::compareDscrDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->description > a2->description; }

QVariant ecsControlGroup::typeIcon() const {
	switch( itemType ) {
	case ecsControlGroup::Controller: { return QIcon(":/graphics/finger.svg"); }
	case ecsControlGroup::Activity: { return QIcon(":/graphics/flash.svg"); }
	case ecsControlGroup::Appliance: { return QIcon(":/graphics/appliance.svg"); }
	}
	return QVariant();
}

//------------------------------------------------------------------------------------

QPoint ecsControlGroup::anchorIn() {
		return QPoint( pos().x() + boxSize.x(), pos().y() + boxSize.y() + boxSize.height()/2 );
}

//------------------------------------------------------------------------------------

QPoint ecsControlGroup::anchorOut() {
		return QPoint( boxSize.x() + boxSize.width(), boxSize.y() + boxSize.height()/2 );
}

//------------------------------------------------------------------------------------

void ecsControlGroup::recalcBoxSize()
{
	float boxWidth, rw, rh, rx, ry;
	qreal penWidth = qApp->property( "cGroupPen" ).value<QPen>().width();

	longestChildWidth = ecsManager::GroupChildMinimumWidth;
	foreach( QGraphicsSimpleTextItem* link, links ) {
		if( link->boundingRect().width() > longestChildWidth )
			longestChildWidth = link->boundingRect().width();
	}

	 boxWidth = longestChildWidth + ecsManager::CtrlFunctionIconWidth + 10;


	rw = boxWidth + penWidth;
	rh = 10 + penWidth + ( links.count() * ecsManager::ApplianceLineSpacing );
	rx = -rw / 2;
	ry = -rh / 2;

	// Make shure we center vertically around box + displayText
	ry = ry + (ecsManager::GroupNameFontSize + ecsManager::GroupNameOffset)/2;

	boxSize.setRect( rx, ry, rw, rh );

	rx -= 5;
	ry -= 25;
	rw += 10;
	rh += 30;
	selectBox.setRect( rx, ry, rw, rh );

}

//------------------------------------------------------------------------------------

QString ecsControlGroup::displayText() {
	return QString::number( id ) + " - " + description;
}

//------------------------------------------------------------------------------------

void ecsControlGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
					   QWidget *widget)
{
	recalcBoxSize();

	if( isSelected() ) {
		painter->setPen( Qt::NoPen );
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
		painter->drawRect( selectBox );
	}

	painter->setBrush( qApp->property( "cGroupBrush" ).value<QBrush>() );
	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
	painter->setFont( qApp->property( "headerFont" ).value<QFont>() );

	painter->drawRoundedRect(boxSize.x(), boxSize.y(), boxSize.width(), boxSize.height(), 6, 6);
	painter->drawText(
			boxSize.x(),
			boxSize.y()-ecsManager::GroupNameOffset,
			displayText()
		);

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

	if( ecsManager::GraphicsDebug ) {
		painter->setPen( Qt::NoPen );
		painter->setBrush( QColor( 0, 255, 0, 30 ) );
		painter->drawRect( childrenBoundingRect().united( selectBox ) );
		painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
		painter->setBrush( QColor( 0, 0, 0 ) );
		painter->drawLine( -10, -10, 10, 10 );
		painter->drawLine( -10,  10, 10, -10 );
		painter->setFont( qApp->property( "buttonFont" ).value<QFont>() );
		painter->drawText( 5, 0, "("+QString::number(pos().x())+","+QString::number(pos().y())+")" );
		painter->drawText( 5, -20, "("+QString::number(childrenBoundingRect().united( selectBox ).width())+","+QString::number(childrenBoundingRect().united( selectBox ).height())+")" );
	}
}

//------------------------------------------------------------------------------------

void ecsControlGroup::dragEnterEvent( QGraphicsSceneDragDropEvent *event ) {
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-appliance-id"));
}

//------------------------------------------------------------------------------------

void ecsControlGroup::recalcLinkPositions() {
	float y_pos;

	recalcBoxSize();
	y_pos = 0;
	foreach ( QGraphicsItem* child, links ) {
		child->setPos( boxSize.x() + 6, boxSize.y() + 4 + y_pos );
		y_pos += ecsManager::ApplianceLineSpacing;
	}
}

//------------------------------------------------------------------------------------

QGraphicsSimpleTextItem* ecsControlGroup::appendLinkedAppliance( ecsControlGroup* appliance ) {
	QGraphicsSimpleTextItem* link;

	link = new QGraphicsSimpleTextItem( appliance->displayText() );
	link->setData( 0, QVariant::fromValue( (void*) appliance ) );
	link->setData( 1, ecsEvent::None );
	link->setParentItem(this);
	link->setZValue(2);
	link->setFont( qApp->property( "contentFont" ).value<QFont>() );
	link->setFlag(QGraphicsItem::ItemIsSelectable, true);

	links.append( link );
	recalcLinkPositions();
	return link;
}

//------------------------------------------------------------------------------------

void ecsControlGroup::dropEvent( QGraphicsSceneDragDropEvent *event ) {
	ecsControlGroupModel* appsModel;
	ecsControlGroup* appliance;

	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-appliance-id") ) return;

	QString idString(data->data("x-application/ecs-appliance-id"));
	int applianceId = idString.toInt() ;

	appsModel = ((MainWindow*)(qApp->activeWindow()))->applianceModel;
	appliance = appsModel->findItem( applianceId );

	prepareGeometryChange();
	appendLinkedAppliance( appliance );
}

//------------------------------------------------------------------------------------

void ecsControlGroup::zap() {

}
