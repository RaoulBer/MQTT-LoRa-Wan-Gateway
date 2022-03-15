#define ETHERNET //commenting this line will switch to Wifi

//Imports for the LoRa Wan Adapter
#include <SPI.h>
#include <LoRa.h>

//Importing networking libraries

#include <ETH.h>
#include <WiFi.h>

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

// Initialize SPIFFS
void initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("An error has occurred while mounting SPIFFS");
  }
  Serial.println("SPIFFS mounted successfully");
}

// Read File from SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n" , path);

  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  file.close();
  return fileContent;
}

// Write file to SPIFFS
void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

// File paths to save input values permanently
const char* specialphrasePath = "/specialphrase.txt";

const char* enablewifiPath = "/enablewifi.txt";
const char* wifissidPath = "/wifissid.txt";
const char* wifipwdPath ="/wifipwd.txt";

const char* mqttloginPath = "/mqttlogin.txt";
const char* mqttipPath = "/mqttip.txt";
const char* mqttpasswordPath = "/mqttpwdpath.txt";

//Variables to save values from HTML form
String specialphrase;

String enableWifi;
String wifissid;
String wifipwd ;

String mqttip;
String mqttlogin;
String mqttpwd;

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

String processor(const String& var){
  //Serial.println(var);
  if(var == "specialphrase"){
    return readFile(SPIFFS, specialphrasePath);
  }
  else if(var == "enablewifi"){
    return readFile(SPIFFS, enablewifiPath);
  }
  else if(var == "wifissid"){
    return readFile(SPIFFS, wifissidPath);
  }
  else if(var == "mqttipaddress"){
    return readFile(SPIFFS, mqttipPath);
  }
  else if (var == "mqttlogin"){
    return readFile(SPIFFS, mqttloginPath);
  }
  else if (var == "mqttpassword"){
    return readFile(SPIFFS, mqttpasswordPath);
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

  /*if (!LoRa.begin(866E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  */

  //Initialize flash filesystem emulation
  initSPIFFS();
  delay(1);

  //Variables to save values from HTML form
  specialphrase = readFile(SPIFFS, specialphrasePath);

  enableWifi = readFile(SPIFFS, enablewifiPath);
  wifissid = readFile(SPIFFS, wifissidPath);
  wifipwd = readFile(SPIFFS, wifipwdPath);

  mqttip = readFile(SPIFFS, mqttipPath);
  mqttlogin = readFile(SPIFFS, mqttloginPath);
  mqttpwd = readFile(SPIFFS, mqttpasswordPath);

  WiFi.onEvent(WiFiEvent);
  ETH.begin();

  
  // Connect to Wi-Fi
  if (strcmp(enableWifi.c_str(), String("on").c_str()) == 0){
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifissid.c_str(), wifipwd.c_str());
    for(int i = 0; i < 30; i++ ) {
      delay(1000);
      Serial.println("Connecting to WiFi..");
    }
  }
  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());
  Serial.println(ETH.localIP());
  mqttClient.setServer(mqtt_server, 1883);
  server.begin();
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Route to load style.css file
  server.on("/index.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.css", "text/css");
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      // HTTP POST Special Phase
      if (p->name() == "specialphrase") {
        specialphrase = p->value().c_str();
        Serial.print("Lora Wan special phrase: ");
        Serial.println(specialphrase);
        // Write file to save value
        if(specialphrase.c_str() != ""){
          writeFile(SPIFFS, specialphrasePath, specialphrase.c_str());  
        }
      }
      // HTTP POST pass value
      if (p->name() == "enablewifi") {
        enableWifi = p->value().c_str();
        Serial.print("Wifi status: ");
        Serial.println(enableWifi);
        // Write file to save value
        writeFile(SPIFFS, enablewifiPath, enableWifi.c_str());
      }
      // HTTP POST ip value
      if (p->name() == "wifissid") {
        wifissid = p->value().c_str();
        Serial.print("Wifi SSID set to: ");
        Serial.println(wifissid);
        // Write file to save value
        if(wifissid.c_str() != ""){
          writeFile(SPIFFS, wifissidPath, wifissid.c_str());
        }
      }
     // HTTP POST ip value
      if (p->name() == "wifipassword") {
        wifipwd = p->value().c_str();
        Serial.print("Wifi password set to: ");
        Serial.println(wifipwd);
        // Write file to save value
        if(wifipwd.c_str() != ""){
          writeFile(SPIFFS, wifipwdPath, wifipwd.c_str());
        }
      }
      // HTTP POST ip value
      if (p->name() == "mqttipaddress") {
        mqttip = p->value().c_str();
        Serial.print("MQTT IP Address set to: ");
        Serial.println(mqttip);
        // Write file to save value
        if(mqttip.c_str() != ""){
          writeFile(SPIFFS, mqttipPath, mqttip.c_str());
        }
      }
      // HTTP POST ip value
      if (p->name() == "mqttlogin") {
        mqttlogin = p->value().c_str();
        Serial.print("MQTT Login set to: ");
        Serial.println(mqttlogin);
        // Write file to save value
        if(mqttlogin.c_str() != ""){
          writeFile(SPIFFS, mqttloginPath, mqttlogin.c_str());
        }
      }
      // HTTP POST ip value
      if (p->name() == "mqttpassword") {
        mqttpwd= p->value().c_str();
        Serial.print("MQTT Password set to: ");
        Serial.println(mqttpwd);
        // Write file to save value
        if(mqttpwd.c_str() != ""){
          writeFile(SPIFFS, mqttpasswordPath, mqttpwd.c_str());
        }
      }
      if (p->name() == "reboot") {
        Serial.println("Rebooting");
        // Write file to save value
        ESP.restart();
      }
      request->send(SPIFFS, "/index.html", String(), false, processor);
    }
  }
  });
  delay(1);
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
