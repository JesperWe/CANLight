#ifndef ECSMANAGER_H
#define ECSMANAGER_H

#include <QtGUI>
#include <QPixmap>

class ecsManager {

public:

	enum ecsManager_GraphisItemTypes_e {
		Unknown,
		ControlGroup,
		ControlGroupGraphic,
		Appliance,
		Event,
		Action
	};

	enum actionType_e {
		NoAction,
		SwitchON,
		SwitchOFF,
		ToggleOnOff,
		FadeStart,
		FadeStop,
		ChangeColor,
		Actuator,
		noActionTypes
	};

	static const float GraphicsDebug  = false;

	static const float GroupNameFontSize = 9;
	static const float GroupNameOffset = 4;
	static const float GroupSpacing = 5;

	static const float GroupChildMinimumWidth = 60;
	static const float ApplianceLineSpacing = 17;
	static const float CtrlFunctionIconWidth = 20;
	static const float CtrlButtonSize = 15;

	static const float EventSpacing = 40;
	static const float EventIconSize = 24;
	static const float EventOffset_X = 80;
	static const float EventOffset_Y = 40;

	static const float ActionIconSize = 40;
	static const float ActionOffset_X = 80;

	static const float TargetGroupOffset_X = 100;
};

#endif // ECSMANAGER_H
