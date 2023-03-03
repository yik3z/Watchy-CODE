#include "calendar.h"



//calendar
RTC_DATA_ATTR int calendarLength = 0; //actual number of entries in the ESP's records
RTC_DATA_ATTR struct calendarEntries calEnt[CALENDAR_ENTRY_COUNT];
RTC_DATA_ATTR bool lastCalendarSyncSuccess = false;

//String calendarRequest = "https://script.google.com/macros/s/" + String(GOOGLE_CALENDAR_KEY) + "/exec"; //check if this is ok
bool fetchCalendar(){
  // Getting calendar from your published google script
  lastCalendarSyncSuccess = false;
  //#ifdef DEBUG
  //Serial.print("Getting calendar from: ");
  //Serial.println(GOOGLE_CALENDAR_KEY);
  //#endif
  if(WiFi.status() != WL_CONNECTED){  //check that WiFi is connected
    #ifdef DEBUG
    Serial.print("E: WiFi not connected");
    #endif
    return lastCalendarSyncSuccess;
  }
  HTTPClient http;
  String payload = "";
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(GOOGLE_CALENDAR_KEY)) {
    #ifdef DEBUG
    Serial.println("Cannot connect to google script");
    #endif
    return lastCalendarSyncSuccess;
  } 
  #ifdef DEBUG
  Serial.println("Connected to google script");
  #endif
  http.setTimeout(30000);  //30s
  int httpResponseCode = http.GET();
  #ifdef DEBUG
  Serial.print("Response Code: "); Serial.println(httpResponseCode);
  #endif
  if(httpResponseCode>0){
    payload = http.getString();
    lastCalendarSyncSuccess = true;
    #ifdef DEBUG
    Serial.print("Calendar payload: "); Serial.println(payload);
    #endif
  } else {
    http.end();
    return lastCalendarSyncSuccess;
  }
  http.end();

  int indexFrom = 0;
  int indexTo = 0;
  int cutTo = 0;

  String strBuffer = "";

  int count = 0;
  int line = 0; //number of lines in the calendar (to be computed)
  #ifdef DEBUG
  Serial.println("IndexFrom");  
  #endif
  indexFrom = payload.lastIndexOf("\n") + 1;

  // Fill calendarEntries with entries from the get-request
  while (indexTo>=0 && line<CALENDAR_ENTRY_COUNT) {
    count++;
    indexTo = payload.indexOf(";",indexFrom);
    cutTo = indexTo;

    if(indexTo != -1) { 
      strBuffer = payload.substring(indexFrom, cutTo);
      
      indexFrom = indexTo + 1;
      
      Serial.println(strBuffer);

      if(count == 1) {
        // Set entry time
        calEnt[line].calDate = strBuffer.substring(4,17); //Exclude end date and time to avoid clutter - Format is "Wed Feb 10 2020 10:00"
        calEnt[line].calDate = strBuffer.substring(16,5);
      } else if(count == 2) {
        // Set entry title
        calEnt[line].calTitle = strBuffer;

      } else {
          count = 0;
          line++;
      }
    }
  }
  calendarLength = line;
  return lastCalendarSyncSuccess;
}

bool internetWorks() {
  HTTPClient http;
  if (http.begin("script.google.com", 443)) {
    http.end();
    return true;
  } else {
    http.end();
    return false;
  }
}