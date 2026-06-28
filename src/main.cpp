#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h> // Required for parsing metadata payload

// --- Current Firmware Version ---
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "0.0.0-dev" // Fallback for local manual builds
#endif

const String CURRENT_VERSION = String(FIRMWARE_VERSION);

const char *ssid = WIFI_SSID;
const char *password = WIFI_PASS;
const char *mqtt_server = MQTT_SERVER;
const char *mqtt_user = MQTT_USER;
const char *mqtt_pass = MQTT_PASS;
const char *ntfy_user = MQTT_USER;
const char *ntfy_pass = MQTT_PASS;
const char *ota_user = MQTT_USER;
const char *ota_pass = MQTT_PASS;

// --- Network Targets ---
const int mqtt_port = 1883;

const char *ntfy_url = "https://ntfy.saransh.qzz.io/PC-ON";

// --- Pull OTA Manifest Endpoint ---
const char *manifest_url = "https://esp.saransh.qzz.io/firmware.json";

unsigned long lastOTACheck = 0;
const unsigned long OTA_CHECK_INTERVAL = 3600000; // 1 hour

const int TEST_PIN = 4;
const int POWER_PIN = 12;
const int RESET_PIN = 13;

const char *test_topic = "pc/control/test";
const char *power_topic = "pc/control/power";
const char *reset_topic = "pc/control/reset";
const char *online_topic = "pc/control/online";

WiFiClient espClient;
PubSubClient client(espClient);
bool powerOutageFlag = false;

void sendPhoneNotification(String message)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
    http.begin(secureClient, ntfy_url);
    http.setAuthorization(ntfy_user, ntfy_pass);
    http.addHeader("Content-Type", "text/plain");
    http.POST(message);
    http.end();
  }
}

void check_for_updates()
{
  if (WiFi.status() != WL_CONNECTED)
    return;

  Serial.println("Polling firmware manifest...");
  WiFiClientSecure secureClient;
  secureClient.setInsecure();

  HTTPClient http;
  http.begin(secureClient, manifest_url);
  http.setAuthorization(ota_user, ota_pass);

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String payload = http.getString();

    // Fixed: Using the modern ArduinoJson v7 non-deprecated approach
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error)
    {
      String serverVersion = doc["version"].as<String>();
      String downloadUrl = doc["url"].as<String>();

      Serial.printf("Local: %s, Server: %s\n", CURRENT_VERSION.c_str(), serverVersion.c_str());

      if (serverVersion != CURRENT_VERSION)
      {
        Serial.println("New firmware detected! Preparing download...");
        sendPhoneNotification("📥 ESP32: Downloading new update : " + serverVersion + "...");

        httpUpdate.rebootOnUpdate(false);

        // Fixed: Pass the basic auth header values directly into the client handling the download path
        http.begin(secureClient, downloadUrl);
        http.setAuthorization(ota_user, ota_pass);

        // Fixed: Use the clean update variant that directly consumes the pre-configured HTTPClient context
        t_httpUpdate_return ret = httpUpdate.update(http);

        if (ret == HTTP_UPDATE_OK)
        {
          sendPhoneNotification("🚀 Firmware updated to : " + serverVersion + " successfully! Rebooting...");
          delay(2000);
          ESP.restart();
        }
        else
        {
          Serial.printf("Update failed. Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
          sendPhoneNotification("❌ Firmware update failed: " + httpUpdate.getLastErrorString());
        }
      }
      else
      {
        Serial.println("Firmware is up to date.");
      }
    }
  }
  http.end();
}

void setup_wifi()
{
  int attempts = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && attempts < 3)
  {
    delay(5000);
    attempts++;
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    powerOutageFlag = true;
    delay(600000);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
      delay(500);
  }
}

void callback(char *topic, byte *payload, unsigned int length)
{
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.print("Command [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);

  if (message == "PRESS")
  {
    if (String(topic) == test_topic)
    {
      sendPhoneNotification("⚠️ Triggered: Test Action (Flash LED)");
      // Simulates a button press execution on your GPIO 4 pin
      digitalWrite(TEST_PIN, HIGH);
      delay(500);
      digitalWrite(TEST_PIN, LOW);
    }
    else if (String(topic) == power_topic)
    {
      sendPhoneNotification("🚀 Triggered: PC Power Button Pressed");
      digitalWrite(POWER_PIN, HIGH);
      delay(500);
      digitalWrite(POWER_PIN, LOW);
    }
    else if (String(topic) == reset_topic)
    {
      sendPhoneNotification("🔄 Triggered: PC Reset Button Pressed");
      digitalWrite(RESET_PIN, HIGH);
      delay(500);
      digitalWrite(RESET_PIN, LOW);
    }
  }
}

void reconnect()
{
  while (!client.connected())
  {
    String clientId = "ESP32CAM-PC-" + String(random(0, 0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass, online_topic, 1, true, "offline"))
    {

      // Publish exact runtime version string explicitly to MQTT online topic
      String statusPayload = "Firmware " + CURRENT_VERSION + " online";
      client.publish(online_topic, statusPayload.c_str(), true);

      if (powerOutageFlag)
      {
        sendPhoneNotification("🚨 Probable Power Outage Restored!");
        powerOutageFlag = false;
      }
      else
      {
        sendPhoneNotification("🟢 ESP32 connected. Running version: " + CURRENT_VERSION);
      }
      client.subscribe(test_topic);
    }
    else
    {
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(TEST_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);
  pinMode(RESET_PIN, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Poll on immediate startup execution
  check_for_updates();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (millis() - lastOTACheck >= OTA_CHECK_INTERVAL)
  {
    lastOTACheck = millis();
    check_for_updates();
  }
}