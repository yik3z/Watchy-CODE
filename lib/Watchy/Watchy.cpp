#include "Watchy.h"

DS3232RTC Watchy::RTC(false); 
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> Watchy::display(GxEPD2_154_D67(CS, DC, RESET, BUSY));

RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR int menuIndex;
#ifdef USING_ACCELEROMETER
RTC_DATA_ATTR BMA423 sensor;
#endif //USING_ACCELEROMETER
//RTC_DATA_ATTR bool WIFI_ON; //whether WiFi is on
RTC_DATA_ATTR bool BLE_CONFIGURED = 0;
RTC_DATA_ATTR weatherData currentWeather;
RTC_DATA_ATTR int weatherIntervalCounter = WEATHER_UPDATE_INTERVAL;
RTC_DATA_ATTR int internetSyncCounter = 0;
RTC_DATA_ATTR bool darkMode = 0; //global darkmode
RTC_DATA_ATTR bool fgColour = GxEPD_BLACK; 
RTC_DATA_ATTR bool bgColour = GxEPD_WHITE; 
RTC_DATA_ATTR uint8_t lowBatt = 0;  //0 = normal, 1 = low, 2 = critical
RTC_DATA_ATTR bool powerSaver = 0; 
RTC_DATA_ATTR bool hourlyTimeUpdate = 0;
volatile uint64_t wakeupBit;
RTC_DATA_ATTR time_t lastNtpSync = 0;
RTC_DATA_ATTR bool lastNtpSyncSuccess = false;
RTC_DATA_ATTR time_t bootTime = 0;
//calendar
//extern const int calEntryCount;        
extern RTC_DATA_ATTR int calendarLength;
extern RTC_DATA_ATTR bool lastCalendarSyncSuccess;
//stopwatch
//for stopwatch (used by ISRs)
extern bool stopBtnPressed;
extern unsigned long stopWatchEndMillis;

//uint32_t Freq = 0; //used for checking CPU frequency

volatile unsigned long lastButtonInterrupt;  //si the last button time pressed


const char *menuItems[] = {"Show Stats", "Vibrate Motor", "Show Calendar", "Set Time","Dark/Light Mode","Power Saver","Show Temperature","Stopwatch","Sync to WiFi","WiFi OTA"};
int16_t menuOptions = sizeof(menuItems) / sizeof(menuItems[0]);

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

Watchy::Watchy(){
} //constructor

//Main "loop" that is run everytime Watchy is woken from sleep
void Watchy::init(String datetime){
    wakeup_reason = esp_sleep_get_wakeup_cause(); //get wake up reason

    #ifdef DEBUG
    Serial.begin(115200);
    Serial.println("wakeup: " + String(millis()));
    //Serial.println(wakeup_reason);

    //to check CPU freq
    // Freq = getCpuFrequencyMhz(); //240Hmz
    // Serial.print("CPU Freq = ");
    // Serial.print(Freq);
    // Serial.println(" MHz");
    // Freq = getXtalFrequencyMhz();
    // Serial.print("XTAL Freq = ");
    // Serial.print(Freq);
    // Serial.println(" MHz");
    // Freq = getApbFrequency();
    // Serial.print("APB Freq = ");
    // Serial.print(Freq);
    // Serial.println(" Hz");

    #endif //DEBUG

    Wire.begin(SDA, SCL); //init i2c
    display.init(0, false); //_initial_refresh to false to prevent full update on init
    
    //critical battery mode
    if(lowBatt == 2){   
        if(!hourlyTimeUpdate){
            RTC.setAlarm(ALM2_MATCH_MINUTES, 0, 0, 0, 0);   //set RTC alarm to hourly (0th minute of the hour)
            hourlyTimeUpdate = 1;
        }
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0: //RTC Alarm
            RTC.alarm(ALARM_2); //resets the alarm flag in the RTC
            if(guiState == WATCHFACE_STATE){
                RTC.read(currentTime);
                showWatchFace(true); //partial updates on tick
            }
        case ESP_SLEEP_WAKEUP_EXT1: //button Press
            wakeupBit = esp_sleep_get_ext1_wakeup_status();
            handleButtonPress();
            break;
        default: //reset
            #ifndef ESP_RTC
            _rtcConfig(datetime);
            #endif
            //_bmaConfig();
            showWatchFace(false); //full update on reset
            break;
            }
    }

    //not critical batt
    else{
    switch (wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0: //RTC Alarm
            RTC.alarm(ALARM_2); //resets the alarm flag in the RTC
            if(guiState == WATCHFACE_STATE){
                RTC.read(currentTime);
                if((currentTime.Hour == 3) && (currentTime.Minute == 0)){ //full refresh + internet sync late at night
                    internetSyncCounter++;
                    if (internetSyncCounter>INTERNET_SYNC_INTERVAL){
                      syncInternetStuff();
                    }
                    showWatchFace(false);
                } else showWatchFace(true); //partial updates on tick
            }
            #ifdef NIGHT_HOURLY_TIME_UPDATE
            if((hourlyTimeUpdate == 0) && (currentTime.Hour >= NIGHT_HOURS_START) && (currentTime.Hour < NIGHT_HOURS_END)){  //set to update every hour from NIGHT_HOURS_START onwards //
                RTC.setAlarm(ALM2_MATCH_MINUTES, 0, 0, 0, 0);   //set RTC alarm to hourly (0th minute of the hour)
                hourlyTimeUpdate = 1;
                #ifdef DEBUG
                Serial.print("hourlyTimeUpdate: ");
                Serial.println(hourlyTimeUpdate);
                Serial.println("ALM2_MATCH_MINUTES: 0");
                #endif  //DEBUG
            }
            else if((hourlyTimeUpdate == 1) && (currentTime.Hour >= NIGHT_HOURS_END)){  //set to update every minute from 7:00am onwards 
          //else if(currentTime.Hour == 7 && currentTime.Minute == 0){ 
                RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0);  //set alarm back to 
                hourlyTimeUpdate = 0; 
                #ifdef DEBUG
                Serial.print("hourlyTimeUpdate: ");
                Serial.println(hourlyTimeUpdate);
                Serial.println("ALM2_EVERY_MINUTE");
                #endif  //DEBUG
            #endif  //NIGHT_HOURLY_TIME_UPDATE
            }
            break;
        case ESP_SLEEP_WAKEUP_EXT1: //button Press
            lastButtonInterrupt = millis();
            wakeupBit = esp_sleep_get_ext1_wakeup_status();
            setISRs();
            while(true){
                handleButtonPress();
                if((wakeupBit == 0) || (millis() - lastButtonInterrupt > BTN_TIMEOUT)){
                    break;
                }
            }
            break;
        default: //reset
            _rtcConfig(datetime);
            bootTime = RTC.get();
            //_bmaConfig(); //crashes watchy
            vibMotor(200, 4);
            showWatchFace(false); //full update on reset
            break;
    }
    #ifdef DEBUG
    Serial.println("Sleep: " + String(millis()));
    #endif //DEBUG
    }
    deepSleep();
}
//    if((internetSyncCounter >= INTERNET_SYNC_INTERVAL)or(usingGui==true)){  
String Watchy::syncInternetStuff(){
  String SSID = "";
  bool connected = initWiFi(); //check
  if(connected) { 
    SSID = WiFi.SSID();
    //syncNtpTime();
    fetchCalendar();
    //getWeatherData(true); //works alone
    #ifdef DEBUG
    //Serial.print("Internet connectivity test: ");
    //Serial.println(internetWorks());
    #endif
    
    internetSyncCounter = 0;  //reset the counter
  }
  WiFi.mode(WIFI_OFF); // shut down the radio to save power
  btStop();
  esp_wifi_stop(); 
  return SSID;
}


/*!
 * @brief NTP time sync. Inspired by etwasmitbaum's Watchy code: https://github.com/etwasmitbaum/Watchy/
 *
 * @param[in] usingGui:  Whether this is a scheduled or forced (GUI) sync. Dafault false
 *
 * @returns String of SSID connected. If failed, returns blank string ""
 *  
 */
void Watchy::syncNtpTime(){ 
  //attempt to sync
  lastNtpSyncSuccess = false;
  struct tm timeinfo;
  //get NTP Time
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
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
  
  WiFi.mode(WIFI_OFF); // shut down the radio to save power
  //WIFI_ON = false;
  btStop();
  esp_wifi_stop(); 
  /*
  else{ //leftover logic from when the ntp sync counter was incremented within syncNtpTime
      internetSyncCounter++;
      #ifdef DEBUG
      Serial.print("internetSyncCounter: ");
      Serial.println(internetSyncCounter);
      #endif  //DEBUG
  }
  */
}

/*!
 * @brief Shuts down everything and puts the ESP32 to deepsleep:
 * - turns on ext0, ext1 wakeups
 * - turns off ADCs (save power)
 * - sets display to hibernate
 *
 */
void Watchy::deepSleep(){
    display.hibernate();
    #ifndef ESP_RTC
    esp_sleep_enable_ext0_wakeup(RTC_PIN, 0); //enable deep sleep wake on RTC interrupt
    #endif  
    #ifdef ESP_RTC
    esp_sleep_enable_timer_wakeup(60000000);
    #endif 
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH); //enable deep sleep wake on button press
    adc_power_off();
    esp_deep_sleep_start();
}

void Watchy::_rtcConfig(String datetime){
    if(datetime != NULL){
        const time_t FUDGE(30);//fudge factor to allow for upload time, etc. (seconds, YMMV)
        tmElements_t tm;
        tm.Year = getValue(datetime, ':', 0).toInt() - YEAR_OFFSET;//offset from 1970, since year is stored in uint8_t        
        tm.Month = getValue(datetime, ':', 1).toInt();
        tm.Day = getValue(datetime, ':', 2).toInt();
        tm.Hour = getValue(datetime, ':', 3).toInt();
        tm.Minute = getValue(datetime, ':', 4).toInt();
        tm.Second = getValue(datetime, ':', 5).toInt();

        time_t t = makeTime(tm) + FUDGE;
        RTC.set(t);

    }
    //https://github.com/JChristensen/DS3232RTC
    RTC.squareWave(SQWAVE_NONE); //disable square wave output
    RTC.setAlarm(ALM2_EVERY_MINUTE, 0, 0, 0, 0); //alarm wakes up Watchy every minute
    RTC.alarmInterrupt(ALARM_2, true); //enable alarm interrupt
    RTC.read(currentTime);
}

/***************  BUTTON HANDLER ***************/
/* add the buttons for your apps here. 
Rn I can't think of a better way to handle the button presses after wakeup
*/

void Watchy::handleButtonPress(){
    #ifdef DEBUG
    //Serial.println("Enter Loop (buttonpress)");
    #endif //DEBUG
  
  //Menu Button
  if (wakeupBit & MENU_BTN_MASK){
      #ifdef DEBUG
      Serial.println("MENU PRESSED");
      #endif
      wakeupBit = 0;    //clear buttonpress flag
    if(guiState == WATCHFACE_STATE){//enter menu state if coming from watch face
        showMenu(menuIndex, true);
        //fastMenu();
    }else if(guiState == MAIN_MENU_STATE){//if already in menu, then select menu item
      switch(menuIndex)
      {
        case 0:
          showStats();
          break;
        case 1:
          showBuzz();
          break;          
        case 2:
          //#ifdef USING_ACCELEROMETER
          showCalendar();
          //#endif //USING_ACCELEROMETER
          break;
        case 3:
          setTime();
          break;
        case 4:
          setDarkMode();
          break;   
        case 5:
          setPowerSaver();      
          break;           
        case 6:
          #ifdef USING_ACCELEROMETER
          //showTemperature();
          #endif //USING_ACCELEROMETER
          break;
	    case 7:
		  stopWatch();
		  break;
        case 8:
		  connectWiFiGUI();
		  break;
        case 9:
          wifiOta();
          break;
        default:
          break;                              
      }
    }
    else if(guiState == APP_STATE){//if it's in the app, tell the app that the button was pressed
        #ifdef DEBUG
        Serial.println(menuIndex);
        #endif

      switch(menuIndex)
      {
        case 0:
          showStats(MENU_BTN_PIN);
          break;
        case 1:
          //showBuzz();
          break;          
        case 2:
          //showCalendar();    
          break;
        case 3:
          //setTime();
          break;
        case 4:
          setDarkMode(MENU_BTN_PIN);
          break;   
        case 5:
          setPowerSaver(MENU_BTN_PIN); 
          break;                
        case 6:
          //showTemperature();      /disabled
          break;
	    case 7:
		  stopWatch(MENU_BTN_PIN);
		  break;
        case 8:
		  //connectWiFiGUI();
		  break;
        case 9:
          //wifiOta();
          break;
        default:
          break;                              
      }
      
    }
  }

  //Back Button
  else if (wakeupBit & BACK_BTN_MASK){
    #ifdef DEBUG
    Serial.println("BACK PRESSED");
    #endif
    wakeupBit = 0;
    if(guiState == MAIN_MENU_STATE){//exit to watch face if already in menu
      RTC.alarm(ALARM_2); //resets the alarm flag in the RTC
      RTC.read(currentTime);
      showWatchFace(false);
    }else if(guiState == APP_STATE){
      showMenu(menuIndex, true);//exit to menu if already in app
      //fastMenu();   //test
    }else if(guiState == WATCHFACE_STATE){
      RTC.read(currentTime);
      showWatchFace(true);     //update watch face (for middle fo the night when the watch updates only hourly)
    }  
  }
  
  //Up Button
  else if (wakeupBit & UP_BTN_MASK){
    #ifdef DEBUG
    Serial.println("UP PRESSED");
    #endif
    wakeupBit = 0;
    if(guiState == MAIN_MENU_STATE){//increment menu index
      menuIndex--;
      if(menuIndex < 0){
        menuIndex = menuOptions - 1;
      }    
      showMenu(menuIndex, true);
      //fastMenu();   //test
    }
    else if (guiState == APP_STATE){
        //ADD YOUR BUTTON EVENTS HERE
        switch(menuIndex)
        {
        case 0: //battery App
          //showStats();
          break;
        case 1:
          //showBuzz();
          break;          
        case 2:
          //showCalendar();
          break;
        case 3:
          //setTime();
          break;
        case 4:
          //setDarkMode(UP_BTN_PIN);
          break;    
        case 5:
          //setPowerSaver(UP_BTN_PIN);  
          break;              
        case 6:
          //showTemperature();
          break;
	    case 7:
		  stopWatch(UP_BTN_PIN);
		  break;
        case 8:
		  //connectWiFiGUI();
		  break;
        case 9:
          //wifiOta();
          break;
        default:
          break;
        }
    }
  }
  //Down Button
  else if (wakeupBit & DOWN_BTN_MASK){
    #ifdef DEBUG
    Serial.println("DOWN PRESSED");
    #endif
    wakeupBit = 0;
    if(guiState == MAIN_MENU_STATE){//decrement menu index
      menuIndex++;
      if(menuIndex > menuOptions - 1){
        menuIndex = 0;
      }
      showMenu(menuIndex, true);
      //fastMenu();
    }
    
    else if (guiState == APP_STATE){ //is in an app
    //ADD YOUR BUTTON EVENTS HERE
        switch(menuIndex)
        {
        case 0: //battery App
          //showStats();
          break;
        case 1:
          //showBuzz();
          break;          
        case 2:
          //showCalendar();
          break;
        case 3:
          //setTime();
          break;
        case 4:
          setDarkMode(DOWN_BTN_PIN);
          break;    
        case 5:
          setPowerSaver(DOWN_BTN_PIN);  
          break;              
        case 6:
          //showTemperature();
          break;
	    case 7:
		  stopWatch(DOWN_BTN_PIN);
		  break;
        case 8:
		  //connectWiFiGUI();
		  break;
        case 9:
          //wifiOta();
          break;
        default:
          break;
        }
    }
  }
}   //handleButtonPress

//scrolling menu by Alex Story
void Watchy::showMenu(byte menuIndex, bool partialRefresh){
    display.setFullWindow();
    display.fillScreen(bgColour);
    display.setFont(&FreeMonoBold9pt7b);

    int16_t  x1, y1;
    uint16_t w, h;
    int16_t yPos;
    int16_t startPos=0;

	//Code to move the menu if current selected index out of bounds
	if(menuIndex+MENU_LENGTH>menuOptions)
	{
			startPos=(menuOptions-1)-(MENU_LENGTH-1);
	}
	else
		{
			startPos=menuIndex;
			}
    for(int i=startPos; i<MENU_LENGTH+startPos; i++){
        yPos = 30+(MENU_HEIGHT*(i-startPos));
        display.setCursor(0, yPos);
        if(i == menuIndex){
            display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);
            display.fillRect(x1-1, y1-10, 200, h+15, fgColour);
            display.setTextColor(bgColour);
            display.println(menuItems[i]);      
        }else{
            display.setTextColor(fgColour);
            display.println(menuItems[i]);
        }   
    }

    display.display(partialRefresh, darkMode);

    guiState = MAIN_MENU_STATE;    
}   //showMenu

//HELPER FUNCTIONS

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
    display.setFullWindow();
    drawWatchFace();
    display.display(partialRefresh, darkMode); //partial refresh
    guiState = WATCHFACE_STATE;
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
        // if(weatherIntervalCounter >= WEATHER_UPDATE_INTERVAL){ //only update if WEATHER_UPDATE_INTERVAL has elapsed i.e. 30 minutes
            // if(initWiFi()){//Use Weather API for live data if WiFi is connected
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
                //turn off radios
                // WiFi.mode(WIFI_OFF);
                // btStop();
            // }else{//No WiFi, use RTC Temperature
            //     uint8_t temperature = RTC.temperature() / 4; //celsius
            //     if(strcmp(TEMP_UNIT, "imperial") == 0){
            //         temperature = temperature * 9. / 5. + 32.; //fahrenheit
            //     }
            //     currentWeather.temperature = temperature;
            //     currentWeather.weatherConditionCode = 800; //placeholder
            // }
            weatherIntervalCounter = 0;
        // }else{
        //     weatherIntervalCounter++;
        // }
    }
    else{
        int8_t temperature = RTC.temperature() / 4; //celsius. Seems like the conversion has some problems - gives me temps of 45deg
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
    adc_power_on();
    // setup

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(ADC1_GPIO33_CHANNEL, ADC_ATTEN_DB_11);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    
// read voltage
    int analog = adc1_get_raw(ADC1_GPIO33_CHANNEL);
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

void IRAM_ATTR ISRStopwatchEnd() {
    stopWatchEndMillis = millis();
    stopBtnPressed = true;
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

bool Watchy::initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  #ifdef DEBUG
  Serial.print("Connecting to WiFi ..");
  #endif    //DEBUG
  unsigned long startMillis = millis();
  bool res = true;
  while (WiFi.status() != WL_CONNECTED) {   
    if(millis() - startMillis > WIFI_TIMEOUT){
        res = false;    //WiFi failed to connect
        break;
    }
    #ifdef DEBUG
    Serial.println(WiFi.status());
    #endif
    //slow down CPU to save power while waiting. DISABLED BECAUSE ANY KIND OF CHANGES TO THE CPU SPEED HERE MAKES WIFI FAIL
    //setCpuFrequencyMhz(80);
    delay(100);
  }
  return res;
}

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