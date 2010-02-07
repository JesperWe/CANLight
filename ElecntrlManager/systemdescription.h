#ifndef SYSTEMDESCRIPTION_H
#define SYSTEMDESCRIPTION_H

#include <QString>
#include <QList>
#include <QXmlSimpleReader>

#include "numberedItemModel.h"

class SystemDescription {

public:
	SystemDescription();
	static QString loadedFileName;
	static void loadFile( QString fromFile, NumberedItemModel* appliances, NumberedItemModel* cGroups  );
	static void saveFile( QString toFile, NumberedItemModel* appliances, NumberedItemModel* cGroups );
};

class SysDescrHandler : public QXmlDefaultHandler {
public:
	SysDescrHandler( NumberedItemModel* appliances, NumberedItemModel* cGroups );
	bool fatalError (const QXmlParseException & exception);
	bool startDocument();
	bool endElement( const QString&, const QString&, const QString &name );
	bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );

private:
	NumberedItemModel* appliances;
	NumberedItemModel* cGroups;
	NumberedItem* currentGroup;
	ecsEvent* currentEvent;
};

#endif // SYSTEMDESCRIPTION_H
