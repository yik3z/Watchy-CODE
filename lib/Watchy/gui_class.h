/*!
 *
 * @brief GUI class for Watchy. Used to simplify GUI elements for the touchscreen
 * Requires the GxEPD library for drawing
 * Designed to work with deep sleep
 * Still foguring out how to do it...
 */

#ifndef GUI_LIBRARY
#define GUI_LIBRARY

#include "Arduino.h"
#include <GxEPD2_BW.h>

class GUI_Menu {
public:
  GUI_Menu(void);
  GUI_Menu(int16_t x, int16_t y, int16_t z, uint8_t gesture);

  void registerApps();
  void generatePage();
  void nextPage();
  void prevPage();
  void handleInput();

  uint32_t totalPages;
  uint32_t currentPage;
  uint32_t buttonWidth;
  uint32_t buttonHeight;

private:
  uint8_t _menuElements_x;
  uint8_t _menuElements_y;
  uint8_t _menuElements;
};




#endif