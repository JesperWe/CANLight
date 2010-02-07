#include <QtGui>
#include "ecsTableView.h"
#include "ecsEvent.h"
#include "numberedItemModel.h"

ecsTableView::ecsTableView( QWidget* parent ) : QTableView( parent ) {

	numberedItemContextMenu = new QMenu( this );

	QAction* addnewItemAction = new QAction( tr("Add item"), this );
	numberedItemContextMenu->addAction( addnewItemAction );

	QAction* changeItemAction = new QAction( tr("Toggle Controller/Activity"), this );
	numberedItemContextMenu->addAction( changeItemAction );

	connect( addnewItemAction, SIGNAL(triggered()), this, SLOT(on_addItemAction()) );
	connect( changeItemAction, SIGNAL(triggered()), this, SLOT(on_changeItemAction()) );

	connect( this, SIGNAL( customContextMenuRequested( QPoint )),
			this, SLOT( on_customContextMenuRequested( QPoint )));
}


void ecsTableView::on_customContextMenuRequested( QPoint pos )
{
	QPoint globalPos = this->mapToGlobal( pos );

	numberedItemContextMenu->actions()[1]->setEnabled( false );

	if( objectName() == "cGroupView"  && rowAt( pos.y() ) >= 0 ) {
		numberedItemContextMenu->actions()[1]->setEnabled(true);
	}

	numberedItemContextMenu->exec( globalPos );
}

void ecsTableView::on_addItemAction() {
	model()->insertRows( model()->rowCount(), 1, QModelIndex() );
	((NumberedItemModel*)model())->updateComplete();
	numberedItemContextMenu->hide();
}

void ecsTableView::on_changeItemAction() {
	model()->setData( this->selectedIndexes()[0], -1, Qt::UserRole ); // XXX Handle multiple seletion.
	numberedItemContextMenu->hide();
}
