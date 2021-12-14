// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEW0154M09 : http://www.e-paper-display.com/products_detail/productId=513.html
// Controller : JD79653A : http://www.e-paper-display.com/download_detail/downloadsId%3d943.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2
//
// This code has been modified from the library

#include "GxEPD2_154_M09.h"

GxEPD2_154_M09::GxEPD2_154_M09(int8_t cs, int8_t dc, int8_t rst, int8_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, LOW, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
}

void GxEPD2_154_M09::clearScreen(uint8_t value)
{
  Serial.println("Clear Screen");
  writeScreenBuffer(value);
  refresh(true);
  writeScreenBufferAgain(value);
}

void GxEPD2_154_M09::writeScreenBuffer(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  _writeScreenBuffer(0x13, value); // set current
  if (_initial_write) _writeScreenBuffer(0x10, value); // set previous  //CHANGED from _initial_refresh
    _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_154_M09::writeScreenBufferAgain(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  _writeScreenBuffer(0x10, value); // set previous
}

void GxEPD2_154_M09::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  _writeCommand(command);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _writeData(value);
  }
}

void GxEPD2_154_M09::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.println("WriteImage");
  _writeImage(0x13, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  //Serial.println("WriteImage Again");
  _writeImage(0x10, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  Serial.println("_writeImg, buff: " + String(command));
  if (_initial_write) {
    writeScreenBuffer(); // initial full screen buffer clean
    Serial.println("Initial Write");
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  //if (!_using_partial_mode) _Init_Part();
  if(_using_partial_mode) _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb, h of bitmap for index!
      int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _writeData(data);
    }
  }
 // _writeCommand(0x92); // partial out
  //Serial.println("PartialOut");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_154_M09::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x13, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x10, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  Serial.println("_writeImgPrt, buff: " + String(command));
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _writeCommand(0x91); // partial in
  Serial.println("PartialIn");
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + ((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + (y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _writeData(data);
    }
  }
  _writeCommand(0x92); // partial out
  Serial.println("PartialOut");
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_154_M09::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_M09::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_M09::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else  //full update mode
  {
    if (_using_partial_mode) _Init_Full();
    _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void GxEPD2_154_M09::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  Serial.println("Refresh Partial");
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  w1 -= x1 - x;
  h1 -= y1 - y;
  //if (!_using_partial_mode) _Init_Part();
  //_writeCommand(0x91); // partial in
  //Serial.println("PartialIn");
  //_setPartialRamArea(x1, y1, w1, h1);
  _Update_Part();
  //_writeCommand(0x92); // partial out
  //Serial.println("PartialOut");
}

void GxEPD2_154_M09::powerOff(void)
{
  _PowerOff();
}

void GxEPD2_154_M09::hibernate()
{
  _writeCommand(0x50);  //cHANGED: added
  _writeData(0xf7);     //cHANGED: added
  _PowerOff();  //TODO seems like it doesn't power off ever if even one partial update is done
  if (_rst >= 0)
  {
    _writeCommand(0x07); // deep sleep
    _writeData(0xA5);    // check code
    _hibernating = true;
  }
}

void GxEPD2_154_M09::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  uint16_t xe = x + w;
  uint16_t ye = y + h;
  if (xe >199){
    xe = 199;
  }
  if (ye >199){
    ye = 199;
  }
  _writeCommand(0x90); // partial window
  
  _writeData(x & 0xFFF8);
  _writeData(xe | 0b111);
  _writeData(0);
  _writeData(y);
  _writeData(0);
  _writeData(ye);
  _writeData(0x01); // don't see any difference
  //_writeData(0x00); // don't see any difference
}

void GxEPD2_154_M09::_PowerOn()
{
  if (!_power_is_on)
  {
    Serial.println("PwrOn");
    _writeCommand(0x04);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_154_M09::_PowerOff()
{
  if (_power_is_on)
  {
    //if (_using_partial_mode) _Update_Part();//_InitDisplay();  // would hang on _powerOn() without
    Serial.println("PwrOff");
    _writeCommand(0x02); // power off
    _waitWhileBusy("_PowerOff", power_off_time);
    _power_is_on = false;
    _using_partial_mode = false;
  }
}

void GxEPD2_154_M09::_InitDisplay()
{
  if (_hibernating) _reset();
  _writeCommand(0x00); // panel setting
  _writeData (0xff);  
  _writeData (0x09);  //was 0x0e
  //_writeCommand(0x01); // power setting
  //_writeData(0x03);
  //_writeData(0x06); // 16V
  //_writeData(0x2A);//
  //_writeData(0x2A);//
  _writeCommand(0x4D); // FITIinternal code
  _writeData (0x55);
  _writeCommand(0xaa);
  _writeData (0x0f);
  _writeCommand(0xE9);
  _writeData (0x02);
  _writeCommand(0xb6);
  _writeData (0x11);
  _writeCommand(0xF3);
  _writeData (0x0a);
  //_writeCommand(0x06); // boost soft start
  //_writeData (0xc7);
  //_writeData (0x0c);
  //_writeData (0x0c);
  _writeCommand(0x61); // resolution setting
  _writeData (0b11001000); // was 200 (0xc8), changed to 25<<3
  _writeData (0x00);
  _writeData (0xc8); // 200
  _writeCommand(0x60); // Tcon setting
  _writeData (0x00);
  //_writeCommand(0x82); // VCOM DC setting
  //_writeData (0x12);
  //_writeCommand(0x30); // PLL control
  //_writeData (0x3C);   // default 50Hz
  _writeCommand(0X50); // VCOM and data interval + V border
  _writeData(0x97);    //or 0x57 for black border, 0x97 for white
  _writeCommand(0XE3); // power saving register
  _writeData(0x00);    // default

}

const unsigned char GxEPD2_154_M09::lut_20_vcomDC[] PROGMEM =
{
  0x01, 0x05, 0x05, 0x05, 0x05, 0x01, 0x01,
  0x01, 0x05, 0x05, 0x05, 0x05, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_21_ww[] PROGMEM =
{
  0x01, 0x45, 0x45, 0x43, 0x44, 0x01, 0x01,
  0x01, 0x87, 0x83, 0x87, 0x06, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_22_bw[] PROGMEM =
{
  0x01, 0x05, 0x05, 0x45, 0x42, 0x01, 0x01,
  0x01, 0x87, 0x85, 0x85, 0x85, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_23_wb[] PROGMEM =
{
  0x01, 0x08, 0x08, 0x82, 0x42, 0x01, 0x01,
  0x01, 0x45, 0x45, 0x45, 0x45, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_24_bb[] PROGMEM =
{
  0x01, 0x85, 0x85, 0x85, 0x83, 0x01, 0x01,
  0x01, 0x45, 0x45, 0x04, 0x48, 0x01, 0x01,
  0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_20_vcomDC_partial[] PROGMEM =
{
  0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_21_ww_partial[] PROGMEM =
{
  0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
const unsigned char GxEPD2_154_M09::lut_22_bw_partial[] PROGMEM =
{
  0x01, 0x84, 0x84, 0x83, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_23_wb_partial[] PROGMEM =
{
  0x01, 0x44, 0x44, 0x43, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const unsigned char GxEPD2_154_M09::lut_24_bb_partial[] PROGMEM =
{
  0x01, 0x04, 0x04, 0x03, 0x01, 0x01, 0x01,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

void GxEPD2_154_M09::initFull(){
  _Init_Full();
}

//inits display in preparation for full mode
void GxEPD2_154_M09::_Init_Full()
{
  Serial.println("Init Full");
  _InitDisplay();
  _writeCommand(0x20);
  _writeDataPGM(lut_20_vcomDC, sizeof(lut_20_vcomDC));
  _writeCommand(0x21);
  _writeDataPGM(lut_21_ww, sizeof(lut_21_ww));
  _writeCommand(0x22);
  _writeDataPGM(lut_22_bw, sizeof(lut_22_bw));
  _writeCommand(0x23);
  _writeDataPGM(lut_23_wb, sizeof(lut_23_wb));
  _writeCommand(0x24);
  _writeDataPGM(lut_24_bb, sizeof(lut_24_bb));
  //_PowerOn();
  _using_partial_mode = false;
}

//inits partial mode and sets display to partial mode in
void GxEPD2_154_M09::initPart(){
  _Init_Part();
  _writeCommand(0x91);  //partial in
}

//partial mode out
void GxEPD2_154_M09::partialOut(){
  _writeCommand(0x92);  //partial out
  _using_partial_mode = false;

}

void GxEPD2_154_M09::_Init_Part()
{
  Serial.println("Init Part");
  _InitDisplay();
  _writeCommand(0x20);
  _writeDataPGM(lut_20_vcomDC_partial, sizeof(lut_20_vcomDC_partial));
  _writeCommand(0x21);
  _writeDataPGM(lut_21_ww_partial, sizeof(lut_21_ww_partial));
  _writeCommand(0x22);
  _writeDataPGM(lut_22_bw_partial, sizeof(lut_22_bw_partial));
  _writeCommand(0x23);
  _writeDataPGM(lut_23_wb_partial, sizeof(lut_23_wb_partial));
  _writeCommand(0x24);
  _writeDataPGM(lut_24_bb_partial, sizeof(lut_24_bb_partial));
  //_PowerOn();
  _using_partial_mode = true;
}

void GxEPD2_154_M09::_Update_Full()
{
  _PowerOn(); //turn on power just before refreshing
  Serial.println("Update Full");
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_154_M09::_Update_Part()
{
  _PowerOn(); //turn on power just before refreshing
  Serial.println("Update Part");
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}