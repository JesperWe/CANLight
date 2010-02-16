#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "ecsAction.h"
#include "ecsGraphicsView.h"

void ecsGraphicsView::setupUI() {

    ecsGraphicsActionMenu = new QMenu( this );

        QAction* makeSwitchON = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::SwitchON],
                tr("Switch On"), this
            );
        QAction* makeSwitchOFF = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::SwitchOFF],
                tr("Switch Off"), this
            );
        QAction* makeToggleOnOff = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::ToggleOnOff],
                tr("Toggle On/Off"), this
            );
        QAction* makeFadeStart = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::FadeStart],
                tr("Start Fade"), this
            );
        QAction* makeFadeStop = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::FadeStop],
                tr("Stop Fade"), this
            );
        QAction* makeChangeColor = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::ChangeColor],
                tr("Change Color"), this
            );
        QAction* makeActuator = new QAction(
                ecsManagerApp::inst()->actionIcons[ecsManager::Actuator],
                tr("Actuator Control"), this
            );

        makeSwitchON->setData( ecsManager::SwitchON );
        makeSwitchOFF->setData( ecsManager::SwitchOFF );
        makeToggleOnOff->setData( ecsManager::ToggleOnOff );
        makeFadeStart->setData( ecsManager::FadeStart );
        makeFadeStop->setData( ecsManager::FadeStop );
        makeChangeColor->setData( ecsManager::ChangeColor );
        makeActuator->setData( ecsManager::Actuator );

        ecsGraphicsActionMenu->addAction( makeSwitchON );
        ecsGraphicsActionMenu->addAction( makeSwitchOFF );
        ecsGraphicsActionMenu->addAction( makeToggleOnOff );
        ecsGraphicsActionMenu->addAction( makeFadeStart );
        ecsGraphicsActionMenu->addAction( makeFadeStop );
        ecsGraphicsActionMenu->addAction( makeChangeColor );
        ecsGraphicsActionMenu->addAction( makeActuator );
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

    onItem->setSelected(true);
    ecsAction* actionItem  = qgraphicsitem_cast<ecsAction*>( onItem );

    if( actionItem ) {
        unselectAll();
        actionItem->setSelected( true );
        QAction* action = ecsGraphicsActionMenu->exec( event->globalPos() );
        if( action ) {
            actionItem->actionType = action->data().toInt();
            actionItem->setSelected( false );
            scene()->update( actionItem->boundingRect() );
        }
    };
}
