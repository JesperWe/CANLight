#ifndef ECSMANAGER_H
#define ECSMANAGER_H

#include <QtGUI>

class ecsManager {

public:

	enum ecsManager_GraphisItemTypes_e {
		Unknown,
		ControlGroup,
		Appliance,
		Event,
		Action
	};

	static const float GroupChildMinimumWidth = 60;
	static const float ApplianceLineSpacing = 20;
	static const float CtrlFunctionIconWidth = 30;
	static const float CtrlButtonSize = 17;

	static const float EventSpacing = 60;
	static const float EventIconSize = 30;
	static const float EventOffset_X = 120;
	static const float EventOffset_Y = 60;

	static const float ActionSize = 60;
	static const float ActionOffset_X = 100;

	static const float TargetGroupOffset_X = 150;
};

#endif // ECSMANAGER_H
