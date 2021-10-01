#ifndef WATCHY_BATT_LUT_H
#define WATCHY_BATT_LUT_H
#include <Arduino.h>

//Look up table for battery voltage to percentage conversion
const uint16_t battPercentLUT[21]{
    3270,   //0
    3610,
    3690,
    3710,
    3730,   //20
    3750,
    3770,
    3790,
    3800,   //40
    3820,
    3840,   //50
    3850,
    3870,   //60
    3910,
    3950,
    3980,
    4020,   //80
    4080,
    4110,
    4150,
    4200    //100
};


#endif //WATCHY_BATT_LUT_H