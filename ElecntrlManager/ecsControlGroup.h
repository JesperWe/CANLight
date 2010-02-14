#ifndef ecsControlGroup_H
#define ecsControlGroup_H

#include <QtGui>
#include "ecsControlGroupGraphic.h"

class ecsControlGroup
{

public:

	enum itemTypes_e {
		Unknown,
		Controller,
		Activity,
		Appliance,
		noItemTypes
	};

	ecsControlGroup();

	//--------------------------------------------------------------------------------------

	static bool compareIdsAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareIdsDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareDscrAsc( const ecsControlGroup* a1, const ecsControlGroup* a2 );
	static bool compareDscrDesc( const ecsControlGroup* a1, const ecsControlGroup* a2 );

	QString displayText();
	QVariant typeIcon() const;
	void toggleItemType();
	void zap();

	//--------------------------------------------------------------------------------------

	int             id;
	QString    description;
	int             itemType;

	QList<ecsControlGroup*> links;
	QHash<int, int> functions;
	QList<ecsEvent*> events;
	ecsControlGroupGraphic* graphic;
};

#endif // ecsControlGroup_H
