#ifndef CUSTOM_DATA_TYPES_H
#define CUSTOM_DATA_TYPES_H

#include "config.h"
#include <Arduino.h>


#ifdef INCLUDE_WEATHER
#include "Weather.h"

typedef struct weatherData{
    int8_t temperature;
    int16_t weatherConditionCode;
}weatherData;
#endif

enum battStatus_t   { 
                    normalBatt = 0b00,      // running on battery
                    acPower = 0b01,         // plugged in but 100% charged
                    chargingBatt = 0b11,

                    lowBatt,
                    criticalBatt
                    };


// ID for each app on Watchy. Append apps to this list when installed.
// Also append apps to _appLauncherHandler function so it can be called.
enum appID_t { 
                none_appState = -1,
                watchFaceState,

                // menus
                mainMenuState,
                clockMenuState,

                // apps
                showStatsState,
                vibMotorState,
                calendarState,
                showAccState,
                setTimeState,
                setDarkModeState,
                setPowerSaverState,
                showTempState,
                stopWatchState,
                connectWiFiState,
                wifiOtaState 
                };


#endif