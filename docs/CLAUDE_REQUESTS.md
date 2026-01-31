**First Request**

I want to make device based on ESP8266 with AHT10 sensor and 18650 battery that will be used to measure temperature and humidity on remote location. Power of AHT10 will come from ESP pin 16 to have it switchable. ESP power comes from 18650 via AMS1117 voltage regulator. Also, plus of 18650 is connected to ESP A0 pin via 1M resistor and also A0 pin is connected to ground via capacitor. And there's a button connected to ground and RST pin of ESP to wake up device on button press. I need a program for this device that will do following:

- When unconfigured or button is pressed for more than 5 second, it should create wifi network without password and name like "sensor-<mac address>" and show page for configuration that include: wifi network to connect to in normal conditions, interval between measures in seconds, address of InfluxDB server where to send data to, username/password to access that server. All config then saved to ROM.

- ESP normally should be in deep sleep mode, waking up on configured interval, turn on AHT10 and save measured temp/humidity together with timestamp in RTC RAM to save ROM Writing cycles.
    
- Number of bytes of timestamp+temperature+humidity should be minimized to save RAM/ROM space. Better have total size of record 4 bytes, for example, 2 for timestamp, 1 for temp, 1 for humidity. Timestamp can be counted from some offset. Temp also can be counted from some offset, for example from -100C to fit 1 digit precision in 1 byte.
    
- When there's enough data buffered in RAM to write as single block in ROM, it should be written in ROM and RAM buffer is reused.
    
- When device is waken up by button it should connect to configured WiFi network and send all stored sensor data from RAM+ROM to InfluxDB server. Also, send current timestamp and battery power level to same server. After sending ROM blocks can be reused but should not be cleared immediately.
    
- Also, when connected to WiFi device should sync time with some NTP server.
    

Any advises are also welcome.
