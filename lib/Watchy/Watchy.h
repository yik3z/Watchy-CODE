#ifndef WATCHY_H
#define WATCHY_H

#include <Arduino.h>
//#include <WiFiManager.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <DS3232RTC.h>
#include <GxEPD2_BW.h>
#include <Wire.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "DSEG7_Classic_Bold_53.h"
#include "BLE.h"
#include "bma.h"
#include "config.h" 
#include "sensitive_config.h"

typedef struct weatherData{
    int8_t temperature;
    int16_t weatherConditionCode;
}weatherData;

class Watchy {
    public:
        static DS3232RTC RTC;
        static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;
        tmElements_t currentTime;
            //for darkmode. This only affects the watchface atm

    public:
        Watchy();
        void init(String datetime = "");
        void deepSleep();
        float getBatteryVoltage();
        void vibMotor(uint8_t intervalMs = 100, uint8_t length = 20);

        void handleButtonPress();
        void showMenu(byte menuIndex, bool partialRefresh);
        void fastMenu();
        void showFastMenu(byte menuIndex);
        void showBattery();
        void showBuzz();
        void showAccelerometer();
        void showUpdateFW();
        void setTime();
        //void setupWifi(); //removed
        bool initWiFi();
        void connectWiFiGUI();
        //bool connectWiFi(); //is a remnant of the original WiFi implementation. Required for the weather update function
        //void setupBLE();  //not yet created...or maybe a separate file for that
        weatherData getWeatherData(bool online = true); //added "online" argument to allow for forced RTC temp measurement
        
        void stopWatch(uint8_t btnPin = 0); //under testing
        //void ISRStopwatchEndTime();   //is now outside of the class
        void showTemperature(uint8_t btnPin = 0);
        void setDarkMode(uint8_t btnPin = 0);
        void setPowerSaver(uint8_t btnPin = 0);
        void syncNtpTime();

        //bool getDarkModeStatus();   //for watchface to determine if it's in darkmode
        void showWatchFace(bool partialRefresh);
        virtual void drawWatchFace(); //override this method for different watch faces

    private:
        void _rtcConfig(String datetime);    
        void _bmaConfig();
        //static void _configModeCallback(WiFiManager *myWiFiManager);
        static uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
        static uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len);
};

void ISRStopwatchEndTime();

extern RTC_DATA_ATTR int guiState;
extern RTC_DATA_ATTR int menuIndex;
extern RTC_DATA_ATTR BMA423 sensor;
extern RTC_DATA_ATTR bool WIFI_CONFIGURED;
extern RTC_DATA_ATTR bool BLE_CONFIGURED;
extern RTC_DATA_ATTR bool darkMode;    //for darkmode. This only affects the watchface atm
extern RTC_DATA_ATTR bool lowBatt;  //not used yet
extern RTC_DATA_ATTR bool powerSaver; //will be a user toggleable 
extern RTC_DATA_ATTR bool hourlyTimeUpdate; //to replace powerSaver in time update function

#endif