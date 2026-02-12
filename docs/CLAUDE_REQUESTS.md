## First Request

I want to make device based on ESP8266 with AHT10 sensor and 18650 battery that will be used to measure temperature and humidity on remote location. Power of AHT10 will come from ESP pin 16 to have it switchable. ESP power comes from 18650 via AMS1117 voltage regulator. Also, plus of 18650 is connected to ESP A0 pin via 1M resistor and also A0 pin is connected to ground via capacitor. And there's a button connected to ground and RST pin of ESP to wake up device on button press. I need a program for this device that will do following:
* When unconfigured or button is pressed for more than 5 second, it should create wifi network without password and name like "sensor-<mac address>" and show page for configuration that include: wifi network to connect to in normal conditions, interval between measures in seconds, address of InfluxDB server where to send data to, username/password to access that server. All config then saved to ROM.
* ESP normally should be in deep sleep mode, waking up on configured interval, turn on AHT10 and save measured temp/humidity together with timestamp in RTC RAM to save ROM Writing cycles.
* Number of bytes of timestamp+temperature+humidity should be minimized to save RAM/ROM space. Better have total size of record 4 bytes, for example, 2 for timestamp, 1 for temp, 1 for humidity. Timestamp can be counted from some offset. Temp also can be counted from some offset, for example from -100C to fit 1 digit precision in 1 byte.
* When there's enough data buffered in RAM to write as single block in ROM, it should be written in ROM and RAM buffer is reused.
* When device is waken up by button it should connect to configured WiFi network and send all stored sensor data from RAM+ROM to InfluxDB server. Also, send current timestamp and battery power level to same server. After sending ROM blocks can be reused but should not be cleared immediately.
* Also, when connected to WiFi device should sync time with some NTP server.
Any advises are also welcome.

## Second request

I checked your recommendations and code. Here's some corrections:
* I have MCP1700, not AMS1117. My mistake.
* I use Wemos D1 Mini, not plain ESP8266. And Wemos board already has voltage divider on pin A0.
* Maybe it is better to move AHT10 power to different pin that won't be used for anything else like waking up board?
* Charging module is not required for now. Device is supposed to have battery replaced and charged somewhere else.
* Device should keep measurements for at least 2 weeks, ideally for a month, before having possibility to offload them to InfluxDB server.
* WiFi should wake up and upload data only when button is pressed. Timer wake up should only record data in memory.
* Wake up by timer should be without RF turned on. WiFi starts only when woken up by button.
* Static IP for WiFi client is not possible.
* Every action should be accompanied with internal Wemod D1 Mini LED blink. Simple measurement - turn on LED when starting, turn off when finished. Button wake up - blink led every 0.5s until transfer is complete. Initial of button-cause configuration - turn LED on until config is complete.
Please correct files and give more advices.

## Third correction (was launched too early and incomplete)

Looks good. Please add more corrections:
* Move all HTML/JS code to some static files.
* Save all long strings to flash to save RAM. Or explain why it cannot or should not be done.
* Split long methods longer than 20 lines in parts. Or explain why it should not be done.

## Fourth request

Thanks, it works. Next:
* What lib do I need to add for #include <Adafruit_AHTX0.h>? I use PlatformIO and it is not found. You can just post platformio.ini modifications.
* Please move all HTML code to external HTML files. I know it is possible with some ESP filesystem.
* Reorder functions in file to avoid forward declarations when possible.
* Rename esp8266_sensor.ino to cpp extension
* Add separate classes with appropriate methods for Config, SensorRecord, RTCData

## Fifth request

Files Config.cpp, SensorRecord.cpp and RTCData.cpp are missing. Please provide them

## Request number six

Good. Some more corrections
* Add unit tests for all .cpp files.
* Change timeOffset used in timestamp calculation from fixed timestamp to datetime of last sync or device start
* Change SensorRecord.timestamp meaning from seconds since offset to minutes since offset. Adjust all calculations accordingly.
* Look for library to use to connect to InfluxDB. If not found, move InfluxDB functions to separate class.
* Add unit test for all .cpp files and place the in folder `test``

## Request seven

* I didn't find success.html file in my project copy. Can you provide it?
* InfluxDBClient.cpp file is missing. Can you also provide it?
* I get following errors when running tests: (skipped)

## Request eight

You forgot to rename files InfluxDBClient to InfluxDBWrapper. Also, there's more errors from tests: (skipped)

## Request nine

* Can you maintain correct directory structure in downloadable archive?
* Please add test compile options to get rid of warnings like:
/home/gena/.platformio/packages/toolchain-xtensa/bin/../lib/gcc/xtensa-lx106-elf/4.8.2/../../../../xtensa-lx106-elf/bin/ld: .pio/build/d1_mini_lite/test/test_influxdb_client.cpp.o:/home/gena/ws/smart-home/meteo-station/test/test_influxdb_client.cpp:7: multiple definition of testConfig'; .pio/build/d1_mini_lite/test/test_config.cpp.o:/home/gena/ws/smart-home/meteo-station/test/test_config.cpp:5: first defined here
/home/gena/.platformio/packages/toolchain-xtensa/bin/../lib/gcc/xtensa-lx106-elf/4.8.2/../../../../xtensa-lx106-elf/bin/ld: .pio/build/d1_mini_lite/test/test_influxdb_client.cpp.o: in function setUp':
/home/gena/ws/smart-home/meteo-station/test/test_influxdb_client.cpp:9: multiple definition of `setUp'; .pio/build/d1_mini_lite/test/test_config.cpp.o:/home/gena/ws/smart-home/meteo-station/test/test_config.cpp:7: first defined here
* Build fails with error. How to fix it? Device is connected to USB and present in system as /dev/ttyUSB0
collect2: error: ld returned 1 exit status
*** [.pio/build/d1_mini_lite/firmware.elf] Error 1
Uploading stage has failed, see errors above. Use pio test -vvv option to enable verbose output.

## Request 10

Good. Please correct platformio.ini:
* Add

-monitor_filters =
-  default   ; Remove typical terminal control codes from input
-  time      ; Add timestamp with milliseconds for each new line
-  log2file  ; Log data to a file “platformio-device-monitor-*.log” located in the current working directory
-  colorize
-  esp8266_exception_decoder
-build_type = debug
* change library tobiasschuerg/InfluxDB Client for Arduino@^3.13.0 to more correct name tobiasschuerg/ESP8266 Influxdb@^3.13.2 for ESP8266
* Rename test_influxdb_client.cpp to test_influxdb_wrapper.cpp .
* Move all MD files except README.md to docs folder.
* Tests still do not work: (skipped errors)
* Uploading stage has failed, see errors above. Use `pio test -vvv` option to enable verbose output.
Is it possible that you run tests yourself and fix code until it works?

## Request 11

* There's still problem with tests: (errors skipped)

## Request 12

* Tests still don't work (errors skipped)

## Request 13

Please don't forget that all MD files except README.md should be in docs folder. And there's  still errors in tests (errors skipped)

## Request 14

Adjust permissions of run_all_tests.sh to include execution permissions. And there's still test errors: (errors skipped)
Check this docs: https://docs.platformio.org/en/latest/advanced/unit-testing/index.html. Tests should be moved to separate folders, not separate environments. Remove environments and add separate folders. Also, remove run_all_tests.sh if it is not needed anymore.

## Request 15

Let's rename all tests to simply test.cpp. Keep them in separate folders, of course. But there's still test failures: (errors skipped)

## Request 16

* You forgot to include FINAL_SOLUTION to archive, please correct.
* Do we need separate section for tests in platformio.ini? It is missing now.
* sensor_main.cpp is quite big and it's code is not covered with tests. What block of code can be moved to external files and covered with unit tests. Content of main file should be minimal and basically just config and calls to other classes.
* sensor_main.cpp is still compiled with tests and causes error.

## Request 17

* I think you can move handleRoot() and handleSave() methods from sensor_main.cpp to WiFiManager class.
* Tests for DataUploader and WiFiManager are missing.
* And you forgot to move MD files to docs folder.
* Besides missing tests there were 2 errors in test_sensor_record:
test.cpp:82: test_sensor_record_temperature_range: Expected 255 Was -1    [FAILED]
test.cpp:105: test_sensor_record_is_valid: Expected TRUE Was FALSE    [FAILED]
* Rename sensor_main.cpp to main.cpp
* Move everything from src folder to lib folder except main.cpp
* Remove separate test environment
* Remove test_build_src = yes and build_src_filter

## Request 18

Good. Tests work without separate environment. But need few corrections:
* Move all classes from lib/sensor to lib. No reason to make another folder level now.
* Add tests for DataUploader and WiFiManager.
* Move all MD files to docs folder except README.md
* Fix tests errors:

test.cpp:73: test_sensor_record_temperature_range: Expected 155 Was -101
test.cpp:94: test_sensor_record_is_valid: Expected TRUE Was FALSE

## Request 19

_Something was fixed on request "Still there?". Such request was sent because for several days there was error about "not able to compact conversation" on any question. I already thought of restarting with new session but then "Still there" helped._

## Request 20

Let's fix some rules:
* *.MD files are always in docs folder.
* You provide single .tar.gz archive as output. No extra files for download. With extra files it look confusing - which version should I use?

Since I get lots of test errors - is it possible that you run tests yourself, then correct errors and then give me already working code? If possible, give me steps how to configure such setup.

And here's latest test errors to correct:

## Request 21

Sorry, but this is wrong. We already passed this step. All source files except tests and main.cpp should be in lib folder or there will be link conflict errors. Please revert files move.

It is possible to run platformio tests natively (on local host) without need for ESP hardware. It is described in this doc: https://docs.platformio.org/en/latest/integration/ci/index.html#ci and this https://docs.platformio.org/en/latest/advanced/unit-testing/runner.html#remote-test-runner. Please correct tests to not require specific hardware.
