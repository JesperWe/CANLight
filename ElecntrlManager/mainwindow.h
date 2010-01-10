#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include "numberedItemModel.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void updateScene();
    NumberedItemModel* applianceModel;
    NumberedItemModel* cGroupModel;

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QGraphicsScene *scene;

private slots:
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
    void on_action_something_triggered();
    void on_actionExit_triggered();
    void on_actionOpen_triggered();
    void on_actionAbout_triggered();

public slots:
    void on_modifiedData();
};

#endif // MAINWINDOW_H
