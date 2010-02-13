#ifndef ecsControlGroup_H
#define ecsControlGroup_H

#include <QtGui>
#include "ecsManager.h"
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
	QGraphicsSimpleTextItem* appendLinkedAppliance( ecsControlGroup* appliance );
	void toggleItemType();

	//--------------------------------------------------------------------------------------

	int             id;
	QString    description;
	int             itemType;

	QList<QGraphicsSimpleTextItem*> links; // data[0] is pointer to appliance item. data[1] is ctrlFunction.
	QList<ecsEvent*> events;
	ecsControlGroupGraphic* graphic;

};

#endif // ecsControlGroup_H
