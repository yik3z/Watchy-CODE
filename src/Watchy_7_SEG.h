#ifndef WATCHY_7_SEG_H
#define WATCHY_7_SEG_H

#include <Watchy.h>

#include "DIN_Black35pt7b.h"
#include "DIN_Medium10pt7b.h"
#include "icons.h"

class Watchy7SEG : public Watchy{
    public:
        Watchy7SEG();
        void drawWatchFace();
        void drawTime();
        void drawDate();
        //void drawTemperature();   //temp is messed up idk why. Seems like hardware issue
        void drawBatteryBar();
        void drawBleWiFi();
};

#endif