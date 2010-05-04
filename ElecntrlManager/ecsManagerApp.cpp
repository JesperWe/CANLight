/*
 * $Revision$
 * $Author$
 * $Date$
 */

#include "ecsManager.h"
#include "ecsManagerApp.h"

ecsManagerApp::ecsManagerApp() {
    actionIcons[ ecsManager::a_SWITCH_ON ] = QPixmap(":/graphics/bulb-lit.svg");
    actionIcons[ ecsManager::a_SWITCH_OFF ] = QPixmap(":/graphics/bulb.svg");
    actionIcons[ ecsManager::a_TOGGLE_STATE ] = QPixmap(":/graphics/onoff.svg");
    actionIcons[ ecsManager::a_START_FADE ] = QPixmap(":/graphics/fade-start.svg");
    actionIcons[ ecsManager::a_STOP_FADE ] = QPixmap(":/graphics/fade-stop.svg");
    actionIcons[ ecsManager::a_CHANGE_COLOR ] = QPixmap(":/graphics/connections.svg");
    actionIcons[ ecsManager::a_SET_LEVEL ] = QPixmap(":/graphics/actuator.svg");
    actionIcons[ ecsManager::a_ON_TIMER ] = QPixmap(":/graphics/timer.svg");
    actionIcons[ ecsManager::a_GOTO_MINIMUM ] = QPixmap(":/graphics/minimum.svg");

    eventSourceIcons[ ecsManager::hw_KEY1 ] = QImage(":/graphics/button.svg");
    eventSourceIcons[ ecsManager::hw_KEY2 ] = QImage(":/graphics/button.svg");
    eventSourceIcons[ ecsManager::hw_KEY3 ] = QImage(":/graphics/button.svg");
    eventSourceIcons[ ecsManager::hw_ANALOG ] = QImage(":/graphics/signal.svg");
    eventSourceIcons[ ecsManager::hw_DIGITAL_IN ] = QImage(":/graphics/connections.svg");
    eventSourceIcons[ ecsManager::hw_LED_RED ] = QImage(":/graphics/light-red.svg");
    eventSourceIcons[ ecsManager::hw_LED_WHITE ] = QImage(":/graphics/light-white.svg");
    eventSourceIcons[ ecsManager::hw_LED_LIGHT ] = QImage(":/graphics/light-both.svg");
    eventSourceIcons[ ecsManager::hw_PWM1 ] = QImage(":/graphics/cogs.svg");
    eventSourceIcons[ ecsManager::hw_SWITCH1 ] = QImage(":/graphics/outlet.svg");
    eventSourceIcons[ ecsManager::hw_SWITCH2 ] = QImage(":/graphics/outlet.svg");
    eventSourceIcons[ ecsManager::hw_SWITCH3 ] = QImage(":/graphics/outlet.svg");
    eventSourceIcons[ ecsManager::hw_SWITCH4 ] = QImage(":/graphics/outlet.svg");

    eventIcons[ ecsManager::e_KEY_CLICKED ] = QPixmap(":/graphics/click-single.svg");
    eventIcons[ ecsManager::e_KEY_DOUBLECLICKED ] = QPixmap(":/graphics/click-double.svg");
    eventIcons[ ecsManager::e_KEY_TRIPLECLICKED ] = QPixmap(":/graphics/click-triple.svg");
    eventIcons[ ecsManager::e_KEY_HOLDING ] = QPixmap(":/graphics/click-hold.svg");
    eventIcons[ ecsManager::e_KEY_RELEASED ] = QPixmap(":/graphics/click-release.svg");
    eventIcons[ ecsManager::e_LEVEL_CHANGED ] = QPixmap(":/graphics/signal.svg");

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
