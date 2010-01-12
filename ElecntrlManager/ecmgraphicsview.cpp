
#include "ecmgraphicsview.h"

ECMGraphicsView::ECMGraphicsView( QWidget* parent ) : QGraphicsView( parent )
{
    setFocusPolicy( Qt::StrongFocus );
}

void ECMGraphicsView::wheelEvent( QWheelEvent *event )
{
    scaleView(pow((double)2, -event->delta() / 240.0));
}

void ECMGraphicsView::scaleView(qreal scaleFactor)
{
    qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
    if (factor < 0.07 || factor > 100)
        return;

    scale(scaleFactor, scaleFactor);
}

void ECMGraphicsView::keyPressEvent( QKeyEvent *event ) {
    switch( event->text() ) {
    case "+": { this->scale( 1.1, 1.1 ); break; }
    case  "-": { this->scale( 0.9, 0.9 ); break; }
    case "f": { this->resetTransform(); break; }
    case "1": { emit keypress( event->text() ); break; }
    case "2": { emit keypress( event->text() ); break; }
    case "3": { emit keypress( event->text() ); break; }
    }
}
