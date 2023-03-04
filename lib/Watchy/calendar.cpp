#include "calendar.h"


//calendar
RTC_DATA_ATTR int calendarLength = 0; //actual number of entries in the ESP's records
RTC_DATA_ATTR calendarEntries calEnt[CALENDAR_ENTRY_COUNT];
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
  if(httpResponseCode==200){
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

  int field = 0;
  int line = 0; //number of lines in the calendar (to be computed)
  indexFrom = payload.lastIndexOf("\n") + 1;

  #ifdef DEBUG
  Serial.println("Calendar entries:");
  #endif

  // Fill calendarEntries with entries from the get-request
  while (indexTo>=0 && line<CALENDAR_ENTRY_COUNT) {
    field++;
    indexTo = payload.indexOf(";",indexFrom);
    cutTo = indexTo;

    if(indexTo != -1) { 
      strBuffer = payload.substring(indexFrom, cutTo);
      #ifdef DEBUG_CALENDAR
			Serial.print("field: ");
      Serial.print(field);
			Serial.print(" | strBuffer: ");
      Serial.print(strBuffer);
    	Serial.print(" | Data: ");
      #endif      
      indexFrom = indexTo + 1;

      if(field == 1) {
        // Set entry time and date
        //Exclude end date and time to avoid clutter
        //Original format is "09/03/2023, 20:00:00"
        //also, .substring() method does not follow official behaviour of C++ std::string.substring()

        calEnt[line].calDate.Day = strBuffer.substring(0,2).toInt();      //09
        calEnt[line].calDate.Month = strBuffer.substring(3,5).toInt();    //03
        calEnt[line].calDate.Year = strBuffer.substring(6,10).toInt()-YEAR_OFFSET;     //2023 -> 53
        calEnt[line].calDate.Hour = strBuffer.substring(12,14).toInt();   //20
        calEnt[line].calDate.Minute = strBuffer.substring(15,17).toInt(); //00        
        
        #ifdef DEBUG_CALENDAR
        Serial.print(calEnt[line].calDate.Day);
				Serial.println(calEnt[line].calDate.Month);
        //Serial.print(", ");
        #endif
      } else if(field == 2) {
        // Set entry title
        //convert str to char array bc RTC memory cannot store str
        for(int i = 0; i<CALENDAR_EVENT_TITLE_LENGTH;i++){
          calEnt[line].calTitle[i] = strBuffer[i];
        }
        if(strBuffer.length()>CALENDAR_EVENT_TITLE_LENGTH){
          calEnt[line].calTitle[CALENDAR_EVENT_TITLE_LENGTH] = '.';
        }
        #ifdef DEBUG_CALENDAR
        for(int j=0; j<CALENDAR_EVENT_TITLE_LENGTH+1;j++){  //not sure if this works
          Serial.print(calEnt[line].calTitle[j]);
        } 
        Serial.println();
        #endif
      } else if(field == 3) {
        // Set all day flag
				calEnt[line].allDay = (strBuffer[0]=='t');	//match first letter of "true"
        #ifdef DEBUG
        Serial.print("All day flag: ");
				Serial.print(strBuffer);
				Serial.print(" | allDay Variable: ");
				Serial.println(calEnt[line].allDay);
				#endif
				field = 0;
				line++;
			}
    }
  }
  calendarLength = line;
  #ifdef DEBUG
  Serial.print("calendar length: ");
  Serial.println(calendarLength);
  #endif
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

// // Selects the next event given a date
// int selectUpcomingEvent(tmElements_t currentTime){
// 	int nextCalEvent = -1;


// 	return nextCalEvent;
// }