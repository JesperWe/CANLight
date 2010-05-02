/*
 * $Revision$
 * $Author$
 * $Date$
 */

#include <QIcon>

#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"
#include "mainwindow.h"
#include "ecsEvent.h"

QPoint ecsControlGroupGraphic::anchorIn() {
		return QPoint( pos().x() + boxSize.x(), pos().y() + boxSize.y() + boxSize.height()/2 );
}

//------------------------------------------------------------------------------------

QPoint ecsControlGroupGraphic::anchorOut() {
		return QPoint( boxSize.x() + boxSize.width(), boxSize.y() + boxSize.height()/2 );
}

//------------------------------------------------------------------------------------

void ecsControlGroupGraphic::recalcBoxSize()
{
	float boxWidth, rw, rh, rx, ry;
	qreal penWidth = qApp->property( "cGroupPen" ).value<QPen>().width();

	longestChildWidth = ecsManager::GroupChildMinimumWidth;
	foreach( QGraphicsItem* link, childItems() ) {
		if( link->type() != QGraphicsSimpleTextItem::Type ) continue;

		if( link->boundingRect().width() > longestChildWidth )
			longestChildWidth = link->boundingRect().width();
	}

	 boxWidth = longestChildWidth + ecsManager::CtrlFunctionIconWidth + 10;


	rw = boxWidth + penWidth;
	rh = 10 + penWidth + ( srcGroup->links.count() * ecsManager::ApplianceLineSpacing );
	rx = -rw / 2;
	ry = -rh / 2;

	// Make sure we center vertically around box + displayText
	ry = ry + (ecsManager::GroupNameFontSize + ecsManager::GroupNameOffset)/2;

	boxSize.setRect( rx, ry, rw, rh );

	rx -= 5;
	ry -= 25;
	rw += 10;
	rh += 30;
	selectBox.setRect( rx, ry, rw, rh );

}

//------------------------------------------------------------------------------------

void ecsControlGroupGraphic::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	QPointF textPos;

	recalcBoxSize();
	updateLinkTexts();
	recalcLinkPositions();

	if( isSelected() ) {
		painter->setPen( Qt::NoPen );
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
		painter->drawRect( selectBox );
	}

	// Surrounding box

	painter->setBrush( qApp->property( "cGroupBrush" ).value<QBrush>() );
	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
	painter->setFont( qApp->property( "headerFont" ).value<QFont>() );

	painter->drawRoundedRect(boxSize.x(), boxSize.y(), boxSize.width(), boxSize.height(), 6, 6);
	painter->drawText(
			boxSize.x(),
			boxSize.y()-ecsManager::GroupNameOffset,
			srcGroup->displayText()
		);

	float buttonSize = ecsManager::CtrlButtonSize;

	// Contained Appliances

	int linkNumber = 0;
	foreach( QGraphicsItem* link, childItems() ) {
		if( link->type() != QGraphicsSimpleTextItem::Type ) continue;
		textPos = link->pos();
		textPos.setX( textPos.x() + link->boundingRect().width() + 10 );

		int func = srcGroup->functions[ linkNumber ];

		if( func != ecsManager::hw_UNKNOWN ) {
			painter->setFont( qApp->property( "buttonFont" ).value<QFont>() );

			painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::HighQualityAntialiasing );
			painter->drawImage( QRectF(textPos.x(), textPos.y(), buttonSize, buttonSize), ecsManagerApp::inst()->eventSourceIcons[ func ] );

			switch( func ) {
			case ecsManager::hw_KEY1: {
					painter->drawText( textPos.x(), textPos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "1", 0 );
					break; }
			case ecsManager::hw_KEY2: {
					painter->drawText( textPos.x(), textPos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "2", 0 );
					break; }
			case ecsManager::hw_KEY3: {
					painter->drawText( textPos.x(), textPos.y(), buttonSize, buttonSize, Qt::AlignHCenter|Qt::AlignVCenter, "3", 0 );
					break; }
			}
		}
		linkNumber++;
	}

	// Now draw the output line to our events.

	foreach( ecsEvent* event, srcGroup->events ) {
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

void ecsControlGroupGraphic::dragEnterEvent( QGraphicsSceneDragDropEvent *event ) {
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-appliance-id"));
}

//------------------------------------------------------------------------------------

void ecsControlGroupGraphic::recalcLinkPositions() {
	float y_pos;

	y_pos = 0;
	foreach ( QGraphicsItem* child, childItems() ) {
		if( child->type() != QGraphicsSimpleTextItem::Type ) continue;
		child->setPos( boxSize.x() + 6, boxSize.y() + 4 + y_pos );
		y_pos += ecsManager::ApplianceLineSpacing;
	}
}

//------------------------------------------------------------------------------------

void ecsControlGroupGraphic::dropEvent( QGraphicsSceneDragDropEvent *event ) {
	ecsControlGroupModel* appsModel;
	ecsControlGroup* appliance;

	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-appliance-id") ) return;

	QString idString(data->data("x-application/ecs-appliance-id"));
	int applianceId = idString.toInt() ;

	appsModel = ((MainWindow*)(qApp->activeWindow()))->applianceModel;
	appliance = appsModel->findItem( applianceId );

	prepareGeometryChange();
	srcGroup->links.append( appliance );
	srcGroup->functions.append( ecsManager::e_UNKNOWN );

	qApp->activeWindow()->setWindowModified( true );
}

//------------------------------------------------------------------------------------

void ecsControlGroupGraphic::updateLinkTexts() {
	QGraphicsSimpleTextItem* linkText;

	int linkNumber = 0;
	foreach( ecsControlGroup* appliance, srcGroup->links ) {
		if( linkTexts.count() > linkNumber ) {
			linkText = linkTexts[ linkNumber ];
		}
		else {
			linkText = new QGraphicsSimpleTextItem( appliance->displayText() );
			linkText->setZValue( 2 );
			linkText->setFont( qApp->property( "contentFont" ).value<QFont>() );
			linkText->setFlag( QGraphicsItem::ItemIsSelectable, true );
			linkText->setParentItem( this );

			linkTexts.append( linkText );
		}

		linkText->setData( 0, QVariant::fromValue( (void*) appliance ) );
		linkText->setText( appliance->displayText() );

		linkNumber++;
	}

	// If a linked appliance was deleted, we need to remove a text item.

	if( linkTexts.count() > srcGroup->links.count() ) {
				QGraphicsSimpleTextItem* unusedItem = linkTexts.last();
				unusedItem->scene()->removeItem( unusedItem );
				linkTexts.removeLast();
				delete unusedItem;
	}
}
