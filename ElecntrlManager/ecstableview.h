#ifndef ECSTABLEVIEW_H
#define ECSTABLEVIEW_H

#include <QTableView>

class ecsTableView : public QTableView
{
    Q_OBJECT

private:
    QMenu* numberedItemContextMenu;

public:
    ecsTableView() : QTableView() {};
    ecsTableView( QWidget* parent );

signals:
    void modified();

private slots:
    void on_customContextMenuRequested( QPoint pos );
    void on_addItemAction();
    void on_changeItemAction();
};

#endif // ECSTABLEVIEW_H
