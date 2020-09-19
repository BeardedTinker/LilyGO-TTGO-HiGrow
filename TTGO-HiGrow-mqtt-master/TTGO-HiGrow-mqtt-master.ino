#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <PubSubClient.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

// Logfile on SPIFFS
#include "SPIFFS.h"

//           rel = "1.2"; // Corrected error if Network not available, battery drainage solved by goto sleep 5 minutes
//           rel = "1.3"; // Corrected error if MQTT broker not available, battery drainage solved by goto sleep 5 minutes
//           rel = "1.4"; // Calibrate SOIL info messages where to be done
//           rel = "1.5"; // Implemented logfile in SPIFFS, and optimizing code. (DHT11 not implementet, but it only affects Air Temperature and Air Humidity, which I currently not use in my project. It will be implemented in a later release.
const String rel = "1.6"; // Implemented MQTT userid and password, and adapted so DHT11, DHT12 and DHT22 can be used.
// *******************************************************************************************************************************
// START userdefined data
// *******************************************************************************************************************************

// Turn logging on/off - turn read logfile on/off, turn delete logfile on/off ---> default is false for all 3, otherwise it can cause battery drainage.
const bool  logging = false;
const bool  readLogfile = false;
const bool  deleteLogfile = false;
String readString; // do not change this variable

// Select DHT type on the module - supported are DHT11, DHT12, DHT22
#define DHT_TYPE DHT11
//#define DHT_TYPE DHT12
//#define DHT_TYPE DHT22

const char* ssid = "Your SSID";
const char* password = "Your Password";
const char* ntpServer = "pool.ntp.org";

// Off-sets for time, and summertime. each hour is 3.600 seconds.
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// Device configuration and name setting, change for each different device, and device placement
const String device_name = "Master";
const String device_placement = "Drivhus";

#define uS_TO_S_FACTOR 1000000ULL  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  3600       //Time ESP32 will go to sleep (in seconds)

// mqtt constants
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

const char broker[] = "192.168.1.64";
int        port     = 1883;
const char mqttuser[] = ""; //add eventual mqtt username
const char mqttpass[] = ""; //add eventual mqtt password

const String topicStr = device_placement + "/" + device_name;
const char* topic = topicStr.c_str();

// *******************************************************************************************************************************
// END userdefined data
// *******************************************************************************************************************************

RTC_DATA_ATTR int bootCount = 0;
int sleep5no = 0;

//json construct setup
struct Config {
  String date;
  String time;
  int bootno;
  int sleep5no;
  float lux;
  float temp;
  float humid;
  float soil;
  float salt;
  float bat;
  String rel;
};
Config config;

const int led = 13;

#define I2C_SDA             25
#define I2C_SCL             26
#define DHT_PIN             16
#define BAT_ADC             33
#define SALT_PIN            34
#define SOIL_PIN            32
#define BOOT_PIN            0
#define POWER_CTRL          4
#define USER_BUTTON         35

BH1750 lightMeter(0x23); //0x23
Adafruit_BME280 bmp;     //0x77 Adafruit_BME280 is technically not used, but if removed the BH1750 will not work - Any suggestions why, would be appriciated.

DHT dht(DHT_PIN, DHT_TYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp1;

bool bme_found = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Void Setup");

  // Initiate SPIFFS and Mount file system
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    if (logging) {
      writeFile(SPIFFS, "/error.log", "An Error has occurred while mounting SPIFFS \n");
    }
    return;
  }
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Void Setup \n");
  }

  listDir(SPIFFS, "/", 0);

  if (logging) {
    writeFile(SPIFFS, "/error.log", "After listDir \n");
  }

  if (readLogfile) {
    // Now we start reading the files..
    readFile(SPIFFS, "/error.log");
    Serial.println("Here comes the logging info:");
    Serial.println(readString);
  }

  if (deleteLogfile) {
    SPIFFS.remove("/error.log");
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Before Start WIFI \n");
  }

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("Connected to network");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Connected to network \n");
  }

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  timeClient.setTimeOffset(7200);

  // Connect to mqtt broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Attempting to connect to the MQTT broker! \n");
  }

  Serial.println(broker);
  mqttClient.setServer(broker, 1883);

  if (!mqttClient.connect(broker, mqttuser, mqttpass)) {
    if (logging) {
      writeFile(SPIFFS, "/error.log", "MQTT connection failed! \n");
    }

    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.state());
    goToDeepSleepFiveMinutes();
  }

  if (logging) {
    writeFile(SPIFFS, "/error.log", "You're connected to the MQTT broker! \n");
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Wire.begin(I2C_SDA, I2C_SCL);
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Wire Begin OK! \n");
  }

  dht.begin();
  if (logging) {
    writeFile(SPIFFS, "/error.log", "DHT12 Begin OK! \n");
  }

  //! Sensor power control pin , use deteced must set high
  pinMode(POWER_CTRL, OUTPUT);
  digitalWrite(POWER_CTRL, 1);
  delay(1000);

  if (!bmp.begin()) {
    Serial.println(F("This check must be done, otherwise the BH1750 does not initiate!!!!?????"));
    bme_found = false;
  } else {
    bme_found = true;
  }

  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
    Serial.println(F("BH1750 Advanced begin"));
  } else {
    Serial.println(F("Error initialising BH1750"));
  }
  static uint64_t timestamp;
  float luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  delay(2000);
  float t12 = dht.readTemperature(); // Read temperature as Fahrenheit then dht.readTemperature(true)
  config.temp = t12;
  delay(2000);
  float h12 = dht.readHumidity();
  config.humid = h12;
  uint16_t soil = readSoil();
  config.soil = soil;
  uint32_t salt = readSalt();
  config.salt = salt;
  float bat = readBattery();
  config.bat = bat;
  config.bootno = bootCount;


  luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  config.lux = luxRead;
  config.rel = rel;

  while (!timeClient.update()) {
    timeClient.forceUpdate();
  }

  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  // Extract date
  int splitT = formattedDate.indexOf("T");
  dayStamp = formattedDate.substring(0, splitT);
  dayStamp = dayStamp.substring(5);
  String dateMonth = dayStamp.substring(0, 2);
  String dateDay = dayStamp.substring(3, 5);
  Serial.print("dateMonth: ");
  Serial.println(dateMonth);
  Serial.print("dateDay: ");
  Serial.println(dateDay);
  dayStamp = dateDay + "-" + dateMonth;
  config.date = dayStamp;
  // Extract time
  timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  config.time = timeStamp1.substring(0, 5);

  // Create JSON file
  Serial.println(F("Creating JSON document..."));
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Creating JSON document...! \n");
  }

  saveConfiguration(config);

  // Go to sleep
  //Increment boot number and print it every reboot
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));


  //Go to sleep now
  delay(1000);
  goToDeepSleep();
}

void goToDeepSleep()
{
  Serial.print("Going to sleep... ");
  Serial.print(TIME_TO_SLEEP);
  Serial.println(" seconds");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for 3600 seconds \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}
void goToDeepSleepFiveMinutes()
{
  Serial.print("Going to sleep... ");
  Serial.print("300");
  Serial.println(" sekunder");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Going to sleep for 300 seconds \n");
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake us up!
  ++sleep5no;
  esp_sleep_enable_timer_wakeup(300 * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}

// READ Sensors
// I am not quite sure how to read and use this number. I know that when put in water wich a DH value of 26, it gives a high number, but what it is and how to use ??????
// Calibration is done it two medias - water only, fertilizer only - this gives minimum of water with no fertilazier and maximum in with concentration of fertilizer.
// Target value for maximum should probably be solution of water + fertilizer in optimal value
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++) {
    array[i] = analogRead(SALT_PIN);
    //    Serial.print("Read salt pin : ");

    //    Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++) {
    if (i == 0 || i == samples - 1)continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return map(humi, 26, 350, 0, 100); // Salt defaults. It needs to be calculated too
}

uint16_t readSoil()
// It is a really good thing to calibrate each unit for soil, first note the number when unit is on the table, the soil number is for zero humidity. Then place the unit up to the electronics into a glass of water, the number now is the 100% humidity.
// By doing this you will get the same readout for each unit. Replace the value below for the dry condition, and the 100% humid condition, and you are done.
{
  uint16_t soil = analogRead(SOIL_PIN);
  Serial.print("Soil before map: ");
  Serial.println(soil);
  return map(soil, 1400, 3400, 100, 0); // Soil defaults - change them to your calibration data
}

float readBattery()
{
  int vref = 1100;
  uint16_t volt = analogRead(BAT_ADC);
  float battery_voltage = ((float)volt / 4095.0) * 2.0 * 3.3 * (vref) / 1000;
  Serial.print("Battery Voltage: ");
  Serial.println(battery_voltage);
  battery_voltage = battery_voltage * 100;
  int battery_voltage_int = int(battery_voltage);
  return map(battery_voltage, 416, 300, 100, 0);
}

// Allocate a  JsonDocument
void saveConfiguration(const Config & config) {
  StaticJsonDocument<1024> doc;
  // Set the values in the document
  // Device changes according to device placement
  JsonObject plants = doc.createNestedObject(device_placement);
  plants[device_placement] = device_name;
  JsonObject date = doc.createNestedObject("date");
  date["date"] = config.date;
  JsonObject time = doc.createNestedObject("time");
  time["time"] = config.time;
  JsonObject sleep5Count = doc.createNestedObject("sleep5Count");
  sleep5Count["sleep5Count"] = config.sleep5no;
  JsonObject bootCount = doc.createNestedObject("bootCount");
  bootCount["bootCount"] = config.bootno;
  JsonObject lux = doc.createNestedObject("lux");
  lux["lux"] = config.lux;
  JsonObject temp = doc.createNestedObject("temp");
  temp["temp"] = config.temp;
  JsonObject humid = doc.createNestedObject("humid");
  humid["humid"] = config.humid;
  JsonObject soil = doc.createNestedObject("soil");
  soil["soil"] = config.soil;
  JsonObject salt = doc.createNestedObject("salt");
  salt["salt"] = config.salt;
  JsonObject bat = doc.createNestedObject("bat");
  bat["bat"] = config.bat;
  JsonObject rel = doc.createNestedObject("rel");
  rel["rel"] = config.rel;

  // Send to mqtt
  char buffer[1024];
  serializeJson(doc, buffer);

  Serial.print("Sending message to topic: ");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Sending message to topic: \n");
  }

  Serial.println(buffer);

  bool retained = true;
  int qos = 0;
  if (mqttClient.publish(topic, buffer, retained)) {
    Serial.println("Message published successfully");
  } else {
    Serial.println("Error in Message, not published");
  }
  Serial.println();
}

void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  if (logging) {
    writeFile(SPIFFS, "/error.log", "Connecting to Network: \n");
  }

  while ( WiFi.status() !=  WL_CONNECTED )
  {
    // wifi down, reconnect here
    WiFi.begin(ssid, password);
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED )
    {
      delay( 100 );
      Serial.printf(".");
      if (UpCount >= 60)  // just keep terminal from scrolling sideways
      {
        UpCount = 0;
        Serial.printf("\n");
      }
      ++UpCount;
      ++WLcount;
      if (WLcount > 200) {
        goToDeepSleepFiveMinutes();
      }
    }
  }
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

void readFile(fs::FS &fs, const char * path) {
  //  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  //  Serial.println("- read from file:");
  while (file.available()) {
    delay(2);  //delay to allow byte to arrive in input buffer
    char c = file.read();
    readString += c;
  }
  //  Serial.println(readString);
  file.close();
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void loop() {
}
