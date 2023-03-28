#ifndef WATCHY_H
#define WATCHY_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

#include <DS3232RTC.h>
#include <GxEPD2_BW.h>
#include <FT6336.h>

#include <Wire.h>
#include <Fonts/FreeMonoBold9pt7b.h>
//#include "DIN_Black35pt7b.h"
#include "BLE.h"
#include <esp_wifi.h>
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "config.h" 
#include "sensitive_config.h"
#include "watchy_batt_LUT.h"
//#include <ESPmDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#ifdef USING_ACCELEROMETER
#include "bma.h"
#endif //USING_ACCELEROMETER
#include "calendar.h" //not ready
//#include "Apps.h"     //not ready
//#include "Weather.h"  //not ready

#ifdef INCLUDE_WEATHER
#include "Weather.h"

typedef struct weatherData{
    int8_t temperature;
    int16_t weatherConditionCode;
}weatherData;
#endif

class Watchy {
    public:
        static DS3232RTC RTC;
        static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;
        static FT6336 ts;
        tmElements_t currentTime;
        esp_adc_cal_characteristics_t adc_chars;
        esp_sleep_wakeup_cause_t wakeup_reason;

    public:
        Watchy();
        void init(String datetime = "");
        //void handleButtonPress();
        void setISRs();

        //helper functions
        uint32_t getBatteryVoltage();
        uint8_t getBatteryPercent(uint32_t vBatt);
        void vibMotor(uint8_t intervalMs = 100, uint8_t length = 20);
        bool initWiFi();
        void syncNtpTime();
        String syncInternetStuff();
        
        virtual void drawWatchFace(); //override this method for different watch faces
        void showWatchFace(bool partialRefresh = true);

        // Apps n GUI
        void showMainMenu(bool initial = false, bool partialRefresh = true);
        void showClockMenu(bool initial = false, bool partialRefresh = true);
        void handleInput();
        void launchApp(appID_t appToLaunch);
        void showStats();
        void showBuzz();
        void showCalendar();
        void connectWiFiGUI();
        void stopWatch();
        void setDarkMode();
        void setPowerSaver();
        void setTime();
        void wifiOta();
        //void setupBLE();  //not yet created...or maybe a separate file for that
        #ifdef USING_ACCELEROMETER
        void showAccelerometer(uint64_t wakeupBit = 0);
        void showTemperature(uint64_t wakeupBit = 0);
        #endif //USING_ACCELEROMETER
        #ifdef INCLUDE_WEATHER
        weatherData getWeatherData(bool online = true); //added "online" argument to allow for forced RTC temp measurement
        #endif //INCLULDE_WEATHER

    private:
        // GUI
        void _watchfaceInteractionHandler();
        void _menuInteractionHandler();
        appID_t _selectMenuApp(int32_t boxNumber);
        appID_t _chooseClockMenuApp(int32_t appIndex);
        appID_t _chooseMainMenuApp(int32_t appIndex);
        void _appLauncherHandler(appID_t launchAppID = none_appState);
        void _drawPageNumber();
        void _drawMenuOptions(const char *menuItems[], uint32_t menuOptions);
        void _drawCloseButton();
        bool _tpWithinBounds(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY);
        bool _tpWithinSelectedMenuBox(uint8_t boxNumber);
        int32_t _tpWithinMenuBox();


        // system functions
        void _deepSleep();
        void _initReset();
        void _rtcConfig();    
        void _bmaConfig();
        #ifdef DEBUG
        void _printCpuSettings();
        #endif
        static uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
        static uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
};

/* ISRs */
void ISRStopwatchEnd(); //ISR delcared outside the Watchy class :0
void IRAM_ATTR ISRMenuBtnPress();
void IRAM_ATTR ISRBackBtnPress();
void IRAM_ATTR ISRUpBtnPress();
void IRAM_ATTR ISRDownBtnPress();

/* Data that needs to be preserved over sleep
Note: You can't put an RTC_DATA_ATTR qualified variable inside a class 
because the class is stored in normal RAM. 
The RTC_DATA_ATTR qualified variable is, by definition, stored in RTC RAM. */
extern RTC_DATA_ATTR int guiState;
extern RTC_DATA_ATTR int menuIndex;
// to replace guiState and menuIndex with the following:
extern RTC_DATA_ATTR appID_t runningApp;

//extern RTC_DATA_ATTR bool WIFI_ON;  
extern RTC_DATA_ATTR bool BLE_CONFIGURED;
extern RTC_DATA_ATTR bool darkMode;    
extern RTC_DATA_ATTR bool fgColour; 
extern RTC_DATA_ATTR bool bgColour;
extern RTC_DATA_ATTR uint8_t lowBatt;  
extern RTC_DATA_ATTR bool lastNtpSyncSuccess;
extern RTC_DATA_ATTR bool lastCalendarSyncSuccess;

#ifdef USING_ACCELEROMETER
extern RTC_DATA_ATTR BMA423 sensor;
#endif //USING_ACCELEROMETER


#endif