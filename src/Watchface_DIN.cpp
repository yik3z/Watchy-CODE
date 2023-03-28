#include "Watchface_DIN.h"

const uint8_t BATTERY_BAR_HEIGHT = 4; 
const uint8_t DATE_TIME_X_0 = 15;
const uint8_t DATE_TIME_Y_0 = 100; 
const uint8_t DATE_X_0 = 137; 
const uint8_t DATE_Y_0 = 134;
const uint8_t SPLITTER_LENGTH = 165;
const uint8_t TEMPERATURE_X_0 = 145;
const uint8_t TEMPERATURE_Y_0 = 175;

extern RTC_DATA_ATTR bool powerSaver; 
extern RTC_DATA_ATTR bool hourlyTimeUpdate;

extern RTC_DATA_ATTR calendarEntries calEnt[CALENDAR_ENTRY_COUNT];  //to print next event on watchface
extern RTC_DATA_ATTR int calendarLength;

Watchface_DIN::Watchface_DIN(){} //constructor

void Watchface_DIN::drawWatchFace(){
    #ifdef DEBUG
    Serial.println("GUI: Watchface");
    #endif
    display.fillScreen(bgColour);
    display.setTextColor(fgColour);
    drawTime();
    drawDate();
    if(!hourlyTimeUpdate){
      drawNextCalendarEvent();
    }
    //drawTemperature();
    drawBatteryBar();
    //drawBleWiFi();
    if(lowBatt != 0){
      drawLowBatt();
    }
}

void Watchface_DIN::drawTime(){
  display.setFont(&DIN_Black35pt7b);
  display.setCursor(DATE_TIME_X_0 - 5, DATE_TIME_Y_0);
  if(currentTime.Hour < 10){
    display.print("0");
  }
  display.print(currentTime.Hour);
  display.print(":");
  if(hourlyTimeUpdate){ // print :xx for mins since the time updates only once an hour
    display.println("xx");  
  } 
  else{
    if(currentTime.Minute < 10){
      display.print("0");
    }  
    display.println(currentTime.Minute);  
  }
  // show actual time on buttonpress during hourlytimeupdate above the :xx time
  if(hourlyTimeUpdate && wakeup_reason == ESP_SLEEP_WAKEUP_EXT1){ // button press during hourly time update
    display.setFont(&DIN_Medium10pt7b);
    display.setCursor(80, 43);
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
}

void Watchface_DIN::drawDate(){
	//draw divider line
	display.fillRect(DATE_TIME_X_0, DATE_TIME_Y_0 + 10, SPLITTER_LENGTH, 3, fgColour);

  //draw day, date
	display.setFont(&DIN_Medium10pt7b);
	String dayOfWeek = dayStr(currentTime.Wday);
	display.setCursor(DATE_TIME_X_0, DATE_Y_0);
	display.println(dayOfWeek);
	display.setCursor(DATE_X_0, DATE_Y_0);
	if(currentTime.Day < 10){
	display.print("0");      
	}     
	display.print(currentTime.Day);

	display.print("."); 
	if(currentTime.Month < 10){
	display.print("0");      
	}
	display.print(currentTime.Month);
	// //removed year printout
	// display.print("."); 
	// uint16_t yearTwoDigits = currentTime.Year + YEAR_OFFSET - 2000; //to get '21 hehe
	// display.println(yearTwoDigits); // offset from 1970, since year is stored in uint8_t
}

void Watchface_DIN::drawNextCalendarEvent(){
  //choose event to display
  int nextCalEvent = -1;
  for (int i=0;i<calendarLength;i++){
    if (calEnt[i].calDate.Day==currentTime.Day){	//check if there's anything happening today (not checking for months cause I'm assuming that we won't get that far)
			if((calEnt[i].calDate.Hour>=currentTime.Hour) or (calEnt[i].allDay)){
				nextCalEvent = i;
				break;
			}
    }
		else if ((calEnt[i].calDate.Day>=currentTime.Day)){	// choose the next event happening after today
			nextCalEvent = i;
      break;
    }
  }
	if (nextCalEvent==-1){														// no calendar events to display. Don't draw anything
		return;
	}
	// print event title
  display.setCursor(5, 170);
  for(int j=0; j<17;j++){	// seems to print the first char after the end of the last one
    if(calEnt[nextCalEvent].calTitle[j]==0){
      break;
    }
    display.print(calEnt[nextCalEvent].calTitle[j]);
  }
	// print time of next event
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(5, 185);
  if (calEnt[nextCalEvent].calDate.Day!=currentTime.Day){	 // event isn't today (checks day only), print date
    if(calEnt[nextCalEvent].calDate.Day < 10){
    display.print("0");      
    } 
    display.print(calEnt[nextCalEvent].calDate.Day);
    display.print("/");
    if(calEnt[nextCalEvent].calDate.Month < 10){
    display.print("0");      
    }
    display.print(calEnt[nextCalEvent].calDate.Month);
    display.print(" ");
  }
	if (!calEnt[nextCalEvent].allDay){										//not all day event, print time
		if(calEnt[nextCalEvent].calDate.Hour < 10){
			display.print("0");      
		}
    display.print(calEnt[nextCalEvent].calDate.Hour);
    display.print(":");
    if(calEnt[nextCalEvent].calDate.Minute < 10){
        display.print("0");      
    }
    display.print(calEnt[nextCalEvent].calDate.Minute);
  }
  return;
}

void Watchface_DIN::drawBatteryBar(){
    //battery bar
    display.fillRect(0, 0, DISPLAY_WIDTH, BATTERY_BAR_HEIGHT, bgColour); //clear battery bar; IS THIS EVEN REQUIRED?
    uint32_t vBatt = getBatteryVoltage();
    //https://github.com/G6EJD/LiPo_Battery_Capacity_Estimator/blob/master/ReadBatteryCapacity_LIPO.ino as linked by Huey's github
    uint8_t percentage = getBatteryPercent(vBatt);
    /*  //removed in favour of LUT
    if (vBatt >= 4200) percentage = 100;
    else if (vBatt <= 3500) percentage = 0;
    else {
        float VBAT = vBatt / 1000.0;    //TODO try to get rid of the floating point calculation
                                        //maybe use a LUT
        percentage = 2808.3808 * pow(VBAT, 4) - 43560.9157 * pow(VBAT, 3) + 252848.5888 * pow(VBAT, 2) - 650767.4615 * VBAT + 626532.5703;
    }
    */
    uint8_t batteryBarWidth = percentage * 2; // beacuse it's 200px wide lol
    
    display.fillRect(0, 0, batteryBarWidth, BATTERY_BAR_HEIGHT, fgColour);
}
/* probably won't be used since we can't keep wifi or BLE on anyway
void Watchface_DIN::drawBleWiFi(){
    if(BLE_CONFIGURED){ 
        display.drawBitmap(150, 20, bluetoothIcon, 13, 21, fgColour);
    }
    if(WIFI_ON){ 
        display.drawBitmap(168, 20, wifiIcon, 26, 18, fgColour);
    }
}
*/

void Watchface_DIN::drawLowBatt(){
        display.setFont(&FreeMonoBold9pt7b);
    if(lowBatt == 1){
        display.drawBitmap(3, 8, lowBattIcon, 16, 9, fgColour);
    }
    else if (lowBatt == 2){
        display.drawBitmap(1, 0, noBattIcon, 19, 15, fgColour);
    }
}

/* function disabled till I figure out what's the issue with the wrong temp
void Watchface_DIN::drawTemperature(){
    weatherData currentWeather = getWeatherData(false); //get temp from RTC
    int8_t temperature = currentWeather.temperature;
    display.setFont(&DIN_Medium10pt7b);

    display.setCursor(TEMPERATURE_X_0, TEMPERATURE_Y_0);
    display.print(temperature);
    display.println(" C");
    //display.drawBitmap(TEMPERATURE_X_CENTER, TEMPERATURE_Y_0, strcmp(TEMP_UNIT, "metric") == 0 ? celsius : fahrenheit, 26, 20, fgColour);
}
*/
