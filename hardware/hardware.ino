//##################################################################################################################
//##                                      GOLDIE B'S ELET2415 WEATHER STATION                                     ##
//##                                                                                                              ##
//##################################################################################################################

// LIBRARY IMPORTS
#include <rom/rtc.h>
#include <MCUFRIEND_kbv.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include <SPI.h>
#include <Adafruit_ST7735.h>              // Hardware-specific library
#include <Adafruit_BMP280.h>              // include  Adafruit BMP280 sensor library
#include "DHT.h"
#include <Wire.h>                         //include Two Wire Interface library
#include <math.h>


#ifndef _WIFI_H 
#include <WiFi.h>
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


// TFT VARIABLES AND PIN MAPPINGS:
#define TFT_CS    5
#define TFT_RST   16
#define TFT_DC    17
#define TFT_MOSI  23
#define TFT_SCK   18
#define TFT_MISO  19

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

// RING METER COLOUR SCHEMES
#define RED2RED     0
#define GREEN2GREEN 1
#define BLUE2BLUE   2
#define BLUE2RED    3
#define GREEN2RED   4
#define RED2GREEN   5

#define soilMoisturePin   32    // ESP: SVP
#define DHTPIN 4 
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define BMP280_ADDRESS    (0x76)


DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280  bmp;
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST, TFT_MISO);


// MQTT CLIENT CONFIG  
static const char* pubtopic      = "620148851";                     // Add your ID number here
static const char* subtopic[]    = {"620148851_sub","/elet2415"};   // Array of Topics(Strings) to subscribe to
static const char* mqtt_server   = "www.yanacreations.com";         // Broker IP address or Domain name as a String 
static uint16_t mqtt_port        = 1883;

// WIFI CREDENTIALS
const char* ssid       = "MonaConnect";                       // Add your Wi-Fi ssid
const char* password   = "";                // Add your Wi-Fi password 

// TASK HANDLES 
TaskHandle_t xMQTT_Connect          = NULL; 
TaskHandle_t xNTPHandle             = NULL;  
TaskHandle_t xLOOPHandle            = NULL;  
TaskHandle_t xUpdateHandle          = NULL;
TaskHandle_t xButtonCheckeHandle    = NULL;  

/* Declare and Initialize variables to use in code: */
float h                   = 0;
float t                   = 0;
float f                   = 0;
float hif                 = 0;
float hic                 = 0;
float altitude            = 0;
int soil_moisture         = 0;
int tesmod                = 0; 
float pressure            = 0;
int count                 = 0;
int8_t ramp               = 1;
boolean alert             = 0;
String str;


// FUNCTION DECLARATION   
void checkHEAP(const char* Name);   // RETURN REMAINING HEAP SIZE FOR A TASK
void initMQTT(void);                // CONFIG AND INITIALIZE MQTT PROTOCOL
unsigned long getTimeStamp(void);   // GET 10 DIGIT TIMESTAMP FOR CURRENT TIME
void callback(char* topic, byte* payload, unsigned int length);
void initialize(void);
bool publish(const char *topic, const char *payload); // PUBLISH MQTT MESSAGE(PAYLOAD) TO A TOPIC
void vButtonCheck( void * pvParameters );
void vUpdate( void * pvParameters );  
bool isNumber(double number);


const int dry = 3200; // value for dry sensor
const int wet = 1400; // value for wet sensor

//############### IMPORT HEADER FILES ##################
#ifndef NTP_H
#include "NTP.h"
#endif

#ifndef MQTT_H
#include "mqtt.h"
#endif





void setup() {
  Serial.begin(115200);

  Serial.println("Weather Station");

  // initialize SPI
  SPI.begin();

  // initialize DHT
  dht.begin();

  while ( !Serial ) delay(100);   // wait for native usb
  unsigned status;
  //status = bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID);
  status = bmp.begin();
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
  // initialize TFT
  tft.begin(0x1580);
  
  tft.setRotation(1);               // Set Rotation at 0 degress (default)

  tft.fillScreen(BLACK);            //Set Background Color with BLACK

  // INIT WIFI, MQTT & NTP 
  initialize();
  // vButtonCheckFunction();        // UNCOMMENT IF USING BUTTONS THEN ADD LOGIC FOR INTERFACING WITH BUTTONS IN THE vButtonCheck FUNCTION
}


void loop() {
  // Wait a few seconds between measurements.
  delay(2000);

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  sensors_event_t pressure_event;
  // bmp pressure & altitude:
  bmp_pressure->getEvent(&pressure_event);
  altitude = bmp.readAltitude(1013.25);
  pressure = pressure_event.pressure;

  // soil moisture
  int sensorVal = analogRead(A0);
  int soil_moisture = map(sensorVal, wet, dry, 100, 0);
  if (soil_moisture>100){
    soil_moisture = 100;
  }
  else if (soil_moisture<100){
    soil_moisture = 0;
  }

  int xpos = 0, ypos = 5, gap = 4, radius = 52;
  xpos = 320/2 - 150, ypos = 15, gap = 100, radius = 50;

  // DEBGGING, TO REMOVE:
  count+=1;

  // Draw analogue meter: temp in C (DHT)
  ringMeter(t, 0, 40, 0, 0, radius, "CELSIUS", GREEN );

  // Draw analogue meter: pressure in hPa (BMP)
  ringMeter(pressure, 0, 1100, 108, 0, radius, "PRESSURE", GREEN);

  // // Draw analogue meter: soil moisture (SOIL_MOISTURE)
  // ringMeter(soil_moisture, 0, 100, 108, 120, radius, "SOIL MOISTURE", GREEN);

  tft.setCursor(108, 170);
  tft.setTextSize(1.5);
  tft.print(soil_moisture, 1);
  tft.setTextSize(1.5);
  tft.setTextColor(GREEN, BLACK);
  tft.print (" SOIL MOISTURE ");

  tft.setCursor(218, 50);
  tft.setTextSize(1.5);
  tft.print(h, 1);
  tft.setTextSize(1.5);
  tft.setTextColor(GREEN, BLACK);
  tft.print (" HUMIDITY ");

  tft.setCursor(0, 170);
  tft.setTextSize(1.5);
  tft.print(altitude, 1);
  tft.setTextSize(1.5);
  tft.setTextColor(GREEN, BLACK);
  tft.print (" ALTITUDE ");


  // update DHT temp. & unit
  tft.setCursor (32, 90);
  tft.setTextSize (1);
  tft.setTextColor (GREEN, BLACK);
  tft.print (t, 1); 
  tft.setTextSize (1);
  tft.setTextColor (GREEN, BLACK);
  tft.print (" C ");
  tesmod = 1;

  // update BMP Pressure & UNIT only
  tft.setCursor (138, 90);
  tft.setTextSize (1);
  tft.setTextColor (MAGENTA, BLACK);
  tft.print (pressure, 1); 
  // tft.setCursor(250, 50);
  tft.setTextSize (1);
  tft.setTextColor (MAGENTA, BLACK);
  tft.print (" hPa ");
  
  // update DHT Humidity & unit
  tft.setCursor (247, 90);
  tft.setTextSize (1);
  tft.setTextColor (GREEN, BLACK);
  tft.print (h, 1);
  tft.setTextSize (1);
  tft.setTextColor (GREEN, BLACK);
  tft.print (" % ");
  tesmod = 1;

  // update BMP altitude & unit
  tft.setCursor(27, 210);  
  tft.setTextSize (1.5);
  tft.setTextColor ( YELLOW , BLACK);
  tft.print (altitude);
  //tft.setCursor(250, 50);  
  tft.setTextSize (1.5);
  tft.setTextColor ( YELLOW , BLACK);
  tft.print (" m ");
  
  // update the soil moisture & unit
  tft.setCursor (142,210);
  tft.setTextSize (1.5);
  tft.setTextColor (GREEN,BLACK);
  tft.print (soil_moisture);
  tft.setTextSize (1);
  tft.setTextColor (GREEN,BLACK);
  tft.print (" % ");
  tesmod=1;

  // PERSONALIZED TEXT:
  tft.setCursor(229, 130);  
  tft.setTextSize (1.5);
  tft.setTextColor ( MAGENTA , BLACK);
  tft.print("GB's Project");
  tft.setCursor(222, 139);  
  tft.setTextSize (1.7);
  tft.setTextColor ( MAGENTA , BLACK);
  tft.print("Weather Station");


  // DEBUGGING, PRINTING TO CONSOLE:
  Serial.print(count);
  Serial.println();

  Serial.print(F("Humidity: "));
  Serial.print(h);

  Serial.print(F("%  Temperature: "));
  Serial.print(t);

  Serial.print(F("°C "));
  Serial.print(f);
  
  Serial.print(F("°F  Heat index: "));
  Serial.print(hif);
  Serial.print(F("°F"));
  Serial.print(hic);
  Serial.println(F("°C "));

  Serial.print("Soil Moisture: ");
  Serial.print(soil_moisture );
  Serial.println("%");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.print(" hPa");
  Serial.println();

  Serial.print("Approx. Altitude: ");
  Serial.print(altitude);
  Serial.println(" m");

  
  Serial.println();
  // delay(1000);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}





//####################################################################
//#                          UTIL FUNCTIONS                          #       
//####################################################################
void vButtonCheck( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );     
      
    for( ;; ) {
        // Add code here to check if a button(S) is pressed
        // then execute appropriate function if a button is pressed  

        vTaskDelay(200 / portTICK_PERIOD_MS);  
    }
}

void vUpdate( void * pvParameters )  {
    configASSERT( ( ( uint32_t ) pvParameters ) == 1 );    
           
    for( ;; ) {
        if(isNumber(t)){
            // ##Publish update according to ‘{"id": "student_id", "timestamp": 1702212234, "temperature": 30, "humidity":90, "heatindex": 30}’

            // 1. Create JSon object
            StaticJsonDocument<1000> doc; // Create JSon object
            
            // 2. Create message buffer/array to store serialized JSON object
            char message[1100]  = {0};

            // 3. Add key:value pairs to JSon object based on above schema
            doc["id"]               = "620148851";
            doc["timestamp"]        = getTimeStamp();
            doc["temperature"]      = t;
            doc["humidity"]         = h;
            doc["heatindex"]        = hic;
            doc["soil moisture"]    = soil_moisture ;
            doc["pressure"]         = pressure;
            doc["altitude"]         = altitude;

            // 4. Serialize / Convert JSon object to JSon string and store in message array
            serializeJson(doc, message);

            // 5. Publish message to a topic subscribed to by both backend and frontend vgfc                
            if(mqtt.connected() ){
                publish(pubtopic, message);
            }
        }
    vTaskDelay(1000 / portTICK_PERIOD_MS);  
    }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // // ############## MQTT CALLBACK  ######################################
  // // RUNS WHENEVER A MESSAGE IS RECEIVED ON A TOPIC SUBSCRIBED TO
  
  // Serial.printf("\nMessage received : ( topic: %s ) \n",topic ); 
  // char *received = new char[length + 1] {0}; 
  
  // for (int i = 0; i < length; i++) { 
  //   received[i] = (char)payload[i];    
  // }

  // // PRINT RECEIVED MESSAGE
  // Serial.printf("Payload : %s \n",received);

 
  // // CONVERT MESSAGE TO JSON
  // StaticJsonDocument<1000> doc;
  // DeserializationError error = deserializeJson(doc, received);  

  // if (error) {
  //   Serial.print("deserializeJson() failed: ");
  //   Serial.println(error.c_str());
  //   return;
  // }


  // // PROCESS MESSAGE
  // const char* type = doc["type"]; 
  // // process messages with ‘{"type": "controls", "brightness": 255, "leds": 7, "color": { "r": 255, "g": 255, "b": 255, "a": 1 } }’ schema

  // if (strcmp(type, "controls") == 0){
  //   // 1. EXTRACT ALL PARAMETERS: NODES, RED,GREEN, BLUE, AND BRIGHTNESS FROM JSON OBJECT
  //   int nodes       = doc["leds"]; 
  //   int brightness  = doc["brightness"];
  //   int red         = doc["color"]["r"];
  //   int green       = doc["color"]["g"];
  //   int blue        = doc["color"]["b"];


  //   // 2. ITERATIVELY, TURN ON LED(s) BASED ON THE VALUE OF NODES. Ex IF NODES = 2, TURN ON 2 LED(s)
  //   for(unsigned char i=0; i<nodes; i++){
  //     leds[i]=CRGB(red, green, blue);
  //     FastLED.setBrightness(brightness);
  //     FastLED.show();
  //     delay(50);
  //   }
  //   // 3. ITERATIVELY, TURN OFF ALL REMAINING LED(s).
  //   for(unsigned char x=nodes; x<NUM_LEDS; x++){
  //     leds[x]=CRGB::Black;
  //     FastLED.setBrightness(brightness);
  //     FastLED.show();
  //     delay(50);
  //   }
  // }
}

unsigned long getTimeStamp(void) {
  // RETURNS 10 DIGIT TIMESTAMP REPRESENTING CURRENT TIME
  time_t now;         
  time(&now);         // Retrieve time[Timestamp] from system and save to &now variable
  return now;
}

bool publish(const char *topic, const char *payload){   
  bool res = false;
  try{
    res = mqtt.publish(topic,payload);
    // Serial.printf("\nres : %d\n",res);
    if(!res){
      res = false;
      throw false;
    }
  }
  catch(...){
    Serial.printf("\nError (%d) >> Unable to publish message\n", res);
  }
  return res;
}

bool isNumber(double number){
  char item[20];
  snprintf(item, sizeof(item), "%f\n", number);
  if(isdigit(item[0]))
    return true;
  return false;
}

int ringMeter(int value, int vmin, int vmax, int x, int y, int r, char *units, byte scheme) {
  // Minimum value of r is about 52 before value text intrudes on ring
  // drawing the text first is an option
  
  x += r; y += r;   // Calculate coords of centre of ring
  int w = r / 3;    // Width of outer ring is 1/4 of radius 
  int angle = 150;  // Half the sweep angle of meter (300 degrees)
  int v = map(value, vmin, vmax, -angle, angle); // Map the value to an angle v
  byte seg = 3; // Segments are 3 degrees wide = 100 segments for 300 degrees
  byte inc = 6; // Draw segments every 3 degrees, increase to 6 for segmented ring
  // Variable to save "value" text colour from scheme and set default
  int colour = GREEN;
 
  // Draw colour blocks every inc degrees
  for (int i = -angle+inc/2; i < angle-inc/2; i += inc) {
    // Calculate pair of coordinates for segment start
    float sx = cos((i - 90) * 0.0174532925);
    float sy = sin((i - 90) * 0.0174532925);
    uint16_t x0 = sx * (r - w) + x;
    uint16_t y0 = sy * (r - w) + y;
    uint16_t x1 = sx * r + x;
    uint16_t y1 = sy * r + y;

    // Calculate pair of coordinates for segment end
    float sx2 = cos((i + seg - 90) * 0.0174532925);
    float sy2 = sin((i + seg - 90) * 0.0174532925);
    int x2 = sx2 * (r - w) + x;
    int y2 = sy2 * (r - w) + y;
    int x3 = sx2 * r + x;
    int y3 = sy2 * r + y;

    if (i < v) { // Fill in coloured segments with 2 triangles
      switch (scheme) {
        case 0: colour = GREEN; break; // Fixed colour
        case 1: colour = RED; break; // Fixed colour
        case 2: colour = BLUE; break; // Fixed colour
        case 3: colour = rainbow(map(i, -angle, angle, 0, 127)); break; // Full spectrum blue to red
        case 4: colour = rainbow(map(i, -angle, angle, 70, 127)); break; // Green to red (high temperature etc)
        case 5: colour = rainbow(map(i, -angle, angle, 127, 63)); break; // Red to green (low battery etc)
        default: colour = GREEN; break; // Fixed colour
      }
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, colour);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, colour);
      //text_colour = colour; // Save the last colour drawn
    }
    else {        // Fill in blank segments
      tft.fillTriangle(x0, y0, x1, y1, x2, y2, RED);
      tft.fillTriangle(x1, y1, x2, y2, x3, y3, BLACK);
    }
  }
  // Convert value to a string
  char buf[10];
  byte len = 2; if ((value > 99) && (value <= 999)) len = 3; else if (value > 999) len = 4;   //        else if (value <= 9) len = 1;
  dtostrf(value, len, 0, buf);
  buf[len] = ' '; buf[len] = 0;             // Add blanking space and terminator, helps to centre text too!
  // Set the text colour to default

  // Serial.println(buf);
  // Serial.println(len);

  tft.setTextSize(1);

  if (len==3){
  tft.setTextColor(CYAN,BLACK);
  tft.setCursor(x-25,y-10);tft.setTextSize(3);
  tft.print(buf);}

  // else{
    if(value>9){
    tft.setTextColor(CYAN,BLACK);
    tft.setCursor(x-25,y-10);
    tft.setTextSize(3);
    if(len==4){tft.setTextSize(2.5);}
    tft.print(buf);}

    if(value<10){
    tft.setTextColor(colour,BLACK);
    tft.setCursor(x-25,y-10);tft.setTextSize(3);
    tft.print(buf);}

    // tft.setTextColor(CYAN,BLACK);
    // tft.setCursor(x-25,y-10);tft.setTextSize(3);
    // tft.print(buf);

    // VARIABLE LABELS
    tft.setTextColor(CYAN,BLACK);
    tft.setCursor(x-20,y+55);tft.setTextSize(1);
    tft.print(units);
  // }
  // Calculate and return right hand side x coordinate
  return x + r;
}

// #########################################################################
// Return a 16 bit rainbow colour
// #########################################################################
unsigned int rainbow(byte value) {
  // Value is expected to be in range 0-127
  // The value is converted to a spectrum colour from 0 = blue through to 127 = red

  byte red = 0; // Red is the top 5 bits of a 16 bit colour value
  byte green = 0;// Green is the middle 6 bits
  byte blue = 0; // Blue is the bottom 5 bits
  byte quadrant = value / 32;

  if (quadrant == 0) {
    blue = 31;
    green = 2 * (value % 32);
    red = 0;
  }
  if (quadrant == 1) {
    blue = 31 - (value % 32);
    green = 63;
    red = 0;
  }
  if (quadrant == 2) {
    blue = 0;
    green = 63;
    red = value % 32;
  }
  if (quadrant == 3) {
    blue = 0;
    green = 63 - 2 * (value % 32);
    red = 31;
  }
  return (red << 11) + (green << 5) + blue;
}
