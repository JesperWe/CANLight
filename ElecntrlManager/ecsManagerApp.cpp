#include "ecsManager.h"
#include "ecsManagerApp.h"

ecsManagerApp::ecsManagerApp() {
	actionIcons[ ecsManager::SwitchON ] = QPixmap(":/graphics/bulb-lit.svg");
	actionIcons[ ecsManager::SwitchOFF ] = QPixmap(":/graphics/bulb.svg");
	actionIcons[ ecsManager::ToggleOnOff ] = QPixmap(":/graphics/onoff.svg");
	actionIcons[ ecsManager::FadeStart ] = QPixmap(":/graphics/fade-start.svg");
	actionIcons[ ecsManager::FadeStop ] = QPixmap(":/graphics/fade-stop.svg");
	actionIcons[ ecsManager::ChangeColor ] = QPixmap(":/graphics/connections.svg");
	actionIcons[ ecsManager::Actuator ] = QPixmap(":/graphics/actuator.svg");

	eventSourceIcons[ ecsManager::Key0 ] = QPixmap(":/graphics/button.svg");
	eventSourceIcons[ ecsManager::Key1 ] = QPixmap(":/graphics/button.svg");
	eventSourceIcons[ ecsManager::Key2 ] = QPixmap(":/graphics/button.svg");
	eventSourceIcons[ ecsManager::AnalogSignal ] = QPixmap(":/graphics/signal.svg");
	eventSourceIcons[ ecsManager::ChangeNotifiation ] = QPixmap(":/graphics/connections.svg");
	eventSourceIcons[ ecsManager::LightRed ] = QPixmap(":/graphics/light-red.svg");
	eventSourceIcons[ ecsManager::LightWhite ] = QPixmap(":/graphics/light-white.svg");
	eventSourceIcons[ ecsManager::LightAll ] = QPixmap(":/graphics/light-both.svg");
	eventSourceIcons[ ecsManager::ActuatorOut ] = QPixmap(":/graphics/cogs.svg");

	eventIcons[ ecsManager::SingleClick ] = QPixmap(":/graphics/click-single.svg");
	eventIcons[ ecsManager::DoubleClick ] = QPixmap(":/graphics/click-double.svg");
	eventIcons[ ecsManager::PressHold ] = QPixmap(":/graphics/click-hold.svg");
	eventIcons[ ecsManager::Release ] = QPixmap(":/graphics/click-release.svg");
	eventIcons[ ecsManager::SignalChange ] = QPixmap(":/graphics/signal.svg");

	statusIcons[ ecsManager::StatusRed ] = QPixmap(":/graphics/icon-red.svg");
	statusIcons[ ecsManager::StatusYellow ] = QPixmap(":/graphics/icon-yellow.svg");
	statusIcons[ ecsManager::StatusBlue ] = QPixmap(":/graphics/icon-blue.svg");
	statusIcons[ ecsManager::StatusGreen ] = QPixmap(":/graphics/icon-green.svg");
	statusIcons[ ecsManager::StatusActive ] = QPixmap(":/graphics/icon-active.svg");

};

ecsManagerApp* ecsManagerApp::inst() {
	static ecsManagerApp* pInstance;
	if( pInstance == 0 )
		pInstance = new ecsManagerApp();
	return pInstance;
};
