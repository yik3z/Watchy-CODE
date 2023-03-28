#include "Watchy.h"
// Contains the menus and the logic to launch apps

extern uint64_t wakeupBit;

// menu variables
RTC_DATA_ATTR uint32_t menuPageNumber = 1;  // page number used by all the menus

// menu constants
const uint32_t numColumnsRows = 2;        // number of rows = number of columns
const uint32_t gap = 10;                  // gap between 2 boxes
const uint32_t boxLength = 70;            // pixels. Can be calculated from (200-(numColumnsRows+1)*gap)/numColumnsRows
const uint32_t pageArrowBoxLength = 20;   // pixels. Length of box around "<" and ">" arrows 

// main menu. Remember to add them to _menuAppLauncher
const char *mainMenuItems[] = {"Stats","Motor","Cal","Acc","Set","Dark/Light Mode","Power","Temp","Timer","WiFi","OTA"};
const uint32_t mainMenuOptions = sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
const uint32_t mainMenuPages = (mainMenuOptions-1)/4 + 1; // total number of pages. TODO: check

// clock menu
const char *clockMenuItems[] = {"Timer","Cal","Set","NTP"};
const uint32_t clockMenuOptions = sizeof(clockMenuItems) / sizeof(clockMenuItems[0]);
const uint32_t clockMenuPages = (clockMenuOptions-1)/4 + 1; // total number of pages. TODO: check

// function specifically for launching apps
inline void Watchy::launchApp(appID_t appToLaunch){
  _appLauncherHandler(appToLaunch);
}

/*!
 * @brief Launches app from menu or dispatches the interaction event to the running app

 *
 * @param appID (optional) the app to launch if launching an app
 *
 */
void Watchy::_appLauncherHandler(appID_t launchAppID){
  if (launchAppID == none_appState){
    // passing on the interaction event to the running app
    launchAppID = runningApp;
  } else if (launchAppID != runningApp){
    //new app is launching, reset menu page
    menuPageNumber = 0;
    wakeupBit = 0;
    #ifdef DEBUG
    Serial.print("Launching App: ");
    Serial.println(launchAppID);
    #endif
  }
  switch(launchAppID)
  {
    case mainMenuState:
      showMainMenu();
      break;
    case clockMenuState:
      showClockMenu();
      break;
    case showStatsState:
      showStats();
      break;
    case vibMotorState:
      showBuzz();
      break;          
    case calendarState:
      showCalendar();
      break;
    case setTimeState:
      setTime();
      break;
    case setDarkModeState:
      setDarkMode();
      break;   
    case setPowerSaverState:
      setPowerSaver();      
      break;           
    case showAccState:
      #ifdef USING_ACCELEROMETER
      //showTemperature(wakeupBit);
      #endif //USING_ACCELEROMETER
      break;
    case stopWatchState:
      stopWatch();
      break;
    case connectWiFiState:
      connectWiFiGUI();
      break;
    case wifiOtaState:
      wifiOta();
      break;
    default:
      break;                              
  }
} // _appLauncherHandler


// Scrolls through menu and opens app if selecteds
void Watchy::_menuInteractionHandler(){
  uint32_t menuPages;
  if(runningApp == mainMenuState){
    menuPages = mainMenuPages;
  } else if(runningApp == clockMenuState){
    menuPages = clockMenuPages;
  } else return;

  // touchscreen interaction
  if(wakeupBit & TS_INT_PIN_MASK){
    wakeupBit = 0;
    // tap to enter app
    int32_t app = _tpWithinMenuBox();
    if(app>-1){
      #ifdef DEBUG_TOUCHSCREEN
      Serial.print("Touched App: ");
      Serial.println(app);
      #endif //DEBUG_TOUCHSCREEN
      _appLauncherHandler(_selectMenuApp(app)); //launch chosen app
      return;
    }
    // tap to change page
    if(_tpWithinBounds(67-10,67+pageArrowBoxLength+10,170-10,170+pageArrowBoxLength+10)){ //added extra space around buttons
      menuPageNumber--;
      #ifdef DEBUG_TOUCHSCREEN
      Serial.print("Menu page-- (touch): ");
      Serial.println(menuPageNumber);
      #endif
    } else if(_tpWithinBounds(112-10,112+pageArrowBoxLength+10,170-10,170+pageArrowBoxLength+10)){  // prev page

      menuPageNumber++;
      #ifdef DEBUG_TOUCHSCREEN
      Serial.print("Menu page++ (touch): ");
      Serial.println(menuPageNumber);
      #endif
    } else return; // touch event wasn't on anything
  } 

  // button interaction
  else if(wakeupBit & UP_BTN_MASK){
    menuPageNumber++;    
  } else if(wakeupBit & DOWN_BTN_MASK){
    menuPageNumber--;
  }  
  // catch out of bounds page number  
  if(menuPageNumber > menuPages){
    menuPageNumber = menuPages;
  } else if(menuPageNumber < 1){
    menuPageNumber = 1;
  }   

  wakeupBit = 0;
  _appLauncherHandler();  //will launch the correct menu based on the runningApp state
} // menuInteractionHandler

/*!
 * @brief Chooses the correct app to launch. 
 * Uses runningApp, boxNumber and menuPageNumber to decide which app to launch
 * @param boxNumber index of the box selected on screen in the menu. Starts from 0
 * @returns appID_t of the correct app to launch
 */
appID_t Watchy::_selectMenuApp(int32_t boxNumber){
  int32_t appIndex = (menuPageNumber-1)*4 + boxNumber;  // position of  app in the menu. starts at 0
    #ifdef DEBUG_TOUCHSCREEN
    Serial.print("appIndex Selected: ");
    Serial.println(appIndex);
    #endif  //DEBUG_TOUCHSCREEN
  if(runningApp == mainMenuState){
    return _chooseMainMenuApp(appIndex);
  } else if(runningApp == clockMenuState){
    return _chooseClockMenuApp(appIndex);
  }
  else return runningApp; // undefined menu chosen
}

///////////// MAIN MENU METHODS //////////////////////////////

// Touch friendly menu
void Watchy::showMainMenu(bool initial, bool partialRefresh){
  #ifdef DEBUG
  Serial.println("App: Main Menu");
  #endif
  //guiState = MAIN_MENU_STATE; 
  runningApp = mainMenuState; 
  if(initial) menuPageNumber = 1;
  display.setFullWindow();
  display.fillScreen(bgColour);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(fgColour);

  // draw all the boxes
  _drawMenuOptions(mainMenuItems, mainMenuOptions);  // TODO: test
  _drawPageNumber();

  #ifdef DEBUG_TIMING_EXTENSIVE
  Serial.print("Start drawing menu: ");
  Serial.println(millis());
  #endif //DEBUG_TIMING_EXTENSIVE
  display.display(partialRefresh, darkMode);
  #ifdef DEBUG_TIMING_EXTENSIVE
  Serial.print("End drawing menu: ");
  Serial.println(millis());
  #endif //DEBUG_TIMING_EXTENSIVE  
}   // showMainMenu

/*!
 * @brief Converts the appIndex from the mainMenu to the appID. 
 * @param appIndex index of the app selected in the main Menu. Starts from 0
 * @returns appID_t of the correct app to launch
 */
inline appID_t Watchy::_chooseMainMenuApp(int32_t appIndex){
  appID_t appID = appID_t(appIndex+3); // appState is 3 positions offset (watchface,menu,clockmenu)
  return appID;
}

/////////// CLOCK MENU /////////////////////

/*!
 * @brief Displays a screen with all the clock-related apps
 */
void Watchy::showClockMenu(bool initial, bool partialRefresh){
  #ifdef DEBUG
  Serial.println("App: Clock Menu");
  #endif
  //guiState = APP_STATE;
  runningApp = clockMenuState;
  if(initial) menuPageNumber = 1;
  #ifdef DEBUG
  Serial.print("Menu Pg No: ");
  Serial.println(menuPageNumber);
  #endif
  display.setFullWindow();
  display.fillScreen(bgColour);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(fgColour);
  _drawMenuOptions(clockMenuItems, clockMenuOptions); // TODO: test
  _drawPageNumber();
  display.display(true, darkMode);
}
 
/*!
 * @brief Converts the appIndex from the clockMenu to the appID. 
 * @param appIndex index of the app selected in the clock Menu. Starts from 0
 * @returns appID_t of the correct app to launch
 */
inline appID_t Watchy::_chooseClockMenuApp(int32_t appIndex){
  appID_t appID;
  switch(appIndex){
    case 0:
      appID = stopWatchState;
      break;
    case 1:
      appID = calendarState;
      break;      
    case 2:
      appID = setTimeState;
      break;
    case 3:
      appID = connectWiFiState;
      break;
      default:
      appID = runningApp; // error: appIndex not in clock menu
  }
  return appID;
}

///////////// OTHER FUNCTIONS ////////////////////
// print page number scroller, i.e.: < 1 >
void Watchy::_drawPageNumber(){
  display.setTextColor(bgColour); 
  // back btn
  if(menuPageNumber > 1){
    display.fillRect(67, 170, pageArrowBoxLength, pageArrowBoxLength, fgColour);  
    display.setCursor(71, 184);
    display.println("<");
  } 
  // forward btn
  if(menuPageNumber < ((runningApp == mainMenuState) ?  mainMenuPages : clockMenuPages)){  // greyed out button at max page
    display.fillRect(112, 170, pageArrowBoxLength, pageArrowBoxLength, fgColour);
    display.setCursor(117, 184);
    display.println(">");
  }
  // page number
  display.setTextColor(fgColour);
  display.setCursor(94, 184);
  display.println(menuPageNumber);
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
      xPos = column*(boxLength+gap)+25;
      yPos = row*(boxLength+gap)+13;
      display.drawRect(xPos, yPos, boxLength, boxLength, fgColour);
      display.setCursor(xPos+10, yPos+40);
      display.println(menuItems[(menuPosition)]);
    }
  }   
}