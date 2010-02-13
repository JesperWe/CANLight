#ifndef SYSTEMDESCRIPTION_H
#define SYSTEMDESCRIPTION_H

#include <QString>
#include <QList>
#include <QXmlSimpleReader>

#include "ecsControlGroupGraphic.h"
#include "ecsControlGroupModel.h"

class SystemDescription {

public:
	SystemDescription();
	static QString loadedFileName;
	static void loadFile( QString fromFile, ecsControlGroupModel* appliances, ecsControlGroupModel* cGroups  );
	static void saveFile( QString toFile, ecsControlGroupModel* appliances, ecsControlGroupModel* cGroups );
};

class SysDescrHandler : public QXmlDefaultHandler {
public:
	SysDescrHandler( ecsControlGroupModel* appliances, ecsControlGroupModel* cGroups );
	bool fatalError (const QXmlParseException & exception);
	bool startDocument();
	bool endElement( const QString&, const QString&, const QString &name );
	bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );

private:
	ecsControlGroupModel* appliances;
	ecsControlGroupModel* cGroups;
	ecsControlGroupGraphic* currentProxy;
	ecsEvent* currentEvent;
};

#endif // SYSTEMDESCRIPTION_H
