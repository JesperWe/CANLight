#include <QtGui>

#include "mainwindow.h"
#include "numberedItemModel.h"
#include "numberedItem.h"
#include "ecsAction.h"

ecsAction::ecsAction() {
	setAcceptDrops(true);
	actionType = ecsAction::None;
}

const int ecsAction::polygon[4][2] = { { 0, size/2 }, { size/2, 0 }, { 0, -size/2 }, { -size/2, 0 } };

//------------------------------------------------------------------------------------

QRectF ecsAction::boundingRect() const {
	return QRectF( -size/2,-size/2, size, size );
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
	painter->drawPixmap(-size*0.25,-size*0.25,size*0.5,size*0.5,*icon);
}

//------------------------------------------------------------------------------------

QPoint ecsAction::anchorIn() {
	return QPoint(
				this->pos().x()-size/2,
				this->pos().y()
		   );
}

//------------------------------------------------------------------------------------

QPoint ecsAction::anchorOut() {
	return QPoint(
				this->pos().x()+size/2,
				this->pos().y()
		   );
}


//------------------------------------------------------------------------------------

void ecsAction::drawOutputTo( QPoint to, QGraphicsScene* scene ) {
	QGraphicsLineItem* line = new QGraphicsLineItem( anchorOut().x(), anchorOut().y(), to.x(), to.y(), 0, 0);
	line->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
	scene->addItem( line );
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
