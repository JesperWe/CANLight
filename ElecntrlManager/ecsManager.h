/*
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef ECSMANAGER_H
#define ECSMANAGER_H

#include <QtGUI>
#include <QPixmap>

class ecsManager {

public:

	enum ecsManager_GraphisItemTypes_e {
		UnknownItemType,
		ControlGroup,
		ControlGroupGraphic,
		Appliance,
		Event,
		Action
	};

	enum eventTypes_e {
		/* 00 */ e_UNKNOWN,
		/* 01 */ e_KEY_CLICKED,
		/* 02 */ e_KEY_HOLDING,
		/* 03 */ e_KEY_RELEASED,
		/* 04 */ e_KEY_DOUBLECLICKED,
		/* 05 */ e_KEY_TRIPLECLICKED,
		/* 06 */ e_SWITCH_ON,
		/* 07 */ e_SWITCH_OFF,
		/* 08 */ e_SWITCH_FAIL,
		/* 09 */ e_FADE_START,
		/* 10 */ e_FADE_MASTER,
		/* 11 */ e_FAST_HEARTBEAT,
		/* 12 */ e_unused,
		/* 13 */ e_NIGHTMODE,
		/* 14 */ e_DAYLIGHTMODE,
		/* 15 */ e_AMBIENT_LIGHT_LEVEL,
		/* 16 */ e_BLACKOUT,
		/* 17 */ e_SLOW_HEARTBEAT,
		/* 18 */ e_THROTTLE_MASTER,
		/* 19 */ e_LEVEL_CHANGED,
		/* 20 */ e_CONFIG_FILE_UPDATE,
		/* 21 */ e_SET_BACKLIGHT_LEVEL,
		/* 22 */ e_NO_EVENTS
	};

	enum actionType_e {
		/* 00 */ a_SWITCH_ON,
		/* 01 */ a_SWITCH_OFF,
		/* 02 */ a_START_FADE,
		/* 03 */ a_STOP_FADE,
		/* 04 */ a_SET_LEVEL,
		/* 05 */ a_TOGGLE_STATE,
		/* 06 */ a_CHANGE_COLOR,
		/* 07 */ a_SET_FADE_MASTER,
		/* 08 */ a_GOTO_MINIMUM,
		/* 09 */ a_ON_TIMER,
		/* 10 */ a_NO_ACTION
	};

	enum eventSources_e {
		/* 00 */ hw_UNKNOWN,
		/* 01 */ hw_CAN_RATE,
		/* 02 */ hw_CAN_EN,
		/* 03 */ hw_LED_RED,
		/* 04 */ hw_LED_WHITE,
		/* 05 */ hw_LED1,
		/* 06 */ hw_LED2,
		/* 07 */ hw_LED3,
		/* 08 */ hw_SWITCH1,
		/* 09 */ hw_SWITCH2,
		/* 10 */ hw_SWITCH3,
		/* 11 */ hw_SWITCH4,
		/* 12 */ hw_KEY1,
		/* 13 */ hw_KEY2,
		/* 14 */ hw_KEY3,
		/* 15 */ hw_LED_LIGHT,	// Composite port RED+WHITE
		/* 16 */ hw_BACKLIGHT,	// Virtual Port.
		/* 17 */ hw_ANALOG,
		/* 18 */ hw_DIGITAL_IN,
		/* 19 */ hw_PWM1,
		/* 20 */ hw_PWM2,
                /* 21 */ hw_NoFunctions
	};

	enum statuses_e  {
		StatusRed,
		StatusYellow,
		StatusBlue,
		StatusGreen,
		StatusActive
	};

	static const float GraphicsDebug  = false;

	static const float GroupNameFontSize = 9;
	static const float GroupNameOffset = 4;
	static const float GroupSpacing = 5;

	static const float GroupChildMinimumWidth = 60;
	static const float ApplianceLineSpacing = 17;
	static const float CtrlFunctionIconWidth = 20;
	static const float CtrlButtonSize = 16;

	static const float EventSpacing = 40;
	static const float EventIconSize = 24;
	static const float EventOffset_X = 80;
	static const float EventOffset_Y = 40;

	static const float ActionIconSize = 40;
	static const float ActionOffset_X = 80;

	static const float TargetGroupOffset_X = 100;
};

#endif // ECSMANAGER_H
