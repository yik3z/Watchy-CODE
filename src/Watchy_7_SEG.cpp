#include "Watchy_7_SEG.h"
#include "config.h"

const uint8_t BATTERY_BAR_HEIGHT = 4; 
const uint8_t DATE_TIME_X_0 = 15;
const uint8_t DARK_TIME_Y_0 = 120; 
const uint8_t LIGHT_TIME_Y_0 = 110; 
const uint8_t DATE_X_0 = 137; 
const uint8_t DARK_DATE_Y_0 = 154;
const uint8_t LIGHT_DATE_Y_0 = 144;
const uint8_t SPLITTER_LENGTH = 165;
const uint8_t TEMPERATURE_X_0 = 145;
const uint8_t TEMPERATURE_Y_0 = 175;

Watchy7SEG::Watchy7SEG(){} //constructor

void Watchy7SEG::drawWatchFace(){
    #ifdef DEBUG
    Serial.print("Power Saver: ");
    Serial.println(powerSaver);
    #endif  //DEBUG
    display.fillScreen(bgColour);
    display.setTextColor(fgColour);
    drawTime();
    drawDate();
    //drawTemperature();
    drawBatteryBar();
    drawBleWiFi();
    syncNtpTime();
    if(lowBatt != 0){
        drawLowBatt();
    }
}

void Watchy7SEG::drawTime(){
    display.setFont(&DIN_Black35pt7b);
    if(darkMode){
        display.setCursor(DATE_TIME_X_0 - 5, DARK_TIME_Y_0);
    } else {
        display.setCursor(DATE_TIME_X_0 - 5, LIGHT_TIME_Y_0);
    }
    if(currentTime.Hour < 10){
        display.print("0");
    }
    display.print(currentTime.Hour);
    display.print(":");
    if(hourlyTimeUpdate && wakeup_reason == ESP_SLEEP_WAKEUP_EXT0){ //is hourly time update, it was an RTC (scheduled) wakeup
        display.println("xx");  
    } 
    else{
        if(currentTime.Minute < 10){
            display.print("0");
        }  
        display.println(currentTime.Minute);  
    }
}

void Watchy7SEG::drawDate(){

    //divider line
    if(darkMode){
        display.fillRect(DATE_TIME_X_0, DARK_TIME_Y_0 + 10, SPLITTER_LENGTH, 3, fgColour);
    } else{
        display.fillRect(DATE_TIME_X_0, LIGHT_TIME_Y_0 + 10, SPLITTER_LENGTH, 3, fgColour);
    }
    display.setFont(&DIN_Medium10pt7b);
    String dayOfWeek = dayStr(currentTime.Wday);
    if(darkMode){
        display.setCursor(DATE_TIME_X_0, DARK_DATE_Y_0);
    } else{
        display.setCursor(DATE_TIME_X_0, LIGHT_DATE_Y_0);
    }
    display.println(dayOfWeek);
    if(darkMode){
        display.setCursor(DATE_X_0, DARK_DATE_Y_0);
    } else{
        display.setCursor(DATE_X_0, LIGHT_DATE_Y_0);
    }
    if(currentTime.Day < 10){
    display.print("0");      
    }     
    display.print(currentTime.Day);

    display.print("."); 
    if(currentTime.Month < 10){
    display.print("0");      
    }
    display.print(currentTime.Month);
    /* //removed year printout
    display.print("."); 
    uint16_t yearTwoDigits = currentTime.Year + YEAR_OFFSET - 2000; //to get '21 hehe
    display.println(yearTwoDigits); // offset from 1970, since year is stored in uint8_t
    */    
}

void Watchy7SEG::drawBatteryBar(){
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
    uint8_t batteryBarWidth = percentage * 2; //beacuse it's 200px wide lol
    
    display.fillRect(0, 0, batteryBarWidth, BATTERY_BAR_HEIGHT, fgColour);
}
void Watchy7SEG::drawBleWiFi(){
    if(BLE_CONFIGURED){ 
        display.drawBitmap(150, 20, bluetoothIcon, 13, 21, fgColour);
    }
    if(WIFI_ON){ 
        display.drawBitmap(168, 20, wifiIcon, 26, 18, fgColour);
    }
}


void Watchy7SEG::drawLowBatt(){
        display.setFont(&FreeMonoBold9pt7b);
    if(lowBatt == 1){
        display.drawBitmap(150, 20, lowBattIcon, 3, 8, fgColour);
    }
    else if (lowBatt == 2){
        display.drawBitmap(150, 20, noBattIcon, 1, 0, fgColour);
    }
}

/* function disabled till I figure out what's the issue with the wrong temp
void Watchy7SEG::drawTemperature(){
    weatherData currentWeather = getWeatherData(false); //get temp from RTC
    int8_t temperature = currentWeather.temperature;
    display.setFont(&DIN_Medium10pt7b);

    display.setCursor(TEMPERATURE_X_0, TEMPERATURE_Y_0);
    display.print(temperature);
    display.println(" C");
    //display.drawBitmap(TEMPERATURE_X_CENTER, TEMPERATURE_Y_0, strcmp(TEMP_UNIT, "metric") == 0 ? celsius : fahrenheit, 26, 20, fgColour);
}
*/
