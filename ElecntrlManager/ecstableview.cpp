#include <QtGui>
#include "ecstableview.h"
#include "numberedItemModel.h"

ecsTableView::ecsTableView( QWidget* parent ) : QTableView( parent ) {

    numberedItemContextMenu = new QMenu( this );
    QAction* addnewItemAction = new QAction( tr("Add item"), this );
    numberedItemContextMenu->addAction( addnewItemAction );

    connect( addnewItemAction, SIGNAL(triggered()), this, SLOT(on_addItemAction()) );

    connect( this, SIGNAL( customContextMenuRequested( QPoint )),
            this, SLOT( on_customContextMenuRequested( QPoint )));
}


void ecsTableView::on_customContextMenuRequested( QPoint pos )
{
    QPoint globalPos = this->mapToGlobal( pos );
    numberedItemContextMenu->exec( globalPos );
}

void ecsTableView::on_addItemAction() {
    model()->insertRows( model()->rowCount(), 1, QModelIndex() );
    ((NumberedItemModel*)model())->updateComplete();
    numberedItemContextMenu->hide();
}
