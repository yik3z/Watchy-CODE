#ifndef CONFIG_H
#define CONFIG_H

#define DEBUG             //enable printing of general debug messages
  #ifdef DEBUG
  #define DEBUG_EXTENSIVE        //enable printing of more messages
  //#define DEBUG_CALENDAR    //enable printing of calendar debug messages
  #define DEBUG_TIMING        //enable printing of timinings (millis())
  #ifdef DEBUG_TIMING
    //#define DEBUG_TIMING_EXTENSIVE        //enable printing of timinings (millis())
    
  #endif
  #define DEBUG_POWERSAVER  //enable printing of power saver - related messages
  #endif //DEBUG
#define INCLUDE_WEATHER

//pins
#define SDA 21
#define SCL 22
#define ADC_PIN 33
#define RTC_PIN GPIO_NUM_27
#define CS 5
#define DC 10
#define RESET 9
#define BUSY 19
#define TS_RESET_PIN        GPIO_NUM_0
#define TS_INTERRUPT_PIN    GPIO_NUM_34
#define CHARGING_SENSE_PIN  GPIO_NUM_35
#define VIB_MOTOR_PIN 13
#define MENU_BTN_PIN 2
#define BACK_BTN_PIN 25
#define UP_BTN_PIN 32
#define DOWN_BTN_PIN 4
#define MENU_BTN_MASK GPIO_SEL_26
#define BACK_BTN_MASK GPIO_SEL_25
#define UP_BTN_MASK GPIO_SEL_32
#define DOWN_BTN_MASK GPIO_SEL_4
#define ACC_INT_MASK GPIO_SEL_14
#define TS_INT_PIN_MASK GPIO_SEL_34
#define BTN_PIN_MASK MENU_BTN_MASK|BACK_BTN_MASK|UP_BTN_MASK|DOWN_BTN_MASK|TS_INT_PIN_MASK

//display
#define DISPLAY_WIDTH 200
#define DISPLAY_HEIGHT 200
#define DISPLAY_REFRESH_INTERVAL 300    //max refresh interval for the display

//weather (note: weather is not shown anywhere and sync is disabled!)
#define CITY_NAME "SINGAPORE" //if your city name has a space, replace with '+'
#define COUNTRY_CODE "SG"
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?q="
#define TEMP_UNIT "metric" //use "imperial" for Fahrenheit"
//#define WEATHER_UPDATE_INTERVAL 30 //minutes not used

//wifi
#define WIFI_TIMEOUT 10000 //ms
//WIFI_SSID and WIFI_PASSWORD defined in sensitive_config.h
#define INTERNET_SYNC_INTERVAL 3 //days, syncs NTP time and calendar

//menu
#define WATCHFACE_STATE -1
#define MAIN_MENU_STATE 0
#define APP_STATE 1
//#define FAST_MENU_SLEEP_TIMEOUT 2000

// for menu GUI
#define MENU_HEIGHT 30
#define MENU_LENGTH 6

// set time
#define SET_HOUR 0
#define SET_MINUTE 1
#define SET_YEAR 2
#define SET_MONTH 3
#define SET_DAY 4
#define YEAR_OFFSET 1970
#define GMT_OFFSET_SEC 28800    //s, set time zone to singapore standard time
                                // ie UTC 8 * 60 * 60 = 28800
#define DAYLIGHT_OFFSET_SEC 0   // if observing Daylight saving time 3600 otherwise 0.
#define NTP_SERVER "pool.ntp.org"

//BLE OTA (unused)
#define BLE_DEVICE_NAME "Watchy BLE OTA"
#define WATCHFACE_NAME "Watchy 7 Segment"
#define SOFTWARE_VERSION_MAJOR 1
#define SOFTWARE_VERSION_MINOR 0
#define SOFTWARE_VERSION_PATCH 0
#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 0

//power saving & battery
#define NIGHT_HOURLY_TIME_UPDATE
#define NIGHT_HOURS_START 1
#define NIGHT_HOURS_END 6
#define LOW_BATT_THRESHOLD 15
#define CRIT_BATT_THRESHOLD 5
//#define USING_ACCELEROMETER   //whether the accelerometer gets initialised. DO NOT ENABLE

#define BTN_DEBOUNCE_INTERVAL 50 //minimum time between buttonpresses (in ms) for it to be counted
#define BTN_TIMEOUT 2500        //max time to wait for any uncleared button events before ignoring them and going to sleep

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