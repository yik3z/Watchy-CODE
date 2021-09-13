# WATCHY WITH PlatformIO

![Watchy](https://watchy.sqfmi.com/img/watchy_render.png)

# Features (Working, at least -ish)

- battery bar
- temperature app but there's a hardware issue
- watchface
    - DIN Black Font
	- looks clean
	- very wow
- [scrolling menu](https://gitlab.com/astory024/watchy/-/blob/master/src/Watchy.cpp) by Alex Story
- NTP timesync (set to sync every 1 week)
    - seems like it works, am lazy to test but the time gets synced periodically so ain't complaining
-dark mode toggle (needs a bit more rigorous testing)
- `DEBUG` mode which prints stuff to serial
- watch "ticks" only once an hour from 2am to 7am to save batt

## What's Not Working / In Progress

- stopwatch (working on it but lower priority)
	- tried using interrupts, it broke everything. Will debug soon.
- power saver toggle (in progress)
- RTC temperature sensor is messed up (hardware issue, shows 45 - 49dec C). Substituted with BMA423 temperature sensor (yes, the IMU has a temp sensor) but temp is still a bit off (34 ish dec C)


## Other Changes

- removed `FWUpdate` cos it's useless
- removed `WiFiManager` library; now uses only default `WiFi` library
- disabled step counter and some other config features on the BMA423 (maybe save batt? no clue lol)
- not sure if feature: disabled `fast menu` in most places; ESP enters sleep mode faster, but menu is harder to scroll when returning from apps

## Wishlist
- low battery warning
	- low batt can engage power saver mode
- maybe a thinner font for the time on the watchface
- double tap to wake stuff
- idk man BLE updates from phone (this is months or years away)
- maybe improve RTC accuracy by changing PPM (?)

# Dependencies

- GxEPD2 by Jean-Marc Zingg
- Adafruit GFX Library by Adafruit
- Adafruit BusIO by Adafruit (I think)
- DS3232RTC by Jack Christensen
- Arduino_JSON by Arduino
- ~~WiFiManager by tzapu~~
    - ~~clone this directly from Github as PlatformIO library registry downloads the release version (v. old), which isn't compatible with ESP32~~
	removed; using the default WiFi library now
	
# PlatformIO Stuff

coming soon