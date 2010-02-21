/*
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtGui>
#include <QtSvg>

#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"
#include "ecsCANUSB.h"

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);
	~MainWindow();
	void updateScene();
	ecsControlGroupModel* applianceModel;
	ecsControlGroupModel* cGroupModel;

protected:
	void changeEvent(QEvent *e);
	void readSettings();
	void writeSettings();
	void closeEvent(QCloseEvent *event);

private:
	void _AddEvent( int eventType );
	void _AddAction( int actionType );
	float calculateEventOffset( bool & first, float eventOffset );
	void updateCANStatus();

	Ui::MainWindow *ui;
	QDialog* monitorDialog;
	QGraphicsScene *scene;
	QLabel *NMEAStatusText;
	QFrame *NMEAFrame;
	QSvgWidget *NMEAIcon;
	QHBoxLayout *horiz;
	ecsCANUSB* canusb;

private slots:
	void on_actionShow_Monitor_triggered();
	void on_actionClose_Connection_triggered();
	void on_actionOpen_Connection_triggered();
	void on_actionUpload_to_Yacht_triggered();
	void on_actionRun_Actuator_triggered();
	void on_actionSignal_Change_triggered();
	void on_actionSwitch_Color_triggered();
	void on_actionStop_Fade_triggered();
	void on_actionStart_Fade_triggered();
	void on_actionSwitch_Off_triggered();
	void on_actionSwitch_On_triggered();
	void on_actionToggle_On_Off_triggered();
	void on_actionNew_triggered();
	void on_actionRelease_triggered();
	void on_actionPress_Hold_triggered();
	void on_actionDouble_Click_triggered();
	void on_actionSingle_Click_triggered();
	void on_actionSave_triggered();
	void on_actionSave_As_triggered();
	void on_actionAbout_Qt_triggered();
	void on_actionExit_triggered();
	void on_actionOpen_triggered();
	void on_actionAbout_triggered();

	void on_MonitorDialogReject();

public slots:
	void onModifiedData();
	void onKeypress( int key );
};

#endif // MAINWINDOW_H
