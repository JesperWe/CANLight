#include <Qt>
#include <QGraphicsSvgItem>
#include <QFileDialog>
#include <QPen>
#include <QGraphicsView>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_about.h"
#include "systemdescription.h"
#include "numberedItem.h"
#include "ecsManager.h"
#include "ecsEvent.h"
#include "ecsAction.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	qApp->setProperty( "headerFont", QVariant( QFont( "Helvetica", 11, QFont::Bold )));
	qApp->setProperty( "contentFont", QVariant( QFont( "Helvetica", 9 )));
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

	applianceModel = new NumberedItemModel(this);
	applianceModel->insertColumn(0);
	applianceModel->insertColumn(0);
	applianceModel->objectType = "x-application/ecs-appliance-id";

	cGroupModel = new NumberedItemModel(this);
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
	connect( this->ui->graphicsView, SIGNAL(keypress(QString)), this, SLOT(on_keypress(QString)) );
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
	NumberedItem* controlGroup;
	float  midpoint, eventOffset, x_pos, accumulatedOffset;
	bool first;
	float evenCountOffset;
	QRectF groupRect;

	// Set the position of this group based on how much space previous
	// groups occupied, and our own height.

	accumulatedOffset = 0;
	foreach ( controlGroup, cGroupModel->numberedItems ) {

		if( controlGroup->itemType != NumberedItem::Controller ) {
			controlGroup->setVisible( false );
			continue;
		}

		controlGroup->setVisible( true );
		controlGroup->setAcceptDrops(true);
		controlGroup->setFlag( QGraphicsItem::ItemIsSelectable );

		// Spread child events in a nice centered fanout.

		first = true;
		evenCountOffset = 0;
		if((controlGroup->events.count() % 2 == 0) && (controlGroup->events.count() > 0))
			evenCountOffset = ecsManager::EventOffset_Y/2;

		foreach( ecsEvent* event, controlGroup->events ) {
			eventOffset = calculateEventOffset( first, eventOffset );
			x_pos = controlGroup->longestChildWidth + ecsManager::EventOffset_X;
			event->setPos( x_pos, eventOffset - evenCountOffset );
		}

		groupRect = controlGroup->childrenBoundingRect().united( controlGroup->selectBox );
		midpoint = accumulatedOffset + 0.5 * groupRect.height();
		accumulatedOffset += groupRect.height();
		controlGroup->setPos( 0, midpoint );

		if( ! scene->items().contains( controlGroup) ) scene->addItem( controlGroup );
	}
	scene->update( scene->sceneRect() );
}

//--------------------------------------------------------------------

void MainWindow::on_actionExit_triggered()
{
	this->close();
}

//--------------------------------------------------------------------

void MainWindow::on_action_something_triggered()
{

	QGraphicsSvgItem* key1 = new QGraphicsSvgItem(QLatin1String(":/kalle.svg"));

	//key1->setElementId(QLatin1String("key1"));

	scene->addItem( key1 );
	key1->setScale( 1.5 );

	key1->setFlag(QGraphicsItem::ItemIsMovable, true);
	key1->setFlag(QGraphicsItem::ItemIsSelectable, true);
}

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
	if( selection[0]->type() != NumberedItem::Type ) return;

	NumberedItem* group = qgraphicsitem_cast<NumberedItem *>(selection[0]);

	thisEvent = new ecsEvent( group->id, eventType );
	group->events.append(  thisEvent  );
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

void MainWindow::on_keypress(QString key) {
	QList<QGraphicsItem*> selection;
	QGraphicsSimpleTextItem* link;

	selection = this->ui->graphicsView->scene()->selectedItems();
	if( selection.count() != 1 ) return;
	if( selection[0]->type() != QGraphicsSimpleTextItem::Type ) return;

	link = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(selection[0]);

	int func = 0;
	if( key == "1" ) func = ecsEvent::Key0;
	else if( key == "2" ) func = ecsEvent::Key1;
	else if( key == "3" ) func = ecsEvent::Key2;
	else if( key == "a" ) func = ecsEvent::AnalogSignal;
	else if( key == "i" ) func = ecsEvent::ChangeNotifiation;

	link->setData( 1, func );
	link->setSelected( false );
}
