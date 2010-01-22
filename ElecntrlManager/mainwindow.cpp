#include <Qt>
#include <QGraphicsSvgItem>
#include <QFileDialog>
#include <QPen>
#include <QGraphicsView>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_about.h"
#include "systemdescription.h"
#include "cGroupItem.h"
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

void MainWindow::updateScene() {
    cGroupItem* gItem;

    scene->clear();

    for( int cGroupNo=0; cGroupNo<cGroupModel->numberedItemData.count(); cGroupNo++ ) {
        gItem = new cGroupItem( cGroupModel, cGroupNo );
        gItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
        scene->addItem( gItem );

        float offset = cGroupModel->accumulatedOffset(cGroupNo);
        float myHeight = cGroupModel->calculateHeight(cGroupNo);
        float midpoint = offset + 0.5*myHeight;

        gItem->setPos( 0, midpoint );

        for( int eventNo=0; eventNo<cGroupModel->numberedItemData[cGroupNo].events.count(); eventNo++ ) {

            float eventOffset = (eventNo%2==0) ? -1 : +1; // Alternate pos/neg offset
            eventOffset *= floor((eventNo+1)/2) * 60;
            qDebug() << "      eventOffset " << eventOffset;

            float xp = gItem->maxChildWidth + 120;
            ecsEvent* eItem = new ecsEvent( gItem->itemIndex, cGroupModel->numberedItemData[cGroupNo].events[eventNo] );
            eItem->eventIndex = eventNo;

            eItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
            eItem->setPos( xp, midpoint + eventOffset );
            scene->addItem( eItem );

            QGraphicsLineItem* line = new QGraphicsLineItem(
                            gItem->anchorOut().x(), gItem->anchorOut().y(),
                            eItem->anchorIn().x(), eItem->anchorIn().y(),
                            0, 0);
            line->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
            scene->addItem( line );

            if( cGroupModel->numberedItemData[cGroupNo].actions[eventNo] != ecsAction::None ) {
                xp += 100;
                ecsAction* aItem = new ecsAction( cGroupNo, cGroupModel->numberedItemData[cGroupNo].actions[eventNo] );
                aItem->eventIndex = eventNo;
                aItem->setFlag(QGraphicsItem::ItemIsSelectable, true);
                aItem->setPos( xp, midpoint + eventOffset );
                scene->addItem( aItem );

                line = new QGraphicsLineItem(
                                eItem->anchorOut().x(), eItem->anchorOut().y(),
                                aItem->anchorIn().x(), aItem->anchorIn().y(),
                                0, 0);
                line->setPen( qApp->property( "cGroupPen" ).value<QPen>() );
                scene->addItem( line );
            }
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    this->close();
}

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

void MainWindow::on_actionSingle_Click_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != cGroupItem::Type ) return;
    cGroupItem* cgi = qgraphicsitem_cast<cGroupItem *>(selection[0]);
    cGroupModel->numberedItemData[cgi->itemIndex].events.append( ecsEvent::SingleClick );
    cGroupModel->numberedItemData[cgi->itemIndex].actions.append( ecsAction::None );
    cGroupModel->numberedItemData[cgi->itemIndex].targetGroups.append( -1 );
    updateScene();
}

void MainWindow::on_actionDouble_Click_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != cGroupItem::Type ) return;
    cGroupItem* cgi = qgraphicsitem_cast<cGroupItem *>(selection[0]);
    cGroupModel->numberedItemData[cgi->itemIndex].events.append( ecsEvent::DoubleClick );
    cGroupModel->numberedItemData[cgi->itemIndex].actions.append( ecsAction::None );
    cGroupModel->numberedItemData[cgi->itemIndex].targetGroups.append( -1 );
    updateScene();
}

void MainWindow::on_actionPress_Hold_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != cGroupItem::Type ) return;
    cGroupItem* cgi = qgraphicsitem_cast<cGroupItem *>(selection[0]);
    cGroupModel->numberedItemData[cgi->itemIndex].events.append( ecsEvent::PressHold );
    cGroupModel->numberedItemData[cgi->itemIndex].actions.append( ecsAction::None );
    cGroupModel->numberedItemData[cgi->itemIndex].targetGroups.append( -1 );
    updateScene();
}

void MainWindow::on_actionRelease_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != cGroupItem::Type ) return;
    cGroupItem* cgi = qgraphicsitem_cast<cGroupItem *>(selection[0]);
    cGroupModel->numberedItemData[cgi->itemIndex].events.append( ecsEvent::Release );
    cGroupModel->numberedItemData[cgi->itemIndex].actions.append( ecsAction::None );
    cGroupModel->numberedItemData[cgi->itemIndex].targetGroups.append( -1 );
    updateScene();
}

void MainWindow::on_actionNew_triggered()
{
    scene->clear();
    cGroupModel->clear();
    applianceModel->clear();
}


void MainWindow::on_actionToggle_On_Off_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::ToggleOnOff;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::ToggleOnOff;

    updateScene();
}


void MainWindow::on_actionSwitch_On_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::SwitchON;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::SwitchON;
    updateScene();
}

void MainWindow::on_actionSwitch_Off_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::SwitchOFF;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::SwitchOFF;
    updateScene();
}

void MainWindow::on_actionStart_Fade_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::FadeStart;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::FadeStart;
    updateScene();
}

void MainWindow::on_actionStop_Fade_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::FadeStop;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::FadeStop;
    updateScene();
}

void MainWindow::on_actionSwitch_Color_triggered()
{
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != ecsEvent::Type ) return;
    ecsEvent* evt = qgraphicsitem_cast<ecsEvent *>(selection[0]);

    evt->eventAction = ecsAction::ChangeColor;
    cGroupModel->numberedItemData[evt->cGroupIndex].actions[evt->eventIndex] = ecsAction::ChangeColor;
    updateScene();
}

void MainWindow::on_keypress(QString key) {
    QList<QGraphicsItem*> selection = this->ui->graphicsView->scene()->selectedItems();
    if( selection.count() != 1 ) return;
    if( selection[0]->type() != QGraphicsSimpleTextItem::Type ) return;
    QGraphicsSimpleTextItem* txt = qgraphicsitem_cast<QGraphicsSimpleTextItem *>(selection[0]);

    int cgIndex = txt->data(0).toInt();
    int childIndex = txt->data(1).toInt();

    int func = 0;
    if( key == "1" ) func = ecsEvent::Key0;
    else if( key == "2" ) func = ecsEvent::Key1;
    else if( key == "3" ) func = ecsEvent::Key2;
    else if( key == "a" ) func = ecsEvent::AnalogSignal;
    else if( key == "i" ) func = ecsEvent::ChangeNotifiation;

    cGroupModel->numberedItemData[cgIndex].ctrlFunctions[childIndex] = func;

    txt->setSelected( false );
}
