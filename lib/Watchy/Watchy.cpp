#include "Watchy.h"

DS3232RTC Watchy::RTC(false); 
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> Watchy::display(GxEPD2_154_D67(CS, DC, RESET, BUSY));
FT6336 Watchy::ts(FT6336(TS_INTERRUPT_PIN, TS_RESET_PIN));

//RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR int menuIndex;

#ifdef USING_ACCELEROMETER
RTC_DATA_ATTR BMA423 sensor;
#endif //USING_ACCELEROMETER
//RTC_DATA_ATTR bool WIFI_ON; //whether WiFi is on
RTC_DATA_ATTR bool BLE_CONFIGURED = 0;
RTC_DATA_ATTR weatherData currentWeather;
//RTC_DATA_ATTR int weatherIntervalCounter = WEATHER_UPDATE_INTERVAL;
RTC_DATA_ATTR int internetSyncCounter = 0;
RTC_DATA_ATTR bool darkMode = 0; //global darkmode
RTC_DATA_ATTR bool fgColour = GxEPD_BLACK; 
RTC_DATA_ATTR bool bgColour = GxEPD_WHITE; 
RTC_DATA_ATTR uint8_t lowBatt = 0;  //0 = normal, 1 = low, 2 = critical
RTC_DATA_ATTR bool powerSaver = 0;  // user-selectable power saver mode
RTC_DATA_ATTR bool hourlyTimeUpdate = 0;
volatile uint64_t wakeupBit;
TS_Point touchLocation;
RTC_DATA_ATTR time_t lastNtpSync = 0;
RTC_DATA_ATTR bool lastNtpSyncSuccess = false;
RTC_DATA_ATTR time_t bootTime = 0;

//calendar
//extern const int calEntryCount;        
extern RTC_DATA_ATTR int calendarLength;
extern RTC_DATA_ATTR bool lastCalendarSyncSuccess;
extern RTC_DATA_ATTR calendarEntries calEnt[CALENDAR_ENTRY_COUNT];

// menu
extern RTC_DATA_ATTR uint32_t menuPageNumber;  // page number used by all the menus
extern const uint32_t clockMenuPages;
extern const uint32_t mainMenuPages;

//stopwatch
//for stopwatch (used by ISRs)
extern bool stopBtnPressed;
extern unsigned long stopWatchEndMillis;

volatile unsigned long lastButtonInterrupt;  //si the last button time pressed

// String getValue(String data, char separator, int index)
// {
//   int found = 0;
//   int strIndex[] = {0, -1};
//   int maxIndex = data.length()-1;

//   for(int i=0; i<=maxIndex && found<=index; i++){
//     if(data.charAt(i)==separator || i==maxIndex){
//         found++;
//         strIndex[0] = strIndex[1]+1;
//         strIndex[1] = (i == maxIndex) ? i+1 : i;
//     }
//   }

//   return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
// }

Watchy::Watchy(){
} //constructor

//Main "loop" that is run everytime Watchy is woken from sleep
void Watchy::init(String datetime){
    wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT1){
      wakeupBit = esp_sleep_get_ext1_wakeup_status(); //get buttonpress (need to get it before display puts watchy in light sleep in _waitWhileBusy())
    }
    #ifdef DEBUG
    Serial.begin(115200);
    #ifdef DEBUG_TIMING
    Serial.println("wakeup: " + String(millis()));
    #endif //DEBUG_TIMING
    #ifdef DEBUG_EXTENSIVE
    Serial.print("Wakeup Reason: ");
    Serial.println(wakeup_reason);
    //_printCpuSettings();
    #endif
    #endif //DEBUG

    Wire.begin(SDA, SCL); //init i2c

    if(wakeupBit & TS_INT_PIN_MASK){
      touchLocation = ts.getPoint(); 

      #ifdef DEBUG_TOUCHSCREEN
      Serial.print("Screen touched. x: ");
      Serial.print(touchLocation.x);
      Serial.print(" | y: ");
      Serial.println(touchLocation.y);
      if (touchLocation.x == 0 && touchLocation.y == 0){
        // probably a bogus touch, just go back to sleep
        _deepSleep();
      }
      #endif
    }
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    checkChargingStatus();

    #ifdef DEBUG_TIMING_EXTENSIVE
    Serial.println("display init'd: " + String(millis()));
    #endif //DEBUG_TIMING_EXTENSIVE

    //critical battery / power saver mode
    if(lowBatt == 2 or powerSaver == 1){   
      if(!hourlyTimeUpdate){
        RTC.setAlarm(ALM2_MATCH_MINUTES, 0, 0, 0, 0);   //set RTC alarm to hourly (0th minute of the hour)
        hourlyTimeUpdate = 1;
        ts.setPowerMode(FT6336_PWR_MODE_HIBERNATE); // hibernate (i.e. turn off) touchscreen
        #ifdef DEBUG_POWERSAVER
        Serial.print("Touch panel put to hibernate (3): ");
        Serial.println(ts.getPowerMode());
        Serial.print("hourlyTimeUpdate: ");
        Serial.println(hourlyTimeUpdate);
        #endif  //DEBUG_POWERSAVER
      }
      switch (wakeup_reason)
      {
      case ESP_SLEEP_WAKEUP_EXT0: //RTC Alarm
        RTC.alarm(ALARM_2); //resets the alarm flag in the RTC
        if(runningApp == watchFaceState){
          RTC.read(currentTime);
          showWatchFace(true); //partial updates on tick
        }
      case ESP_SLEEP_WAKEUP_EXT1: //button Press
        handleInput();
        break;
      default: //reset
        _initReset();
        break;
      }
    }

    //not critical batt
    else{
    switch (wakeup_reason)
    {
      case ESP_SLEEP_WAKEUP_EXT0: //RTC Alarm
        RTC.alarm(ALARM_2); //resets the alarm flag in the RTC
        RTC.read(currentTime);
        #ifdef NIGHT_HOURLY_TIME_UPDATE
        if((hourlyTimeUpdate == 0) && (currentTime.Hour >= NIGHT_HOURS_START) && (currentTime.Hour < NIGHT_HOURS_END)){  //set to update every hour from NIGHT_HOURS_START onwards //
          RTC.setAlarm(ALM2_MATCH_MINUTES, 0, 0, 0, 0);   //set RTC alarm to hourly (0th minute of the hour)
          hourlyTimeUpdate = 1;
          ts.setPowerMode(FT6336_PWR_MODE_HIBERNATE); // hibernate (i.e. turn off) touchscreen
          #ifdef DEBUG_POWERSAVER
          Serial.print("Touch panel put to hibernate (3): ");
          Serial.println(ts.getPowerMode());
          Serial.print("hourlyTimeUpdate: ");
          Serial.println(hourlyTimeUpdate);
          #endif  //DEBUG_POWERSAVER
        }
        else if((hourlyTimeUpdate == 1) && (currentTime.Hour >= NIGHT_HOURS_END)){  //set to update every minute from 7:00am onwards
          RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);  //set alarm back to alarm every minute
          hourlyTimeUpdate = 0; 
          #ifdef DEBUG_POWERSAVER
          Serial.print("Touch panel woken.");
          #endif
          //ts.wakePanel(false);  // re-enable touch panel after night hours end
          #ifdef DEBUG_POWERSAVER
          Serial.print("hourlyTimeUpdate: ");
          Serial.println(hourlyTimeUpdate);
          Serial.println("ALM2_EVERY_MINUTE");
          #endif  //DEBUG_POWERSAVER
        }
        #endif  //NIGHT_HOURLY_TIME_UPDATE
        if(runningApp == watchFaceState){
          if((currentTime.Hour == 3) && (currentTime.Minute == 0)){ //full refresh + internet sync late at night
            internetSyncCounter++;
            if (internetSyncCounter>INTERNET_SYNC_INTERVAL){
              syncInternetStuff();
            }
            showWatchFace(false);
          } else showWatchFace(true); //partial updates on tick
        }
        break;
      case ESP_SLEEP_WAKEUP_EXT1: //button Press
          if(hourlyTimeUpdate == 1){
            // enable touchscreen
            //ts.wakePanel(false);
            RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);  //set alarm for 1 minute, so that touchscreen will be put to sleep at next minute
            hourlyTimeUpdate = 0;
          }

          lastButtonInterrupt = millis();
          //wakeupBit = esp_sleep_get_ext1_wakeup_status(); //has been assigned earlier, before display.init()
          setISRs();
          while(true){
              handleInput();
              if((wakeupBit == 0) || (millis() - lastButtonInterrupt > BTN_TIMEOUT)){
                  break;
              }
          }
          break;
      default: //reset
        _initReset();
        vibMotor(200, 4);
        break;
    } // switch
    } // !(lowBatt == 2 or powerSaver == 1)
    _deepSleep();
}

/***************  BUTTON HANDLER ***************/

/* 
 * Updated handler for button and touchscreen events
*/
void Watchy::handleInput(){

  #ifdef DEBUG
  // print input for debug
  if (wakeupBit & MENU_BTN_MASK){
    Serial.println("MENU PRESSED");
  } else if (wakeupBit & BACK_BTN_MASK){
    Serial.println("BACK PRESSED");
  }else if (wakeupBit & UP_BTN_MASK){
    Serial.println("UP PRESSED");
  }else if (wakeupBit & DOWN_BTN_MASK){
    Serial.println("DOWN PRESSED");
  }else if (wakeupBit & TS_INT_PIN_MASK){
    Serial.println("TOUCH DETECTED");
  }
  #endif //DEBUG

  // Global overrides
  // back/close button always goes to watch face
  if (wakeupBit & BACK_BTN_MASK){ 
    if (runningApp != watchFaceState){
      wakeupBit = 0;
      menuPageNumber = 1;   // reset menu page number when returning to watch face
      RTC.alarm(ALARM_2);   // reset the alarm flag in the RTC
      RTC.read(currentTime);
      showWatchFace(false);
      return;
    }
  } else if (wakeupBit & TS_INT_PIN_MASK){
    if (runningApp != watchFaceState){
      if(_tpWithinBounds(0,30,0,30)){
        wakeupBit = 0;
        menuPageNumber = 1;   // reset menu page number when returning to watch face
        RTC.alarm(ALARM_2);   // reset the alarm flag in the RTC
        RTC.read(currentTime);
        showWatchFace(false);
        return;
      } 
    }
    if(_tpWithinBounds(0,30,170,200)){
      wakeupBit = 0;
      showMainMenu(true);
      return;
    }
  }
  
  // GUI state-based event handling
  if (runningApp == watchFaceState){
    _watchfaceInteractionHandler();
  }  else if (runningApp == mainMenuState || runningApp == clockMenuState){
    _menuInteractionHandler();
  }  else if (runningApp > clockMenuState){
    _appLauncherHandler();  //pass input on to app
  }
  wakeupBit = 0;  //TODO: remove when UP and DOWN are properly handled
} // handleInput

// Handles button and touch events when on the watchface
void Watchy::_watchfaceInteractionHandler(){
  if (wakeupBit & MENU_BTN_MASK){
    wakeupBit = 0;
    showMainMenu();
  } else if (wakeupBit & BACK_BTN_MASK){ // update watch face
    wakeupBit = 0;
    RTC.read(currentTime);
    showWatchFace(true);
  } else if(wakeupBit & TS_INT_PIN_MASK){
    wakeupBit = 0;
    if(_tpWithinBounds(15,185,50,130)){
      showClockMenu();
    }else if((calEnt[0].calTitle[0] != 0) && (_tpWithinBounds(7,193,145,195))){ // check that calendar isn't empty too
      showCalendar();
    }else if(_tpWithinBounds(0,200,0,30)){
      showStats();
    }else{ // update watch face
      // RTC.read(currentTime);
      // showWatchFace(true);
      return;
    }
  }
} // watchfaceInteractionHandler


// checks whether the existing touch point is within the bounds
bool Watchy::_tpWithinBounds(int16_t minX, int16_t maxX, int16_t minY, int16_t maxY){
  if (touchLocation.x<minX){
    return false;
  } else if (touchLocation.x>maxX){
    return false;
  } else if (touchLocation.y<minY){
    return false;
  } else if (touchLocation.y>maxY){
    return false;
  }
  return true;
}

/*!
 * @brief Checks whether touch point is within the given box
 * @param boxNumber (uint8_t) box number to check; 1 to 4
 *  
 */
bool Watchy::_tpWithinSelectedMenuBox(uint8_t boxNumber){
  switch(boxNumber){
    case 0:
      return (_tpWithinBounds(25,95,13,83));
    case 1:
      return (_tpWithinBounds(105,175,13,83));
    case 2:
      return (_tpWithinBounds(25,95,93,163));
    case 3:
      return(_tpWithinBounds(105,175, 93,163));
    default:
      return false;
  }

}

// checks whether the existing touch point is within any of the 4 menu boxes
// Returns -1 if not within any menu box
int32_t Watchy::_tpWithinMenuBox(){
  if(_tpWithinBounds(25,95,13,83)) return 0;
  else if (_tpWithinBounds(105,175,13,83)) return 1;
  else if (_tpWithinBounds(25,95,93,163)) return 2;
  else if (_tpWithinBounds(105,175, 93,163)) return 3;
  else return -1;
}

// // scrolling menu by Alex Story
// void Watchy::showMenu(byte menuIndex, bool partialRefresh){
//   #ifdef DEBUG
//   Serial.println("GUI: Menu");
//   #endif
//   guiState = MAIN_MENU_STATE;  
//   runningApp = mainMenuState;
//   display.setFullWindow();
//   display.fillScreen(bgColour);
//   display.setFont(&FreeMonoBold9pt7b);

//   int16_t  x1, y1;
//   uint16_t w, h;
//   int16_t yPos;
//   int16_t startPos=0;

//   //Code to move the menu if current selected index out of bounds
//   if(menuIndex+MENU_LENGTH>menuOptions){
//     startPos=(menuOptions-1)-(MENU_LENGTH-1);
//   }
//   else{
//     startPos=menuIndex;
//   }
//   for(int i=startPos; i<MENU_LENGTH+startPos; i++){
//     yPos = 30+(MENU_HEIGHT*(i-startPos));
//     display.setCursor(0, yPos);
//     if(i == menuIndex){
//       display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
//       display.fillRect(x1-1, y1-10, 200, h+15, fgColour);
//       display.setTextColor(bgColour);
//       display.println(menuItems[i]);      
//     }else{
//       display.setTextColor(fgColour);
//       display.println(menuItems[i]);
//     }   
//   }
//   #ifdef DEBUG_TIMING_EXTENSIVE
//   Serial.print("Start drawing menu: ");
//   Serial.println(millis());
//   #endif //DEBUG_TIMING_EXTENSIVE
//   display.display(partialRefresh, darkMode);
//   #ifdef DEBUG_TIMING_EXTENSIVE
//   Serial.print("End drawing menu: ");
//   Serial.println(millis());
//   #endif //DEBUG_TIMING_EXTENSIVE  
// }   //showMenu

//HELPER FUNCTIONS

/*
 * Shuts down everything and puts the ESP32 to deepsleep:
 *
 * - turns on ext0, ext1 wakeups
 * - turns off ADCs (save power)
 * - sets display to hibernate
 *
 */
void Watchy::_deepSleep(){ //TODO: set all pins to inputs to save power??
  
  pinMode(TS_INTERRUPT_PIN, INPUT); 
  if (digitalRead(TS_INTERRUPT_PIN) == HIGH){ // sleep and wait for touchscreen interrupt pin to go low again (Otherwise watchy will just wake again immediately)
    #ifdef DEBUG
    Serial.print("waiting for INT pin to go low. ");
    #ifdef DEBUG_TIMING
    Serial.print(millis());
    #endif // DEBUG_TIMING
    Serial.println();
    #endif // DEBUG
    esp_sleep_enable_ext1_wakeup(TS_INT_PIN_MASK, ESP_EXT1_WAKEUP_ALL_LOW); // wake when INT goes low
    esp_light_sleep_start();
    //sleep//
    #ifdef DEBUG_TIMING
    Serial.print("Wakeup: ");
    Serial.println(millis());
    #endif // DEBUG_TIMING
  }
  // if(ts.getPowerMode() != 3){ // touchscreen isn't hibernating
  //   #ifdef DEBUG_TOUCHSCREEN
  //   Serial.println("Setting display to monitor mode.");
  //   #endif
  //   ts.setPowerMode(FT6336_PWR_MODE_MONITOR); // set touchscreen to monitor (low power) mode 
  // }

  // for testing
  ts.setPowerMode(FT6336_PWR_MODE_HIBERNATE); // set touchscreen to hibernate mode 
  ts.getPowerMode();
  delay(3000);
  Serial.print(ts.getPowerMode());

  #ifdef DEBUG_TIMING
  Serial.println("Hibernate Display: " + String(millis()));
  #endif //DEBUG_TIMING
  display.hibernate();
  esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
  adc_power_off();

  #ifdef DEBUG_TIMING
  Serial.println("Sleep: " + String(millis()));
  #endif //DEBUG_TIMING

  esp_deep_sleep_start();
}

void Watchy::_rtcConfig(){
  //https://github.com/JChristensen/DS3232RTC
  RTC.squareWave(SQWAVE_NONE); //disable square wave output
  RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
  RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
  RTC.read(currentTime);
}
 
String Watchy::syncInternetStuff(){
  String SSID = "";  //used to return the SSID
  bool connected = initWiFi();
  if(connected) { 
    SSID = WiFi.SSID();
    syncNtpTime();
    fetchCalendar();
    //getWeatherData(true); //works alone, not tested together with the other 2. I don't need weather anyway
    #ifdef DEBUG
    //Serial.print("Internet connectivity test: ");
    //Serial.println(internetWorks());
    #endif
    
    internetSyncCounter = 0;  //reset the counter
  }
  WiFi.mode(WIFI_OFF); // calls esp_wifi_stop() to shut down the radio to save power
  btStop();
  //esp_wifi_stop(); 
  return SSID;
}

/*!
 * @brief NTP time sync. Inspired by etwasmitbaum's Watchy code: https://github.com/etwasmitbaum/Watchy/
 *  
 */
void Watchy::syncNtpTime(){ 
  //attempt to sync
  lastNtpSyncSuccess = false;
  struct tm timeinfo;

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);  //get NTP Time
  delay(4000); //delay 4 secods so configTime can finish recieving the time from the internet
  getLocalTime(&timeinfo);
  // convert NTP time into proper format
  currentTime.Month = timeinfo.tm_mon + 1;// 0-11 based month so we have to add 1
  currentTime.Day = timeinfo.tm_mday;
  currentTime.Year = timeinfo.tm_year + 1900 - YEAR_OFFSET;//offset from 1970, since year is stored in uint8_t
  currentTime.Hour = timeinfo.tm_hour;
  currentTime.Minute = timeinfo.tm_min;
  currentTime.Second = timeinfo.tm_sec;
  lastNtpSync = makeTime(currentTime);
  RTC.set(lastNtpSync);
  lastNtpSyncSuccess = true;
  #ifdef DEBUG
  Serial.println("Time Synced and set"); //debug
  #endif //DEBUG
}

/*!
 * @brief Vibrates motor
 *
 * @param[in] intervalMs:  Length of pulse & delay
 *
 * @param[in] length:  number of pulses * 2
 *  
 */
void Watchy::vibMotor(uint8_t intervalMs, uint8_t length){
    pinMode(VIB_MOTOR_PIN, OUTPUT);
    bool motorOn = false;
    for(int i=0; i<length; i++){
        motorOn = !motorOn;
        digitalWrite(VIB_MOTOR_PIN, motorOn);
        delay(intervalMs);
    }
    digitalWrite(VIB_MOTOR_PIN, 0);
}

void Watchy::showWatchFace(bool partialRefresh){
    //guiState = WATCHFACE_STATE;
    runningApp = watchFaceState;
    display.setFullWindow();
    drawWatchFace();
    #ifdef DEBUG_TIMING
    Serial.print("Start drawing watch face: ");
    Serial.println(millis());
    #endif //DEBUG_TIMING
    display.display(partialRefresh, darkMode); //partial refresh
    #ifdef DEBUG_TIMING
    Serial.print("End drawing watch face: ");
    Serial.println(millis());
    #endif //DEBUG_TIMING
}


void Watchy::drawWatchFace(){   //placeholder
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(5, 53+60);
    if(currentTime.Hour < 10){
        display.print("0");
    }
    display.print(currentTime.Hour);
    display.print(":");
    if(currentTime.Minute < 10){
        display.print("0");
    }  
    display.println(currentTime.Minute);    
}

#ifdef INCLUDE_WEATHER
/*!
 * @brief Gets weather data (duh)
 *
 * @param[in] online:   whether to get weather data from the internet or just use the temperature of the (BMA423) chip
 *
 * @returns Temperature and weather conditions as type: weatherData
 */
weatherData Watchy::getWeatherData(bool online){
  if(online){
    HTTPClient http;
    http.setConnectTimeout(3000);//3 second max timeout
    String weatherQueryURL = String(OPENWEATHERMAP_URL) + String(CITY_NAME) + String(",") + String(COUNTRY_CODE) + String("&units=") + String(TEMP_UNIT) + String("&appid=") + String(OPENWEATHERMAP_APIKEY);
    http.begin(weatherQueryURL.c_str());
    int httpResponseCode = http.GET();
    if(httpResponseCode == 200) {
        String payload = http.getString();
        #ifdef DEBUG
        Serial.print("Weather payload: ");
        Serial.println(payload);
        #endif
        JSONVar responseObject = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp"]);
        currentWeather.weatherConditionCode = int(responseObject["weather"][0]["id"]);            
    }else{
        //http error
    }
    http.end();
  }else{ // offline mode; just get temperature
    uint8_t temperature = RTC.temperature() / 4; //celsius
    if(strcmp(TEMP_UNIT, "imperial") == 0){
        temperature = temperature * 9. / 5. + 32.; //fahrenheit
    }
    currentWeather.temperature = temperature;
    currentWeather.weatherConditionCode = 800; //placeholder
  } 
  return currentWeather;
}
#endif //INCLUDE_WEATHER

/*!
 * @brief Calculates battery voltage using the ESP32 calibrated ADC
 *
 * @returns Battery voltage in mV
 *  
 */
uint32_t Watchy::getBatteryVoltage(){
    adc_chars = esp_adc_cal_characteristics_t(); 
    adc_power_acquire();
    // setup

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_GPIO33_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
// read voltage
    int analog = adc1_get_raw(ADC1_GPIO33_CHANNEL);
    adc_power_release();
    return esp_adc_cal_raw_to_voltage(analog, &adc_chars) * 2;
    //return analogRead(ADC_PIN) / 4096.0 * 7.23;
}

/*!
 * @brief Calculates battery percentage using battery voltage
 *
 * @param[in] vBatt:  Battery voltage in millivolts
 *
 * @returns Battery percentage
 *  
 */
uint8_t Watchy::getBatteryPercent(uint32_t vBatt){
    uint32_t percentage;
    if((uint32_t)battPercentLUT[0]>vBatt){        //vBatt lower than lowest value in LUT; 0%
        percentage = 0;
    } 
    else if((uint32_t)battPercentLUT[20]<=vBatt){ //vBatt higher than highest value in LUT; 100%
        percentage = 100;
    }else{                              //vBatt within LUT
        uint8_t percentLowerBound = 0;
        for(uint8_t i = 0; i<21; i++){
            if((uint32_t)battPercentLUT[i] > vBatt){
                percentLowerBound = i-1;
                break;
            }
        }   
        //Interpolation. y0 + delta y; where delta y = grad * delta x
        //percentage is multiplied 1000x for higher resolution
        percentage = percentLowerBound * 5000 + ((5000/(battPercentLUT[percentLowerBound+1]-battPercentLUT[percentLowerBound])) * (vBatt-battPercentLUT[percentLowerBound])); 
        percentage = percentage / 1000; //to get 0-100%
    }
    if(percentage < CRIT_BATT_THRESHOLD){
        lowBatt = 2;
    } else if(percentage < LOW_BATT_THRESHOLD){
        lowBatt = 1;
    } else lowBatt = 0;
    return (uint8_t)percentage;
} 

// Reads the charging sense pin to determine if battery is currently charging
// Not sure what "not charging but AC present" outputs
void Watchy::checkChargingStatus(){
  pinMode(CHARGING_SENSE_PIN, INPUT_PULLUP);
  bool chargingStatus = digitalRead(CHARGING_SENSE_PIN);
  pinMode(CHARGING_SENSE_PIN, INPUT);
  bool dischargingStatus = digitalRead(CHARGING_SENSE_PIN);
  chargingFlag = (chargingStatus << 1) | dischargingStatus;
  // !charging , discharging
  // 00 = charging
  // 10 = AC
  // 11 = on battery
  #ifdef DEBUG
  Serial.print("Battery Charging Status: ");
  Serial.println(chargingFlag);
  #endif
}

#ifdef DEBUG
// Prints the cpu settings to serial port, for debug
void Watchy::_printCpuSettings(){
  //to check CPU freq
  Serial.print("CPU Freq = ");
  Serial.print(getCpuFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("XTAL Freq = ");
  Serial.print(getXtalFrequencyMhz());
  Serial.println(" MHz");
  Serial.print("APB Freq = ");
  Serial.print(getApbFrequency());
  Serial.println(" Hz");
}
#endif

// ISR for stopwatch buttonpress
void IRAM_ATTR ISRStopwatchEnd() {
    stopWatchEndMillis = millis();
    stopBtnPressed = true;
}

// Init routine to be run after a reset
void Watchy::_initReset(){
  _rtcConfig();
  bootTime = RTC.get();
  ts.begin(200);
  //_bmaConfig(); //crashes watchy. B<A starts up in suspend mode so no need to do anything
  showWatchFace(false); //full update on reset
}

//TODO: work in progress, crashes watchy
void Watchy::_bmaConfig(){
    #ifndef USING_ACCELEROMETER
    //sensor.softReset();

    #else
    if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
        //fail to init BMA
        return;
    }

    // Accel parameter structure
    Acfg cfg;
    /*!
        Output data rate in Hz, Optional parameters:
            - BMA4_OUTPUT_DATA_RATE_0_78HZ
            - BMA4_OUTPUT_DATA_RATE_1_56HZ
            - BMA4_OUTPUT_DATA_RATE_3_12HZ
            - BMA4_OUTPUT_DATA_RATE_6_25HZ
            - BMA4_OUTPUT_DATA_RATE_12_5HZ
            - BMA4_OUTPUT_DATA_RATE_25HZ
            - BMA4_OUTPUT_DATA_RATE_50HZ
            - BMA4_OUTPUT_DATA_RATE_100HZ
            - BMA4_OUTPUT_DATA_RATE_200HZ
            - BMA4_OUTPUT_DATA_RATE_400HZ
            - BMA4_OUTPUT_DATA_RATE_800HZ
            - BMA4_OUTPUT_DATA_RATE_1600HZ
    */
    cfg.odr = BMA4_OUTPUT_DATA_RATE_25HZ;
    /*!
        G-range, Optional parameters:
            - BMA4_ACCEL_RANGE_2G
            - BMA4_ACCEL_RANGE_4G
            - BMA4_ACCEL_RANGE_8G
            - BMA4_ACCEL_RANGE_16G
    */
    cfg.range = BMA4_ACCEL_RANGE_2G;
    /*!
        Bandwidth parameter, determines filter configuration, Optional parameters:
            - BMA4_ACCEL_OSR4_AVG1
            - BMA4_ACCEL_OSR2_AVG2
            - BMA4_ACCEL_NORMAL_AVG4
            - BMA4_ACCEL_CIC_AVG8
            - BMA4_ACCEL_RES_AVG16
            - BMA4_ACCEL_RES_AVG32
            - BMA4_ACCEL_RES_AVG64
            - BMA4_ACCEL_RES_AVG128
    */
    cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

    /*! Filter performance mode , Optional parameters:
        - BMA4_CIC_AVG_MODE
        - BMA4_CONTINUOUS_MODE
    */
    cfg.perf_mode = BMA4_CONTINUOUS_MODE;

    // Configure the BMA423 accelerometer
    sensor.setAccelConfig(cfg);

    // Enable BMA423 accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    // Warning : Need to use feature, you must first enable the accelerometer
    //sensor.enableAccel();
    sensor.disableAccel();

    struct bma4_int_pin_config config ;
    config.edge_ctrl = BMA4_LEVEL_TRIGGER;
    config.lvl = BMA4_ACTIVE_HIGH;
    config.od = BMA4_PUSH_PULL;
    config.output_en = BMA4_OUTPUT_ENABLE;
    config.input_en = BMA4_INPUT_DISABLE;
    // The correct trigger interrupt needs to be configured as needed
    sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

    struct bma423_axes_remap remap_data;
    remap_data.x_axis = 1;
    remap_data.x_axis_sign = 0xFF;
    remap_data.y_axis = 0;
    remap_data.y_axis_sign = 0xFF;
    remap_data.z_axis = 2;
    remap_data.z_axis_sign = 0xFF;
    // Need to raise the wrist function, need to set the correct axis
    sensor.setRemapAxes(&remap_data);

    // Enable BMA423 isStepCounter feature
    sensor.enableFeature(BMA423_STEP_CNTR, false); //disabled
    // Enable BMA423 isTilt feature
    sensor.enableFeature(BMA423_TILT, false); //disabled
    // Enable BMA423 isDoubleClick feature
    sensor.enableFeature(BMA423_WAKEUP, false); //disabled

    // Reset steps
    //sensor.resetStepCounter();

    // Turn on feature interrupt
    //sensor.enableStepCountInterrupt(); //disabled
    //sensor.enableTiltInterrupt(); //disabled
    // It corresponds to isDoubleClick interrupt
    //sensor.enableWakeupInterrupt();   //disabled


    sensor.shutDown();
    #endif //USING_ACCELEROMETER
}

// Connects to the WiFi SSID defined in config_sensitive.h
bool Watchy::initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #ifdef DEBUG
  Serial.println("Connecting to WiFi ..");
  #endif    //DEBUG
  unsigned long startMillis = millis();
  bool res = true;
  while (WiFi.status() != WL_CONNECTED) {   
    if(millis() - startMillis > WIFI_TIMEOUT){
        res = false;    //WiFi failed to connect
        break;
    }
    #ifdef DEBUG
    //Serial.println(WiFi.status());
    #endif
    //setCpuFrequencyMhz(80); //DISABLED BECAUSE ANY KIND OF CHANGES TO THE CPU SPEED HERE MAKES WIFI FAIL
    delay(100);
  }
  return res;
}

// Attached intterupts to the 4 buttons. Called when watchy wakes up on a buttonpress, to listen for more button presses
void Watchy::setISRs(){
    attachInterrupt(MENU_BTN_PIN, ISRMenuBtnPress, RISING);
    attachInterrupt(BACK_BTN_PIN, ISRBackBtnPress, RISING);
    attachInterrupt(UP_BTN_PIN, ISRUpBtnPress, RISING);
    attachInterrupt(DOWN_BTN_PIN, ISRDownBtnPress, RISING);
}

void IRAM_ATTR ISRMenuBtnPress() {
    if(millis()-lastButtonInterrupt>BTN_DEBOUNCE_INTERVAL){
        lastButtonInterrupt = millis();
        wakeupBit = MENU_BTN_MASK;
        #ifdef DEBUG
        //SERIAL.PRINT IN ISR. TRY NOT TO USE THIS UNLESS YOU REALLY NEED IT!!
        //Serial.println("M_ISR: " + String(lastButtonInterrupt));
        #endif
    }
    
}

void IRAM_ATTR ISRBackBtnPress() {
    if(millis()-lastButtonInterrupt>BTN_DEBOUNCE_INTERVAL){
        lastButtonInterrupt = millis();
        wakeupBit = BACK_BTN_MASK;
        #ifdef DEBUG
        //SERIAL.PRINT IN ISR. TRY NOT TO USE THIS UNLESS YOU REALLY NEED IT!!
        //Serial.println("B_ISR: " + String(lastButtonInterrupt));
        #endif

    }
}

void IRAM_ATTR ISRUpBtnPress() {
    if(millis()-lastButtonInterrupt>BTN_DEBOUNCE_INTERVAL){
        lastButtonInterrupt = millis();
        wakeupBit = UP_BTN_MASK;
        #ifdef DEBUG
        //SERIAL.PRINT IN ISR. TRY NOT TO USE THIS UNLESS YOU REALLY NEED IT!!
        //Serial.println("U_ISR: " + String(lastButtonInterrupt));
        #endif

    }
}

void IRAM_ATTR ISRDownBtnPress() {
    if(millis()-lastButtonInterrupt>BTN_DEBOUNCE_INTERVAL){
        lastButtonInterrupt = millis();
        wakeupBit = DOWN_BTN_MASK;
        #ifdef DEBUG
        //SERIAL.PRINT IN ISR. TRY NOT TO USE THIS UNLESS YOU REALLY NEED IT!!
        //Serial.println("D_ISR: " + String(lastButtonInterrupt));
        #endif

    }
}

uint16_t Watchy::_readRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)address, (uint8_t)len);
    uint8_t i = 0;
    while (Wire.available()) {
        data[i++] = Wire.read();
    }
    return 0;
}

uint16_t Watchy::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data, uint16_t len)
{
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(data, len);
    return (0 !=  Wire.endTransmission());
}

/***************** UNUSED CODE *****************/
/*
//put any unused code here

*/