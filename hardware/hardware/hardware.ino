#include <Wire.h> 
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <Adafruit_ImageReader.h>
#include <WiFi.h>
#include "time.h"
//#include <PubSubClient.h>
#include <Fonts/FreeSerifBoldItalic24pt7b.h>
#include <Fonts/FreeSerifBoldItalic18pt7b.h>
#include <Fonts/FreeSerifBoldItalic12pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerifItalic12pt7b.h>
#include <Fonts/FreeSerifItalic9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>

/ IMPORT ALL REQUIRED LIBRARIES
#include <rom/rtc.h>
#include <MCUFRIEND_kbv.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <SPI.h>
#include <Adafruit_ST7735.h>              // Hardware-specific library
#include <Adafruit_BMP280.h>              // include  Adafruit BMP280 sensor library
#include <DHT.h>
#include <Wire.h>                         //include Two Wire Interface library
#include <math.h>

#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
#endif
#ifndef _WIFI_H 
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifndef STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDIO_H
#include <stdio.h>
#endif

#ifndef ARDUINO_H
#include <Arduino.h>
#endif
 
#ifndef ARDUINOJSON_H
#include <ArduinoJson.h>
#endif

// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620148851";                     // Add your ID number here
static const char* subtopic[]    = {"620148851_sub","/elet2415"};   // Array of Topics(Strings) to subscribe to
static const char* mqtt_server   = "broker.hivemq.com";         // Broker IP address or Domain name as a String 
static uint16_t mqtt_port        = 1883;


const char* ssid       = "MonaConnect";
const char* password   = "";


// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL;  
//WiFiClient espClient;
//PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321
#define DHTPIN 4

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)

#define TFT_DC 9
#define TFT_CS 10
#define TFT_MOSI 23
#define TFT_CLK 24
#define TFT_RST 2
#define TFT_MISO 25

// SENSOR VARIABLES AND PIN MAPPINGS:
#define DHTPIN    32
#define DHTTYPE   DHT22
#define BMP280_ADDRESS  (0x76)
#define analogPin 35    // ESP: SVP

// TFT COLOUR SCHEMES
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x2108

#define SW1 27
#define SW2 2
#define SW3 33

Adafruit_BMP280 bmp; 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
// const int dry = 2300; // value for dry sensor
// const int wet = 900; // value for wet sensor

// const char* ssid       = "Lynx";
// const char* password   = "CR892023";
// const char* mqtt_server = "176.58.103.182";
// const char* mqtt_SubTopic = "/2415/project"; 
// const char* IDNumber      = "620133617";   
// const char* device_name   = IDNumber;
// const char* mqtt_PubTopic = "OG_Project";

// const char* ntpServer = "pool.ntp.org";
unsigned long epochTime;

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// //SetUp WiFi
// void setup_wifi() {
//   delay(10);
//   // We start by connecting to a WiFi network
//   Serial.println();
//   Serial.print("Connecting to ");
//   Serial.println(ssid);

//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("");
//   Serial.println("WiFi connected");
//   Serial.print("IP address: ");
//   Serial.println(WiFi.localIP());
// }

// void callback(char* topic, byte* message, unsigned int length) {
//   Serial.print("Message arrived on topic: ");
//   Serial.print(topic);
//   Serial.print(". Message: ");
//   String messageTemp;
  
//   for (int i = 0; i < length; i++) {
//     Serial.print((char)message[i]);
//     messageTemp += (char)message[i];
//   }
//   Serial.println();
//   Serial.print(messageTemp);
 
// }

// void reconnect(){
//   // Loop until we're reconnected
//   while (!client.connected()) {
//     Serial.print("Attempting MQTT connection...");
//     // Attempt to connect
//     if(client.connect(device_name)) {
//       Serial.println("connected");
//       Serial.print("Subscribing to: ");
//       Serial.println(mqtt_PubTopic);
//       // Subscribe
//       client.subscribe(mqtt_SubTopic);
//     } else {
//       Serial.print("failed, rc=");
//       Serial.print(client.state());
//       Serial.println(" try again in 2 seconds");
//       // Wait 2 seconds before retrying
//       delay(2000);
//     }
//   }
// }

void setup() {
  Serial.begin(115200);
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);
  pinMode(SW3, INPUT_PULLUP);
  
  // setup_wifi();
  // client.setServer(mqtt_server, 1883);
  // client.setCallback(callback);
  
  
  // //init and get the time
  // configTime(0, 0, ntpServer);

  Serial.println(F("BMP280 test"));
if (!bmp.begin(0x76)) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    while (1);
}
/* Default settings from datasheet. */
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                  Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                  Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                  Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                  Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

   Serial.println(F("DHT22 test!"));
   dht.begin();
  
  tft.begin();

  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(30, 30);
  tft.setFont(&FreeSerif18pt7b);
  tft.println("Welcome To");
  tft.setFont(&FreeSerifBoldItalic24pt7b);
  tft.setCursor(80, 100);
  tft.println("OG");
  tft.setCursor(0, 150);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Weather System");
  tft.setFont(&FreeSansBold9pt7b);
  tft.setCursor(0, 200);
  tft.println("Press any button to see the respective parameters:");
  tft.setFont(&FreeSerifItalic12pt7b);
  tft.setCursor(20, 280);
  tft.println("Soil");
  tft.setCursor(80, 280);
  tft.println("Thermal");
  tft.setCursor(190, 280);
  tft.println("Air");
  tft.drawTriangle(30, 318, 10, 290, 50, 290, ILI9341_WHITE);
  tft.fillTriangle(30, 318, 10, 290, 50, 290, ILI9341_WHITE);
  tft.drawTriangle(120, 318, 100, 290, 140, 290, ILI9341_WHITE);
  tft.fillTriangle(120, 318, 100, 290, 140, 290, ILI9341_WHITE);
  tft.drawTriangle(200, 318, 180, 290, 220, 290, ILI9341_WHITE);
  tft.fillTriangle(200, 318, 180, 290, 220, 290, ILI9341_WHITE);
  delay (7000);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextWrap(false);

  
    
}


// void loop() {

epochTime = getTime();
   Serial.print("Epoch Time: ");
   Serial.println(epochTime);
   delay(1000);

  
float Temp = bmp.readTemperature();
float DHTemp = dht.readTemperature();
float P = (bmp.readPressure()/100);
float Alt = bmp.readAltitude(1016);
float Humi = dht.readHumidity();
float HIndex = dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false);

int Soil = 0;
int sensorVal = analogRead(A4); //A4
    Serial.println(sensorVal);
    if((sensorVal>=1900) && (sensorVal<=2400)){
      Soil = 0;
    }
    else if((sensorVal>=1400) && (sensorVal<=1500)){
      Soil = 25;
    }
    else if((sensorVal>=1200) && (sensorVal<=1300)){
      Soil = 50;
    }
    else if((sensorVal>=1140) && (sensorVal<=1190)){
      Soil = 75;
    }
    else if((sensorVal>=900) && (sensorVal<=1000)){
      Soil = 100;
    }
    Serial.print(Soil);
    Serial.println("%");
  
    delay(100);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(1000);
 String message = "{\"ID\": "+ String(IDNumber) + ",\"Timestamp\":" + String(epochTime)+",\"BMPTemp\":"+String(BMPTemp)+",\"DHTemp\":"+String(DHTemp)+",\"Heat Index\":"+String(HeatIndex)+",\"Humidity\":"+String(Humidity)+",\"Pressure\":"+String(Pressure)+",\"Altitude\":"+String(Altitude)+",\"Soil Moisture\":"+String(Soil)+   "} "; 
  Serial.println(message);
  client.publish(mqtt_PubTopic, message.c_str());   
  value++;

  
    Serial.print(F("BMP280Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" °C");

    Serial.print(F("BMP280Pressure = "));
    Serial.print(bmp.readPressure()/100);
    Serial.println(" hPa");

    Serial.print(F("BMP280Approx altitude = "));
    Serial.print(bmp.readAltitude(1016)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();
    delay(1000);

  tft.setFont();
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(3);
  tft.setCursor(40, 10);
  tft.println("OG Weather");
  
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  
  tft.setCursor(90, 40);
  tft.println("DHT22");

  tft.setCursor(80, 150);
  tft.println("BMP280");

   tft.setCursor(40, 260);
  tft.println("Soil Moisture");
    
    tft.setTextColor(ILI9341_YELLOW,ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 170);
    tft.print("BMP Temp:");
    tft.print(bmp.readTemperature());
    tft.drawCircle(180, 160, 4, ILI9341_YELLOW);
    tft.print("  ");
    tft.println("C");
    tft.println(" ");
  //tft.print(" & ");
  //tft.print((bmp.readTemperature()*1.8)+32);
  //tft.print(" *F");
    tft.print("Pressure:");
    tft.print(bmp.readPressure()/100);
    tft.println("hPa");
    tft.println(" ");
    tft.print("Altitude:");
    tft.print(bmp.readAltitude(1016));
    tft.println(" m");

    tft.setTextColor(ILI9341_YELLOW, ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 60);
    tft.print("DHT Temp: ");
    tft.print(dht.readTemperature());
    tft.drawCircle(180, 50, 4, ILI9341_YELLOW);
    tft.print("  ");
    tft.println("C");
    tft.println(" ");
 //tft.print(" & ");
 //tft.print(dht.readTemperature(true));
 //tft.print(" *F");
    tft.print("Humidity: ");
    tft.print(dht.readHumidity());
    tft.println(" %");
    tft.println(" ");
    tft.print("HeatIndx: ");
    tft.print(dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false));
    tft.drawCircle(180, 120, 4, ILI9341_YELLOW);
    tft.print("  ");
    tft.println("C");
    
    tft.setTextColor(ILI9341_YELLOW,ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(0, 290);
    tft.print("Level   :");
    tft.print(Soil);
    tft.print(" %");



   if(digitalRead(SW1)==LOW){
    delay(200);
  tft.fillScreen(ILI9341_RED);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(20, 30);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Thermal Data");
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.setCursor(90, 280);
  tft.println("OG");
  tft.setCursor(0, 310);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Weather System");

  tft.setCursor(0, 60);
  tft.setFont();
  tft.setTextSize(2);
    tft.print("BMP Temp  :");
    tft.print(bmp.readTemperature());
    tft.drawCircle(200, 50, 4, ILI9341_WHITE);
    tft.print("  ");
    tft.print("C");
     tft.setCursor(0, 110);
    tft.print("DHT Temp  :");
    tft.print(dht.readTemperature());
    tft.drawCircle(200, 100, 4, ILI9341_WHITE);
    tft.print("  ");
    tft.println("C");
    tft.println(" ");
 //tft.print(" & ");
 //tft.print(dht.readTemperature(true));
 //tft.print(" *F");
 tft.setCursor(0, 160);
    tft.print("Humidity  :");
    tft.print(dht.readHumidity());
    tft.println(" %");
    tft.println(" ");
    tft.setCursor(0, 210);
    tft.print("Heat Index:");
    tft.print(dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false));
     tft.drawCircle(200, 200, 4, ILI9341_WHITE);
    tft.print("  ");
    tft.println("C");
    
  }
    
  if(digitalRead(SW2)==LOW){
  delay(200);
  tft.fillScreen(ILI9341_WHITE);
  tft.setTextColor(ILI9341_BLUE);
  tft.setCursor(50, 30);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Air Data");
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.setCursor(90, 280);
  tft.println("OG");
  tft.setCursor(0, 310);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Weather System");

    tft.setCursor(80, 60);
    tft.setFont();
    tft.setTextSize(2);
    tft.print("Pressure");
    tft.setCursor(60, 110);
    tft.print(bmp.readPressure()/100);
    tft.println(" hPa");
    tft.println(" ");
    tft.setCursor(80, 160);
    tft.print("Altitude");
    tft.setCursor(90, 210);
    tft.print(bmp.readAltitude(1016));
    tft.println(" m");
  } 
    if(digitalRead(SW3)==LOW){
  delay(200);
  tft.fillScreen(ILI9341_BLUE);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(50, 30);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Soil Data");
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.setCursor(90, 280);
  tft.println("OG");
  tft.setCursor(0, 310);
  tft.setFont(&FreeSerifBoldItalic18pt7b);
  tft.println("Weather System");

    tft.setFont();
    tft.setTextSize(3);
    tft.setCursor(50, 110);
    tft.print("Moisture");
    tft.setCursor(100, 160);
    tft.print(Soil);
    tft.print(" %");
  }
   
    // Wait a few seconds between measurements.
  delay(1000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  Serial.print(F("DHT22Humidity: "));
  Serial.print(dht.readHumidity());
  Serial.println(F("%"));
  Serial.print(F("DHT22Temperature: "));
  Serial.print(dht.readTemperature());// Read temperature as Celsius (the default)
  Serial.print(F("°C "));
  Serial.print(dht.readTemperature(true));// Read temperature as Fahrenheit (isFahrenheit = true)
  Serial.println(F("°F"));
  Serial.print(F("DHT22Heat index: "));
  Serial.print(dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity(), false));// Compute heat index in Celsius (isFahreheit = false)
  Serial.print(F("°C "));
  Serial.print(dht.computeHeatIndex(dht.readTemperature(), dht.readHumidity()));// Compute heat index in Fahrenheit (the default)
  Serial.println(F("°F"));
}


