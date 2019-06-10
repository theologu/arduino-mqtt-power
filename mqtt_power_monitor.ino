#include <Ethernet.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "ACS712.h"


long t;
long lastinetupdate = 0;
long inetupdate = 0;
long mqttupdate = 0;
long tellstate = 0;
boolean inetOK;
int RelayPin = 3;    // RELAY connected to digital pin 3
int sensorIn = A5; //Current sensor connected to analog pin 5
int FloodPin = 4;
int cnt = 0;
int isup = 0;
boolean fault = false;
ACS712 sensor(ACS712_20A, A0);

void callback(char* topic, byte* payload, unsigned int length);

//Edit this to match your mqtt setup
#define MQTT_SERVER "gw.example.ro" 
#define MQTT_USER "ssss"
#define MQTT_PASS "1234" 

//Edit this for dht sensor
#define DHTPIN 2
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
unsigned long SISinterval = 7200000;  
unsigned long noLoadTime = 0;
unsigned long loadTime = 0;
unsigned long partTime = 0;
unsigned long onTime = 0;
unsigned long totalTime = 0;
unsigned long OnDelay = 2000;
unsigned long OffDelay = 2000;
unsigned long lastRead = 0;
float upTime = 0;

byte mac[]    = { 0x41, 0x22, 0x00, 0x60, 0x4B, 0x08 };  

// Unique static IP address of this Arduino - change to adapt to your network
IPAddress dnServer(10, 10, 10, 254);
// the router's gateway address:
IPAddress gateway(10, 10, 10, 254);
// the subnet:
IPAddress subnet(255, 255, 255, 0);

//the IP address is dependent on your network
IPAddress ip(10, 10, 10, 206);

//PIN for Local command trigger


char const* beciTopic1 = "/beci/RB1/";
char const* beciTopic2 = "/beci/temp/";
char const* beciTopic3 = "/beci/hum/";
char const* beciTopic4 = "/beci/flood/";
char const* beciTopic5 = "/beci/hidropwr/";
char const* beciTopic6 = "/beci/hidropwrup/";
char const* beciTopic7 = "/beci/hidrofault/";

EthernetClient ethClient;
PubSubClient client(MQTT_SERVER, 1883, callback, ethClient);


void setup() {
delay(500);             //wait for voltage to stabilize
pinMode(7, OUTPUT);   //pin connected to w5100 shield's reset
digitalWrite(7, LOW);  //pull line low for 100ms to reset ethernet shield
delay(100);
digitalWrite(7, HIGH);  //set line high and now ignore pin the rest of the time

  //start the serial line for debugging
Serial.begin(9600);
dht.begin();
//Serial.println("Calibrating... Ensure that no current flows through the sensor at this moment");
pinMode(RelayPin,OUTPUT);
digitalWrite(RelayPin, HIGH);
sensor.calibrate();
//  Serial.println("Done!");

delay (3000);
digitalWrite(RelayPin, LOW);

//start network subsystem
Serial.println("Init Ethernet/DHCP...");
  inetInit();
 
if (inetOK) {
delay (2000);
Serial.print("\tConnecting to MQTT");
client.setCallback(callback);

if (client.connect("beci-ot",MQTT_USER,MQTT_PASS)) {
    
  Serial.print("\tMQTT Connected");

        client.subscribe(beciTopic1);

  }
} 

pinMode(RelayPin, OUTPUT);
pinMode(FloodPin, INPUT);
  //wait a bit before starting the main loop
      delay(200);
}


void loop(){

//
float U = 230;
float I = sensor.getCurrentAC();
float P = U * I;

//  Serial.println(String("I = ") + I + " A");
//Serial.println(String("P = ") + P + " Watts");
//
unsigned long currentMillis = millis();

if(P < 100) {
noLoadTime = millis();
if ((noLoadTime - loadTime) >= OffDelay && cnt==2) { // If power is lower than 100W longer than 2 seconds
//  Serial.println(String("P = ") + P + " Watts");
//  Serial.println("Bucla lowpower\n");
//  Serial.println("total time before:");
//  Serial.println(totalTime);
//  Serial.println("lowpower time\n");
//  Serial.println(noLoadTime);
//  Serial.println("highpower time\n");
//  Serial.println(loadTime);
  partTime = noLoadTime - onTime;
//  Serial.println("parttime:\n");
//  Serial.println(partTime);
  totalTime = totalTime + partTime;
//  Serial.println("total time:");
//  Serial.println(totalTime);
//  Serial.println("\n");
  cnt = 0;
}

//Serial.println("\tmai mic:");

}

if(P > 100) {
if (cnt==0){
onTime = millis();
cnt = 2;
}
loadTime = millis();
//if water pump is continuously on for more than 15 minutes, cut the power 
if ((loadTime - onTime) > 900000) {
fault = true;
digitalWrite(RelayPin, HIGH);
}  

//Serial.println("\tmai mare:"); 
}

 
  
if ((millis() - lastRead) >= SISinterval){  //how much was the powerON in the SISinterval(2h)
if ( cnt == 2 ) {
partTime = loadTime - onTime;
totalTime = totalTime + partTime;
cnt = 0;
}
upTime = totalTime / SISinterval; 
totalTime = 0;
lastRead = millis();
isup = 1;
}

if (inetOK) { 
//check every 10 minutes if we are still connected to the network  
inetupdate = millis();
mqttupdate = millis();
if ( inetupdate - lastinetupdate > 600000 ) {
inetCheck();
}

//maintain MQTT connection
  client.loop();
if (!client.connected()) {
    reconnect();
  }
if ( (mqttupdate - tellstate) > 15000 ) {
    if ( digitalRead(RelayPin) ) {
       client.publish("/beci/RB1_state/","0");
    } else {
      client.publish("/beci/RB1_state/","1");
    }
    if ( digitalRead(FloodPin) == LOW ) {
       client.publish("/beci/flood/","0");
    } 
if ( digitalRead(FloodPin) == HIGH ) {
      client.publish("/beci/flood/","1");
    }
    if ( fault ) {
       client.publish("/beci/hidrofault/","1");
    }
    else {
      client.publish("/beci/hidrofault/","0");
    }
    float h = dht.readHumidity();
    // Read temperature in Celcius
float t = dht.readTemperature();
if ( isnan(t) || isnan(h)) {
Serial.println("KO, Please check DHT sensor !");
}
     Serial.println("Publishing temp and hum and current consumption\n");
    client.publish(beciTopic2, String(t).c_str(), true);   
    client.publish(beciTopic3, String(h).c_str(), true);
    client.publish(beciTopic5, String(P).c_str(), true);
    if (isup==1) {
    client.publish(beciTopic6, String(upTime).c_str(), true);
    isup = 0;
    }
    tellstate = millis();
   
  }


}
if (!inetOK) {
inetInit();
delay(1000);
reconnect();
}

}

void callback(char* topic, byte* payload, unsigned int length) {
  String topicStr = topic; 
  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);

   if (topicStr == "/beci/RB1/") 
    {

     //turn the switch on if the payload is '1' and publish to the MQTT server a confirmation message
     if(payload[0] == '1'){
       digitalWrite(RelayPin, LOW);
       client.publish("/beci/RB1_state/", "1");
      
       }

      //turn the switch off if the payload is '0' and publish to the MQTT server a confirmation message
     else if (payload[0] == '0'){
       digitalWrite(RelayPin, HIGH);
       client.publish("/beci/RB1_state/", "0");
     
       }
     }

    
 
/////////////////////////////////////////////////////////////
}

void inetInit()
{

  if (Ethernet.hardwareStatus() == EthernetW5100) {
    Serial.println("W5100 Ethernet controller detected.\n");
  }
  
  
  Serial.print("\tCalling Inet INIT: \n");
  Ethernet.begin(mac, ip, dnServer, gateway, subnet);
  
  inetOK = true;
  Serial.print("Local IP: ");
  Serial.println(Ethernet.localIP());
  delay(50);


 if (ethClient.connect(gateway, 22)) 
  {
inetOK = true;
Serial.print("inet OK\n");
lastinetupdate = millis();
ethClient.stop();
  }
  else
  {
inetOK = false;
Serial.print("\tinet NOK \n");
  }
}


///// TCP Check
void inetCheck()
{

Serial.print("\tCalling Inet Check: \n");

ethClient.stop();
if (ethClient.connect(gateway, 22)) 
  {
Serial.println("Server is reachable\n");
inetOK = true;
lastinetupdate = millis();
reconnect();
  }
  else
  {
Serial.println("Server is NOT reachable\n");
inetOK = false;
  }
}

///MQTT Reconnect

void reconnect() {
ethClient.stop();
  
    Serial.print("Attempting MQTT reconnection...\n");
    // Attempt to connect
    if (client.connect("beci-ot",MQTT_USER,MQTT_PASS)) {
    
  Serial.print("MQTT ReConnected\n");

        client.subscribe(beciTopic1);
        

  }
  else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
 
  }



