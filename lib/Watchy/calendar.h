#ifndef CALENDAR_H
#define CALENDAR_H

#include "Arduino.h"
#include "HTTPClient.h"
#include "sensitive_config.h"
#include "config.h"
#include "Watchy.h"

#define CALENDAR_ENTRY_COUNT            10  // max number of entries in the calendar
#define CALENDAR_MAX_PAGES              4   // max number of pages in the calendar (Assuming 3 entries per page), hardcoded for now
#define CALENDAR_EVENT_TITLE_LENGTH     30  // max length for the title of an event. This may take up A LOT of RTC memory lol

//structure to store the calendar events synced from google calendar
typedef struct calendarEntries
{
  tmElements_t calDate;                           // date of event
  char calTitle[CALENDAR_EVENT_TITLE_LENGTH+1];   // title or event. extra character '.' at position 51 for titles that overflow
  bool allDay;                                    // whether the event is an all day event
} calendarEntries;

bool fetchCalendar();
bool internetWorks();
//int selectUpcomingEvent(tmElements_t currentTime);

#endif