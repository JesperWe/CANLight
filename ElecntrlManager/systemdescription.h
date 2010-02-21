/*
 * Revision $Rev$
 * By $Author$
 * Date $Date$
 */

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
	static void loadFile( QString fromFile );
	static void saveFile( QString toFile );
	void buildNMEAConfig( QByteArray &configFile );
};

class SysDescrHandler : public QXmlDefaultHandler {
public:
	bool fatalError (const QXmlParseException & exception);
	bool startDocument();
	bool endElement( const QString&, const QString&, const QString &name );
	bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );

private:
	ecsControlGroupGraphic* currentGraphic;
	ecsEvent* currentEvent;
};

#endif // SYSTEMDESCRIPTION_H
