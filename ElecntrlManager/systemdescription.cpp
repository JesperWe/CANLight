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

		int linkNumber = 0;
		foreach( ecsControlGroup* linkedapp,  group->links ) {
			out.writeStartElement( "appliance-function" );
			out.writeAttribute( "id", QString::number( linkedapp->id ) );
			out.writeAttribute( "function", QString::number( group->functions[linkNumber] ) );
			out.writeEndElement();
			linkNumber++;
		}
		out.writeEndElement();
	}

	foreach( ecsControlGroup* group, ecsManagerApp::inst()->cGroups->ecsControlGroups ) {
		if( group->itemType != ecsControlGroup::Controller ) continue;
		out.writeStartElement( "controlgroup" );
		out.writeAttribute( "id", QString::number( group->id ) );
		out.writeAttribute( "description", group->description );
		out.writeAttribute( "type", QString::number( group->itemType) );

		int linkNumber = 0;
		foreach( ecsControlGroup* linkedapp,  group->links ) {
			out.writeStartElement( "appliance-function" );
			out.writeAttribute( "id", QString::number( linkedapp->id ) );
			out.writeAttribute( "function", QString::number( group->functions[linkNumber] ) );
			out.writeEndElement();
			linkNumber++;
		}

		foreach( ecsEvent* e, group->events ) {
			out.writeStartElement( "controlevent" );
			if( e->eventType ) {
				out.writeAttribute( "type", QString::number(e->eventType) );
				if( e->eventAction ) {

					out.writeAttribute( "action", QString::number(e->eventAction->actionType) );

					foreach( ecsControlGroupGraphic* target, e->eventAction->targetGroups ) {
						out.writeStartElement( "targetgroup");
						out.writeAttribute( "id", QString::number(target->srcGroup->id) );

						if( target->parentItem() == e->eventAction )
							out.writeAttribute( "parented", "1" );

						out.writeEndElement();
					}
				}
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

int SystemDescription::buildNMEAConfig( 	QByteArray &configFile ) {

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

                if( cGroup->links.count() == 0 ) continue;
                if( cGroup->events.count() == 0 ) continue;
                if( cGroup->events[0]->eventAction == 0 ) continue;
		if( cGroup->events[0]->eventAction->targetGroups.count() == 0 ) continue;

		configFile.append( (uint8_t)( cGroup->id )); // Start new group.
                qDebug() << "Starting Group " << cGroup->id;

		int linkNumber = 0;
		foreach( QGraphicsItem* link, cGroup->graphic->childItems() ) {
			if( link->type() != QGraphicsSimpleTextItem::Type ) continue;
			ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
			int func = cGroup->functions[ linkNumber ];
			configFile.append( (uint8_t)( linkedApp->id ));
			configFile.append( (uint8_t)( func ));
			linkNumber++;
                        qDebug() << "    Device " << linkedApp->id << " port " << func;
		}
		configFile.append( END_GROUP_APPLIANCES );

                if( linkNumber == 0 ) {
                    // XXX Houston! We have a problem!
                    return -1;
                }
		// Now find our listening group.
		// XXX Support multiple targets?

                ecsControlGroup* taskGroup = cGroup->events[0]->eventAction->targetGroups[0]->srcGroup;

                configFile.append( (uint8_t)( taskGroup->id )); // Start new group.
                qDebug() << "  Task Group " << taskGroup->id;

		linkNumber = 0;
                foreach( QGraphicsItem* link, taskGroup->graphic->childItems() ) {
			if( link->type() != QGraphicsSimpleTextItem::Type ) continue;
			ecsControlGroup* linkedApp = (ecsControlGroup*)(link->data(0).value<void*>());
                        int func = taskGroup->functions[ linkNumber ];
			configFile.append( (uint8_t)( linkedApp->id ));
			configFile.append( (uint8_t)( func ));
			linkNumber++;
                        qDebug() << "    Device " << linkedApp->id << " port " << func;
                }
		configFile.append( END_GROUP_APPLIANCES );

                if( linkNumber == 0 ) {
                    // XXX Houston! We have a problem!
                    return -2;
                }

		foreach( ecsEvent* event, cGroup->events ) {
			configFile.append( (uint8_t)( event->eventType ) );
			configFile.append( (uint8_t)( event->eventAction->actionType ) );
                        qDebug() << "    Event " << event->eventType << " -> " << event->eventAction->actionType;

		}
		configFile.append( END_GROUP_EVENTS );

		configFile.append( END_GROUP ); // End of the whole group.
	}
	configFile.append( END_OF_FILE );
        return 0;
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
			currentGraphic->srcGroup->functions.append( attrs.value("function").toInt() );
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

                    ecsControlGroup* targetGroup = cGroups->findItem( attrVal.toInt() );

                    if( targetGroup->itemType == ecsControlGroup::Activity ) {
                        qDebug() << "Targetgroup " << attrVal;
                        currentEvent->eventAction->targetGroups.append( targetGroup->graphic );
                        if( attrs.value("parented") != "" )
                            currentEvent->eventAction->targetGroups.last()->setParentItem(currentEvent->eventAction);
                    }
		}

	}
	return true;
}
