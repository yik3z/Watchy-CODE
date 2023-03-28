#include "Watchy.h"

extern uint64_t wakeupBit;

// menu variables
RTC_DATA_ATTR uint32_t menuPageNumber = 1;  // page number used by all the menus

// menu constants
const uint32_t numColumnsRows = 2;        // number of rows = number of columns
const uint32_t gap = 10;                  // gap between 2 boxes
const uint32_t boxLength = 70;            // pixels. Can be calculated from (200-(numColumnsRows+1)*gap)/numColumnsRows
const uint32_t pageArrowBoxLength = 20;   // pixels. Length of box around "<" and ">" arrows 

// main menu. Remember to add them to _menuAppLauncher
const char *mainMenuItems[] = {"Stats","Motor","Cal","Set Time","Dark/Light Mode","Power","Temp","Timer","WiFi","OTA"};
const uint32_t mainMenuOptions = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
const uint32_t mainMenuPages = (mainMenuOptions-1)/4 + 1; // total number of pages. TODO: check

// clock menu
const char *clockMenuItems[] = {"Timer","Cal","Set Time","NTP"};
const uint32_t clockMenuOptions = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
const uint32_t clockMenuPages = (clockMenuOptions-1)/4 + 1; // total number of pages. TODO: check

// Scrolls through menu and opens app if selected
// TODO
void Watchy::_menuInteractionHandler(void (*showMenu)(bool), uint32_t menuPages){
  if(wakeupBit & UP_BTN_MASK){
    menuPageNumber--;
    if(menuPageNumber < 1){
      menuPageNumber = menuPages;
    }    
    showMenu(true);
  } else if(wakeupBit & DOWN_BTN_MASK){
    menuPageNumber++;
    if(menuPageNumber > menuPages){
      menuPageNumber = 1;
    }
    showMenu(true);
  } else if(wakeupBit & TS_INT_PIN_MASK){
    // click go to app

    // click to change page
    if(_tpWithinBounds(67,67+pageArrowBoxLength,170,170+pageArrowBoxLength)){
      menuPageNumber--;
      if(menuPageNumber < 1){
        menuPageNumber = menuPages;
      }  
    } else if(_tpWithinBounds(112,112+pageArrowBoxLength,170,170+pageArrowBoxLength)){
      menuPageNumber++;
      if(menuPageNumber > menuPages){
        menuPageNumber = 1;
      }
    }
  }
} // menuInteractionHandler

// Touch friendly menu
void Watchy::showMainMenu(bool partialRefresh){
  #ifdef DEBUG
  Serial.println("GUI: Menu");
  #endif
  guiState = MAIN_MENU_STATE; 
  runningApp = mainMenuState; 
  display.setFullWindow();
  display.fillScreen(bgColour);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(fgColour);

  //int16_t  x1, y1;
  //uint16_t w, h;

  // draw all the boxes
  _drawMenuOptions(mainMenuItems, mainMenuOptions);  // TODO: test
  _printPageNumber();

  #ifdef DEBUG_TIMING
  Serial.print("Start drawing menu: ");
  Serial.println(millis());
  #endif //DEBUG_TIMING
  display.display(partialRefresh, darkMode);
  #ifdef DEBUG_TIMING
  Serial.print("End drawing menu: ");
  Serial.println(millis());
  #endif //DEBUG_TIMING  
}   // showMainMenu

/////////// CLOCK MENU /////////////////////

/*!
 * @brief Displays a screen with all the clock-related apps
 */
void Watchy::showClockMenu(bool partialRefresh){
  #ifdef DEBUG
  Serial.println("App: Clock Menu");
  #endif
  guiState = APP_STATE;
  runningApp = clockMenuState;
  display.setFullWindow();
  display.fillScreen(bgColour);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(fgColour);
  _drawMenuOptions(clockMenuItems, clockMenuOptions); // TODO: test
  _printPageNumber();
  display.display(true, darkMode); // full refresh 
}

void Watchy::_printPageNumber(){
  // print page number scroller
  display.fillRect(67, 170, pageArrowBoxLength, pageArrowBoxLength, fgColour);
  display.fillRect(112, 170, pageArrowBoxLength, pageArrowBoxLength, fgColour);
  display.setTextColor(bgColour);
  display.setCursor(71, 184);
  display.println("<");
  display.setCursor(94, 184);
  display.println(menuPageNumber);
  display.setCursor(117, 184);
  display.println("<");
}  

// draws a touch friendly menu with boxes for each option
void Watchy::_drawMenuOptions(const char *menuItems[], uint32_t menuOptions){
  int16_t xPos;
  int16_t yPos;
  for(int row=0; row<numColumnsRows; row++){
    for(int column=0; column<numColumnsRows; column++){
      uint32_t menuPosition = row*numColumnsRows+column+(menuPageNumber-1)*4;
      if(menuPosition>=menuOptions){
        break;
      }
      xPos = column*(boxLength+gap)+13;
      yPos = row*(boxLength+gap)+25;
      display.drawRect(xPos, yPos, boxLength, boxLength, fgColour);
      display.setCursor(xPos+10, yPos+40);
      display.println(menuItems[(menuPosition)]);
    }
  }   
}