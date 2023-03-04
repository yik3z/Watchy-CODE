#ifndef WATCHFACE_DIN
#define WATCHFACE_DIN

#include <Watchy.h>
#include "DIN_Black35pt7b.h"
#include "DIN_Medium10pt7b.h"
#include "icons.h"

class Watchface_DIN : public Watchy{
    public:
        Watchface_DIN();
        void drawWatchFace();
        void drawTime();
        void drawDate();
        void drawNextCalendarEvent();
        //void drawTemperature();   //temp is messed up idk why. Seems like hardware issue
        void drawBatteryBar();
        //void drawBleWiFi();   //likely won't be using it since wifi/ble won't be constantly on
        void drawLowBatt();
};

#endif