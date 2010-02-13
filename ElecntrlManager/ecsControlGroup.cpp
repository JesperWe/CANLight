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
	graphic = new ecsControlGroupGraphic();
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
	return QString::number( id ) + " - " + description;
}

//------------------------------------------------------------------------------------

void ecsControlGroup::toggleItemType() {
	switch( itemType ) {
	case ecsControlGroup::Controller: {
			itemType = ecsControlGroup::Activity;
			graphic->scene()->removeItem( graphic );
			delete graphic;
			break;
		}
	case ecsControlGroup::Activity: {
			itemType = ecsControlGroup::Controller;
			graphic = new ecsControlGroupGraphic( this );
			break;
		}
	}
}
