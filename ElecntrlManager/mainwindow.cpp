#include <Qt>
#include <QGraphicsSvgItem>
#include <QFileDialog>
#include <QPen>
#include <QGraphicsView>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_about.h"
#include "systemdescription.h"
#include "ecsControlGroup.h"
#include "ecsManager.h"
#include "ecsEvent.h"
#include "ecsAction.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	qApp->setProperty( "headerFont", QVariant( QFont( "Helvetica", ecsManager::GroupNameFontSize, QFont::Bold )));
	qApp->setProperty( "contentFont", QVariant( QFont( "Helvetica", 8 )));
	qApp->setProperty( "buttonFont", QVariant( QFont( "Helvetica", 7, QFont::Bold )));
	qApp->setProperty( "SelectionColor", QVariant( QColor( 0, 50, 255, 60 )));
	qApp->setProperty( "EventColor", QVariant( QColor( 255, 210, 60, 170 )));

	QLinearGradient cgb( 0, 0, 0, 50 );
	cgb.setSpread( QGradient::ReflectSpread );
	cgb.setColorAt( 0.0, QColor( 150, 150, 150 ) );
	cgb.setColorAt( 0.4, QColor( 220, 220, 220 ) );
	cgb.setColorAt( 0.42, QColor( 180, 180, 180 ) );
	cgb.setColorAt( 1.0, QColor( 220, 220, 220 ) );
	qApp->setProperty( "cGroupBrush", QBrush(cgb) );

	QPen cgp( Qt::black, 2 );
	qApp->setProperty( "cGroupPen", cgp );

	scene = new QGraphicsScene;

	this->ui->graphicsView->setScene(scene);
	this->ui->graphicsView->show();

	applianceModel = new ecsControlGroupModel(this);
	applianceModel->insertColumn(0);
	applianceModel->insertColumn(0);
	applianceModel->objectType = "x-application/ecs-appliance-id";

	cGroupModel = new ecsControlGroupModel(this);
	cGroupModel->insertColumn(0);
	cGroupModel->insertColumn(0);
	cGroupModel->objectType = "x-application/ecs-controlgroup-id";

	this->ui->appliancesView->setModel( applianceModel );
	this->ui->appliancesView->setColumnWidth( 0, 45 );
	this->ui->appliancesView->setColumnWidth( 1, 120 );
	this->ui->appliancesView->setColumnWidth( 2, 20 );
	this->ui->appliancesView->verticalHeader()->hide();

	this->ui->cGroupView->setModel( cGroupModel );
	this->ui->cGroupView->setColumnWidth( 0, 45 );
	this->ui->cGroupView->setColumnWidth( 1, 120 );
	this->ui->cGroupView->setColumnWidth( 2, 20 );
	this->ui->cGroupView->verticalHeader()->hide();

	connect( this->ui->cGroupView->model(), SIGNAL( modified() ), this, SLOT(on_modifiedData()) );
	connect( this->ui->graphicsView, SIGNAL(keypress(int)), this, SLOT(on_keypress(int)) );
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
	QMainWindow::changeEvent(e);
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui->retranslateUi(this);
		break;
	default:
		break;
	}
}

void MainWindow::on_actionAbout_triggered()
{
	QDialog *aboutBox = new QDialog;
	Ui::AboutBox ui;
	ui.setupUi(aboutBox);
	aboutBox->setAttribute( Qt::WA_DeleteOnClose, true );
	aboutBox->show();
}

void MainWindow::on_actionOpen_triggered()
{
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this,
		tr("Open System Description File"), "", tr("Electric System Files (*.esf)"));
	SystemDescription::loadFile( fileName, applianceModel, cGroupModel );
	updateScene();
}

void MainWindow::on_modifiedData()
{
	updateScene();
}

//--------------------------------------------------------------------
// Alternate pos/neg offset

float MainWindow::calculateEventOffset( bool & first, float eventOffset ) {
	if( first ) {
		eventOffset = 0;
		first = false;
	}
	else {
		if( eventOffset > 0 ) eventOffset = - eventOffset;
		else {
			eventOffset = - eventOffset;
			eventOffset = eventOffset + ecsManager::EventOffset_Y;
		}
	}
	return eventOffset;
}

//--------------------------------------------------------------------

void MainWindow::updateScene() {
	ecsControlGroup* control;
	ecsControlGroupGraphic* groupGraphic;
	float  midpoint, eventOffset, x_pos, accumulatedOffset;
	bool first;
	float evenCountOffset;
	int srcGroupCounter;
	QRectF groupRect;

	// Set the position of this group based on how much space previous
	// groups occupied, and our own height.

	accumulatedOffset = 0;
	srcGroupCounter = 0;
	foreach ( control, cGroupModel->ecsControlGroups ) {

		if( control->itemType != ecsControlGroup::Controller ) {
			continue;
		}

		groupGraphic = control->graphic;
		if( ! groupGraphic->scene() ) scene->addItem( groupGraphic );

		// Spread child events in a nice centered fanout.

		first = true;
		evenCountOffset = 0;
		if((control->events.count() % 2 == 0) && (control->events.count() > 0))
			evenCountOffset = ecsManager::EventOffset_Y/2;

		foreach( ecsEvent* event, control->events ) {
			eventOffset = calculateEventOffset( first, eventOffset );
			x_pos = groupGraphic->longestChildWidth + ecsManager::EventOffset_X;
			event->setPos( x_pos, eventOffset - evenCountOffset );
		}

		// Now push the control groups around so they don't overlap.

		groupRect = groupGraphic->childrenBoundingRect().united( groupGraphic->selectBox );
		midpoint = accumulatedOffset + 0.5 * (groupRect.height() + ecsManager::GroupSpacing);
		accumulatedOffset += groupRect.height() + ecsManager::GroupSpacing;
		groupGraphic->setPos( 0, midpoint );

		if( ! groupGraphic->scene() ) scene->addItem( groupGraphic );
	}
	scene->update( scene->sceneRect() );
}

//--------------------------------------------------------------------

void MainWindow::on_actionExit_triggered()
{
	this->close();
}

//--------------------------------------------------------------------

void MainWindow::on_actionAbout_Qt_triggered()
{
	qApp->aboutQt();
}

void MainWindow::on_actionSave_As_triggered()
{
	QString fileName;
	fileName = QFileDialog::getSaveFileName(this,
		tr("Save New System Description File"), "", tr("Electric System Files (*.esf)"));
	SystemDescription::saveFile( fileName, applianceModel, cGroupModel );
}

void MainWindow::on_actionSave_triggered()
{
	SystemDescription::saveFile( SystemDescription::loadedFileName, applianceModel, cGroupModel );
}

//-------------------------------------------------------------------------------------------------

void MainWindow::_AddEvent( int eventType )
{
	ecsEvent* thisEvent;

	QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
	if( selection.count() != 1 ) return;
	if( selection[0]->type() != ecsControlGroupGraphic::Type ) return;

	ecsControlGroupGraphic* group = qgraphicsitem_cast<ecsControlGroupGraphic *>(selection[0]);

	thisEvent = new ecsEvent( group->srcGroup->id, eventType );
	group->srcGroup->events.append(  thisEvent  );
	thisEvent->setParentItem( group );
	updateScene();
}

void MainWindow::on_actionSingle_Click_triggered() { _AddEvent( ecsEvent::SingleClick ); }
void MainWindow::on_actionDouble_Click_triggered() { _AddEvent( ecsEvent::DoubleClick ); }
void MainWindow::on_actionPress_Hold_triggered() { _AddEvent( ecsEvent::PressHold ); }
void MainWindow::on_actionRelease_triggered() { _AddEvent( ecsEvent::Release ); }

//-------------------------------------------------------------------------------------------------

void MainWindow::on_actionNew_triggered()
{
	scene->clear();
	cGroupModel->clear();
	applianceModel->clear();
}

//-------------------------------------------------------------------------------------------------

void MainWindow::_AddAction( int actionType ) {
	QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
	if( selection.count() != 1 ) return;
	if( selection[0]->type() != ecsEvent::Type ) return;

	ecsEvent* event = qgraphicsitem_cast<ecsEvent *>(selection[0]);

	event->eventAction = new ecsAction( actionType );
	event->eventAction->setParentItem( event );
	event->eventAction->setX( ecsManager::ActionOffset_X );
	updateScene();
}

void MainWindow::on_actionToggle_On_Off_triggered() { _AddAction( ecsAction::ToggleOnOff ); }
void MainWindow::on_actionSwitch_On_triggered() { _AddAction( ecsAction::SwitchON ); }
void MainWindow::on_actionSwitch_Off_triggered() { _AddAction( ecsAction::SwitchOFF ); }
void MainWindow::on_actionStart_Fade_triggered() { _AddAction( ecsAction::FadeStart ); }
void MainWindow::on_actionStop_Fade_triggered() { _AddAction( ecsAction::FadeStop ); }
void MainWindow::on_actionSwitch_Color_triggered() { _AddAction( ecsAction::ChangeColor ); }

//-------------------------------------------------------------------------------------------------

void MainWindow::on_keypress( int key ) {
	QList<QGraphicsItem*> selection;
	QGraphicsSimpleTextItem* link;

	selection = this->ui->graphicsView->scene()->selectedItems();
	if( selection.count() != 1 ) return;

	if( key == Qt::Key_Delete ) {
		switch( selection[0]->type() ) {
		case ecsEvent::Type: {
				ecsEvent* event = qgraphicsitem_cast<ecsEvent*>(selection[0]);
				event->zap();
				break; }
		case ecsAction::Type: {
				ecsAction* action = qgraphicsitem_cast<ecsAction*>(selection[0]);
				action->zap();
				break; }
		case QGraphicsSimpleTextItem::Type: {
				link = qgraphicsitem_cast<QGraphicsSimpleTextItem*>(selection[0]);
				ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
				ecsControlGroupGraphic* groupGraphic = (ecsControlGroupGraphic*)(link->parentItem());
				groupGraphic->srcGroup->links.removeAt( groupGraphic->srcGroup->links.indexOf( linkedApp ) );
				groupGraphic->recalcLinkPositions();;
				updateScene();
				break; }
		}
	}

	if( selection[0]->type() != QGraphicsSimpleTextItem::Type ) return;

	 link = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(selection[0]);

	int func = 0;
	switch( key ) {
	case Qt::Key_1: { func = ecsEvent::Key0; break; }
	case Qt::Key_2: { func = ecsEvent::Key1; break; }
	case Qt::Key_3: { func = ecsEvent::Key2; break; }
	case Qt::Key_A: { func = ecsEvent::AnalogSignal; break; }
	case Qt::Key_I: { func = ecsEvent::ChangeNotifiation; break; }
	default:  { return; } // Ignore unknow keypress.
	}

	ecsControlGroup* linkedapp = (ecsControlGroup*)(link->data(0).value<void*>());
	ecsControlGroupGraphic* groupGraphic = (ecsControlGroupGraphic*)(link->parentItem());
	groupGraphic->srcGroup->functions[ linkedapp->id ] = func;
	link->setSelected( false );
}
