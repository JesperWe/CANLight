#include <assert.h>

#include <QXmlSimpleReader>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>

#include "systemdescription.h"
#include "ecsEvent.h"
#include "ecsControlGroup.h"
#include "ecsControlGroupGraphic.h"
#include "ecsEvent.h"

SystemDescription::SystemDescription() {}

QString SystemDescription::loadedFileName;

//-------------------------------------------------------------------------------------------------

void SystemDescription::loadFile( QString fromFile, ecsControlGroupModel* appliances, ecsControlGroupModel* cGroups ) {

	QFile file( fromFile );
	if (!file.open(QIODevice::ReadOnly) ) return;

	QXmlSimpleReader xmlReader;
	QXmlInputSource *source = new QXmlInputSource( &file );

	SysDescrHandler *handler = new SysDescrHandler( appliances, cGroups );

	xmlReader.setContentHandler(handler);
	xmlReader.setErrorHandler(handler);

	if( appliances->rowCount() > 0 ) {
		appliances->removeRows( 0, appliances->rowCount(), QModelIndex() );
	}

	if( cGroups->rowCount() > 0 ) {
		cGroups->removeRows( 0, cGroups->rowCount(), QModelIndex() );
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

void SystemDescription::saveFile( QString toFile, ecsControlGroupModel* appliances, ecsControlGroupModel* cGroups )
{
	QFile file( toFile );
	if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate) ) return;

	QXmlStreamWriter out( &file );
	out.setAutoFormatting( true );
	out.writeStartDocument( "1.0" );
	out.writeStartElement( "systemdescription" );

	foreach( ecsControlGroup* appliance, appliances->ecsControlGroups ) {
		out.writeStartElement( "appliance" );
		out.writeAttribute( "id", QString::number(appliance->id) );
		out.writeAttribute( "description", appliance->description );
		out.writeEndElement();
	}

	foreach( ecsControlGroup* group, cGroups->ecsControlGroups ) {
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

SysDescrHandler::SysDescrHandler( ecsControlGroupModel* a, ecsControlGroupModel* cg ) {
	appliances = a;
	cGroups = cg;
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
			currentEvent->eventAction->targetGroups.append( cGroups->findItem( attrVal.toInt() )->graphic );
		}

	}
	return true;
}
