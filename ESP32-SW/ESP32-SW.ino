// last update May 31st 2020, saved in https://github.com/Volkermyco2
// This is part of the initiative: "My Daily CO2 footprint", see MyCO2.org
// ELMduino by https://github.com/PowerBroker2/ELMduino
// ELM327 compatible OBD-scanner connects via BLE to the ESP32. ESP32 forwards mileage and tank level via MQTT into any home automation system
// works well for me with this one https://www.veepeak.com/obd2-scanner/ - purchased directly to make sure not to get a bad clone
// OLED display is optional, I am using this one: https://www.az-delivery.de/collections/alle-produkte/products/1-3zoll-i2c-oled-display They also sell the ESP32
// to be compiled with board: "ESP32 ROVER Module"
// as it is too big, select: "No OTA (2MB APP / 2MB SPIFFS"
// Find comments like "ENTER YOUR" below. change them for your own network before compiling

// optional OLED 128 x 64 pixel: display in 5 rows:
// 1: Temperature 2: ELM connection status 3: Wifi+MQTT connection status 4: fuel content 5: km absolute
#define TextHeight 12

// Temperature measurement optional feature.  Using an NTC20K - cheaper than the Dallas stuff.... Resitor 20K to 3V, Thermistor to GND
float Temp_C;
int TempIONo = 34;  // GPIO channels used 39 did not work  34 35 33
int sensorValue = 0;  // variable to store the value coming from the sensor
int loops =0;  // to average the AD reading
float AverageAD =0; // to sum up AD readingd
int ChannelNo = 0;
float TankContent = 66.0;

// definitions for 1.3inch LCD 128 x 64 pixel ********************************************************************
#include <U8g2lib.h>
#ifdef U8X8_HAVE_HW_SPI
  #include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
  #include <Wire.h>
#endif
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, 22,21);
// Try: G22: SCL, G21: SDA
int x_pos=0;
char disp_msg[500];
char disp_msg2[500];

// definitions for WLAN and MQTT *********************************************************************
#include <PubSubClient.h>
#include <WiFi.h>
//#include "Esp32MQTTClient.h"

const char* ssid = "YOUR_SSID_here";               // ENTER YOUR WIFI HERE
const char* password = "YOUR_Pass_here";       // ENTER YOUR WIFI PASSWORD HERE
const char* mqtt_server = "192.168.178.27";  // ENTER YOUR MQTT BROKER IP ADRESS HERE
const char* NodeName ="Auto1";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
char TempMsg[50];
int sekunde = 0;
long lastSec = 0;
long lastOBDSec =0;  // when did we read OBD last time
int Elm_conn_status =0;
int Wifi_conn_status =0;
int MQTT_conn_status =0;
int actblink =0;   // blink 1/sec if CPU active

// definitions for OBD communication *****************************************************************
#include "BluetoothSerial.h"
#include "ELMduino.h"
#define DEBUG_PORT Serial
#define ELM_PORT SerialBT
#define ESP_BLUETOOTH_NAME "ESP32"
BluetoothSerial SerialBT;
ELM327 myELM327;

uint32_t rpm = 0;
uint32_t TankLiter = 0;
uint32_t KmGefahren = 0;

// definitions for IO ports and if you like: LEDs ****************************************************
#define LED_BUILTIN 22

// Setup routines ************************************************************************************
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  DEBUG_PORT.begin(115200);

  client.setServer(mqtt_server,1883);

  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.setFontMode(0); //Defines, whether the glyph and string drawing functions will write the background color (mode 0/solid 
  u8g2.setDrawColor(1);
  u8g2.sendBuffer();
}

// setup ELM *************** connection to OBD II Scanner***********************************************************
int setup_ELM(void)
{
  int Elm_conn_succ=1;
  ELM_PORT.begin("ESP32test", true);
  Serial.println("Attempting to connect to VEEPEAK...");

  u8g2.setFontMode(0); //Defines, whether the glyph and string drawing functions will write the background color (mode 0/solid, is_transparent = 0) 
  u8g2.setDrawColor(1);
  u8g2.drawStr(0,2*TextHeight,"ELM");  // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display

  if (!ELM_PORT.connect("VEEPEAK-1912AC95"))    // ENTER YOUR OBD SCANNER NAME HERE
  {
    Serial.println("Couldn't connect to VEEPEAK OBD scanner - Phase 1");
    Elm_conn_succ=0;   
  }
  if (!myELM327.begin(ELM_PORT))
  {
    u8g2.setDrawColor(1);
    u8g2.drawStr(60,2*TextHeight,"failed");  // write something to the internal memory
    u8g2.sendBuffer();          // transfer internal memory to the display
    Serial.println("Couldn't connect to VEEPEAK OBD scanner - Phase 2");
    Elm_conn_succ=0;  //while (1);
  }
  if (Elm_conn_succ ==1) {
    u8g2.setDrawColor(1);
    u8g2.drawStr(60,2*TextHeight,"connected");  // write something to the internal memory
    u8g2.sendBuffer();          // transfer internal memory to the display
  }
 return(Elm_conn_succ);
}

// setup Wifi *************************************************************************
int setup_wifi(void) {
  int wifi_succ =1;
  delay(10);
  int Tries=1;
  IPAddress gateway(192,168,178,1); // IP-Adresse des WLAN-Gateways
  IPAddress subnet(255,255,255,0); // Subnetzmaske
  IPAddress ip(192,168,178,28); // feste IP-Adresse für den WeMos (z.B. falls noch frei)
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);
  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  u8g2.drawStr(0,3*TextHeight,"WIFI ");  // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display

  while ( (WiFi.status() != WL_CONNECTED) && (Tries<40)) { 
    u8g2.setDrawColor(0); // Clear pixel
    u8g2.drawBox(60, 2*TextHeight, 68, TextHeight); // ohne das gehts nicht
    u8g2.setDrawColor(1); // set pixel when printing 
    u8g2.drawStr(60,3*TextHeight,"conn "); 
    u8g2.sendBuffer();
    delay(500);
    // text blinking invers
    u8g2.setDrawColor(1); // set pixel - invers
    u8g2.drawBox(60, 2*TextHeight, 128, TextHeight); // ohne das gehts nicht
    u8g2.setDrawColor(0); // set pixel when printing 
    u8g2.drawStr(60,24,"conn "); 
    u8g2.sendBuffer();
    
    Serial.print(".");  
    delay(500);
    Tries++;
   } // try loop Try for 1 min - give up after this

    if (WiFi.status()== WL_CONNECTED) {
      Serial.println("");  
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());

      u8g2.setDrawColor(0); // Clear pixel
      u8g2.drawBox(60, 2*TextHeight, 67, TextHeight); // doesn't work without..
      u8g2.setDrawColor(1); // set pixel when printing 
      u8g2.drawStr(60,3*TextHeight,"conn"); 
      u8g2.sendBuffer();
      wifi_succ=1;
     }

  if (WiFi.status()!= WL_CONNECTED) { 
    Serial.println("WiFi connection failed");
    wifi_succ=0;
    u8g2.setDrawColor(1); // Clear pixel
    u8g2.drawBox(60,2*TextHeight, 67, TextHeight); 
    u8g2.setDrawColor(0); // set pixel when printing 
    u8g2.drawStr(60,3*TextHeight,"fail");   
    u8g2.sendBuffer();
  }
  return wifi_succ;
}

// Reconnecting MQTT ***************************************************************************
int MQTTreconnect(void) {
  int MQTT_succ=0;
  int Tries=1;
  // Loop until we're reconnected
  while (!client.connected()&& (Tries<32)) {
    Serial.print("Attempting MQTT connection....");
    
    // Attempt to connect
    if (client.connect("Auto1Client")) {
      Serial.println("connected !");
      delay(300);
      MQTT_succ =1;
      Tries=99;  // exit loop
      u8g2.setFontMode(0);
      u8g2.setDrawColor(1);
      u8g2.drawStr(90,3*TextHeight,"MQ ok");  // write something to the internal memory
      u8g2.sendBuffer();  
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(client.state());
      MQTT_succ =0;
      Tries++;
      u8g2.setFontMode(0);
      u8g2.setDrawColor(1);
      Serial.println("MQTT conn try again in 5 seconds");
      u8g2.drawStr(90,3*TextHeight,"no MQ");  // write something to the internal memory
      u8g2.sendBuffer();
      delay(500);
    }
  // HIER NOCH AENDERN:  FAIL ERST NACH 32 VERSUCHEN EINTRAGEN  
  }
  return MQTT_succ;
}

// handle incoming messages we subscribed to - dont need it in this application yet. Just for completeness
void callback(char* topic, byte* payload, unsigned int length) {
  String str1, str2;
   
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  // handling incoming Light intensity via MQTT --------------------------------------------------------------------------------------
  str1 = String("Haus/DayNight");
  str2 = String(topic);
  if (str1 == str2) {        // Garage light command received
   if ((char)payload[0] == '0') {
     //DayNight=0;
     Serial.print("MQTT command received: DayNight = night \n");
   } //night
    if ((char)payload[0] == '1') {
     //DayNight=1;
     Serial.print("MQTT command received: DayNight = day \n");
   } //day
  } // light command received by MQTT
} // callback = handling incoming MQTT commands

// convert the incoming raw AD value into a temperature ------------------------------------------------------------------------------
float AD2temp (int ad_value){
        float Temperatur = 0.0;
        int i = 0;
        float Werte[201]; 
        long Res;
        long ResRaw;
        float a = 0; // for linear interpolation between full degrees:  Y(temp) = a * x(res) + b
        float b = 0; 
 // this is a one time entry if you like to have Temperature reading with a NTC resistor:  [0] is resistance at -50deg C  [1] is -49C etc               
        Werte[0]=1659706;    Werte[1]=1541379;  Werte[2]=1432919;  Werte[3]=1332091;  Werte[4]=1238358;
        Werte[5]=1153525;    Werte[6]=1073429;  Werte[7]=999894;   Werte[8]=932327;   Werte[9]=869327;
        Werte[10]=814000;    Werte[11]=759391;  Werte[12]=708806;  Werte[13]=661924;  Werte[14]=618451;
        Werte[15]=578119;    Werte[16]=540677;  Werte[17]=505902;  Werte[18]=473588;  Werte[19]=443546;
        Werte[20]=415600;    Werte[21]=389298;  Werte[22]=364833;  Werte[23]=342063;  Werte[24]=320860;
        Werte[25]=301107;    Werte[26]=282696;  Werte[27]=265528;  Werte[28]=249511;  Werte[29]=234561;
        Werte[30]=220600;    Werte[31]=207607;  Werte[32]=195459;  Werte[33]=184096;  Werte[34]=173463;
        Werte[35]=163508;    Werte[36]=154185;  Werte[37]=145450;  Werte[38]=137262;  Werte[39]=129583;
        Werte[40]=122380;    Werte[41]=115575;  Werte[42]=109189;  Werte[43]=103194;  Werte[44]=97564;
        Werte[45]=92274;        Werte[46]=87303;        Werte[47]=82628;        Werte[48]=78232;        Werte[49]=74094;
        Werte[50]=70200;        Werte[51]=66515;        Werte[52]=63046;        Werte[53]=59777;        Werte[54]=56697;
        Werte[55]=53793;        Werte[56]=51055;        Werte[57]=48472;        Werte[58]=46034;        Werte[59]=43733;
        Werte[60]=41560;        Werte[61]=39500;        Werte[62]=37553;        Werte[63]=35714;        Werte[64]=33975;
        Werte[65]=32331;        Werte[66]=30775;        Werte[67]=29303;        Werte[68]=27909;        Werte[69]=26590;
        Werte[70]=25340;        Werte[71]=24155;        Werte[72]=23032;        Werte[73]=21967;        Werte[74]=20958;
        Werte[75]=20000;        Werte[76]=19089;        Werte[77]=18224;        Werte[78]=17404;        Werte[79]=16624;
        Werte[80]=15884;        Werte[81]=15180;        Werte[82]=14511;        Werte[83]=13875;        Werte[84]=13270;
        Werte[85]=12695;        Werte[86]=12148;        Werte[87]=11627;        Werte[88]=11131;        Werte[89]=10659;
        Werte[90]=10210;        Werte[91]=9781;        Werte[92]=9373;        Werte[93]=8983;        Werte[94]=8612;
        Werte[95]=8258;        Werte[96]=7920;        Werte[97]=7598;        Werte[98]=7291;        Werte[99]=6998;
        delay(10);
        Werte[100]=6718;        Werte[101]=6450;        Werte[102]=6195;        Werte[103]=5951;        Werte[104]=5718;
        Werte[105]=5495;        Werte[106]=5282;        Werte[107]=5078;        Werte[108]=4883;        Werte[109]=4696;
        Werte[110]=4518;        Werte[111]=4347;        Werte[112]=4184;        Werte[113]=4027;        Werte[114]=3877;
        Werte[115]=3734;        Werte[116]=3596;        Werte[117]=3464;        Werte[118]=3338;        Werte[119]=3216;
        Werte[120]=3100;        Werte[121]=2989;        Werte[122]=2882;        Werte[123]=2779;        Werte[124]=2681;
        Werte[125]=2587;        Werte[126]=2496;        Werte[127]=2409;        Werte[128]=2325;        Werte[129]=2245;
        Werte[130]=2168;        Werte[131]=2094;        Werte[132]=2022;        Werte[133]=1954;        Werte[134]=1888;
        Werte[135]=1824;        Werte[136]=1763;        Werte[137]=1705;        Werte[138]=1648;        Werte[139]=1594;
        Werte[140]=1542;        Werte[141]=1491;        Werte[142]=1443;        Werte[143]=1396;        Werte[144]=1351;
        Werte[145]=1308;        Werte[146]=1266;        Werte[147]=1226;        Werte[148]=1187;        Werte[149]=1150;
        Werte[150]=1114;        Werte[151]=1079;        Werte[152]=1046;        Werte[153]=1014;        Werte[154]=982.8;
        Werte[155]=952.8;        Werte[156]=923.9;        Werte[157]=896.0;        Werte[158]=869.1;        Werte[159]=843.1;
        Werte[160]=818.0;        Werte[161]=793.7;        Werte[162]=770.3;        Werte[163]=747.7;        Werte[164]=725.8;
        Werte[165]=704.7;        Werte[166]=684.2;        Werte[167]=664.5;        Werte[168]=645.3;        Werte[169]=626.9;
        Werte[170]=609.0;        Werte[171]=591.7;        Werte[172]=575.0;        Werte[173]=558.8;        Werte[174]=543.2;
        Werte[175]=528.0;        Werte[176]=513;        Werte[177]=499;        Werte[178]=485;        Werte[179]=472;
        Werte[180]=459;        Werte[181]=447;        Werte[182]=435;        Werte[183]=423;        Werte[184]=411;
        Werte[185]=400;        Werte[186]=390;        Werte[187]=380;        Werte[188]=370;        Werte[189]=360;
        Werte[190]=351;        Werte[191]=342;        Werte[192]=333;        Werte[193]=324;        Werte[194]=316;
        Werte[195]=308;        Werte[196]=300;        Werte[197]=293;        Werte[198]=285;        Werte[199]=278;
        Werte[200]=271;

// AD value into resistance
  //Serial.print("adval=");
  //Serial.print(ad_value);
  if (ad_value < 1) ad_value =1;
  if (ad_value > 4095) ad_value = 4095;  // 12 bit resolution in ESP32!
  ResRaw = 44500.0 / ( 4095.0 / ad_value - 1);  // Rohwert, noch mit 330K Parallelwiderstand
  //                   1024 sollte das sein, aber bei U/2 war der AD Wert 535. Kam dann mit der R-Dekade sehr genau hin!
  //Res = 1.0/(1.0/ResRaw - 1/330000.0) ;          // 330K im NodeMCU rausrechnen, der noch parallel zum R-NTC liegt
  Res = ResRaw;  // assume no input current into analog input of ESP32
  Serial.print(" Res= ");
  Serial.print(Res);
  
  // search table until the real value is bigger
  Temperatur = 999;  // failure mode case
  for (int i = 0; i < 200; i++) {
    delay(5);
    if (Res > Werte[i]) {
      // do linear interpolation between i and i+1:  X= resistance in the middle between 2 known values i = Y= temp = 1 kelvin steps  
      // y = ax + b    a = (y2 - y1) / (x2 - x1)   b= y1 - a * y1
      a = 1.0 / (Werte[i+1] - Werte[i]);     // 1 is x2 - x1 or temp(i+1) - temp(i)
      b = i * 1.0 - a * Werte[i];
      Temperatur = a * Res + b -50.0; 
      //Temperatur = i-50.0;  //table starts at -50degC in 1 Kelvin steps
      break;
      } // if - found in table
    } // for

    //Serial.print(" T= ");
  //Serial.println(Temperatur);
return Temperatur;
}

// *********************************************************************************************************************************
// Main loop ***********************************************************************************************************************
// *********************************************************************************************************************************
void loop()
{
// Timing  each second - each minute or longer ****************************
  // keep MQTT alive
  if (WiFi.status() == WL_CONNECTED) {
      client.loop();    
  }

  delay(50);
  long now = millis();
  if (now - lastMsg > 1000) {  // from here on execution once a second
    lastMsg = now;
    ++sekunde;  // action each second
    if (actblink==0){  // blink if CPU is alive
      //digitalWrite(LED_ACT,LOW);
    }
    if (actblink==1){
      //digitalWrite(LED_ACT,HIGH);  // not implemented: in case you like to have LEDs instead of the OLED
    }
    if (actblink>1){
      actblink=0;
    }
    ++actblink;
    
    if (sekunde > 28800) {
      sekunde = 0;  //prevent overflow  max 32K, 28K8 = 8 hours
      lastSec=0;
      lastOBDSec=0;
      }

 // ********************************          execute each 6 seconds - or per minute *********************************   
    if (sekunde - lastSec > 1) {   // start after 1 second - wait at end
        lastSec = sekunde + 6;  //  each 6 seconds here, adjust to whatever you like it to be
        Serial.println(" minute-loop ");
  
 // action1: measure and display temperature 
    AverageAD=0;
    for (loops=0; loops <16; loops++) {  // make an average of 16 AD readings
      AverageAD=AverageAD + analogRead(TempIONo);
      }
    sensorValue = int(AverageAD / 16); 
    //sensorValue = analogRead(TempIONo[ChannelNo]); that was for 1 reading only
    Temp_C = AD2temp(sensorValue);

    // display Temp
    u8g2.clearBuffer();          // clear the internal memory
    u8g2.setFontMode(0); //Defines, whether the glyph and string drawing functions will write the background color (mode 0/solid, is_transparent = 0) 
    u8g2.setDrawColor(1);
    u8g2.drawStr(0,1*TextHeight,"Temp=");  // write something to the internal memory
    u8g2.sendBuffer(); 

    dtostrf(Temp_C,3,1,TempMsg);
    u8g2.drawStr(60,1*TextHeight,TempMsg);  // write something to the internal memory
    u8g2.sendBuffer();  
   
// try to connect to OBD --------------------------------------  
  Elm_conn_status = setup_ELM();

// Spaeter : erst nach 10 Minuten Werte veralten lassen - evtl ELM aus, dann Wifi connection
  Serial.print(" ElmConnStatus = "); Serial.println(Elm_conn_status);
  if (Elm_conn_status ==1) {
// if connected to OBD, read values ---------------------------
    float temprpm = myELM327.rpm();
    if (myELM327.status == ELM_SUCCESS)
    {
      rpm = (uint32_t)temprpm;
      Serial.print("rpm: "); Serial.print(rpm); Serial.print("  ");
    }
    else
    {
      Serial.print(F("\tERROR: "));
      Serial.println(myELM327.status);
    }
  
    float temptank = myELM327.tanklevel() * TankContent / 256.0;
    if (myELM327.status == ELM_SUCCESS)
    {
      TankLiter = (uint32_t)temptank;
      dtostrf(TankLiter,5,2,msg);     // syntax: dtostrf(var,len,prec,str)
      Serial.print("TankLevel: "); Serial.print(TankLiter); Serial.print("  ");
      u8g2.setFontMode(0);
      u8g2.setDrawColor(1);
      u8g2.drawStr(0,4*TextHeight,"Tank:");  // write something to the internal memory
      u8g2.drawStr(60,4*TextHeight,msg);
      u8g2.sendBuffer();  
    }
    else
    {
      Serial.print(F("\tERROR: "));
      Serial.println(myELM327.status);
    }

    float tempkm = myELM327.Km_scc();
    if (myELM327.status == ELM_SUCCESS)
    {
      KmGefahren = (uint32_t)tempkm;
      dtostrf(KmGefahren,5,0,msg);
      Serial.print("Km since code cleared: "); Serial.println(KmGefahren);
      u8g2.setFontMode(0);
      u8g2.setDrawColor(1);
      u8g2.drawStr(0,5*TextHeight,"km:");  // write something to the internal memory
      u8g2.drawStr(60,5*TextHeight,msg);
      u8g2.sendBuffer();  
      lastOBDSec=sekunde;  // make a time stamp whenever we got a successful read   
    }
    else
    {
      Serial.print(F("\tERROR: "));
      Serial.println(myELM327.status);
    }
  } // Elm connect was successful

// try to connect to Wifi --------------------------------------------------
       if (WiFi.status() != WL_CONNECTED) {   // are we still wifi-connected?
         Serial.print(" NO WIFI CONNECTION - reconnect");
         Wifi_conn_status=setup_wifi();
         delay(500);
         if (Wifi_conn_status==1){
          Serial.println(" wifi-ok");
         }
         if (Wifi_conn_status==0){
          Serial.println(" wifi-unconn");
         }
       } else {
        // we are connected to Wifi still
        Serial.println(" wifi-still conn");
       }

// try to connect to MQTT - if we have a Wifi connect-------------------------
  if (WiFi.status() == WL_CONNECTED) {
       if (!client.connected()) {   
          MQTT_conn_status = MQTTreconnect();
          // if Wifi connection established AND MQTT connection established: Publish car values
       }
          if (MQTT_conn_status ==1) {
            //digitalWrite(LED_MQTT, LOW);
 // we are on Wifi and on MQTT, so let's publish car temperature
            client.publish("auto1/temp",TempMsg);
            if (sekunde - lastOBDSec < 300) {   // publish if OBD information still fresh means < 5 min
              client.setCallback(callback); // not needed, just in case we will send something back one fine day
              
// publish the OBD values now            
              if (KmGefahren>0) {  // if it is 0, we have a misreading
                 dtostrf(KmGefahren,5,1,msg);     // syntax: dtostrf(var,len,prec,str)
                client.publish("auto1/km",msg);   
              }          
              
              if (TankLiter >0) {   // if it is 0, we have a misreading. 
                dtostrf(TankLiter,5,2,msg);     
                client.publish("auto1/tankinhalt",msg);
              }
              Serial.print("Published car info at ");
              Serial.println(sekunde-lastOBDSec);
            } else {
              Serial.println("Car info NOT published - outdated");
              rpm=0;  TankLiter=0;  KmGefahren=0;  // reset values not to publish old stuff
            } // OBD message outdated
          } // MQTT connected
        } // wifi connected for MQTT
    } // each minute  
  } //each second
 } // loop
  

// to be done later: implement deep sleep from: ESP32- examples
// definitions for deep sleep ************************************************************************
//#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
//#define TIME_TO_SLEEP  5        /* Time ESP32 will go to sleep (in seconds) */

//esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
// Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +  " Seconds");
//Serial.println("Going to sleep now");
//  Serial.flush(); 
//  esp_deep_sleep_start();
