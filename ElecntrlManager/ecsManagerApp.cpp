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
};

ecsManagerApp* ecsManagerApp::inst() {
	static ecsManagerApp* pInstance;
	if( pInstance == 0 )
		pInstance = new ecsManagerApp();
	return pInstance;
};
