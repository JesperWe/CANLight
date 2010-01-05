#include "systemdescription.h"
#include <QXmlSimpleReader>
#include <QMessageBox>
#include <QFile>
#include <QXmlStreamWriter>

SystemDescription::SystemDescription() {}

QString SystemDescription::loadedFileName;

void SystemDescription::loadFile( QString fromFile, NumberedItemModel* appliances, NumberedItemModel* cGroups ) {

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

void SystemDescription::saveFile( QString toFile, NumberedItemModel* appliances, NumberedItemModel* cGroups )
{
    QFile file( toFile );
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate) ) return;

    QXmlStreamWriter out( &file );
    out.setAutoFormatting( true );
    out.writeStartDocument( "1.0" );
    out.writeStartElement( "systemdescription" );

    for( int i=0; i<appliances->rowCount(); i++ ) {
        out.writeStartElement( "appliance" );
        out.writeAttribute( "id", QString::number(appliances->numberedItemData[i].id) );
        out.writeAttribute( "description", appliances->numberedItemData[i].description );
        out.writeEndElement();
    }

    for( int i=0; i<cGroups->rowCount(); i++ ) {
        out.writeStartElement( "controlgroup" );
        out.writeAttribute( "id", QString::number(cGroups->numberedItemData[i].id) );
        out.writeAttribute( "description", cGroups->numberedItemData[i].description );

        for( int j=0; j<cGroups->numberedItemData[i].links.count(); j++) {
            out.writeStartElement( "linked-appliance" );
            out.writeAttribute( "id", QString::number(cGroups->numberedItemData[i].links[j]->id) );
            out.writeEndElement();
        }

        for( int j=0; j<cGroups->numberedItemData[i].events.count(); j++) {
            out.writeStartElement( "controlevent" );
            out.writeAttribute( "type", QString::number(cGroups->numberedItemData[i].events[j]) );
            out.writeEndElement();
        }

        out.writeEndElement();
    }

    out.writeEndDocument();
    file.close();
}

//-------------------------------------------------------------------------------------------------

SysDescrHandler::SysDescrHandler( NumberedItemModel* a, NumberedItemModel* cg ) {
    appliances = a;
    cGroups = cg;
}

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

bool SysDescrHandler::startDocument()
{
    return true;
}
bool SysDescrHandler::endElement( const QString&, const QString&, const QString &name )
{
    return true;
}

bool SysDescrHandler::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    int row;

    if( name == "appliance" ) {
        row = appliances->rowCount();
        appliances->insertRows( row, 1, QModelIndex() );

        QModelIndex index = appliances->index( row, 0, QModelIndex() );
        appliances->setData( index, attrs.value("id"), Qt::EditRole );

        index = appliances->index( row, 1, QModelIndex() );
        appliances->setData( index, attrs.value("description"), Qt::EditRole );
    }

    else if( name == "controlgroup" ) {
        row = cGroups->rowCount();

        cGroups->insertRows( row, 1, QModelIndex() );

        QModelIndex index = cGroups->index( row, 0, QModelIndex() );
        cGroups->setData( index, attrs.value("id"), Qt::EditRole );

        index = cGroups->index( row, 1, QModelIndex() );
        cGroups->setData( index, attrs.value("description"), Qt::EditRole );
    }

    else if(name == "linked-appliance") {
        NumberedItem* app = appliances->findItem(attrs.value("id").toInt());
        if( app ) {
            cGroups->numberedItemData.last().links.append(app);
        }
    }

    return true;
}
