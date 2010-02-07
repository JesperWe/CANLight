#include "ecsEvent.h"
#include "ecsManager.h"

QRectF ecsEvent::boundingRect() const {
	float iconDim = ecsManager::EventSize;
	return QRectF( -0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4 );
}

//------------------------------------------------------------------------------------

void ecsEvent::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QPixmap* icon;
	float iconDim = ecsManager::EventSize;

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
}

//------------------------------------------------------------------------------------

QPoint ecsEvent::anchorIn() {
	return QPoint(
			this->pos().x() - ecsManager::EventSize*0.7,
				this->pos().y()
		   );
}

//------------------------------------------------------------------------------------

QPoint ecsEvent::anchorOut() {
	return QPoint(
				this->pos().x() + ecsManager::EventSize*0.7,
				this->pos().y()
		   );
}
