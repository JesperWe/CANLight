/*
 * Revision $Rev$
 * By $Author$
 * Date $Date$
 */

#include <assert.h>

#include <QXmlSimpleReader>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>
#include <QByteArray>

#include "ecsManager.h"
#include "ecsManagerApp.h"
#include "systemdescription.h"
#include "ecsEvent.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupGraphic.h"
#include "ecsEvent.h"

SystemDescription::SystemDescription() {}

QString SystemDescription::loadedFileName;

//-------------------------------------------------------------------------------------------------

void SystemDescription::loadFile( QString fromFile ) {

	QFile file( fromFile );
	if (!file.open(QIODevice::ReadOnly) ) return;

	QXmlSimpleReader xmlReader;
	QXmlInputSource *source = new QXmlInputSource( &file );

	SysDescrHandler *handler = new SysDescrHandler();

	xmlReader.setContentHandler(handler);
	xmlReader.setErrorHandler(handler);

	if( ecsManagerApp::inst()->appliances->rowCount() > 0 ) {
		ecsManagerApp::inst()->appliances->removeRows( 0, ecsManagerApp::inst()->appliances->rowCount(), QModelIndex() );
	}

	if( ecsManagerApp::inst()->cGroups->rowCount() > 0 ) {
		ecsManagerApp::inst()->cGroups->removeRows( 0, ecsManagerApp::inst()->cGroups->rowCount(), QModelIndex() );
	}

	bool ok = xmlReader.parse(source);

	loadedFileName = fromFile;
	file.close();

	if (!ok) {
		QMessageBox msgBox;
		msgBox.setText("Parsing failed :-P");
		msgBox.exec();
	}
}

//-------------------------------------------------------------------------------------------------

void SystemDescription::saveFile( QString toFile )
{
	QFile file( toFile );
	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate) ) return;

	QXmlStreamWriter out( &file );
	out.setAutoFormatting( true );
	out.writeStartDocument( "1.0" );
	out.writeStartElement( "systemdescription" );
	out.writeAttribute( "version", QString::number(ecsManagerApp::inst()->systemDescriptionVersion) );

	foreach( ecsControlGroup* appliance, ecsManagerApp::inst()->appliances->ecsControlGroups ) {
		out.writeStartElement( "appliance" );
		out.writeAttribute( "id", QString::number(appliance->id) );
		out.writeAttribute( "description", appliance->description );
		out.writeEndElement();
	}

	// Do all Activity type groups first, since they need to be read before the controllers,
	// so that they can be referenced by <targetgroup>.

	foreach( ecsControlGroup* group, ecsManagerApp::inst()->cGroups->ecsControlGroups ) {
		if( group->itemType != ecsControlGroup::Activity ) continue;
		out.writeStartElement( "controlgroup" );
		out.writeAttribute( "id", QString::number( group->id ) );
		out.writeAttribute( "description", group->description );
		out.writeAttribute( "type", QString::number( group->itemType) );

		foreach( ecsControlGroup* linkedapp,  group->links ) {
			out.writeStartElement( "appliance-function" );
			out.writeAttribute( "id", QString::number( linkedapp->id ) );
			out.writeAttribute( "function", QString::number( group->functions[linkedapp->id] ) );
			out.writeEndElement();
		}
		out.writeEndElement();
	}

	foreach( ecsControlGroup* group, ecsManagerApp::inst()->cGroups->ecsControlGroups ) {
		if( group->itemType != ecsControlGroup::Controller ) continue;
		out.writeStartElement( "controlgroup" );
		out.writeAttribute( "id", QString::number( group->id ) );
		out.writeAttribute( "description", group->description );
		out.writeAttribute( "type", QString::number( group->itemType) );

		foreach( ecsControlGroup* linkedapp,  group->links ) {
			out.writeStartElement( "appliance-function" );
			out.writeAttribute( "id", QString::number( linkedapp->id ) );
			out.writeAttribute( "function", QString::number( group->functions[linkedapp->id] ) );
			out.writeEndElement();
		}

		foreach( ecsEvent* e, group->events ) {
			out.writeStartElement( "controlevent" );
			out.writeAttribute( "type", QString::number(e->eventType) );
			out.writeAttribute( "action", QString::number(e->eventAction->actionType) );

			foreach( ecsControlGroupGraphic* target, e->eventAction->targetGroups ) {
				out.writeStartElement( "targetgroup");
				out.writeAttribute( "id", QString::number(target->srcGroup->id) );

				if( target->parentItem() == e->eventAction )
					out.writeAttribute( "parented", "1" );

				out.writeEndElement();
			}

			out.writeEndElement();
		}
		out.writeEndElement();
	}
	out.writeEndDocument();
	file.close();
}

//-------------------------------------------------------------------------------------------------
// Shuffle the configuration data around a bit to generate the binary config file before sending
// it on the yacht network. See Elecntrl/config.c for the syntax.

#define END_GROUP_APPLIANCES 0xFE
#define END_GROUP_EVENTS 0xFE
#define END_GROUP 0xFE
#define END_OF_FILE 0xFF

void  SystemDescription::buildNMEAConfig( 	QByteArray &configFile ) {

	configFile.clear();

	// Magic word == 4713 = 0x1269
	configFile[0] = 0x12;
	configFile[1] = 0x69;

	configFile[2] = (uint8_t)(ecsManagerApp::inst()->systemDescriptionVersion >> 8);
	configFile[3] = (uint8_t)(ecsManagerApp::inst()->systemDescriptionVersion % 0xFF);

	// Go through all controller groups.

	foreach( ecsControlGroup* cGroup, ecsManagerApp::inst()->cGroups->ecsControlGroups ) {

		if( cGroup->itemType != ecsControlGroup::Controller ) continue;

		// Check that this group is fully configured before adding it.

		if( cGroup->events.count() == 0 ) continue;
		if( cGroup->events[0]->eventAction == 0 ) continue;
		if( cGroup->events[0]->eventAction->targetGroups.count() == 0 ) continue;

		configFile.append( (uint8_t)( cGroup->id )); // Start new group.

		foreach( QGraphicsItem* link, cGroup->graphic->childItems() ) {
			if( link->type() != QGraphicsSimpleTextItem::Type ) continue;
			ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
			int func = cGroup->functions[ linkedApp->id ];
			configFile.append( (uint8_t)( linkedApp->id ));
			configFile.append( (uint8_t)( func ));
		}
		configFile.append( END_GROUP_APPLIANCES );

		// Now find our listening group.
		// XXX Support multiple targets?

		ecsControlGroup* listenGroup = cGroup->events[0]->eventAction->targetGroups[0]->srcGroup;

		configFile.append( (uint8_t)( listenGroup->id )); // Start new group.

		foreach( QGraphicsItem* link, listenGroup->graphic->childItems() ) {
			if( link->type() != QGraphicsSimpleTextItem::Type ) continue;
			ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
			int func = listenGroup->functions[ linkedApp->id ];
			configFile.append( (uint8_t)( linkedApp->id ));
			configFile.append( (uint8_t)( func ));
		}
		configFile.append( END_GROUP_APPLIANCES );

		foreach( ecsEvent* event, cGroup->events ) {
			configFile.append( (uint8_t)( event->eventType ) );
			configFile.append( (uint8_t)( event->eventAction->actionType ) );
		}
		configFile.append( END_GROUP_EVENTS );

		configFile.append( END_GROUP ); // End of the whole group.
	}
	configFile.append( END_OF_FILE );
}

//-------------------------------------------------------------------------------------------------

bool SysDescrHandler::fatalError(const QXmlParseException &exception) {

	QString msg = "Fatal error on line ";
	msg.append(QString::number(exception.lineNumber()));
	msg.append(", column ");
	msg.append(QString::number(exception.columnNumber()));
	msg.append(": ");
	msg.append(exception.message());

	QMessageBox msgBox;
	msgBox.setText( msg );
	msgBox.setIcon( QMessageBox::Critical );
	msgBox.exec();

	return false;
}

//-------------------------------------------------------------------------------------------------

bool SysDescrHandler::startDocument()
{
	return true;
}

//-------------------------------------------------------------------------------------------------

bool SysDescrHandler::endElement( const QString&, const QString&, const QString &name )
{
	return true;
}

//-------------------------------------------------------------------------------------------------

bool SysDescrHandler::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
	QModelIndex index;
	QString attrVal;
	ecsControlGroupModel* appliances = ecsManagerApp::inst()->appliances;
	ecsControlGroupModel* cGroups = ecsManagerApp::inst()->cGroups;

	if( name == "systemdescription" ) {
		int version = 0;
		version = attrs.value( "version" ).toInt();
		ecsManagerApp::inst()->systemDescriptionVersion = version + 1;
	}

	if( name == "appliance" ) {
		index = appliances->insertRow();
		appliances->setData( index, attrs.value("id"), Qt::EditRole );
		index = appliances->index( index.row(), 1, QModelIndex() );
		appliances->setData( index, attrs.value("description"), Qt::EditRole );
	}

	else if( name == "controlgroup" ) {
		index = cGroups->insertRow();
		cGroups->setData( index, attrs.value("id"), Qt::EditRole );
		index = cGroups->index( index.row(), 1, QModelIndex() );
		cGroups->setData( index, attrs.value("description"), Qt::EditRole );
		cGroups->setData( index, attrs.value("type"), Qt::UserRole );

		currentGraphic = cGroups->ecsControlGroups.last()->graphic;
	}

	else if(name == "appliance-function") {
		ecsControlGroup* app = appliances->findItem(attrs.value("id").toInt());
		if( app ) {
			currentGraphic->srcGroup->links.append( app );
			currentGraphic->srcGroup->functions[ app->id ] = attrs.value("function").toInt();
		}
	}

	else if(name == "controlevent") {
		attrVal = attrs.value("type");
		if( attrVal != "" ) {
			currentEvent = new ecsEvent( attrVal.toInt() );
			currentEvent->setParentItem( currentGraphic );
			currentEvent->cGroupId = currentGraphic->srcGroup->id;

			currentGraphic->srcGroup->events.append( currentEvent );
		}
		attrVal = attrs.value("action");
		if( attrVal != "" ) {
			currentEvent->eventAction = new ecsAction( attrVal.toInt() );
			currentEvent->eventAction->setX( ecsManager::ActionOffset_X );
			currentEvent->eventAction->setParentItem( currentEvent );
		}
	}

	else if(name == "targetgroup") {
		attrVal = attrs.value("id");
		if( attrVal != "" ) {
			qDebug() << "Targetgroup " << attrVal;
			currentEvent->eventAction->targetGroups.append( cGroups->findItem( attrVal.toInt() )->graphic );
			if( attrs.value("parented") != "" )
				currentEvent->eventAction->targetGroups.last()->setParentItem(currentEvent->eventAction);
		}

	}
	return true;
}
