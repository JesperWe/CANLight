#ifndef ECSMANAGERAPP_H
#define ECSMANAGERAPP_H

#include <QHash>
#include <QPixmap>

class ecsManagerApp
{
public:
	QHash<int, QPixmap> actionIcons;
	QHash<int, QPixmap> eventIcons;
	QHash<int, QPixmap> eventSourceIcons;
	QHash<int, QPixmap> statusIcons;

	// Singelton pattern

	ecsManagerApp();
	static ecsManagerApp* inst();

	int systemDescriptionVersion;

private:
	ecsManagerApp* pInstance;
};
#endif // ECSMANAGERAPP_H
