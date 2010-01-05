#ifndef ECMGRAPHICSVIEW_H
#define ECMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

#include <math.h>

class ECMGraphicsView : public QGraphicsView
{
public:
    ECMGraphicsView( QWidget* parent );
    void keyPressEvent( QKeyEvent *event );

protected:
    void wheelEvent( QWheelEvent *event );
    void scaleView(qreal scaleFactor);
};


#endif // ECMGRAPHICSVIEW_H
