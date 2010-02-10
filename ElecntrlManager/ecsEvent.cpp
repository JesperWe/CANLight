#include "ecsEvent.h"
#include "ecsManager.h"

QRectF ecsEvent::boundingRect() const {
	float iconDim = ecsManager::EventIconSize;
	return QRectF( -0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4 );
}

//------------------------------------------------------------------------------------

void ecsEvent::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QPixmap* icon;
	float iconDim = ecsManager::EventIconSize;

	switch( eventType ) {
	case ecsEvent::SingleClick: { icon = new QPixmap(":/graphics/click-single.svg"); break; }
	case ecsEvent::DoubleClick: { icon = new QPixmap(":/graphics/click-double.svg"); break; }
	case ecsEvent::PressHold: { icon = new QPixmap(":/graphics/click-hold.svg"); break; }
	case ecsEvent::Release: { icon = new QPixmap(":/graphics/click-release.svg"); break; }
	default: { return; }
	}

	if( isSelected() ) {
		painter->setBrush( qApp->property( "SelectionColor" ).value<QColor>() );
	} else {
		painter->setBrush( qApp->property( "EventColor" ).value<QColor>() );
	}

	painter->setPen( qApp->property( "cGroupPen" ).value<QPen>() );

	painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

	painter->drawEllipse(-0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4);
	painter->drawPixmap(-iconDim/2,-iconDim/2,iconDim,iconDim,*icon);

	if( eventAction ) {
		painter->drawLine(
				 anchorOut() ,
				 eventAction->anchorIn()
		);
	}
}

//------------------------------------------------------------------------------------

QPointF ecsEvent::anchorIn() {
	return QPointF(
			this->pos().x() - ecsManager::EventIconSize*0.7,
				this->pos().y()
		   );
}

//------------------------------------------------------------------------------------

QPointF ecsEvent::anchorOut() {
	return QPointF( ecsManager::EventIconSize*0.7, 0 );
}

//------------------------------------------------------------------------------------

void ecsEvent::zap() {

}
