#define ETHERNET //commenting this line will switch to Wifi

//Imports for the LoRa Wan Adapter
#include <SPI.h>
#include <LoRa.h>

//Importing networking libraries
#ifdef  ETHERNET
#include <ETH.h>
#else
#include <WiFi.h>
// WIFI CONNECTION FOR HTTP SERVER AND CREDENTIALS
char* ssid = "scwl";
char* password = "*seitcomAccess2021!";
#endif

//Imports mqtt Client and the http async webserver
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <AsyncTCP.h>

//Imports for permanently saving data to device
#include <EEPROM.h>

//EEPROM CONFIGURATION
// define the number of bytes you want to access
#define EEPROM_SIZE 1

// LORA WAN MODULE
#define ss 5
#define rst 1
#define dio0 3

// Credentials for MQTT Server
char* mqtt_server = "192.168.100.114";
char* mqttUser = "admin";
char* mqttPassword = "123";

// VARIABLES OF THE HTTP WEB SERVER
AsyncWebServer server(80);

// VARIABLES OF THE HTTP WEB wifiClient
WiFiClient wifiClient;

// VARIABLES OF THE MQTT wifiClient
PubSubClient mqttClient(wifiClient);

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect("ESP32wifiClient", mqttUser, mqttPassword)) {
      Serial.println("connected");
      // Subscribe
      mqttClient.subscribe("esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

//Dummy variable for webserver
bool ledState = 0;

#ifdef ETHERNET
static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event)
//Printing out status of lan connection
{
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}
#endif

String getprocessor(const String& var){
  //Serial.println(var);
  if(var == "SPECIALPHRASE"){
    return temperature;
  }
  else if(var == "SSID"){
    return humidity;
  }
  else if(var == "WIFIPWD"){
    return EEPROM.read(3);
  }
  else if(var == "MQTTIP"){
    return timestamp;
  }
  else if (var == "MQTTLOGIN"){
    return String(rssi);
  }
  return String();
}

String setprocessor(const String& var){
  //Serial.println(var);
  if(var == "SPECIALPHRASE"){
    return temperature;
  }
  else if(var == "SSID"){
    return humidity;
  }
  else if(var == "WIFIPWD"){
    return EEPROM.read(3);
  }
  else if(var == "MQTTIP"){
    return timestamp;
  }
  else if (var == "MQTTLOGIN"){
    return String(rssi);
  }
  return String();
}

void setup() {
  //Initialize Serial connection
  Serial.begin(115200);
  while (!Serial);
  //Initialize LoRa WAN Receiver
  Serial.println("LoRa Receiver");
  LoRa.setPins(ss, rst, dio0);

  if (!LoRa.begin(866E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  #ifdef ETHERNET
  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  #else
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  #endif

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  mqttClient.setServer(mqtt_server, 1883);
  //Start HTTP server
  server.begin();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();
  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");
    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }
    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
    mqttClient.publish("Abstand", "Was is man");
  }
  mqttClient.publish("Andere", "Funktioniert es auch mit http server?");
}
