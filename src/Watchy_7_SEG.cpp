#include "Watchy_7_SEG.h"
#include "config.h"

const uint8_t BATTERY_BAR_HEIGHT = 4; 
const uint8_t DATE_TIME_X_0 = 15;
const uint8_t TIME_Y_0 = 110; 
const uint8_t DATE_Y_0 = 144;
const uint8_t TEMPERATURE_X_0 = 145;
const uint8_t TEMPERATURE_Y_0 = 175;

//battery LUT to replace float calculations
const uint32_t battPercentArray[11] = {
    3406,
    3504,
    3595,
    3633,
    3674,
    3764,
    3878,
    3941,
    4012,
    4080,
    4177
};

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
}
void Watchy7SEG::drawTime(){
    display.setFont(&DIN_Black35pt7b);

    display.setCursor(DATE_TIME_X_0 - 5, TIME_Y_0);
    if(currentTime.Hour < 10){
        display.print("0");
    }
    display.print(currentTime.Hour);
    display.print(":");
    if(hourlyTimeUpdate){
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
    display.fillRect(DATE_TIME_X_0, TIME_Y_0 + 10, 165, 3, fgColour);

    display.setFont(&DIN_Medium10pt7b);
    String dayOfWeek = dayStr(currentTime.Wday);
    display.setCursor(DATE_TIME_X_0, DATE_Y_0);
    display.println(dayOfWeek);

    display.setCursor(113, DATE_Y_0);
    if(currentTime.Day < 10){
    display.print("0");      
    }     
    display.print(currentTime.Day);

    display.print("."); 
    if(currentTime.Month < 10){
    display.print("0");      
    }
    display.print(currentTime.Month);
    display.print("."); 
    uint16_t yearTwoDigits = currentTime.Year + YEAR_OFFSET - 2000; //to get '21 hehe
    display.println(yearTwoDigits); // offset from 1970, since year is stored in uint8_t

    
}

void Watchy7SEG::drawBatteryBar(){
    //battery bar
    display.fillRect(0, 0, DISPLAY_WIDTH, BATTERY_BAR_HEIGHT, bgColour); //clear battery bar; IS THIS EVEN REQUIRED?
    uint32_t vBatt = getBatteryVoltage();
    //https://github.com/G6EJD/LiPo_Battery_Capacity_Estimator/blob/master/ReadBatteryCapacity_LIPO.ino as linked by Huey's github
    uint8_t percentage = 100;
    if (vBatt >= 4200) percentage = 100;
    else if (vBatt <= 3500) percentage = 0;
    else {
        float VBAT = vBatt / 1000.0;    //TODO try to get rid of the floating point calculation
                                        //maybe use a LUT
        percentage = 2808.3808 * pow(VBAT, 4) - 43560.9157 * pow(VBAT, 3) + 252848.5888 * pow(VBAT, 2) - 650767.4615 * VBAT + 626532.5703;
    }
    
    uint8_t batteryBarWidth = percentage * 2; //beacuse it's 200px wide lol
    display.fillRect(0, 0, batteryBarWidth, BATTERY_BAR_HEIGHT, fgColour);
}
void Watchy7SEG::drawBleWiFi(){
    if(BLE_CONFIGURED){ 
        display.drawBitmap(150, 20, bluetooth, 13, 21, fgColour);
    }
    if(WIFI_ON){ 
        display.drawBitmap(168, 20, wifi, 26, 18, fgColour);
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
