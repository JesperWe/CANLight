#include "ecsManager.h"
#include "ecsManagerApp.h"

ecsManagerApp::ecsManagerApp() {
	icons[ ecsManager::SwitchON ] = QPixmap(":/graphics/bulb-lit.svg");
	icons[ ecsManager::SwitchOFF ] = QPixmap(":/graphics/bulb.svg");
	icons[ ecsManager::ToggleOnOff ] = QPixmap(":/graphics/onoff.svg");
	icons[ ecsManager::FadeStart ] = QPixmap(":/graphics/fade-start.svg");
	icons[ ecsManager::FadeStop ] = QPixmap(":/graphics/fade-stop.svg");
	icons[ ecsManager::ChangeColor ] = QPixmap(":/graphics/connections.svg");
	icons[ ecsManager::Actuator ] = QPixmap(":/graphics/actuator.svg");
};

ecsManagerApp* ecsManagerApp::inst() {
	static ecsManagerApp* pInstance;
	if( pInstance == 0 )
		pInstance = new ecsManagerApp();
	return pInstance;
};
