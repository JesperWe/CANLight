/*
 * $Revision$
 * $Author$
 * $Date$
 */

#include "assert.h"

#include <QtGui>
#include <QtSvg>

#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_about.h"
#include "ui_NMEAMonitor.h"
#include "systemdescription.h"
#include "ecsControlGroup.h"
#include "ecsEvent.h"
#include "ecsAction.h"
#include "ecsCANUSB.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi( this );

	readSettings();

	qApp->setProperty( "headerFont", QVariant( QFont( "Helvetica", ecsManager::GroupNameFontSize, QFont::Bold )));
	qApp->setProperty( "contentFont", QVariant( QFont( "Helvetica", 9 )));
	qApp->setProperty( "buttonFont", QVariant( QFont( "Helvetica", 7, QFont::Bold )));
	qApp->setProperty( "SelectionColor", QVariant( QColor( 0, 50, 255, 60 )));
	qApp->setProperty( "EventColor", QVariant( QColor( 255, 210, 60, 170 )));

	QLinearGradient cgb( 0, -30, 0, 70 );
	cgb.setColorAt( 0.0, QColor( 140, 140, 140 ) );
	cgb.setColorAt( 0.3, QColor( 235, 235, 235 ) );
	cgb.setColorAt( 1.0, QColor( 140, 140, 140 ) );
	qApp->setProperty( "cGroupBrush", QBrush(cgb) );

	QPen cgp( Qt::black, 2 );
	qApp->setProperty( "cGroupPen", cgp );

	scene = new QGraphicsScene;

	ui->graphicsView->setScene(scene);
	ui->graphicsView->show();
	ui->graphicsView->setCursor( Qt::ArrowCursor );

	applianceModel = new ecsControlGroupModel(this);
	applianceModel->insertColumn(0);
	applianceModel->insertColumn(0);
	applianceModel->objectType = "x-application/ecs-appliance-id";

	cGroupModel = new ecsControlGroupModel(this);
	cGroupModel->insertColumn(0);
	cGroupModel->insertColumn(0);
	cGroupModel->objectType = "x-application/ecs-controlgroup-id";

	ui->appliancesView->setModel( applianceModel );
	ui->appliancesView->setColumnWidth( 0, 45 );
	ui->appliancesView->setColumnWidth( 1, 120 );
	ui->appliancesView->setColumnWidth( 2, 20 );
	ui->appliancesView->verticalHeader()->hide();
	ecsManagerApp::inst()->appliances = applianceModel;

	ui->cGroupView->setModel( cGroupModel );
	ui->cGroupView->setColumnWidth( 0, 45 );
	ui->cGroupView->setColumnWidth( 1, 120 );
	ui->cGroupView->setColumnWidth( 2, 20 );
	ui->cGroupView->verticalHeader()->hide();
	ecsManagerApp::inst()->cGroups = cGroupModel;

	connect( ui->cGroupView->model(), SIGNAL( modified() ), this, SLOT(onModifiedData()) );
	connect( ui->graphicsView, SIGNAL(keypress(int)), this, SLOT(onKeypress(int)) );

	monitorDialog = new QDialog;
	Ui::NMEA2000Monitor monitorUi;
	monitorUi.setupUi(monitorDialog);

	connect( monitorDialog, SIGNAL(rejected()), this, SLOT(on_MonitorDialogReject()) );

	ui->statusBar->showMessage( tr("Welcome, master!"), 4000 );

	NMEAFrame = new QFrame();
	horiz = new QHBoxLayout(NMEAFrame);
	NMEAIcon = new QSvgWidget( ":/graphics/icon-blue.svg" );
	NMEAIcon->setMaximumWidth( 14 );
	NMEAIcon->setMaximumHeight( 20 );

	NMEAStatusText = new QLabel( tr("Yacht Network: ?") );
	NMEAStatusText->setMaximumHeight( 20 );
	horiz->addWidget( NMEAIcon );
	horiz->addWidget( NMEAStatusText );
	horiz->setContentsMargins( 0, 0, 4, 0 );
	horiz->setSpacing( 4 );
	NMEAFrame->setLayout( horiz );
	NMEAFrame->setMaximumHeight( 20 );
	ui->statusBar->addPermanentWidget( NMEAFrame );

	// Now try the CANUSB Hardware.

	canusb = new ecsCANUSB();
	ecsManagerApp::inst()->canusb_Instance = canusb;
	connect( canusb, SIGNAL(addLogLine(QString)), monitorUi.logWidget, SLOT(appendPlainText(QString)) );
	updateCANStatus();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	writeSettings();
	event->accept();
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

void MainWindow::readSettings()
{
	QSettings settings("Journeyman", "Electric Control System");
	QPoint pos = settings.value("window-position", QPoint(20, 20)).toPoint();
	QSize size = settings.value("window-size", QSize(800, 600)).toSize();
	resize(size);
	move(pos);
	ui->splitter->restoreState(settings.value("main-splitter-sizes").toByteArray());
}

void MainWindow::writeSettings()
{
	QSettings settings("Journeyman", "Electric Control System");
	settings.setValue("window-position", pos());
	settings.setValue("window-size", size());
	settings.setValue("main-splitter-sizes", ui->splitter->saveState());
}

//--------------------------------------------------------------------

void MainWindow::updateCANStatus() {
	bool canActive = false;

	switch( canusb->status() ) {
	case ecsCANUSB::DeviceError: {
			NMEAIcon->load( QString(":/graphics/icon-red.svg") );
			NMEAStatusText->setText( tr("USB CAN Interface Unknown Error") );
			canActive = false;
			break; }
	case ecsCANUSB::DriverNotInstalled: {
			NMEAIcon->load( QString(":/graphics/icon-red.svg") );
			NMEAStatusText->setText( tr("USB CAN Driver Not Found") );
			canActive = false;
			break; }
	case ecsCANUSB::DongleNotPresent: {
			NMEAIcon->load( QString(":/graphics/icon-orange.svg") );
			NMEAStatusText->setText( tr("USB Interface not plugged in.") );
			canActive = false;
			break; }
	case ecsCANUSB::BusDisconnected: {
			NMEAIcon->load( QString(":/graphics/icon-yellow.svg") );
			NMEAStatusText->setText( tr("USB Interface not connected to yacht network.") );
			canActive = false;
			break; }
	case ecsCANUSB::BusOpen: {
			NMEAIcon->load( QString(":/graphics/icon-green.svg") );
			NMEAStatusText->setText( tr("Connected.") );
			canActive = true;
			break; }
	case ecsCANUSB::BusIdle: {
			NMEAIcon->load( QString(":/graphics/icon-green.svg") );
			NMEAStatusText->setText( tr("Connected.") );
			canActive = true;
			break; }
	case ecsCANUSB::BusActive: {
			NMEAIcon->load( QString(":/graphics/icon-active.svg") );
			NMEAStatusText->setText( tr("Connected.") );
			canActive = true;
			break; }
	case ecsCANUSB::ReceiveError: {
			NMEAIcon->load( QString(":/graphics/icon-red.svg") );
			NMEAStatusText->setText( tr("Receive Error.") );
			canActive = true;
			break; }
	case ecsCANUSB::TransmitError: {
			NMEAIcon->load( QString(":/graphics/icon-red.svg") );
			NMEAStatusText->setText( tr("Transmit Error.") );
			canActive = true;
			break; }
	}

	ui->actionOpen_Connection->setEnabled( ! canActive );
	ui->actionClose_Connection->setEnabled( canActive );
	ui->actionShow_Monitor->setEnabled( canActive );
	ui->actionUpload_to_Yacht->setEnabled( canActive );
}

//--------------------------------------------------------------------

void MainWindow::on_actionAbout_triggered()
{
	QDialog *aboutBox = new QDialog;
	Ui::AboutBox aboutUi;
	aboutUi.setupUi(aboutBox);
	aboutBox->setAttribute( Qt::WA_DeleteOnClose, true );

	if( canusb && canusb->info() ) {
		aboutUi.CANUSBInfo->setText( canusb->info() );
	}

	aboutBox->show();
}

void MainWindow::on_actionOpen_triggered()
{
	QString fileName;
	fileName = QFileDialog::getOpenFileName(this,
		tr("Open System Description File"), "", tr("Electric System Files (*.esf)"));
	on_actionNew_triggered();
	SystemDescription::loadFile( fileName );
	ui->statusBar->showMessage( tr("System Description Version Counter: ") + QString::number( ecsManagerApp::inst()->systemDescriptionVersion ) );
	updateScene();
}

void MainWindow::onModifiedData()
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

		// Only show graphics for groups that are control sources or targeted by an action.

		if( control->itemType != ecsControlGroup::Controller ) {
			if( ! control->graphic->parentItem() && control->graphic->scene() ) {
				control->graphic->scene()->removeItem( control->graphic );
			}
			continue;
		}

		groupGraphic = control->graphic;
		groupGraphic->updateLinkTexts();
		groupGraphic->recalcLinkPositions();
		groupGraphic->recalcBoxSize();

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
	SystemDescription::saveFile( fileName );
	setWindowModified( false );
}

void MainWindow::on_actionSave_triggered()
{
	SystemDescription::saveFile( SystemDescription::loadedFileName );
	setWindowModified( false );
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
	setWindowModified( true );
	updateScene();
}

void MainWindow::on_actionSingle_Click_triggered() { _AddEvent( ecsManager::SingleClick ); }
void MainWindow::on_actionDouble_Click_triggered() { _AddEvent( ecsManager::DoubleClick ); }
void MainWindow::on_actionPress_Hold_triggered() { _AddEvent( ecsManager::PressHold ); }
void MainWindow::on_actionRelease_triggered() { _AddEvent( ecsManager::Release ); }
void MainWindow::on_actionSignal_Change_triggered() { _AddEvent( ecsManager::SignalChange ); }


//-------------------------------------------------------------------------------------------------

void MainWindow::on_actionNew_triggered()
{
	scene->clear();
	cGroupModel->clear();
	applianceModel->clear();
	setWindowModified( false );
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
	setWindowModified( true );
	updateScene();
}

void MainWindow::on_actionToggle_On_Off_triggered() { _AddAction( ecsManager::ToggleOnOff ); }
void MainWindow::on_actionSwitch_On_triggered() { _AddAction( ecsManager::SwitchON ); }
void MainWindow::on_actionSwitch_Off_triggered() { _AddAction( ecsManager::SwitchOFF ); }
void MainWindow::on_actionStart_Fade_triggered() { _AddAction( ecsManager::FadeStart ); }
void MainWindow::on_actionStop_Fade_triggered() { _AddAction( ecsManager::FadeStop ); }
void MainWindow::on_actionSwitch_Color_triggered() { _AddAction( ecsManager::ChangeColor ); }
void MainWindow::on_actionRun_Actuator_triggered() { _AddAction( ecsManager::Actuator ); }

//-------------------------------------------------------------------------------------------------

void MainWindow::onKeypress( int key ) {
	QList<QGraphicsItem*> selection;
	QGraphicsSimpleTextItem* link;

	selection = this->ui->graphicsView->scene()->selectedItems();
	if( selection.count() != 1 ) return;

	switch( key ) {

	case Qt::Key_Delete: {
		switch( selection[0]->type() ) {
		case ecsEvent::Type: {
				ecsEvent* event = qgraphicsitem_cast<ecsEvent*>(selection[0]);
				event->zap();
				setWindowModified( true );
				break; }
		case ecsAction::Type: {
				ecsAction* action = qgraphicsitem_cast<ecsAction*>(selection[0]);
				ecsEvent* event = qgraphicsitem_cast<ecsEvent*>(action->parentItem());
				action->zap();
				event->eventAction = NULL;
				setWindowModified( true );
				break; }
		case QGraphicsSimpleTextItem::Type: {
				link = qgraphicsitem_cast<QGraphicsSimpleTextItem*>(selection[0]);
				ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
				ecsControlGroupGraphic* groupGraphic = (ecsControlGroupGraphic*)(link->parentItem());
				groupGraphic->srcGroup->links.removeAt( groupGraphic->srcGroup->links.indexOf( linkedApp ) );
				groupGraphic->recalcLinkPositions();
				setWindowModified( true );
				updateScene();
				break; }
		}
		break;
	}

	case Qt::Key_Control | Qt::Key_S: {
		on_actionSave_triggered();
	}
	}

	QGraphicsItem* selectedItem = selection[0];
	if( selectedItem->type() != QGraphicsSimpleTextItem::Type ) return;

	 link = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(selection[0]);

	int func = 0;
	switch( key ) {
	case Qt::Key_1: { func = ecsManager::hw_KEY1; break; }
	case Qt::Key_2: { func = ecsManager::hw_KEY2; break; }
	case Qt::Key_3: { func = ecsManager::hw_KEY3; break; }
	case Qt::Key_A: { func = ecsManager::hw_ANALOG; break; }
	case Qt::Key_I: { func = ecsManager::hw_DIGITAL_IN; break; }
	case Qt::Key_R: { func = ecsManager::hw_LED_RED; break; }
	case Qt::Key_W: { func = ecsManager::hw_LED_WHITE; break; }
	case Qt::Key_L: { func = ecsManager::hw_LED_LIGHT; break; }
	case Qt::Key_O: { func = ecsManager::hw_PWM1; break; }
	default:  { return; } // Ignore unknow keypress.
	}

	ecsControlGroup* linkedapp = (ecsControlGroup*)(link->data(0).value<void*>());
	ecsControlGroupGraphic* groupGraphic = (ecsControlGroupGraphic*)(link->parentItem());

	int linkNumber = groupGraphic->linkTexts.indexOf( link );
	assert( linkNumber != -1 );
	groupGraphic->srcGroup->functions[ linkNumber ] = func;

	link->setSelected( false );
	setWindowModified( true );
}

//-------------------------------------------------------------------------------------------------

void MainWindow::on_actionUpload_to_Yacht_triggered()
{
	QByteArray configFile;
	SystemDescription::buildNMEAConfig( configFile );

	if( configFile.length() > 1024 ) {
		QString msg = "Configuration file size ";
		msg.append( QString::number(configFile.length()) );
		msg.append( " exceeds maximum supported size of 1024 bytes." );

		QMessageBox msgBox;
		msgBox.setText( msg );
		msgBox.setIcon( QMessageBox::Critical );
		msgBox.exec();

		return;
	}

	ui->statusBar->showMessage( tr("Transmitting ") + QString::number(configFile.length()) + " bytes of configuration information." );

	canusb->sendConfig( configFile );
}

//-------------------------------------------------------------------------------------------------

void MainWindow::on_actionOpen_Connection_triggered()
{
	canusb->open();
	updateCANStatus();
}

void MainWindow::on_actionClose_Connection_triggered()
{
	canusb->close();
	updateCANStatus();
}

void MainWindow::on_actionShow_Monitor_triggered()
{
	canusb->registerReader();
	monitorDialog->show();
}

//-------------------------------------------------------------------------------------------------

void MainWindow::on_MonitorDialogReject()
{
	canusb->unregisterReader();
}

void MainWindow::on_actionSend_NMEA_Test_Sequence_A_triggered()
{
	QByteArray testdata;
	canusb->sendTest( testdata );
}
