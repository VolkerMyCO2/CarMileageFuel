/*
 from MyCO2.org
 last update: Nov 6th 2020
 This is to read a simple NTC sensor element with a Wemos - then transmitting it into MQTT.
 for example: To detect if you burner in the basement runs or not.
 board: LOLON(WEMOS) D1 R1 & Mini
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Update these with values suitable for your network.


const char* ssid = "YourSSIDHere";                 // enter your SSID here
const char* password = "YourWifiPasswordHere";     // enter your Wifi Password here
const char* mqtt_server = "192.168.178.27";

#define SLEEP_TIME 180  // sleep timer in seconds. Disable if zero

int sensorPin = A0;    // select the input pin for the potentiometer
int sensorValue = 0;  // variable to store the value coming from the sensor
int RetryIfNotSent =0; // if we could not connect, try again a bit later, do not sleep

float GaragenTemp = 9.9 ; // default start value

int DayNight =1;   // usually not used, just for fun

int Wifi_conn_status =0;
int MQTT_conn_status =0;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int sekunde = 0;
long lastSec = 0;


// convert the incoming raw AD value into a temperature
// This is for a NTC20K. If you use a different NTC: you need to adopt the values below. [0] = -50C  [200] = + 150C. Values are in Ohm
float AD2temp (int ad_value){
        float Temperatur = 0.0;
        int i = 0;
        float Werte[201]; 
        long Res;
        long ResRaw;
        float a = 0; // for linear interpolation between full degrees:  Y(temp) = a * x(res) + b
        float b = 0; 
                
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
// Keep in mind you have to have a pullup resistor of 26K - or similar to 3.3Volts. NTC between GND and Analog input.
  Serial.print("adval=");
  Serial.print(ad_value);
  if (ad_value < 1) ad_value =1;
  if (ad_value > 957) ad_value = 957;
  ResRaw = 26380.0 / ( 1100.0 / ad_value - 1);  // Rohwert, noch mit 330K Parallelwiderstand
  //                   1024 sollte das sein, aber bei U/2 war der AD Wert 535. Kam dann mit der R-Dekade sehr genau hin!
  Res = 1.0/(1.0/ResRaw - 1/330000.0) ;          // 330K im NodeMCU rausrechnen, der noch parallel zum R-NTC liegt
  
  Serial.print("  Res=");
  Serial.print(Res);
  Serial.print("  Resrwaw=");
  Serial.println(ResRaw);
  // Analog input has a 220K - 100K internal divider   A0 is related to full scale output 3.3V--> 10uAmps
  // also R-Roh ist parallel R-Mess mit 320K
  // gemessen 10.3.18: 0R -->ADVAL =1
  //                   open --> ADVAL = 957
  //Analog input: 0Volt  AI=3  / 1.478V  AI=446 / RV = 0R = 3.2V  AI=974  / 5V AI= 1024   seems to be a voltage divider to -0.1V under VCC
  // 
       
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
return Temperatur;
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
 
  digitalWrite(BUILTIN_LED, LOW); // no problem
  
  Serial.begin(115200);
  Wifi_conn_status=setup_wifi();
  client.setServer(mqtt_server, 1883);

  if (!client.connected()) {   
          MQTT_conn_status=MQTTreconnect();
  }
  if (MQTT_conn_status==1) {
    client.setCallback(callback);
  }
}

int setup_wifi(void) {
  int wifi_succ =1;
  delay(10);
  int Tries=1;

  IPAddress gateway(192,168,178,1); // IP-Adresse des WLAN-Gateways
  IPAddress subnet(255,255,255,0); // Subnetzmaske
  IPAddress ip(192,168,178,26); // fix IP-Adress for the WeMos - if free in your WIFI network

  
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

while ( (WiFi.status() != WL_CONNECTED) && (Tries<20)) { 
    digitalWrite(BUILTIN_LED, LOW); // no problem
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH); // no problem
    Serial.print("c.");
    Tries++;
    delay(100);
  }

  if (WiFi.status()== WL_CONNECTED) {
      Serial.println("");  
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      wifi_succ=1;
     }

  if (WiFi.status()!= WL_CONNECTED) { 
    Serial.println("WiFi connection failed");
    wifi_succ=0;
   }
   return wifi_succ;
}

void callback(char* topic, byte* payload, unsigned int length) {
  String str1, str2;
   
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  
  // handling incoming Light intensity via MQTT NOT NEEDED HERE, just for fun--------------------------------------------------------------------------------------
  str1 = String("Haus/DayNight");
  str2 = String(topic);
  
  if (str1 == str2) {        // Garage light command received
    
   if ((char)payload[0] == '0') {
     DayNight=0;
     Serial.print("MQTT command received: DayNight = night \n");
   } //night
    if ((char)payload[0] == '1') {
     DayNight=1;
     Serial.print("MQTT command received: DayNight = day \n");
   } //day
     
  } // light command received by MQTT
  
} // callback = handling incoming MQTT commands

int MQTTreconnect(void) {
  int MQTT_succ=0;
  int Tries=1;
  // Loop until we're reconnected
  while (!client.connected()&& (Tries<5)) {
    Serial.print("Attempting MQTT connection....");
    digitalWrite(BUILTIN_LED, LOW); // no problem
    delay(100);
    digitalWrite(BUILTIN_LED, HIGH); // no problem
    delay(400);
    // Attempt to connect
    if (client.connect("AllgTempClient1")) {
      Serial.println("connected !");
      delay(500);
      MQTT_succ =1;
      Tries=99;  // exit loop
      //client.subscribe("Haus/DayNight");
    } else {
      Serial.print("MQTT failed, rc=");
      Serial.print(client.state());
      MQTT_succ =0;
      Tries++;
      Serial.println("MQTT conn try again in 5 sekundes");
      delay(500);
      Serial.println(" try again in 1 second");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
return MQTT_succ;
}


void loop() {
  //if (!client.connected()) {  changing connection check to once a minute, not 10 times a second
  //  reconnect();
  //}
  client.loop();
  delay(50);

  long now = millis();
  if (now - lastMsg > 1000) {  // from here on execution once a second
    lastMsg = now;
    ++sekunde;  // action each second
    if (sekunde > 28800) {
      sekunde = 0;  //prevent overflow  max 32K, 28K8 = 8 hours
      lastSec = 0;
      }

 // ********************************    execute each minute *********************************   
    if (sekunde - lastSec > 1) {   // start after 1 second - wait at end
        lastSec = sekunde + 6; 

  // read temperature value each minute   
    sensorValue = analogRead(sensorPin);
    GaragenTemp = AD2temp(sensorValue);
    if (GaragenTemp >99) GaragenTemp= 98.9;
    if (GaragenTemp < -20) GaragenTemp= -19.9;
    
    dtostrf(GaragenTemp,5,1,msg);     // syntax: dtostrf(var,len,prec,str)

 // are we still wifi-connected?
    if (WiFi.status() != WL_CONNECTED) {
         Serial.println(" NO WIFI CONNECTION - reconnect");
         Wifi_conn_status = setup_wifi();
         delay(1000);
     }

     if (!client.connected()) {   // check all minute, if we are connected still
       Serial.println(" NO MQTT CONNECTION - reconnect");
       delay(1000);
       MQTT_conn_status = MQTTreconnect();
     }   

     //if ((WiFi.status()==WL_CONNECTED) && (client.connected())) {
     if ((Wifi_conn_status ==1)  && (MQTT_conn_status ==1)) {
        client.publish("allg/temp",msg);           // Adopt here if you like to have a different MQTT naming
        Serial.print("Temp: >");  Serial.print(msg); Serial.print("<");  
        Serial.print(" published");
        RetryIfNotSent = 0;
    }
    if ((Wifi_conn_status ==0)  ||  (MQTT_conn_status ==0)) {
      RetryIfNotSent ++;
      if (RetryIfNotSent > 10) {
        RetryIfNotSent=0;
      }
    }
    
 // job done - or not done, go to sleep
    if ((SLEEP_TIME>0) && (RetryIfNotSent ==0)) { 
          delay(2000);
          Serial.println(" go to sleep");
          ESP.deepSleep(SLEEP_TIME * 1000000);
          delay(2000);
     }
    } // each minute  
  } //each second
 } // loop
