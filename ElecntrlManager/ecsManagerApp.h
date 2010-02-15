#ifndef ECSMANAGERAPP_H
#define ECSMANAGERAPP_H

#include <QHash>
#include <QPixmap>

class ecsManagerApp
{
public:
	QHash<int, QPixmap> actionIcons;
	QHash<int, QPixmap> eventSourceIcons;

	// Singelton pattern

	ecsManagerApp();
	static ecsManagerApp* inst();

private:
	ecsManagerApp* pInstance;
};
#endif // ECSMANAGERAPP_H
