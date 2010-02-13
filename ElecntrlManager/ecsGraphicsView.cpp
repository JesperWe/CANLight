#include "ecsManager.h"
#include "ecsGraphicsView.h"

void ecsGraphicsView::wheelEvent( QWheelEvent *event )
{
	scaleView(pow((double)2, -event->delta() / 240.0));
}

void ecsGraphicsView::scaleView(qreal scaleFactor)
{
	qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
	if (factor < 0.07 || factor > 100)
		return;

	scale(scaleFactor, scaleFactor);
}


void ecsGraphicsView::keyPressEvent( QKeyEvent *event ) {
	if( event->text() == "+" ) this->scale( 1.1, 1.1 );
	else if( event->text() == "-" ) this->scale( 0.9, 0.9 );
	else if( event->text() == "f" ) this->resetTransform();

	else  {
		emit keypress( event->key() );
	}
}
