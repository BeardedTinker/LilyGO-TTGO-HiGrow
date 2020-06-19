#include <Arduino.h>
#include <Button2.h>
#include <Wire.h>
#include <BH1750.h>
#include <DHT12.h>
#include <Adafruit_BME280.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoMqttClient.h>

#include "driver/adc.h"
#include <esp_wifi.h>
#include <esp_bt.h>

const String rel = "1.1";

const char* ssid = "Your SSID";
const char* password = "Your PassWord";
const char* ntpServer = "pool.ntp.org";

// Off-sets for time, and summertime. each hour is 3.600 seconds.
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// Device configuration and name setting, change for each different device, and device placement
const String device_name = "Greenhouse";
const String device_placement = "Master";

#define uS_TO_S_FACTOR 1000000ULL  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  3600       //Time ESP32 will go to sleep (in seconds)
RTC_DATA_ATTR int bootCount = 0;

//json construct setup
struct Config {
  String date;
  String time;
  int bootno;
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
#define DHT12_PIN           16
#define BAT_ADC             33
#define SALT_PIN            34
#define SOIL_PIN            32
#define BOOT_PIN            0
#define POWER_CTRL          4
#define USER_BUTTON         35

BH1750 lightMeter(0x23); //0x23
Adafruit_BME280 bmp;     //0x77 Adafruit_BME280 is technically not used, but if removed the BH1750 will not work - Any suggestions why, would be appriciated.
DHT12 dht12(DHT12_PIN, true);
Button2 button(BOOT_PIN);
Button2 useButton(USER_BUTTON);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
String formattedDate;
String dayStamp;
String timeStamp1;

// mqtt constants
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
const char broker[] = "192.168.1.64";
int        port     = 1883;
const String topic  = device_placement + "/" + device_name ;

bool bme_found = false;

void setup() {
  Serial.begin(115200);
  Serial.println("Void Setup");
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  // Start WiFi and update time
  connectToNetwork();
  Serial.println(" ");
  Serial.println("Connected to network");
  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  timeClient.setTimeOffset(7200);

  // Connect to mqtt broker
  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    while (1);
  }
  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  Wire.begin(I2C_SDA, I2C_SCL);

  dht12.begin();

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
  button.loop();
  useButton.loop();
  float luxRead = lightMeter.readLightLevel();
  Serial.print("lux ");
  Serial.println(luxRead);
  config.lux = luxRead;
  float t12 = dht12.readTemperature(); // Read temperature as Fahrenheit (isFahrenheit = true)
  config.temp = t12;
  float h12 = dht12.readHumidity();
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
  String dateMonth = dayStamp.substring(0,2);
  String dateDay = dayStamp.substring(3,5);
  Serial.print("dateMonth: ");
  Serial.println(dateMonth);
  Serial.print("dateDay: ");
  Serial.println(dateDay);
  dayStamp = dateDay + "-" + dateMonth;
  config.date = dayStamp;
  // Extract time
  timeStamp1 = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
  config.time = timeStamp1.substring(0,5);

  Serial.print("Subscribing to topic: ");
//  Serial.println(result);
  Serial.println();

  // subscribe to a topic
  mqttClient.subscribe(topic);

  // topics can be unsubscribed using:
  // mqttClient.unsubscribe(topic);

  Serial.print("Waiting for messages on topic: ");
  Serial.println(topic);
  Serial.println();


  // Create JSON file
  Serial.println(F("Creating JSON document..."));
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
  Serial.println("Going to sleep...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  btStop();

  adc_power_off();
  esp_wifi_stop();
  esp_bt_controller_disable();

  // Configure the timer to wake us up!
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);

  // Go to sleep! Zzzz
  esp_deep_sleep_start();
}

// READ Sensors
uint32_t readSalt()
{
  uint8_t samples = 120;
  uint32_t humi = 0;
  uint16_t array[120];

  for (int i = 0; i < samples; i++) {
    array[i] = analogRead(SALT_PIN);
//    Serial.print("læs salt pin : ");
//    Serial.println(array[i]);
    delay(2);
  }
  std::sort(array, array + samples);
  for (int i = 0; i < samples; i++) {
    if (i == 0 || i == samples - 1)continue;
    humi += array[i];
  }
  humi /= samples - 2;
  return humi;
}

uint16_t readSoil()
{
  uint16_t soil = analogRead(SOIL_PIN);
//  Serial.print("Soil before map: ");
//  Serial.println(soil);
  return map(soil, 1400, 3300, 100, 0);
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

  
  mqttClient.poll();
  Serial.print("Sending message to topic: ");
  Serial.println(buffer);
  // send message, the Print interface can be used to set the message contents
  bool retained = true;
  int qos = 0;
  mqttClient.beginMessage(topic, retained, qos);
  mqttClient.print(buffer);
  mqttClient.endMessage();
  Serial.println();
}

void connectToNetwork() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while ( WiFi.status() !=  WL_CONNECTED )
  {
    // wifi down, reconnect here
    WiFi.begin(ssid, password);
    int WLcount = 0;
    int UpCount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount < 200 )
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
    }
  }
}

void loop() {
}
