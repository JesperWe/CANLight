#include <QtGui>

#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "mainwindow.h"
#include "ecsControlGroupModel.h"
#include "ecsControlGroup.h"
#include "ecsAction.h"

const int ecsAction::polygon[4][2] = {
	{ 0, 34/2 },
	{ 34/2, 0 },
	{ 0, -34/2 },
	{ -34/2, 0 }
};

//------------------------------------------------------------------------------------

QRectF ecsAction::boundingRect() const {
	return QRectF( -34/2,-34/2, 34, 34 );
}

//------------------------------------------------------------------------------------

void ecsAction::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	scene()->views()[0]->setCursor( Qt::ArrowCursor );

	dragToPos  = anchorOut();
	update();
	QGraphicsItem::mousePressEvent(event);
}

void ecsAction::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	dragToPos = event->lastPos();
	update();
}

void ecsAction::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	// Did we drag to any target?
	QGraphicsItem* targetItem;

	targetItem = scene()->itemAt( mapToScene( dragToPos ) );

	if( targetItem ) {
		ecsControlGroupGraphic* target = qgraphicsitem_cast<ecsControlGroupGraphic *>( targetItem );

		// Try parent if we hit a text.
		if( ! target )
			target = qgraphicsitem_cast<ecsControlGroupGraphic *>( targetItem->parentItem() );

		if( target && ! targetGroups.contains(target) )
			targetGroups.append( target );
	}

	if( ! boundingRect().contains(dragToPos) )
		setSelected(false );

	update();
}

//------------------------------------------------------------------------------------

void ecsAction::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	bool  placeBelow, first;
	float yPos;
	QRectF targetRect;

	painter->setBrush( QColor( 255, 125, 135, 255 ) );
	if( isSelected() ) {
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
	}

	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );

	painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

	painter->drawPolygon( QPolygon( 4, &ecsAction::polygon[0][0] ) );
	painter->drawPixmap( -34*0.25,-34*0.25,34*0.5,34*0.5, ecsManagerApp::inst()->actionIcons[actionType] );

	// Now draw a rubberband if the user is dragging.

	if( scene()->mouseGrabberItem() == this ) {
		QLineF line( anchorOut(), dragToPos );
		if ( line.length() > 1.0 ) painter->drawLine( line );
	}

	// Target groups

	placeBelow = false;
	first = true;
	targetRect.setCoords( 0, 0, 0, 0 );
	foreach( ecsControlGroupGraphic* target, targetGroups ) {

		// Become owner of orphaned targets.
		if( ! target->parentItem() )
			target->setParentItem( this );

		// Only paint target grops that are our children.
		if( target->parentItem() == this ) {
			if( ! target->scene() ) scene()->addItem( target );

			if( first ) {
				yPos = 0;
				first = false;
			}
			else {
				if( placeBelow ) {
					yPos = targetRect.bottom() + target->boundingRect().height()/2;
				}
				else {
					yPos = targetRect.top() - target->boundingRect().height()/2;
				}
			}
			target->setParentItem( this );
			target->setPos( ecsManager::ActionOffset_X + target->boundingRect().width()/2, yPos );
			targetRect = targetRect.united( target->boundingRect() );
			placeBelow = ! placeBelow;
		}

		// Connection lines may go to groups that are children of other actions.

		painter->drawLine( anchorOut(), mapFromItem( target->parentItem(), target->anchorIn() ) );

	}
}

//------------------------------------------------------------------------------------

QPointF ecsAction::anchorIn() {
	return QPointF( this->pos().x()-34/2, this->pos().y()  );
}

//------------------------------------------------------------------------------------

QPointF ecsAction::anchorOut() {
	return mapFromParent( QPointF( this->pos().x()+34/2, this->pos().y()  ) );
}

//------------------------------------------------------------------------------------

void ecsAction::dragEnterEvent(QGraphicsSceneDragDropEvent *event)
{
	event->setAccepted(event->mimeData()->hasFormat("x-application/ecs-controlgroup-id"));
}

//------------------------------------------------------------------------------------

void ecsAction::dropEvent(QGraphicsSceneDragDropEvent *event)
{
	ecsControlGroupGraphic* targetGroup;

	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-controlgroup-id") ) return;

	QString idString(data->data("x-application/ecs-controlgroup-id"));
	int cGroupId = idString.toInt();

	prepareGeometryChange();
	targetGroup = new ecsControlGroupGraphic( ((MainWindow*)qApp->activeWindow())->cGroupModel->findItem( cGroupId ) );
	targetGroups.append( targetGroup );
	((MainWindow*)qApp->activeWindow())->updateScene();
}

//------------------------------------------------------------------------------------

void ecsAction::zap() {
	foreach( ecsControlGroupGraphic* target, targetGroups ) {
		if( target->parentItem() == this )
			target->setParentItem( NULL );
		if( scene() )
			scene()->removeItem( target );
	}
	if( scene() )
		scene()->removeItem( this );
}
