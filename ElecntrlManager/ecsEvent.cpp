#include "ecsEvent.h"

ecsEvent::ecsEvent() {}

QRectF ecsEvent::boundingRect() const {
    return QRectF( -0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4 );
}

//------------------------------------------------------------------------------------

QPoint ecsEvent::anchorIn() {
    return QPoint(
                this->pos().x()-iconDim*0.7,
                this->pos().y()
           );
}

//------------------------------------------------------------------------------------

void ecsEvent::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QPixmap* icon;

    switch( type ) {
    case ecsEvent::SingleClick: { icon = new QPixmap(":/graphics/click-single.svg"); break; }
    case ecsEvent::DoubleClick: { icon = new QPixmap(":/graphics/click-double.svg"); break; }
    case ecsEvent::PressHold: { icon = new QPixmap(":/graphics/click-hold.svg"); break; }
    case ecsEvent::Release: { icon = new QPixmap(":/graphics/click-release.svg"); break; }
    }

    painter->setBrush( QColor( 255, 210, 60, 170 ) );
    painter->drawEllipse(-0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4);
    painter->drawPixmap(-iconDim/2,-iconDim/2,iconDim,iconDim,*icon);
}