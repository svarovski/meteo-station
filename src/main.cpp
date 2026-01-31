#include <Wire.h>
#include <AHT10.h>
#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

AHT10 aht10(AHT10_ADDRESS_0X38, AHT10_SENSOR); //sensor address, sensor type

// Настройка времени сна (в микросекундах)
// 600e6 = 10 минут
const unsigned long SLEEP_TIME = 30e6;
const unsigned int SDA_PIN = 4;
const unsigned int SLC_PIN = 5;
const unsigned int AHT10_POWER_PIN = 16;
const unsigned int LED_PIN = 2;
const unsigned int BUTTON_PIN = 2;

// Твой калибровочный коэффициент для резистора 980кОм
// Подправь его после первого замера мультиметром
float calibration_coeff = 13.0; 

void setup() {
  // 1. Сразу выключаем WiFi, чтобы не жать батарею
//   WiFi.mode(WIFI_OFF);
//   WiFi.forceSleepBegin();
//   delay(1); 


  Serial.begin(115200);
  // Serial.println("\n--- Пробуждение ---");

  // 2. Читаем АЦП. Делаем серию замеров для точности
  // Конденсатор 120нФ сгладит помехи, но усреднение не помешает
//   long raw_sum = 0;
//   for(int i = 0; i < 20; i++) {
//     raw_sum += analogRead(A0);
//     delay(2);
//   }
//   float raw_avg = raw_sum / 20.0;

//   // 3. Считаем напряжение
//   float voltage = raw_avg * (calibration_coeff / 1024.0);

//   // 4. Вывод результата
//   Serial.printf("ADC Raw: %f | Напряжение:  %.2fV\n", raw_avg, voltage);

  // 5. Уходим в глубокий сон
  // Не забудь соединить D0 и RST (лучше через резистор 1к)
//   Serial.println("Ухожу в Deep Sleep...");
//   ESP.deepSleep(SLEEP_TIME);

//   pinMode(AHT10_POWER_PIN, OUTPUT);
//   digitalWrite(AHT10_POWER_PIN, HIGH);

//   while (aht10.begin() != true) //for ESP-01 use aht10.begin(0, 2);
//   {
//     Serial.println(F("AHT1x not connected or fail to load calibration coefficient")); //(F()) save string to flash & keeps dynamic memory free

//     delay(5000);
//   }

//   Serial.println(F("AHT10 OK"));
  pinMode(BUILTIN_LED, OUTPUT);
}

int i=HIGH;

void loop() {
	Serial.println(i ? "Hi!" : "Lo!");
	digitalWrite(BUILTIN_LED, i = !i);

  // В Deep Sleep этот блок никогда не выполняется

  // readVoltage();
//   readAHT10data();
  delay(10000); //recomended polling frequency 8sec..30sec
}

void readVoltage()
{
  long raw_sum = 0;
  for(int i = 0; i < 20; i++) {
    raw_sum += analogRead(A0);
    delay(2);
  }
  float raw_avg = raw_sum / 20.0;

  // 3. Считаем напряжение
  float voltage = raw_avg * (calibration_coeff / 1024.0);

  // 4. Вывод результата
  Serial.printf("ADC Raw: %f | Напряжение:  %.2fV\n", raw_avg, voltage);

}

void readAHT10data()
{
  /* DEMO - 1, every temperature or humidity call will read 6-bytes over I2C, total 12-bytes */
  Serial.println();
  Serial.println(F("DEMO 1: read 12-bytes"));

  float ahtValue = aht10.readTemperature(); //read 6-bytes via I2C, takes 80 milliseconds

  Serial.print(F("Temperature...: "));
  
  if (ahtValue != AHT10_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Serial.print(ahtValue);
    Serial.println(F(" +-0.3C"));
  }
  else
  {
    // printStatus(); //print temperature command status

    if   (aht10.softReset() == true) Serial.println(F("reset success")); //as the last chance to make it alive
    else                             Serial.println(F("reset failed"));
  }

  delay(2000); //measurement with high frequency leads to heating of the sensor, see NOTE

  ahtValue = aht10.readHumidity(); //read another 6-bytes via I2C, takes 80 milliseconds

  Serial.print(F("Humidity......: "));
  
  if (ahtValue != AHT10_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Serial.print(ahtValue);
    Serial.println(F(" +-2%"));
  }
  else
  {
    // printStatus(); //print humidity command status
  }

  delay(2000); //measurement with high frequency leads to heating of the sensor, see NOTE

  /* DEMO - 2, temperature call will read 6-bytes via I2C, humidity will use same 6-bytes */
  Serial.println();
  Serial.println(F("DEMO 2: read 6-byte"));

  ahtValue = aht10.readTemperature(); //read 6-bytes via I2C, takes 80 milliseconds

  Serial.print(F("Temperature: "));
  
  if (ahtValue != AHT10_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Serial.print(ahtValue);
    Serial.println(F(" +-0.3C"));
  }
  else
  {
    // printStatus(); //print temperature command status
  }

  ahtValue = aht10.readHumidity(AHT10_USE_READ_DATA); //use 6-bytes from temperature reading, takes zero milliseconds!!!

  Serial.print(F("Humidity...: "));
  
  if (ahtValue != AHT10_ERROR) //AHTXX_ERROR = 255, library returns 255 if error occurs
  {
    Serial.print(ahtValue);
    Serial.println(F(" +-2%"));
  }
  else
  {
    // printStatus(); //print temperature command status not humidity!!! RH measurement use same 6-bytes from T measurement
  }
}

/**************************************************************************/
/*
    printStatus()

    Print last command status
*/
/**************************************************************************/
// void printStatus()
// {
//   switch (aht10.readStatusByte())
//   {
//     case AHT10_NO_ERROR:
//       Serial.println(F("no error"));
//       break;

//     case AHT10_BUSY_ERROR:
//       Serial.println(F("sensor busy, increase polling time"));
//       break;

//     case AHT10_ACK_ERROR:
//       Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
//       break;

//     case AHT10_DATA_ERROR:
//       Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
//       break;

//     case AHT10_CRC8_ERROR:
//       Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
//       break;

//     default:
//       Serial.println(F("unknown status"));    
//       break;
//   }
// }