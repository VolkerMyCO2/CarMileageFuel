/*********************************************************************
This is to display your personal CO2 footprint and other nice stuff
last updated: Aug 22n 2020 by Volker Ebert
it also includes a little CO2 room sensor from Senseair. If you haven't got one: Don't worry or comment out


This is an example for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

This example is for a 128x64 size display using I2C to communicate
3 pins are required to interface (2 I2C and one reset)

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

font definition
https://github.com/olikraus/u8g2/wiki/fntlistall#20-pixel-height
*********************************************************************/

#include <Arduino.h>
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include <ESP8266WiFi.h>
#include <PubSubClient.h>


const char* ssid = "your WIFI SSID";
const char* password = "your WIFI password";
const char* mqtt_server = "192.168.178.27";  //or whatever your Openhab MQTT server is
const char* NodeName ="OLED3";  // free name

unsigned long DurHi;  // GPIO14 = D05 pwm input from CO2 sensor
unsigned long DurLo;
float Pwm_Val;
float CO2_Level;


#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[500];
int second = 0;
long lastSec = 0;
long remoteSec = 0;
int x_pos=0;
char disp_msg[500];
char disp_msg2[500];

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, 5,4);

void setup()   {                
  Serial.begin(115200);
  u8g2.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

}

void setup_wifi() {

  delay(10);
   Serial.println("Clear Display, start setup wifi");
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.drawStr(0,10,"Connecting to");  // write something to the internal memory
  u8g2.drawStr(0,20,ssid);
  u8g2.sendBuffer();          // transfer internal memory to the display
  
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    u8g2.print(".");
    u8g2.setCursor(x_pos, 40);
    x_pos+=5;
    u8g2.sendBuffer();
  }

  Serial.println(" ");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  u8g2.setCursor(0, 50);
  u8g2.print("IP=");
  u8g2.print(WiFi.localIP());
  u8g2.setCursor(0, 60);
  u8g2.print("Connected");
  u8g2.sendBuffer();
  


  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  String str1, str2,str3, str_head, str_val, str_einheit;
  char *ptr;

  str_head="";
  str_val="";
  str_einheit="";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  str3=String("");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
      if (i<11) disp_msg[i] = (char)payload[i];
      if (i>10) disp_msg2[i-11] = (char)payload[i];
      str3 = str3 + (char)payload[i];
  }
  Serial.println(" ");
  int blankPos = str3.indexOf(' ');
  if (blankPos >0 ) {
    str_head=str3.substring(0,blankPos);
    str_val=str3.substring(blankPos+1); //,pointPos-1);
  }
  str_val.trim();
  int pointPos = str_val.indexOf('.');
  if (pointPos>0) str_val=str_val.substring(0,pointPos);

  int einheitPos = str3.indexOf(':');
  if (einheitPos > 0) str_einheit = str3.substring(einheitPos + 1);

  // Problem: the dec C circle is being displayed with 2 characters. Means: kill one
  int gradPos = str_einheit.indexOf('C');
  if (gradPos >0) {
    str_einheit = str_einheit.substring(1);
  }
  
  str_val+=  str_einheit;
  
  Serial.println(str3);
  Serial.print("einheit=");  // means: unit
  Serial.println(str_einheit);
  Serial.print("str-val=");
  Serial.println(str_val);
  Serial.println();

 //  --------------------------------------------------------------------------------------
  //str1 = String("allg/temp");
  str1 = String("allg/display1"); 
  str2 = String(topic);
  
  if (str1 == str2) {        // MQTT command received
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_timB14_tf); // choose a suitable font
      u8g2.setCursor(0, 14);
      //u8g2.print(disp_msg);
      u8g2.print(str_head);
      
      u8g2.setFont(u8g2_font_osb26_tf);
      //u8g2.setFont(u8g2_font_logisoso26_tf);  // this font can do the deg C circle
      u8g2.setCursor(0, 55);

        Serial.print("allg/display1 Anzeige >>");
        Serial.print(str_val);
        Serial.println("<<");
        Serial.println(str_val.length());
        u8g2.print(str_val);
        u8g2.sendBuffer();

  } // light command received by MQTT
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection....");
    // Attempt to connect
    if (client.connect(NodeName)) {
      Serial.println("connected !");
      
      delay(2000);
      // ... and resubscribe
      client.subscribe("allg/display1");
      Serial.println("subscribed to allg/display1");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void loop() {

if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  

  
  
  if (now - lastMsg > 1000) {  // from here on execution once a second
    lastMsg = now;
    ++second;  // action each second
    if (second > 28800) {
      second = 0;  //prevent overflow  max 32K, 28K8 = 8 hours
      lastSec=0;
      remoteSec=0;
      }
  }

 // ********************************          execute each minute *********************************   
    if (second - lastSec > 60) {   // CHANGE HERE LATER TO 60 or do once each 5 seconds for debugging
        lastSec = second;  

// this is for a CO2 sensor from Senseair with pulse width output
    DurHi = pulseIn(14,HIGH);  // GPIO14 = D05
    DurLo = pulseIn(14,LOW);
    CO2_Level = 2000; // just in case there is no Low-value
      if (DurLo > 0)
      {
        CO2_Level = 2000 * (float)DurHi / ((float)DurHi + (float)DurLo);
        dtostrf(CO2_Level,4,1,msg);
        client.publish("Wozi/CO2",msg);
      }
      Serial.print("Hi =");
        Serial.print(DurHi);
        Serial.print("Lo =");
        Serial.print(DurLo);
        Serial.print("CO2 =");
        Serial.print(CO2_Level);
    } // each minute  
}  // END LOOP
