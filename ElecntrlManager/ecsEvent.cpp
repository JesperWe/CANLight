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

QPoint ecsEvent::anchorOut() {
    return QPoint(
                this->pos().x()+iconDim*0.7,
                this->pos().y()
           );
}

//------------------------------------------------------------------------------------

void ecsEvent::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QPixmap* icon;

    switch( eventType ) {
    case ecsEvent::SingleClick: { icon = new QPixmap(":/graphics/click-single.svg"); break; }
    case ecsEvent::DoubleClick: { icon = new QPixmap(":/graphics/click-double.svg"); break; }
    case ecsEvent::PressHold: { icon = new QPixmap(":/graphics/click-hold.svg"); break; }
    case ecsEvent::Release: { icon = new QPixmap(":/graphics/click-release.svg"); break; }
    }

    painter->setBrush( QColor( 255, 210, 60, 170 ) );
    if( isSelected() ) {
        painter->setBrush( QColor( 0, 50, 255, 60 ) );
    }

    painter->setRenderHints( QPainter::Antialiasing | QPainter::SmoothPixmapTransform );

    painter->drawEllipse(-0.7*iconDim,-0.7*iconDim,iconDim*1.4,iconDim*1.4);
    painter->drawPixmap(-iconDim/2,-iconDim/2,iconDim,iconDim,*icon);
}
