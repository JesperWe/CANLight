#ifndef ECSGRAPHICSVIEW_H
#define ECSGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

#include <math.h>

class ecsGraphicsView : public QGraphicsView
{

Q_OBJECT

public:
    ecsGraphicsView( QWidget* parent ) : QGraphicsView( parent )
        { setFocusPolicy( Qt::StrongFocus ); };

    void keyPressEvent( QKeyEvent *event );

protected:
    void wheelEvent( QWheelEvent *event );
    void scaleView( qreal scaleFactor );

signals:
    void keypress( QString key );
};
#endif // ECSGRAPHICSVIEW_H
