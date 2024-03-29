/*
 * Revision $Rev$
 * By $Author$
 * Date $Date$
 */

 #include <QIcon>

#include "ecsManager.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"
#include "mainwindow.h"
#include "ecsEvent.h"

//--------------------------------------------------------------------------------------
// Constructors

ecsControlGroup::ecsControlGroup()  {
	id = 0;
	description = "N/A";
	itemType = ecsControlGroup::Controller;
	qDebug() << "Create ControlGroup (default)";
	graphic = new ecsControlGroupGraphic( this );
}

//--------------------------------------------------------------------------------------

bool ecsControlGroup::compareIdsAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->id < a2->id; }
bool ecsControlGroup::compareIdsDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->id > a2->id; }
bool ecsControlGroup::compareDscrAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->description < a2->description; }
bool ecsControlGroup::compareDscrDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 ) { return a1->description > a2->description; }

QVariant ecsControlGroup::typeIcon() const {
	switch( itemType ) {
	case ecsControlGroup::Controller: { return QIcon(":/graphics/finger.svg"); }
	case ecsControlGroup::Activity: { return QIcon(":/graphics/flash.svg"); }
	case ecsControlGroup::Appliance: { return QIcon(":/graphics/appliance.svg"); }
	}
	return QVariant();
}

//------------------------------------------------------------------------------------

QString ecsControlGroup::displayText() {
	QString idStr;
	if( id == -1 ) idStr = "Any - ";
	else idStr = QString::number( id ) + " - ";
	return idStr + description;
}

//------------------------------------------------------------------------------------

void ecsControlGroup::toggleItemType() {

	switch( itemType ) {

	case ecsControlGroup::Controller: {
			itemType = ecsControlGroup::Activity;
			if( graphic->scene() )
				graphic->scene()->removeItem( graphic );
			foreach( ecsEvent* event, events ) {
				event->zap();
			}
			break;
		}

	case ecsControlGroup::Activity: {
			itemType = ecsControlGroup::Controller;
			graphic = new ecsControlGroupGraphic( this );
			break;
		}
	}
	events.clear();
	controllers.clear();
}

//------------------------------------------------------------------------------------

void ecsControlGroup::zap() {

	if( itemType == ecsControlGroup::Activity &&  graphic->scene() ) {
		foreach( QGraphicsItem* item, graphic->scene()->items() ) {
			if( ecsAction* action = qgraphicsitem_cast<ecsAction *>( item ) ) {
				foreach( ecsControlGroupGraphic* target, action->targetGroups ) {
					if( target->srcGroup == this ) {
						action->targetGroups.removeAt( action->targetGroups.indexOf( target ) );
					}
				}
			}
		}
	}

	foreach( QGraphicsItem* child, graphic->childItems() ) {

		if( ecsEvent* event = qgraphicsitem_cast<ecsEvent *>( child ) ) event->zap();

		else {
			child->scene()->removeItem( child );
			delete child;
		}
	}

	if( graphic->scene() ) {
		graphic->scene()->removeItem( graphic );
	}

	delete graphic;
}
