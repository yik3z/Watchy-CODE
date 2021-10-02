# WATCHY WITH PlatformIO

![Watchy](https://watchy.sqfmi.com/img/watchy_render.png)

# Features (Working, at least -ish)

### Battery
- battery bar
- more accurate battery voltage. ADC callibration code derived from [peanutman on discord](https://discord.com/channels/804832182006579270/808787590060048465/877194857402232852) (requires sign in to access).

### UI
- watchface
    - `DIN Black` Font
	- looks clean
	- very wow
- [scrolling menu](https://gitlab.com/astory024/watchy/-/blob/master/src/Watchy.cpp) by Alex Story
- `dark mode` toggle. Changes watchface, menus, apps (everything)
- borders around the screen are syncronised to background colour. Based on findings from [peanutman and gewoon_maarten on discord](https://discord.com/channels/804832182006579270/808787590060048465/887013190616117288).

### Performance / Responsiveness
- Removed floating point operations: Look up table (LUT) implemented to convert battery voltage to percentage
- Removed `display.init()` before each function. Is now called upon wake from sleep and `display.hibernate()` just before going to sleep.
- Added interrupts to check for buttonpresses. `handleButtonPress()` loops until all button presses are cleared
- modified bootloader for faster wake from sleep (See [this guide](https://hackaday.io/project/174898-esp-now-weather-station/log/183782-bootloader-wake-time-improvements)).
  settings changed:
  *general*
   - flash speed=80MHz  
   - timers used for `gettimeofday` function set to `none`
   
   *bootloader config*
   - compiler optimization to `-O2` for speed
   - `Skip Image Validation` when waking from Deep Sleep
   - turned off all logging
   
   Flash mode `QIO` doesn't work :(
   
   **Not extensively tested, but seems to work for now. I haven't timed it too. Use with a pinch of salt.**

### Misc Features
- temperature app, but there's a hardware issue
- NTP timesync (set to sync every 1 week)
    - seems like it works, am lazy to test but the time gets synced periodically so ain't complaining
- `DEBUG` mode which prints stuff to serial
- watch "ticks" only once an hour from 1am to 7am to save batt. In between those hours the watchface only shows (e.g.) `03:xx` instead of `03:00`. (See [Power Saving](###-power-saving) for all the other power saving hacks I've done)


## What's Not Working / In Progress
- ~~trying to configure the BMA423 properly, for low power~~ I think I'll just leave it uninitialised as I don't have any use for it now
- low battery warning
	- low batt can engage power saver mode
- (not working)RTC temperature sensor is messed up (hardware issue, shows 45 - 49dec C). Substituted with BMA423 temperature sensor (yes, the IMU has a temp sensor) but temp is still a bit off (34 ish dec C)


## Other Changes
### Power Saving
- CPU set to 80Mhz (in the hope of power saving)
- disabled initialisation of BMA423 - seems to save a lot of battery!
- ~~`fast menu` used to force the code to wait for the timeout before entering sleep even when in an app. Changed it to only wait for the timeout when in the menu. Otherwise (i.e. in an app) enter sleep immediately~~ `fastmenu` **is not used anymore since the UI is much more responsive anyway.** No more waiting for timeouts :)

### Libraries Removed
- removed `FWUpdate` cos it's useless
- removed `WiFiManager` library; now uses only default `WiFi` library


## Wishlist
- maybe a thinner font for the time on the watchface
- ~~double tap to wake stuff~~
- idk man BLE updates from phone (this is months or years away)
- maybe improve RTC accuracy by changing PPM (?) (or I might just buy a new RTC chip and resolder)

# Bugs
- Anything that depends on the BMA423 will crash
- WiFi seems to take a long time to initialise
- Battery drain is a bit uneven, I think (battery suddenly drops from 40% to 30%)
- `Set time` probably doesn't work anymore lol

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