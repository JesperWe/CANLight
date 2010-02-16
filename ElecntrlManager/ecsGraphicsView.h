#ifndef ECSGRAPHICSVIEW_H
#define ECSGRAPHICSVIEW_H

#include <QtGui>
#include <math.h>

class ecsGraphicsView : public QGraphicsView
{

Q_OBJECT

private:
	void setupUI();

public:
	ecsGraphicsView() {
		setupUI();
	}

	ecsGraphicsView( QWidget* parent ) : QGraphicsView( parent ) {
		setFocusPolicy( Qt::StrongFocus );
		setupUI();
	};

	void keyPressEvent( QKeyEvent *event );

protected:
	void wheelEvent( QWheelEvent *event );
	void scaleView( qreal scaleFactor );
	void contextMenuEvent( QContextMenuEvent *widgetevent );
	QMenu* ecsGraphicsActionMenu;
	QMenu* ecsGraphicsEventMenu;
	void unselectAll();


signals:
	void keypress( int key );
};
#endif // ECSGRAPHICSVIEW_H
