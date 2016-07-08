#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Bounce2.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Wifi + Server Configuration
const char* ssid = "*****";
const char* password = "*****";
const char* mqtt_server = "*****";

//OTA Config
const char* deviceName = "ESP8266-Mirror-Controls";
const char* OTAPass = "update";

//Variables and pinouts
WiFiClient espClient;
PubSubClient client(espClient);
const int button1 = D6;
const int button2 = D7;
const int led1 = D1;
const int led2 = D2;
bool buttonState1 = LOW;
bool buttonState2 = LOW;

// Instantiate a Bounce object
Bounce debouncer1 = Bounce(); 
Bounce debouncer2 = Bounce(); 

long lastMsg = 0;
char msg[50];
int value = 0;

const char* outTopic1 = "mirror/buttons/one/out";
const char* inTopic1 = "mirror/buttons/one/in";

const char* outTopic2 = "mirror/buttons/two/out";
const char* inTopic2 = "mirror/buttons/two/in";

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(2000);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  if(String((char*)topic) == String((char*)inTopic1)) {
    if ((char)payload[0] == '0') {
      digitalWrite(led1, LOW);
      Serial.println("led1 -> LOW");
      buttonState1 = LOW;
      EEPROM.write(0, buttonState1);    // Write state to EEPROM
      EEPROM.commit();
      client.publish(outTopic1, "0");
    } else if ((char)payload[0] == '1') {
      digitalWrite(led1, HIGH);
      Serial.println("led1 -> HIGH");
      buttonState1 = HIGH;
      EEPROM.write(0, buttonState1);    // Write state to EEPROM
      EEPROM.commit();
      client.publish(outTopic1, "1");
      client.publish(outTopic2, "0");
    }
  } else if (String((char*)topic) == String((char*)inTopic2)) {
     if ((char)payload[0] == '0') {
      digitalWrite(led2, LOW);
      Serial.println("led2 -> LOW");
      buttonState2 = LOW;
      EEPROM.write(1, buttonState2);    // Write state to EEPROM
      EEPROM.commit();
      client.publish(outTopic2, "0");
    } else if ((char)payload[0] == '1') {
      digitalWrite(led2, HIGH);
      Serial.println("led2 -> HIGH");
      buttonState2 = HIGH;
      EEPROM.write(1, buttonState2);    // Write state to EEPROM
      EEPROM.commit();
      client.publish(outTopic2, "1");
      client.publish(outTopic1, "0");
    }
  }
}

//If connection lost
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Mirror Controls")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic1, "0");
      client.publish(outTopic2, "0");
      // ... and resubscribe
      client.subscribe(inTopic1);
      client.subscribe(inTopic2);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void physButton1() {
  debouncer1.update();
   
   // Call code if Bounce fell (transition from HIGH to LOW) :
   if ( debouncer1.fell() ) {
     Serial.println("Debouncer fell");
     // Toggle relay state :
     buttonState1 = !buttonState1;
     digitalWrite(led1,buttonState1);
     EEPROM.write(0, buttonState1);    // Write state to EEPROM
     EEPROM.write(1, !buttonState1);    // Write state to EEPROM
     if (buttonState1 == 1){
      client.publish(outTopic1, "1");
      client.publish(outTopic2, "0");
      digitalWrite(led1,buttonState1);
      digitalWrite(led2,!buttonState1);
     }
     else if (buttonState1 == 0){
      client.publish(outTopic1, "0");
      digitalWrite(led1,buttonState1);
     }
   }
}

void physButton2() {
  debouncer2.update();
   
   // Call code if Bounce fell (transition from HIGH to LOW) :
   if ( debouncer2.fell() ) {
     Serial.println("Debouncer fell");
     // Toggle relay state :
     buttonState2 = !buttonState2;
     EEPROM.write(1, buttonState2);    // Write state to EEPROM
     EEPROM.write(0, !buttonState2);    // Write state to EEPROM
     if (buttonState2 == 1){
      client.publish(outTopic2, "1");
      client.publish(outTopic1, "0");
      digitalWrite(led2,buttonState2);
      digitalWrite(led1,!buttonState2);
     }
     else if (buttonState2 == 0){
      client.publish(outTopic2, "0");
      digitalWrite(led2,buttonState2);
     }
   }
}

void setup() {
  EEPROM.begin(512);              // Begin eeprom to store on/off state
  Serial.begin(115200);

  //OTA setup
  WiFi.mode(WIFI_STA);
  ArduinoOTA.setPassword(OTAPass);
  ArduinoOTA.setHostname(deviceName);

  //Define modes for pins
  pinMode(button1, INPUT);
  pinMode(button2, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  
  buttonState1 = EEPROM.read(0);
  buttonState2 = EEPROM.read(1);
  digitalWrite(led1,buttonState1);
  digitalWrite(led2,buttonState2);
  
  debouncer1.attach(button1);   // Use the bounce2 library to debounce the built in button
  debouncer2.attach(button2);   // Use the bounce2 library to debounce the built in button
  debouncer1.interval(50);         // Input must be low for 50 ms
  debouncer2.interval(50);         // Input must be low for 50 ms
  
  digitalWrite(0, LOW);          // Blink to indicate setup
  delay(500);
  digitalWrite(0, HIGH);
  delay(500);
  
  Serial.begin(115200);
  setup_wifi();                   // Connect to wifi 
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

//Loop this without delays. pressing a button lasts 500ms
void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  ArduinoOTA.handle();
  
  physButton1();
  physButton2();
}
