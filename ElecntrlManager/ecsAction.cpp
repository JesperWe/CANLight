#include <QtGui>

#include "ecsAction.h"

ecsAction::ecsAction() {}
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

    painter->setBrush( QColor( 200, 110, 110, 255 ) );
    if( isSelected() ) {
        painter->setBrush( QColor( 0, 50, 255, 60 ) );
    }

    painter->drawPolygon( QPolygon( 4, &ecsAction::polygon[0][0] ) );
    painter->drawPixmap(-size*0.35,-size*0.35,size*0.7,size*0.7,*icon);
}

//------------------------------------------------------------------------------------

QPoint ecsAction::anchorIn() {
    return QPoint(
                this->pos().x()-size,
                this->pos().y()
           );
}
