#ifndef CALENDAR_H
#define CALENDAR_H

#include "Arduino.h"
#include "HTTPClient.h"
#include "sensitive_config.h"
#include "config.h"

#define CALENDAR_ENTRY_COUNT 10 //max number of entries in the calendar
struct calendarEntries
{
  String calDate;
  String calTime;
  String calTitle;
};

bool displayCalendar();
bool fetchCalendar();
bool internetWorks();
#endif