// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2
//
// This code has been modified from the library

#include "GxEPD2_154_D67.h"

GxEPD2_154_D67::GxEPD2_154_D67(int8_t cs, int8_t dc, int8_t rst, int8_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, HIGH, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
}

void GxEPD2_154_D67::clearScreen(uint8_t value)
{
  writeScreenBuffer(value);
  refresh(true);
  writeScreenBufferAgain(value);
}

void GxEPD2_154_D67::writeScreenBuffer(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  if (_initial_write) _writeScreenBuffer(0x26, value); // set previous
  _writeScreenBuffer(0x24, value); // set current
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_154_D67::writeScreenBufferAgain(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  _writeScreenBuffer(0x24, value); // set current
}

void GxEPD2_154_D67::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  //_writeCommand(command);
  _startTransfer();
  _transferCommand(command);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    //_writeData(value);
    _transfer(value);
  }
  _endTransfer();
}

void GxEPD2_154_D67::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x26, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  #if defined(ESP8266) || defined(ESP32)
  yield(); // to avoid WDT on ESP8266 and ESP32
  #endif
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
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  //_writeCommand(command);
  _startTransfer();
  _transferCommand(command);
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
      //_writeData(data);
      _transfer(data);
    }
  }
  _endTransfer();
  #if defined(ESP8266) || defined(ESP32)
  yield(); // to avoid WDT on ESP8266 and ESP32
  #endif
}

void GxEPD2_154_D67::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  #if defined(ESP8266) || defined(ESP32)
  yield(); // to avoid WDT on ESP8266 and ESP32
  #endif
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
  _setPartialRamArea(x1, y1, w1, h1);
  //_writeCommand(command);
  _startTransfer();
  _transferCommand(command);
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
      //_writeData(data);
      _transfer(data);
    }
  }
  _endTransfer();
  #if defined(ESP8266) || defined(ESP32)
  yield(); // yield() to avoid WDT on ESP8266 and ESP32
  #endif
}

void GxEPD2_154_D67::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else
  {
    if (_using_partial_mode) _Init_Full();
    _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void GxEPD2_154_D67::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  w1 -= x1 - x;
  h1 -= y1 - y;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _Update_Part();
}

void GxEPD2_154_D67::powerOff()
{
  _PowerOff();
}

//is actually deep sleep in the manufacturer's documentation
void GxEPD2_154_D67::hibernate()
{
  //_PowerOff();  //removed, seems like it's wrong code anyway
  if (_rst >= 0)
  {
    _writeCommand(0x10); // deep sleep mode
    _writeData(0x1);     // enter deep sleep (mode 1, RAM is retained)
    //_waitWhileBusy("_PowerOff", power_off_time);  //disabled since watch is also going to sleep and won't interact with display (not sure if a quick wake might cause issues)
    _hibernating = true;
  }
}

void GxEPD2_154_D67::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _startTransfer();
  _transferCommand(0x11); // set ram entry mode
  _transfer(0x03);    // x increase, y increase : normal mode
  _transferCommand(0x44);
  _transfer(x / 8);
  _transfer((x + w - 1) / 8);
  _transferCommand(0x45);
  _transfer(y % 256);
  _transfer(y / 256);
  _transfer((y + h - 1) % 256);
  _transfer((y + h - 1) / 256);
  _transferCommand(0x4e);
  _transfer(x / 8);
  _transferCommand(0x4f);
  _transfer(y % 256);
  _transfer(y / 256);
  _endTransfer();
}

//seems all wrong...disabled becuase it seems to not do anything
void GxEPD2_154_D67::_PowerOn()
{
  if (!_power_is_on)
  {
    // _writeCommand(0x22);
    // _writeData(0xf8);
    // _writeCommand(0x20);
    // _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

//seems all wrong...disabled becuase it seems to not do anything
void GxEPD2_154_D67::_PowerOff()  
{
  if (_power_is_on)
  {
    //_writeCommand(0x22);
    //_writeData(0x83);
    //_writeCommand(0x20);

    //_waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_154_D67::_InitDisplay()
{
  if (_hibernating) _reset();
  _writeCommand(0x12); // soft reset
  delay(10); // 10ms according to specs
  _startTransfer();
  _transferCommand(0x01); // Driver output control
  _transfer(0xC7);        // Default
  _transfer(0x00);
  _transfer(0x00);
  _transferCommand(0x03); // Gate Driving Voltage
  _transfer(0x03);        // 10V
  _transferCommand(0x04); // Source Driving Voltage
  _transfer(0xc1);        // VSH1 7.5 V
  _transfer(0x8f);        // VSH2 2.5 V
  _transfer(0x14);        // VSL -7.5 V
  _transferCommand(0x3C); // BorderWavefrom
  _transfer(borderColour ? 0x02 : 0x05);    //0x05 for white or 0x02 for black border
  _transferCommand(0x18); // Read built-in temperature sensor
  _transfer(0x80);
  _endTransfer();

  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
}

void GxEPD2_154_D67::_Init_Full()
{
  _InitDisplay();
  //_PowerOn();
  _using_partial_mode = false;
}

void GxEPD2_154_D67::_Init_Part()
{
  _InitDisplay();
  //_PowerOn();
  _using_partial_mode = true;
}

void GxEPD2_154_D67::_Update_Full()
{
  #ifdef DEBUG_TIMING
  Serial.println("start updatefull: " + String(millis()));
  #endif //DEBUG_TIMING
  _startTransfer();
  _transferCommand(0x22);
  _transfer(0xf4); //was 0xf4, supposed to be 0xf7??
  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_Update_Full", full_refresh_time);
  #ifdef DEBUG_TIMING
  Serial.println("end updatefull: " + String(millis()));
  #endif //DEBUG_TIMING
}

void GxEPD2_154_D67::_Update_Part()
{
  #ifdef DEBUG_TIMING
  Serial.println("start updatepart: " + String(millis()));
  #endif //DEBUG_TIMING
  _startTransfer();
  _transferCommand(0x22);
  _transfer(0xfc);   //was 0xfc, supposd to be 0xff??
  _transferCommand(0x20);
  _endTransfer();
  _waitWhileBusy("_Update_Part", partial_refresh_time);  
  #ifdef DEBUG_TIMING
  Serial.println("end updatepart: " + String(millis()));
  #endif //DEBUG_TIMING
}