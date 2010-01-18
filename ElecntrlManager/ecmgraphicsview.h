#ifndef ECMGRAPHICSVIEW_H
#define ECMGRAPHICSVIEW_H

#include <QGraphicsView>
#include <QWheelEvent>

#include <math.h>

class ECMGraphicsView : public QGraphicsView
{

Q_OBJECT

public:
    ECMGraphicsView( QWidget* parent ) : QGraphicsView( parent )
        { setFocusPolicy( Qt::StrongFocus ); };

    void keyPressEvent( QKeyEvent *event );

protected:
    void wheelEvent( QWheelEvent *event );
    void scaleView(qreal scaleFactor);

signals:
    void keypress( QString key );
};
#endif // ECMGRAPHICSVIEW_H
