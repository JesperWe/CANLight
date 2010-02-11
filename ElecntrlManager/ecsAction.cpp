#include <QtGui>

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

void ecsAction::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QPixmap* icon;

	switch( actionType ) {
	case ecsAction::SwitchON:    { icon = new QPixmap(":/graphics/bulb-lit.svg"); break; }
	case ecsAction::SwitchOFF:   { icon = new QPixmap(":/graphics/bulb.svg"); break; }
	case ecsAction::ToggleOnOff: { icon = new QPixmap(":/graphics/onoff.svg"); break; }
	case ecsAction::FadeStart:   { icon = new QPixmap(":/graphics/fade-start.svg"); break; }
	case ecsAction::FadeStop:    { icon = new QPixmap(":/graphics/fade-stop.svg"); break; }
	case ecsAction::ChangeColor: { icon = new QPixmap(":/graphics/connections.svg"); break; }
	}

	painter->setBrush( QColor( 255, 125, 135, 255 ) );
	if( isSelected() ) {
		painter->setBrush( QColor( 0, 50, 255, 60 ) );
	}

	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );

	painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

	painter->drawPolygon( QPolygon( 4, &ecsAction::polygon[0][0] ) );
	painter->drawPixmap(-34*0.25,-34*0.25,34*0.5,34*0.5,*icon);

	foreach( ecsControlGroup* target, targetGroups ) {
		target->setParentItem( this );
		target->setPos( ecsManager::ActionOffset_X + target->boundingRect().width()/2, 0 );
		target->setVisible( true );
		if( ! target->scene() ) scene()->addItem( target );
		painter->drawLine( anchorOut(), target->anchorIn() );
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
	const QMimeData* data = event->mimeData();

	if( ! data->hasFormat("x-application/ecs-controlgroup-id") ) return;

	QString idString(data->data("x-application/ecs-controlgroup-id"));
	int cGroupId = idString.toInt();

	prepareGeometryChange();
	targetGroups.append( ((MainWindow*)qApp->activeWindow())->cGroupModel->findItem( cGroupId ) );
	((MainWindow*)qApp->activeWindow())->updateScene();
}

//------------------------------------------------------------------------------------

void ecsAction::zap() {
	ecsEvent* event = qgraphicsitem_cast<ecsEvent*>(parentItem());
	event->eventAction = NULL;

	foreach( ecsControlGroup* target, targetGroups ) {
		scene()->removeItem( target );
	}

	scene()->removeItem( this );

}
