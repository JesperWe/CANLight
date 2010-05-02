#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "ecsAction.h"
#include "ecsGraphicsView.h"

void ecsGraphicsView::setupUI() {

	ecsGraphicsActionMenu = new QMenu( this );

	QAction* makeSwitchON = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_SWITCH_ON],
			tr("Switch On"), this
			);
	QAction* makeSwitchOFF = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_SWITCH_OFF],
			tr("Switch Off"), this
			);
	QAction* makeToggleOnOff = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_TOGGLE_STATE],
			tr("Toggle On/Off"), this
			);
	QAction* makeFadeStart = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_START_FADE],
			tr("Start Fade"), this
			);
	QAction* makeFadeStop = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_STOP_FADE],
			tr("Stop Fade"), this
			);
	QAction* makeChangeColor = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_CHANGE_COLOR],
			tr("Change Color"), this
			);
	QAction* makeActuator = new QAction(
			ecsManagerApp::inst()->actionIcons[ecsManager::a_SET_LEVEL],
			tr("Actuator Control"), this
			);

	makeSwitchON->setData( ecsManager::a_SWITCH_ON );
	makeSwitchOFF->setData( ecsManager::a_SWITCH_OFF );
	makeToggleOnOff->setData( ecsManager::a_TOGGLE_STATE );
	makeFadeStart->setData( ecsManager::a_START_FADE );
	makeFadeStop->setData( ecsManager::a_STOP_FADE );
	makeChangeColor->setData( ecsManager::a_CHANGE_COLOR );
	makeActuator->setData( ecsManager::a_SET_LEVEL );

	ecsGraphicsActionMenu->addAction( makeSwitchON );
	ecsGraphicsActionMenu->addAction( makeSwitchOFF );
	ecsGraphicsActionMenu->addAction( makeToggleOnOff );
	ecsGraphicsActionMenu->addAction( makeFadeStart );
	ecsGraphicsActionMenu->addAction( makeFadeStop );
	ecsGraphicsActionMenu->addAction( makeChangeColor );
	ecsGraphicsActionMenu->addAction( makeActuator );

	ecsGraphicsEventMenu = new QMenu( this );

	QAction* makeSingleClick = new QAction(
			ecsManagerApp::inst()->eventIcons[ecsManager::e_KEY_CLICKED],
			tr("Single Click"), this
			);
	QAction* makeDoubleClick = new QAction(
			ecsManagerApp::inst()->eventIcons[ecsManager::e_KEY_DOUBLECLICKED],
			tr("Double Click"), this
			);
	QAction* makePressHold = new QAction(
			ecsManagerApp::inst()->eventIcons[ecsManager::e_KEY_HOLDING],
			tr("Press and Hold"), this
			);
	QAction* makeRelease = new QAction(
			ecsManagerApp::inst()->eventIcons[ecsManager::e_KEY_RELEASED],
			tr("Release Key"), this
			);
	QAction* makeSignalChange = new QAction(
			ecsManagerApp::inst()->eventIcons[ecsManager::e_LEVEL_CHANGED],
			tr("Signal Change"), this
			);

	makeSingleClick->setData( ecsManager::e_KEY_CLICKED );
	makeDoubleClick->setData( ecsManager::e_KEY_DOUBLECLICKED );
	makePressHold->setData( ecsManager::e_KEY_HOLDING );
	makeRelease->setData( ecsManager::e_KEY_RELEASED );
	makeSignalChange->setData( ecsManager::e_LEVEL_CHANGED );

	ecsGraphicsEventMenu->addAction( makeSingleClick );
	ecsGraphicsEventMenu->addAction( makeDoubleClick );
	ecsGraphicsEventMenu->addAction( makePressHold );
	ecsGraphicsEventMenu->addAction( makeRelease );
	ecsGraphicsEventMenu->addAction( makeSignalChange );
}

void ecsGraphicsView::wheelEvent( QWheelEvent *event )
{
	float scaleFactor = pow((double)2, -event->delta() / 240.0);
	scaleView( scaleFactor );
	if( scaleFactor > 0 ) 	centerOn( event->pos() );
}

//------------------------------------------------------------------------------------

void ecsGraphicsView::scaleView(qreal scaleFactor)
{
	qreal factor = matrix().scale(scaleFactor, scaleFactor).mapRect(QRectF(0, 0, 1, 1)).width();
	if (factor < 0.07 || factor > 100)
		return;

	scale(scaleFactor, scaleFactor);
}

//------------------------------------------------------------------------------------

void ecsGraphicsView::keyPressEvent( QKeyEvent *event ) {
	if( event->text() == "+" ) this->scale( 1.1, 1.1 );
	else if( event->text() == "-" ) this->scale( 0.9, 0.9 );
	else if( event->text() == "f" ) this->resetTransform();

	else  {
		emit keypress( event->key() );
	}
}

//------------------------------------------------------------------------------------

void ecsGraphicsView::unselectAll() {
	foreach( QGraphicsItem* item, scene()->selectedItems() )
		item->setSelected( false );
}

//------------------------------------------------------------------------------------

void ecsGraphicsView::contextMenuEvent( QContextMenuEvent *event ) {
	qDebug() << "Context " << event->pos() << "  " << event->globalPos() ;
	QGraphicsItem* onItem = scene()->itemAt( mapToScene(event->pos()) );
	if( ! onItem ) return;

	if( ecsAction* actionItem  = qgraphicsitem_cast<ecsAction*>( onItem ) ) {
		unselectAll();
		actionItem->setSelected( true );
		QAction* action = ecsGraphicsActionMenu->exec( event->globalPos() );
		if( action ) {
			actionItem->actionType = action->data().toInt();
			actionItem->setSelected( false );
			scene()->update( actionItem->boundingRect() );
		}
	};

	if( ecsEvent* eventItem  = qgraphicsitem_cast<ecsEvent*>( onItem ) ) {
		unselectAll();
		eventItem->setSelected( true );
		QAction* action = ecsGraphicsEventMenu->exec( event->globalPos() );
		if( action ) {
			eventItem->eventType = action->data().toInt();
			eventItem->setSelected( false );
			scene()->update( eventItem->boundingRect() );
		}
	};
}
