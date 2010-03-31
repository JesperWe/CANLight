/*
 * $Revision$
 * $Author$
 * $Date$
 */

 #ifndef ECSMANAGERAPP_H
#define ECSMANAGERAPP_H

#include <QHash>
#include <QPixmap>

#include "ecsControlGroupModel.h"
#include "ecsCANUSB.h"

class ecsManagerApp
{
public:
	QHash<int, QPixmap> actionIcons;
	QHash<int, QPixmap> eventIcons;
	QHash<int, QImage> eventSourceIcons;
	QHash<int, QPixmap> statusIcons;

	// Singelton pattern

	ecsManagerApp();
	static ecsManagerApp* inst();

	int systemDescriptionVersion;
	ecsCANUSB* canusb_Instance;
	ecsControlGroupModel* appliances;
	ecsControlGroupModel* cGroups;

private:
	ecsManagerApp* pInstance;
};
#endif // ECSMANAGERAPP_H
