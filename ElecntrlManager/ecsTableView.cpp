#include <QtGui>

#include "ecsManager.h"
#include "ecsTableView.h"
#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"
#include "ecsEvent.h"

ecsTableView::ecsTableView( QWidget* parent ) : QTableView( parent ) {

	ecsControlGroupContextMenu = new QMenu( this );

	QAction* addnewItemAction = new QAction( tr("Add item"), this );
	ecsControlGroupContextMenu->addAction( addnewItemAction );

	QAction* changeItemAction = new QAction( tr("Toggle Controller/Activity"), this );
	ecsControlGroupContextMenu->addAction( changeItemAction );

	QAction* deleteItemAction = new QAction( tr("Delete Item"), this );
	ecsControlGroupContextMenu->addAction( deleteItemAction );

	connect( addnewItemAction, SIGNAL(triggered()), this, SLOT(on_addItemAction()) );
	connect( changeItemAction, SIGNAL(triggered()), this, SLOT(on_changeItemAction()) );
	connect( deleteItemAction, SIGNAL(triggered()), this, SLOT(on_deleteItemAction()) );

	connect( this, SIGNAL( customContextMenuRequested( QPoint )),
			this, SLOT( on_customContextMenuRequested( QPoint )));
}


void ecsTableView::on_customContextMenuRequested( QPoint pos )
{
	QPoint globalPos = this->mapToGlobal( pos );

	ecsControlGroupContextMenu->actions()[1]->setEnabled( false );

	if( objectName() == "cGroupView"  && rowAt( pos.y() ) >= 0 ) {
		ecsControlGroupContextMenu->actions()[1]->setEnabled(true);
	}

	ecsControlGroupContextMenu->exec( globalPos );
}

void ecsTableView::on_addItemAction() {
	model()->insertRows( model()->rowCount(), 1, QModelIndex() );
	((ecsControlGroupModel*)model())->updateComplete();
	ecsControlGroupContextMenu->hide();
}

void ecsTableView::on_changeItemAction() {
	model()->setData( selectedIndexes()[0], -1, Qt::UserRole ); // XXX Handle multiple seletion.
	ecsControlGroupContextMenu->hide();
}

void ecsTableView::on_deleteItemAction() {
	model()->removeRows( selectedIndexes()[0].row(), 1, QModelIndex() );
	ecsControlGroupContextMenu->hide();
}
