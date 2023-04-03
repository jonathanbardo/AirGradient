/**
  Important: This code is only for the DIY PRO PCB Version 3.7 that has a push button mounted.

  This is the code for the AirGradient DIY PRO Air Quality Sensor with an ESP8266 Microcontroller without the TVOC module.

  It is a high quality sensor showing PM2.5, CO2, Temperature and Humidity on a small display and can send data over Wifi.

  Build Instructions: https://www.airgradient.com/open-airgradient/instructions/diy-pro/

  Kits (including a pre-soldered version) are available: https://www.airgradient.com/open-airgradient/kits/

  The codes needs the following boards installed:
  esp8266 version 3.0.2

  ESP8266 board config:
  LOLIN(WEMOS) D1 R2 & mini

  The codes needs the following libraries installed:
  “AirGradient” by AirGradient tested with version 2.4.0
  “WifiManager by tzapu, tablatronix” tested with version 2.0.11-beta
  “U8g2” by oliver tested with version 2.33.15

  Configuration:
  Please set in the code below the configuration parameters.

  If you have any questions please visit our forum at https://forum.airgradient.com/

  If you are a school or university contact us for a free trial on the AirGradient platform.
  https://www.airgradient.com/

  License: CC BY-NC 4.0 Attribution-NonCommercial 4.0 International
*/

#include <AirGradient.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>

#include <EEPROM.h>

// Oled display library
#include <U8g2lib.h>

// CONFIGURATION START

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// PM2.5 in US AQI (default ug/m3)
boolean inUSAQI = false;

// set to true if you want to connect to wifi. You have 60 seconds to connect. Then it will go into an offline mode.
boolean connectWIFI = true;

// CONFIGURATION END

AirGradient ag = AirGradient();

// for persistent saving and loading
int addr = 0;
byte value;

// If you have the display bottom right use bellow
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

const int port = 9926;
ESP8266WebServer server(port);

unsigned long currentMillis = 0;

const int oledInterval = 5000;
unsigned long previousOled = 0;

const int co2Interval = 5000;
unsigned long previousCo2 = 0;
int Co2 = 0;

const int pm25Interval = 5000;
unsigned long previousPm25 = 0;
int pm25 = 0;

const int tempHumInterval = 2500;
unsigned long previousTempHum = 0;
float temp = 0;
int hum = 0;

int buttonConfig=0;
int lastState = LOW;
int currentState;
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  u8g2.setBusClock(100000);
  u8g2.begin();
  // If you have display on top, comment out the line below.
  u8g2.setFlipMode(1);

  EEPROM.begin(512);
  delay(500);

  buttonConfig = String(EEPROM.read(addr)).toInt();
  setConfig();

  updateOLED2("Press Button", "Now for", "Config Menu");
  delay(2000);

  currentState = digitalRead(D7);
  if (currentState == HIGH) {
    updateOLED2("Entering", "Config Menu", "");
    delay(3000);
    lastState = LOW;
    inConf();
  }

  if (connectWIFI) {
    connectToWifi();
  }

  server.on("/", HandleMetricsJSON);
  server.on("/metrics/prometheus", HandleMetricsPrometheus);
  server.on("/metrics/json", HandleMetricsJSON);
  server.onNotFound(HandleNotFound);

  server.begin();

  updateOLED2("Warming up the", "sensors.", "");
  ag.CO2_Init();
  ag.PMS_Init();
  ag.TMP_RH_Init(0x44);
}

void loop() {
  currentMillis = millis();
  updateOLED();
  updateCo2();
  updatePm25();
  updateTempHum();
  server.handleClient();
}

void inConf() {
  setConfig();
  currentState = digitalRead(D7);

  if (lastState == LOW && currentState == HIGH) {
    pressedTime = millis();
  } else if (lastState == HIGH && currentState == LOW) {
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;
    if( pressDuration < 1000 ) {
      buttonConfig=buttonConfig+1;
      if (buttonConfig>7) buttonConfig=0;
    }
  }

  if (lastState == HIGH && currentState == HIGH) {
    long passedDuration = millis() - pressedTime;
    if ( passedDuration > 4000 ) {
      updateOLED2("Saved", "Release", "Button Now");
      delay(1000);
      updateOLED2("Rebooting", "in", "5 seconds");
      delay(5000);
      EEPROM.write(addr, char(buttonConfig));
      EEPROM.commit();
      delay(1000);
      ESP.restart();
    }
  }
  lastState = currentState;
  delay(100);
  inConf();
}

void setConfig() {
  if (buttonConfig == 0) {
    updateOLED2("Temp. in C", "PM in ug/m3", "Display Top");
    u8g2.setDisplayRotation(U8G2_R2);
    inF = false;
    inUSAQI = false;
  }
  if (buttonConfig == 1) {
    updateOLED2("Temp. in C", "PM in US AQI", "Display Top");
    u8g2.setDisplayRotation(U8G2_R2);
    inF = false;
    inUSAQI = true;
  }
  if (buttonConfig == 2) {
    updateOLED2("Temp. in F", "PM in ug/m3", "Display Top");
    u8g2.setDisplayRotation(U8G2_R2);
    inF = true;
    inUSAQI = false;
  }
  if (buttonConfig == 3) {
    updateOLED2("Temp. in F", "PM in US AQI", "Display Top");
    u8g2.setDisplayRotation(U8G2_R2);
    inF = true;
    inUSAQI = true;
  }
  if (buttonConfig == 4) {
    updateOLED2("Temp. in C", "PM in ug/m3", "Display Bottom");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = false;
    inUSAQI = false;
  }
  if (buttonConfig == 5) {
    updateOLED2("Temp. in C", "PM in US AQI", "Display Bottom");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = false;
    inUSAQI = true;
  }
  if (buttonConfig == 6) {
    updateOLED2("Temp. in F", "PM in ug/m3", "Display Bottom");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = true;
    inUSAQI = false;
  }
  if (buttonConfig == 7) {
    updateOLED2("Temp. in F", "PM in US AQI", "Display Bottom");
    u8g2.setDisplayRotation(U8G2_R0);
    inF = true;
    inUSAQI = true;
  }
}

void updateCo2() {
  if (currentMillis - previousCo2 <= co2Interval) {
    return;
  }

  previousCo2 += co2Interval;
  Co2 = ag.getCO2_Raw();

  Serial.println("CO2:" + String(Co2));
}

void updatePm25() {
  if (currentMillis - previousPm25 <= pm25Interval) {
    return;
  }

  previousPm25 += pm25Interval;
  pm25 = ag.getPM2_Raw();
  Serial.println("PM:" + String(pm25));
}

void updateTempHum() {
  if (currentMillis - previousTempHum <= tempHumInterval) {
    return;
  }

  previousTempHum += tempHumInterval;
  TMP_RH result = ag.periodicFetchData();

  temp = result.t;
  hum = result.rh;

  Serial.println("Temp:" + String(temp));
  Serial.println("Hum:" + String(hum));
}

void updateOLED() {
  if (currentMillis - previousOled <= oledInterval) {
    return;
  }

  previousOled += oledInterval;

  String ln1;
  String ln2;
  String ln3;

  if (inUSAQI) {
    ln1 = "AQI:" + String(pmToUsaQi(pm25)) +  " CO2:" + String(Co2);
  } else {
    ln1 = "PM:" + String(pm25) +  " CO2:" + String(Co2);
  }

  if (inF) {
    ln3 = "F:" + String((temp* 9 / 5) + 32) + " H:" + String(hum)+"%";
  } else {
    ln3 = "C:" + String(temp) + " H:" + String(hum)+"%";
  }

  updateOLED2(ln1, ln2, ln3);
}

void updateOLED2(String ln1, String ln2, String ln3) {
  char buf[9];
  u8g2.firstPage();
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_t0_16_tf);
    u8g2.drawStr(1, 10, String(ln1).c_str());
    u8g2.drawStr(1, 30, String(ln2).c_str());
    u8g2.drawStr(1, 50, String(ln3).c_str());
  } while (
    u8g2.nextPage()
  );
}

String GeneratePrometheusMetrics() {
  String message = "";
  String idString = "{mac=\"" + String(WiFi.macAddress().c_str()) + "\"}";

  message += "# HELP pm02 Particulate Matter PM2.5 value\n";
  message += "# TYPE pm02 gauge\n";
  message += "pm02";
  message += idString;
  message += String(pm25);
  message += "\n";

  message += "# HELP rco2 CO2 value, in ppm\n";
  message += "# TYPE rco2 gauge\n";
  message += "rco2";
  message += idString;
  message += String(Co2);
  message += "\n";

  message += "# HELP atmp Temperature, in degrees Celsius\n";
  message += "# TYPE atmp gauge\n";
  message += "atmp";
  message += idString;
  message += String(temp);
  message += "\n";

  message += "# HELP rhum Relative humidity, in percent\n";
  message += "# TYPE rhum gauge\n";
  message += "rhum";
  message += idString;
  message += String(hum);
  message += "\n";

  return message;
}

String GenerateJSONMetrics() {
  String message = "";

  message += "\"pm02\":" + String(pm25) + ",";
  message += "\"rco2\":" + String(Co2) + ",";
  message += "\"atmp\":" + String(temp) + ",";
  message += "\"rhum\":" + String(hum) + "";

  return "{" + message + "}";
}

void HandleMetricsPrometheus() {
  server.send(200, "text/plain", GeneratePrometheusMetrics());
}

void HandleMetricsJSON() {
  server.send(200, "application/json", GenerateJSONMetrics());
}

void HandleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/html", message);
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AG-" + String(ESP.getChipId(), HEX);
  updateOLED2("60s to connect", "to Wifi Hotspot", HOTSPOT);
  wifiManager.setTimeout(60);

  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    updateOLED2("booting into", "offline mode", "");
    Serial.println("failed to connect and hit timeout");
    delay(6000);
  }
}

int pmToUsaQi(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
