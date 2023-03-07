# WATCHY (with PlatformIO)

![Watchy](https://watchy.sqfmi.com/img/watchy_render.png)

# Features (Working, at least -ish)

### UI

![watchface](https://github.com/yik3z/Watchy-CODE/blob/main/others/pictures/watchface.jpg?raw=true)
- watchface
    - `DIN Black` Font
    - calendar events
    - battery bar
	- looks clean
	- very wow
- [scrolling menu](https://gitlab.com/astory024/watchy/-/blob/master/src/Watchy.cpp) by Alex Story
- `dark mode` toggle. Changes watchface, menus, apps (everything)
- borders around the screen are syncronised to background colour. Based on findings from [peanutman and gewoon_maarten on discord](https://discord.com/channels/804832182006579270/808787590060048465/887013190616117288).

### Performance / Responsiveness
_This Watchy works slightly different from the original. With each interaction (buttonpress), watchy wakes, does its thing, and goes back to sleep ASAP. No waiting around with the CPU awake. (There's also an ISR to catch button presses while Watchy is "doing its thing") The only exceptions to this are some legacy functions like `setTime()` which stays awake while waiting for more inputs, and also waiting for the display to refresh._
- Removed `display.init()` before each function. Is now called upon wake from sleep and `display.hibernate()` just before going to sleep.
- Added interrupts to check for buttonpresses. `handleButtonPress()` loops until all button presses are cleared
- Modified bootloader for faster wake from sleep (See [this guide](https://hackaday.io/project/174898-esp-now-weather-station/log/183782-bootloader-wake-time-improvements)).
  settings changed:

  *general*
   - flash speed=80MHz  
   - timers used for `gettimeofday` function set to `none`
   
   *bootloader config*
   - compiler optimization to `-O2` for speed
   - `Skip Image Validation` when waking from Deep Sleep
   - turned off all logging
   
   Flash mode `QIO` doesn't work :(

   **Not extensively tested, but seems to work for now. Sleep+wake together take ~50ms (based on my janky testing).**
   
- Modified the display library (GxEPD) to reduce unnecessary wait times. A full loop (sleep>wake>sleep) is 300ms shorter now 🙂 :

|  | Before (ms) | After (ms) |
| --- | --- | --- |
| Partial Refresh | 894 | 592 |
| Full refresh | 2637 | 2312 |

Here is the refresh broken down (for a partial update):

|  | Before (ms) | After (ms) | Reason |
| --- | --- | --- | --- |
| `display.init()` | 250++ | 40 | Unnecessary 200ms delay was removed, shortened delays to minimum sped in datasheet |
| graphics processing | 7-8 | 7-8 | `display.print()` and all that. Small enough not to bother optimising |
| ?? | ~50 | ~50 | was lazy to check what |
| `_Update_Part()` | 400 | 400 | can’t change anything here |
| ?? | 60 | 60 | was lazy to check |
| `display.hibernate()` | 110 | 0 | removed `_Poweroff()` and removed `_wait_while_busy()` since the ESP is also going to sleep anyway |


### Batt/Power Saving
- ~~CPU set to 80Mhz (in the hope of power saving)~~ 240MHz is actually faster (noticeable in the UI) so I'm keeping it to that. The CPU spends very little time awake anyway.
- downclock CPU to 10MHz while waiting for screen to update.
- disable initialisation of BMA423 - seems to save a lot of battery!
- ~~`fast menu` used to force the code to wait for the timeout before entering sleep even when in an app. Changed it to only wait for the timeout when in the menu. Otherwise (i.e. in an app) enter sleep immediately~~ `fastmenu` **is not used anymore since the UI is much more responsive anyway.** No more waiting for timeouts - Watchy goes to sleep immediately :)
- low/critical battery warning
	- critical battery (<5%) engages power saver mode. Watch updates time once an hour.
- User toggle-able power saver, makes Watchy 'tick' once every hour when activated (untested)
- more accurate battery voltage. ADC callibration code derived from [peanutman on discord](https://discord.com/channels/804832182006579270/808787590060048465/877194857402232852) (requires sign in to access).
- more accurate battery percentage, using a Look Up Table (LUT)


### Misc Features
- NTP time sync (set to sync every 3 days).
    - seems like it works, am lazy to test but the time gets synced periodically so ain't complaining.
- sync to google calendar (internet). Followed [this tutorial.](https://www.instructables.com/E-Ink-Family-Calendar-Using-ESP32/). The code for Google Apps Script is in `\others\share_calendar.gs`. (set to sync every 3 days). Next event also shows on watchface.
![Calendar](https://github.com/yik3z/Watchy-CODE/blob/main/others/pictures/calendar_app.jpg?raw=true)

- `DEBUG` modes which prints stuff to serial.
- WiFi OTA to save your CP2104 USB chip.
- watch "ticks" only once an hour from 1am to 7am to save batt. In between those hours the watchface only shows (e.g.) `03:xx` instead of `03:14`. (Actual time will show if you press the back button.) (See [Power Saving](###-power-saving) for all the other power saving hacks I've done)
- stopwatch.
- stats screen showing things like uptime, battery.
![Stats_Screen](https://github.com/yik3z/Watchy-CODE/blob/main/others/pictures/stats_screen.jpg?raw=true)

- ~~temperature app~~ I disabled it because I have a hardware issue.


## What's Not Working / In Progress
- ~~trying to configure the BMA423 properly, for low power~~ I think I'll just leave it uninitialised as I don't have any use for it now
- (not working)RTC temperature sensor is messed up; shows 45 - 49dec C (I have a hardware issue with it so I've stopped working on temperature.). This is probably why my RTC drifts...~~Substituted with BMA423 temperature sensor (yes, the IMU has a temp sensor) but temp is still a bit off (34 ish dec C) ~~ (Update: Disabled the BMA423 sensor.) Maybe I should get the temperatre from the screen lol...
- weather is disabled. I don't use it so I didn't spend a lot of time developing it. It may or may not work.


## Other Changes


### Libraries Removed
- removed `FWUpdate` cos it's useless
- removed `WiFiManager` library; now uses only default `WiFi` library


## Wishlist
- ~~BLE updates from phone (this is months or years away)~~ Not happening. ESP32 can't sleep with BLE connected. Needs a 32kHz external crystal for lightsleep w BLE, which Watchy doesn't have.
- on that note, I would like to eventually make a nrf52840 version of Watchy (but this is a long time away)
- put Watchy into lightsleep while waiting for screen to update. Use interrupt from display's `BUSY` pin to resume.
- touchscreen hehe
- maybe improve RTC accuracy by changing PPM (?) (or I might just buy a new RTC chip and resolder)

# Bugs
- Anything that depends on the BMA423 will crash (I have since de-initialised it.)
- ~~WiFi seems to take a long time to initialise~~ It is what it is :(
- Battery drain is a bit uneven, I think (battery suddenly drops from 40% to 30%)

# Dependencies
- GxEPD2 by Jean-Marc Zingg (modified, it's in `\lib` now)
- Adafruit GFX Library by Adafruit
- Adafruit BusIO by Adafruit (I think)
- DS3232RTC by Jack Christensen
- Arduino_JSON by Arduino
- ~~WiFiManager by tzapu~~
    - ~~clone this directly from Github as PlatformIO library registry downloads the release version (v. old), which isn't compatible with ESP32~~
	removed; using the default WiFi library now
	
# Getting Started
**Note that this is untested!** So there may be some missed steps along the way. Only attempt this if you're moderately familiar with using PlatformIO (or have a PlatformIO guide on hand).
- clone the repo
- rename `sensitive_config-template.h` in `\lib\Watchy\` to `sensitive_config.h` and input your WiFi etc
- change relevant settings in `config.h`. For example:
    - `GMT_OFFSET_SEC` for NTP time sync
    - `INTERNET_SYNC_INTERVAL`
    - `NIGHT_HOURLY_TIME_UPDATE` (display updates once an hour)
    - `LOW_BATT_THRESHOLD` and `CRIT_BATT_THRESHOLD`
- (optional) To enable faster wake from deep sleep, copy `others\bootloader_dio_80m.bin` to `\packages_dir\framework-arduinoespressif32\tools\sdk\bin` (and maybe make a backup of the original `bootloader_dio_80m.bin`). (Or use [this guide](https://hackaday.io/project/174898-esp-now-weather-station/log/183782-bootloader-wake-time-improvements) to generate your own.)
- Change `upload_port` in `platformio.ini` to match the COM port, and comment out `upload_port`.
- Once PlatformIO downloads all the necessary dependencies, you _should_ be able to compile and upload the firmware to your Watchy. The first upload has to be through USB, but subsequently you can flash it via WiFi OTP:
    - click WiFi OTP on Watchy. It'll connect to your WiFi SSID and show you its IP address (ensure you set up the WiFi correctly in `config.h`).
    - uncomment `#upload_port` in `platformio` and change the IP to the IP shown on Watchy's screen.
    - build n upload the project in PlatformIO. 

# Quick Notes
## Fonts
DPI is 141
[Font converter](https://rop.nl/truetype2gfx/) for ttf fonts