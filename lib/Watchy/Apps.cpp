//GUI apps for watchy are here for readability

#include "Watchy.h"
#include "Apps.h"

//for stopwatch
unsigned long stopWatchEndMillis = 0;
RTC_DATA_ATTR unsigned long finalTimeElapsed = 0;
bool stopBtnPressed = false;

//for status
extern RTC_DATA_ATTR time_t bootTime;
extern RTC_DATA_ATTR time_t lastNtpSync;
extern RTC_DATA_ATTR bool powerSaver;

//for calendar
extern RTC_DATA_ATTR calendarEntries calEnt[CALENDAR_ENTRY_COUNT];
RTC_DATA_ATTR int calendarPage = 1;
extern RTC_DATA_ATTR int calendarLength;

/*!
 * @brief Displays a GUI and buzzes the motor for a few seconds
 */
void Watchy::showBuzz(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: Vib");
    #endif
    guiState = APP_STATE;
    display.setFullWindow();
    display.fillScreen(bgColour);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(fgColour);
    display.setCursor(70, 80);
    display.println("Buzz!");
    display.display(false, darkMode); //full refresh
    vibMotor();
    showMenu(menuIndex, false);    
}

/*!
 * @brief Displays a screen with all the clock-related apps
 */
void Watchy::showClockMenu(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: Vib");
    #endif
    guiState = APP_STATE;
    // display.setFullWindow();
    // display.fillScreen(bgColour);
    // display.setFont(&FreeMonoBold9pt7b);
    // display.setTextColor(fgColour);
    // display.setCursor(70, 80);
    // display.println("Buzz!");
    // show boxes with clock
    display.display(true, darkMode); //full refresh 
}

/*! 
 * @brief Stopwatch function. Uses an interrupt (ISRStopwatchEnd()) to trigger the end time correctly.
 * Might be quite power hungry while timing becuase there's no way to sleep while counting millis
 */
void Watchy::stopWatch(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: StpWtch");
    #endif
    guiState = APP_STATE;
    if((btnPin == 0) || (btnPin == UP_BTN_PIN)){    //entering the app for the first time
        finalTimeElapsed = 0;
        display.setFullWindow();
        display.fillScreen(bgColour);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(fgColour);
        display.setCursor(52, 50);
        display.println("Stopwatch");
        display.setCursor(52, 100);
        display.print("00:00.000");
        display.setCursor(130, 195);
        display.println("Start>");
        display.setCursor(5,20);
        display.println("<Exit");
        if(btnPin == UP_BTN_PIN){
            display.setCursor(70, 120);
            display.println("Reset!");
        }
        display.display(true, darkMode); //partial refresh
    }  
    else if(btnPin == DOWN_BTN_PIN){
        //start stopwatch
        unsigned long startMillis = millis();
        unsigned long previousMillis = millis();
        attachInterrupt(DOWN_BTN_PIN, ISRStopwatchEnd, RISING); //use interrupt to stop stopwatch

        display.setFullWindow();
        display.fillScreen(bgColour);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(fgColour);
        display.setCursor(52, 50);
        display.println("Stopwatch");
        display.setCursor(52, 100);
        display.print("00:00.000");
        display.setCursor(140, 195);
        display.println("Stop>");

        #ifdef DEBUG
        Serial.println("Stopwatch Started");
        #endif  //DEBUG
        display.display(true, darkMode);

        while(true){   //stopwatch running  
            
            //"stop" ISR has been triggered   
            if (stopBtnPressed == true){
                detachInterrupt(DOWN_BTN_PIN);
                stopBtnPressed = false;
                break;
            }
            //stopwatch update loop
            if(millis() - previousMillis > STOPWATCH_INTERVAL){
                previousMillis = millis();
                //get time
                unsigned long timeElapsed = previousMillis - startMillis;
                uint16_t minutes = timeElapsed/60000;
                uint8_t seconds = (timeElapsed % 60000)/1000;
                // dsiplay time
                display.fillScreen(bgColour); 
                //maybe can remove the next 2 lines if I fill only the time section with black
                //display.setFont(&FreeMonoBold9pt7b);
                //display.setTextColor(fgColour);
                display.setCursor(52, 50);
                display.println("Stopwatch"); 
                display.setCursor(52, 100);
                if(minutes<10){display.print("0");}
                display.print(minutes);
                display.print(":");
                if(seconds<10){display.print("0");}
                display.print(seconds);
                display.print(".xxx");  //no ms readout while counting
                display.setCursor(140, 195);
                display.println("Stop>");
                display.setCursor(5,20);
                display.println("<Exit");
                display.display(true, darkMode); //partial refresh
                
            } //stopwatch update loop
        }   //while true
     
        finalTimeElapsed = stopWatchEndMillis - startMillis; //get final time
        uint16_t minutes = finalTimeElapsed/60000;
        uint8_t seconds = (finalTimeElapsed % 60000)/1000;   
        uint16_t ms = finalTimeElapsed % 1000;
        
        display.fillScreen(bgColour);
        display.setCursor(52, 100);
        if(minutes<10){display.print("0");}
        display.print(minutes);
        display.print(":");
        if(seconds<10){display.print("0");}
        display.print(seconds);
        display.print(".");
        if(ms<10){display.print("00");}
        else if(ms<100){display.print("0");}
        display.println(ms);
        display.setCursor(130, 21);
        display.println("Reset>");
        display.setCursor(5,20);
        display.println("<Exit");
        display.display(true, darkMode); //partial refresh
    } //down button pressed
}   //stopWatch

/*
 * Shows a GUI with the core stastics and states of the watch:
 *
 * Uptime, Last NTP Sync, Power Saver Mode, Battery
 */
void Watchy::showStats(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: Stats");
    #endif
    guiState = APP_STATE;
    display.setFullWindow();
    display.fillScreen(bgColour);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(fgColour);
    display.setCursor(67, 30);
    display.println("SYSTEM");
    display.setCursor(64, 50);
    display.println("Uptime:");
    //print time difference between currentTime and bootTime
    //there is a bug where there is an additional 1m1d added to the uptime
    RTC.read(currentTime);
    time_t currentTime_time = makeTime(currentTime);
    time_t uptime_time = currentTime_time - bootTime;
    tmElements_t uptime_elements;
    breakTime(uptime_time, uptime_elements);
    display.setCursor(40, 65);
    display.print(uptime_elements.Month);
    display.print("m");
    display.print(uptime_elements.Day);
    display.print("d");
    display.print(uptime_elements.Hour);
    display.print("h");
    display.print(uptime_elements.Minute);
    display.print("m");
    display.setCursor(5, 85);
    display.print("NTP:");
    //print last NTP sync time
    tmElements_t lastNtpSync_elements;
    breakTime(lastNtpSync, lastNtpSync_elements);
    if(lastNtpSync_elements.Hour < 10){
      display.print("0");
    }
    display.print(lastNtpSync_elements.Hour);
    if(lastNtpSync_elements.Minute < 10){
      display.print("0");
    }  
    display.print(lastNtpSync_elements.Minute); 
    display.print("h");
    if(lastNtpSync_elements.Day < 10){
      display.print("0");
    }
    display.print(lastNtpSync_elements.Day);
    if(lastNtpSync_elements.Month < 10){
      display.print("0");
    } 
    display.print(lastNtpSync_elements.Month);  
    display.print(";");
    display.println(lastNtpSyncSuccess ? "s" : "f");
    display.setCursor(12, 100);
    display.println("Pwr Saver:");
    display.setCursor(155, 100);
    display.println(powerSaver ? "On":"Off");
    display.setCursor(62, 140);
    display.println("BATTERY");
    display.setCursor(12, 160);
    display.println("Voltage:");
    uint32_t mV = getBatteryVoltage();
    uint8_t percentage = getBatteryPercent(mV);
    display.setCursor(120, 160);
    display.print(mV);
    display.println("mV");
    display.setCursor(12, 185);
    display.println("Percentage:");
    display.setCursor(140, 185);
    display.print(percentage);
    display.println("%");
    display.display(true, darkMode); //partial refresh (true)
    guiState = APP_STATE;      
}

void Watchy::showCalendar(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: Calndr");
    #endif
	guiState = APP_STATE;
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(fgColour);
	if(calendarLength==0){
		display.setCursor(70, 90);
		display.print("Empty...");
		display.setCursor(15, 132);
		display.print("Have you synced?");
	}
	// Set position for the first calendar entry
	int x = 0;
	int y = 20;
	
	if(btnPin == UP_BTN_PIN){
		calendarPage--;
	}else if(btnPin == DOWN_BTN_PIN){
		calendarPage++;
	}
	else{   // no button was pressed -> app just entered, set calendar to page 1
		calendarPage = 1;
	}
	if(calendarPage>CALENDAR_MAX_PAGES){ // stop calendar scrolling at last page
		calendarPage = CALENDAR_MAX_PAGES;
	}
	else if(calendarPage<1){ // 3 pages is hardcoded for now
		calendarPage = 1;
	}
	int endEntry = min((calendarPage*3), calendarLength);  //last entry to be displayed on a certain page

	// Print calendar entries from first [0] to the last fetched [calendarLength-1] - in case there is fewer events than the maximum allowed
	for(int i=(calendarPage-1)*3;  i<endEntry; i++) {  //prints 3 events per page

		// Print event time
		display.setCursor(x, y);
		if(calEnt[i].calDate.Day < 10){
			display.print("0");      
		} 
		display.print(calEnt[i].calDate.Day);
		display.print("/");
		if(calEnt[i].calDate.Month < 10){
			display.print("0");      
		}
		display.print(calEnt[i].calDate.Month);
		display.print(" ");
		if (!calEnt[i].allDay){												//not all day event, print time
			if(calEnt[i].calDate.Hour < 10){
				display.print("0");      
			}
			display.print(calEnt[i].calDate.Hour);
			display.print(":");
			if(calEnt[i].calDate.Minute < 10){
				display.print("0");      
			}
			display.print(calEnt[i].calDate.Minute);
		}
		// else{
		// 	display.print("all day");	//just don't print anything
		// }
		
		// Print event title
		display.setCursor(x, y+15);
		for(int j=0; j<CALENDAR_EVENT_TITLE_LENGTH;j++){
			if(calEnt[i].calTitle[j]==NULL){
        #ifdef DEBUG_CALENDAR
        Serial.println("Null character found, breaking print loop");
        #endif
        break;
			}
			display.print(calEnt[i].calTitle[j]);
		}
		#ifdef DEBUG_CALENDAR
		Serial.print(calEnt[i].calDate.Day);
		Serial.print(" | ");
		Serial.println(calEnt[i].calTitle[0]);
		#endif //DEBUG_CALENDAR
		// Prepare y-position for next event entry
		y = y + 60;

		// print page number
		display.setCursor(185, 195);
		display.print(calendarPage);
		#ifdef DEBUG_CALENDAR
		Serial.print("Page: ");
		Serial.println(calendarPage);
		#endif
	}
	display.display(true, darkMode); //partial refresh (true)
}

void Watchy::connectWiFiGUI(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: WiFi");
    #endif
	//TODO: add in functionality to retry wifi
	guiState = APP_STATE;  
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(fgColour);
	display.setCursor(30, 90);
	display.println("Connecting...");
	display.display(false, darkMode);
	display.hibernate();
	String SSID = syncInternetStuff();    //perform NTP n calendar syncing
	display.init(0, false);
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setTextColor(fgColour);
	if(SSID != ""){
		display.setCursor(30, 40);
		display.println("Connected to");
		display.setCursor(30, 60);
		display.println(SSID);
		display.setCursor(5, 20);
		display.println("<Exit");        
		if(lastNtpSyncSuccess){
			display.setCursor(25, 100);
			display.println("NTP Synced to:");
			display.setCursor(70, 120);
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
		if(lastCalendarSyncSuccess){
			display.setCursor(15, 155);
			display.println("Calendar Synced");
		}
	}else{
		display.setCursor(30, 50);
		display.println("Sync failed");
		display.setCursor(30, 70);
		display.println("& timed out!");
		display.setCursor(5, 20);
		display.println("<Exit");
	}
	display.display(false, darkMode);//full refresh
	WiFi.mode(WIFI_OFF);    //calls esp_wifi_stop() to turn off the radio
	//esp_wifi_stop();
	btStop();
}   //connectWiFiGUI

void Watchy::wifiOta(uint8_t btnPin){
	//TODO: add in functionality to retry wifi connection
	guiState = APP_STATE;  
    #ifdef DEBUG
    Serial.println("App: OTA");
    #endif
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(fgColour);
	display.setCursor(55, 80);
	display.println("WiFi OTA");
	display.setCursor(43, 110);
	display.println("Connecting...");
	display.display(true, darkMode);//full refresh
	display.hibernate();    //hibernate to wait for wifi to connect
	#ifndef DEBUG
	Serial.begin(115200);   //enable serial if not enabled already
	#endif
	bool connected = initWiFi();    //start wifi
	if(connected){
		ArduinoOTA
			.onStart([]() {
			String type;
			if (ArduinoOTA.getCommand() == U_FLASH)
					type = "sketch";
			else // U_SPIFFS
					type = "filesystem";

			// NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
			Serial.println("Start updating " + type);
			})
			.onEnd([]() {
			Serial.println("\nEnd");
			})
			.onProgress([](unsigned int progress, unsigned int total) {
			Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
			})
			.onError([](ota_error_t error) {
			Serial.printf("Error[%u]: ", error);
			if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
			else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
			else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
			else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
			else if (error == OTA_END_ERROR) Serial.println("End Failed");
			});

		ArduinoOTA.begin();
		Serial.println("Ready");
		Serial.print("IP address: ");
		Serial.println(WiFi.localIP());
		Serial.println("Press down button to cancel");
        
		attachInterrupt(DOWN_BTN_PIN, ISRStopwatchEnd, RISING); //use the same stopwatch interrupt function to stop OTA

		display.init(0, false); //re-init after hibernating (waiting for wifi)
		display.setFullWindow();
		display.fillScreen(bgColour);
		//display.setTextColor(fgColour);   //TESTING
		display.setCursor(37, 40);
		display.println("Connected to");
		display.setCursor(40, 60);
		display.println(WiFi.SSID());
		display.setCursor(20, 100);
		display.println(WiFi.localIP());
		display.setCursor(20, 130);
		display.println("Upload code now");
		display.setCursor(120, 195);
		display.println("Cancel>"); 
		display.display(true, darkMode);
		display.hibernate();
		while(true){
			if (stopBtnPressed == true){
				detachInterrupt(DOWN_BTN_PIN);
				stopBtnPressed = false;
				#ifndef DEBUG
				Serial.end();    //disables serial if it was disabled before OTA
				#endif //DEBUG
				break;
			}
			ArduinoOTA.handle();    // no display updates here because it'll probably break the OTA update. A successful OTA should reboot the ESP
		} //while(true)
	} //if(connected)
	//if we get this far it means the OTA loop was cancelled

	//turn off WiFi
	WiFi.mode(WIFI_OFF);    // calls esp_wifi_stop()
	//esp_wifi_stop();
	btStop();

	//update screen
	display.init(0, false); //re-init after hibernating
	display.setFullWindow();
	display.fillScreen(bgColour);
	//display.setFont(&FreeMonoBold9pt7b);
	//display.setTextColor(fgColour);
	display.setCursor(40, 110);
	display.println("OTA Aborted");
	display.setCursor(5,20);
	display.println("<Exit");
	display.display(true, darkMode); //partial refresh
} //wifiOta

void Watchy::setPowerSaver(uint8_t btnPin){ //does not do anything at the moment
    #ifdef DEBUG
    Serial.println("App: PwrSaver");
    #endif
	guiState = APP_STATE;
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(fgColour);
	display.setCursor(35, 50);
	display.println("Power Saver");
	display.setCursor(85, 100);
	if(btnPin == DOWN_BTN_PIN){    //toggle power saver if button has been pressed
		powerSaver = !powerSaver;
		#ifdef DEBUG_POWERSAVER
		Serial.print("Power Saver: ");
		Serial.println(powerSaver);  
		#endif  //DEBUG_POWERSAVER
	}
	#ifdef DEBUG
	Serial.print("Button Pressed: ");
	Serial.println(btnPin);  
	#endif  //DEBUG
	display.println(powerSaver ? "On" : "Off");
	display.setCursor(45, 125);
	display.println("Not in use!");
	display.display(true, darkMode);  //partial update
	display.setCursor(115, 195);
	display.println("Toggle>");
	guiState = APP_STATE;    
}

void Watchy::setDarkMode(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: DarkMode");
    #endif
	guiState = APP_STATE;
	if(btnPin == DOWN_BTN_PIN){    //toggle darkmode if button has been pressed
		darkMode = !darkMode;
		fgColour = darkMode ? GxEPD_WHITE : GxEPD_BLACK; 
		bgColour = darkMode ? GxEPD_BLACK : GxEPD_WHITE; 
		#ifdef DEBUG
		Serial.println(darkMode);   //debug
		#endif
	}
	#ifdef DEBUG
	Serial.println(btnPin);   //debug
	#endif
	display.setFullWindow();
	display.fillScreen(bgColour);
	display.setFont(&FreeMonoBold9pt7b);
	display.setTextColor(fgColour);
	display.setCursor(50, 75);
	display.println("Dark Mode");
	display.setCursor(85, 100);
	display.println(darkMode ? "On" : "Off");
	display.setCursor(115, 195);
	display.println("Toggle>");

	if(btnPin == DOWN_BTN_PIN){
			display.display(false, darkMode); //full update since darkmode changes the entire display
	} else{
			display.display(true, darkMode); //partial update
	}   
}

//  GUI to allow the user to manually set the date and time
void Watchy::setTime(uint8_t btnPin){
    #ifdef DEBUG
    Serial.println("App: SetTime");
    #endif
    guiState = APP_STATE;
    RTC.read(currentTime);
    int8_t minute = currentTime.Minute;
    int8_t hour = currentTime.Hour;
    int8_t day = currentTime.Day;
    int8_t month = currentTime.Month;
    int8_t year = currentTime.Year + YEAR_OFFSET - 2000;

    int8_t setIndex = SET_HOUR;

    int8_t blink = 0;

    pinMode(DOWN_BTN_PIN, INPUT);
    pinMode(UP_BTN_PIN, INPUT);
    pinMode(MENU_BTN_PIN, INPUT);  
    pinMode(BACK_BTN_PIN, INPUT);  

    display.setFullWindow();

    while(1){

    if(digitalRead(MENU_BTN_PIN) == 1){
			setIndex++;
			if(setIndex > SET_DAY){
			break;
			}
    }
    if(digitalRead(BACK_BTN_PIN) == 1){
			if(setIndex != SET_HOUR){
			setIndex--;
			}
    }      

    blink = 1 - blink;

    if(digitalRead(DOWN_BTN_PIN) == 1){
			blink = 1;
			switch(setIndex){
			case SET_HOUR:
				hour == 23 ? (hour = 0) : hour++;
				break;
			case SET_MINUTE:
				minute == 59 ? (minute = 0) : minute++;
				break;
			case SET_YEAR:
				year == 99 ? (year = 20) : year++;
				break;
			case SET_MONTH:
				month == 12 ? (month = 1) : month++;
				break;
			case SET_DAY:
				day == 31 ? (day = 1) : day++;
				break;                         
			default:
				break;
			}      
    }

    if(digitalRead(UP_BTN_PIN) == 1){
        blink = 1;
        switch(setIndex){
        case SET_HOUR:
            hour == 0 ? (hour = 23) : hour--;
            break;
        case SET_MINUTE:
            minute == 0 ? (minute = 59) : minute--;
            break;
        case SET_YEAR:
            year == 20 ? (year = 99) : year--;
            break;
        case SET_MONTH:
            month == 1 ? (month = 12) : month--;
            break;
        case SET_DAY:
            day == 1 ? (day = 31) : day--;
            break;          
        default:
            break;
        }   
    }    

    display.fillScreen(bgColour);
    display.setTextColor(fgColour);
    display.setFont(&FreeMonoBold9pt7b);

    display.setCursor(72, 80);
    if(setIndex == SET_HOUR){//blink hour digits
        display.setTextColor(blink ? fgColour : bgColour);
    }
    if(hour < 10){
        display.print("0");      
    }
    display.print(hour);

    display.setTextColor(fgColour);
    display.print(":");

    display.setCursor(104, 80);
    if(setIndex == SET_MINUTE){//blink minute digits
        display.setTextColor(blink ? fgColour : bgColour);
    }
    if(minute < 10){
        display.print("0");      
    }
    display.print(minute);

    display.setTextColor(fgColour);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(45, 150);
    if(setIndex == SET_YEAR){//blink minute digits
        display.setTextColor(blink ? fgColour : bgColour);
    }    
    display.print(2000+year);

    display.setTextColor(fgColour);
    display.print("/");

    if(setIndex == SET_MONTH){//blink minute digits
        display.setTextColor(blink ? fgColour : bgColour);
    }   
    if(month < 10){
        display.print("0");      
    }     
    display.print(month);

    display.setTextColor(fgColour);
    display.print("/");

    if(setIndex == SET_DAY){//blink minute digits
        display.setTextColor(blink ? fgColour : bgColour);
    }       
    if(day < 10){
        display.print("0");      
    }     
    display.print(day); 
    display.display(true, darkMode); //partial refresh
    }

    const time_t FUDGE(10);//fudge factor to allow for upload time, etc. (seconds, YMMV)
    tmElements_t tm;
    tm.Month = month;
    tm.Day = day;
    tm.Year = year + 2000 - YEAR_OFFSET;//offset from 1970, since year is stored in uint8_t
    tm.Hour = hour;
    tm.Minute = minute;
    tm.Second = 0;

    time_t t = makeTime(tm) + FUDGE;
    RTC.set(t);

    showMenu(menuIndex, false);
}   //setTime

#ifdef USING_ACCELEROMETER
//GUI to show temperature
//temperature taken from BMA423 Accelerometer lol
void Watchy::showTemperature(uint8_t btnPin){
    guiState = APP_STATE;
    display.setFullWindow();
    display.fillScreen(bgColour);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(fgColour);
    display.setCursor(40, 30);
    display.println("Temperature");
    float temperature = sensor.readTemperature();

    display.setCursor(70, 80);
    display.print(temperature);
    display.println(" C");
    display.display(true, darkMode); //partialrefresh = true

    guiState = APP_STATE;      
}
#endif //USING_ACCELEROMETER


#ifdef USING_ACCELEROMETER
void Watchy::showAccelerometer(uint8_t btnPin){
    guiState = APP_STATE;
    display.setFullWindow();
    display.fillScreen(bgColour);
    display.setFont(&FreeMonoBold9pt7b);
    display.setTextColor(fgColour);

    Accel acc;

    long previousMillis = 0;

    guiState = APP_STATE;

    pinMode(BACK_BTN_PIN, INPUT);

    while(1){

    unsigned long currentMillis = millis();

    if(digitalRead(BACK_BTN_PIN) == 1){
        break;
    }

    if(currentMillis - previousMillis > DISPLAY_REFRESH_INTERVAL){
        previousMillis = currentMillis;
        // Get acceleration data
        bool res = sensor.getAccel(acc);
        uint8_t direction = sensor.getDirection();
        display.fillScreen(bgColour);      
        display.setCursor(0, 30);
        if(res == false) {
            display.println("getAccel FAIL");
        }else{
        display.print("  X:"); display.println(acc.x);
        display.print("  Y:"); display.println(acc.y);
        display.print("  Z:"); display.println(acc.z);

        display.setCursor(30, 130);
        switch(direction){
            case DIRECTION_DISP_DOWN:
                display.println("FACE DOWN");
                break;
            case DIRECTION_DISP_UP:
                display.println("FACE UP");
                break;
            case DIRECTION_BOTTOM_EDGE:
                display.println("BOTTOM EDGE");
                break;
            case DIRECTION_TOP_EDGE:
                display.println("TOP EDGE");
                break;
            case DIRECTION_RIGHT_EDGE:
                display.println("RIGHT EDGE");
                break;
            case DIRECTION_LEFT_EDGE:
                display.println("LEFT EDGE");
                break;
            default:
                display.println("ERROR!!!");
                break;
        }   //switch(direction)
        }
        display.display(true, darkMode); //partial refresh
    }   //if(currentMillis - previousMillis > interval)
    }   //while(1)
    showMenu(menuIndex, false);
}   //showAccelerometer
#endif //USING_ACCELEROMETER