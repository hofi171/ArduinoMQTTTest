/*
 * Arduino ESP8266 - DHT21 Sensor to MQTT Broker
 *
 * Reads temperature and humidity from a DHT21 (AM2301) sensor
 * and publishes the values to an MQTT broker over WiFi.
 *
 * Required Libraries:
 *   - ESP8266WiFi       (bundled with ESP8266 Arduino core)
 *   - PubSubClient      (by Nick O'Leary, install via Library Manager)
 *   - DHT sensor library (by Adafruit, install via Library Manager)
 *   - Adafruit Unified Sensor (dependency of DHT library)
 *
 * Wiring:
 *   DHT21 DATA pin -> D4 (GPIO 2) on NodeMCU / ESP8266
 *   DHT21 VCC      -> 3.3V or 5V
 *   DHT21 GND      -> GND
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ---------------------------------------------------------------------------
// Configuration  (edit config.h to change these values)
// ---------------------------------------------------------------------------
#include "config.h"

// ---------------------------------------------------------------------------
// DHT sensor setup
// ---------------------------------------------------------------------------
#define DHTTYPE DHT21

DHT dht(DHT_PIN, DHTTYPE);

// ---------------------------------------------------------------------------
// MQTT / WiFi clients
// ---------------------------------------------------------------------------
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// ---------------------------------------------------------------------------
// Helper: connect to WiFi
// ---------------------------------------------------------------------------
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }

  Serial.print("Connecting to WiFi SSID: ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttempt > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi connection timed out. Retrying later.");
      return;
    }
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

// ---------------------------------------------------------------------------
// Helper: connect to MQTT broker
// ---------------------------------------------------------------------------
void connectMQTT() {
  if (mqttClient.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT broker: ");
  Serial.print(MQTT_SERVER);
  Serial.print(":");
  Serial.println(MQTT_PORT);

  unsigned long startAttempt = millis();
  while (!mqttClient.connected()) {
    if (millis() - startAttempt > MQTT_TIMEOUT_MS) {
      Serial.println("MQTT connection timed out. Retrying later.");
      return;
    }

    bool connected;
    if (strlen(MQTT_USER) > 0) {
      connected = mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
    } else {
      connected = mqttClient.connect(MQTT_CLIENT_ID);
    }

    if (connected) {
      Serial.println("MQTT connected.");
    } else {
      Serial.print("MQTT connect failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" – retrying in 2 s");
      delay(2000);
    }
  }
}

// ---------------------------------------------------------------------------
// setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\nESP8266 DHT21 -> MQTT");

  dht.begin();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);

  connectWiFi();
  connectMQTT();
}

// ---------------------------------------------------------------------------
// loop
// ---------------------------------------------------------------------------
unsigned long lastPublishMs = 0;

void loop() {
  // Maintain connections continuously (non-blocking)
  connectWiFi();
  connectMQTT();
  mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublishMs < PUBLISH_INTERVAL_MS) {
    return;
  }
  lastPublishMs = now;

  // Read sensor
  float humidity    = dht.readHumidity();
  float temperature = dht.readTemperature();  // Celsius

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor! Retrying shortly.");
    // Reset lastPublishMs to a shorter retry window (2 s)
    lastPublishMs = now - PUBLISH_INTERVAL_MS + 2000UL;
    return;
  }

  // Build payload strings
  char humBuf[10];
  char tempBuf[10];
  dtostrf(humidity,    1, 2, humBuf);
  dtostrf(temperature, 1, 2, tempBuf);

  // Publish
  bool humOk  = mqttClient.publish(MQTT_TOPIC_HUMIDITY,    humBuf,  true);
  bool tempOk = mqttClient.publish(MQTT_TOPIC_TEMPERATURE, tempBuf, true);

  Serial.print("Temperature: ");
  Serial.print(tempBuf);
  Serial.print(" °C  |  Humidity: ");
  Serial.print(humBuf);
  Serial.println(" %");

  if (!humOk || !tempOk) {
    Serial.println("MQTT publish failed.");
  }
}
