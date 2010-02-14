#ifndef ECSMANAGERAPP_H
#define ECSMANAGERAPP_H

#include <QHash>
#include <QPixmap>

class ecsManagerApp
{
public:
	QHash<int, QPixmap> icons;

	// Singelton pattern

	ecsManagerApp();
	static ecsManagerApp* inst();

private:
	ecsManagerApp* pInstance;
};
#endif // ECSMANAGERAPP_H
