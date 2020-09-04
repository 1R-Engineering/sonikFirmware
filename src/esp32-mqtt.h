#ifndef __ESP32_MQTT_H__
#define __ESP32_MQTT_H__

#include <Client.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include <MQTT.h>

#include <CloudIoTCore.h>
#include <CloudIoTCoreMqtt.h>
#include "ciotc_config.h" // Update this file with your configuration

// #define LEDWIFICON  19
// #define LEDIOTCON   23
// #define LEDMQTT     18

// !!REPLACEME!!
// The MQTT callback function for commands and configuration updates
// Place your message handler code here.

///////////////////////////////
// Initialize WiFi and MQTT for this board
Client *netClient;
CloudIoTCoreDevice *device;
CloudIoTCoreMqtt *mqtt;
MQTTClient *mqttClient;
unsigned long iat = 0;
String jwt;

///////////////////////////////
// Helpers specific to this board
///////////////////////////////
String getDefaultSensor() {
  return  "Wifi: " + String(WiFi.RSSI()) + "db";
}

String getJwt() {
  iat = time(nullptr);
  Serial.println("Refreshing JWT");
  jwt = device->createJWT(iat, jwt_exp_secs);
  return jwt;
}

String terimaBL(){
  String rawJsonBT;
  rawJsonBT = program();
  return rawJsonBT;
}

void setupWifi() { 
  DynamicJsonDocument configurasiWifi(116);
  String wifiConfig;
  wifiConfig = terimaBL();

  deserializeJson(configurasiWifi, wifiConfig);

  char *ssid = configurasiWifi["ssid"];
  char *pass = configurasiWifi["pass"];

  Serial.println("Starting wifi");


  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LEDWIFICON, LOW);
    delay(50);
    digitalWrite(LEDWIFICON, HIGH);
    delay(50);
  }

  configTime(25200, 0, ntp_primary, ntp_secondary);

  Serial.println("Waiting on time sync...");
  while (time(nullptr) < 1510644967) {
    delay(10);
  }
}

void connectWifi() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
}

///////////////////////////////
// Orchestrates various methods from preceeding code.
///////////////////////////////
bool publishTelemetry(String data) {
  return mqtt->publishTelemetry(data);
}

bool publishTelemetry(const char* data, int length) {
  return mqtt->publishTelemetry(data, length);
}

bool publishTelemetry(String subfolder, String data) {
  return mqtt->publishTelemetry(subfolder, data);
}

bool publishTelemetry(String subfolder, const char* data, int length) {
  return mqtt->publishTelemetry(subfolder, data, length);
}

void connect() {
  connectWifi();
  mqtt->mqttConnect();
  digitalWrite(13, HIGH);
}

void setupCloudIoT() {
  device = new CloudIoTCoreDevice(
      project_id, location, registry_id, device_id,
      private_key_str);

  setupWifi();
  netClient = new WiFiClientSecure();
  mqttClient = new MQTTClient(512);
  mqttClient->setOptions(180, true, 1000); // keepAlive, cleanSession, timeout
  mqtt = new CloudIoTCoreMqtt(mqttClient, netClient, device);
  mqtt->setUseLts(true);
  mqtt->startMQTT();
}

#endif //__ESP32_MQTT_H__