/*
 * config.h
 *
 * Edit the values below to match your environment before flashing.
 * Do NOT commit real credentials to version control.
 */

#pragma once

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
#define WIFI_SSID       "your-wifi-ssid"
#define WIFI_PASSWORD   "your-wifi-password"

// Milliseconds to wait for WiFi before giving up and retrying next loop
#define WIFI_TIMEOUT_MS 15000

// ---------------------------------------------------------------------------
// MQTT Broker
// ---------------------------------------------------------------------------
#define MQTT_SERVER     "192.168.1.100"   // IP or hostname of your broker
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "esp8266-dht21"

// Leave MQTT_USER as an empty string for brokers without authentication
#define MQTT_USER       ""
#define MQTT_PASSWORD   ""

// Milliseconds to wait for MQTT broker connection before giving up
#define MQTT_TIMEOUT_MS 10000

// ---------------------------------------------------------------------------
// MQTT Topics
// ---------------------------------------------------------------------------
#define MQTT_TOPIC_TEMPERATURE  "home/sensor/temperature"
#define MQTT_TOPIC_HUMIDITY     "home/sensor/humidity"

// ---------------------------------------------------------------------------
// DHT Sensor
// ---------------------------------------------------------------------------
// GPIO pin connected to the DHT21 DATA line (D4 = GPIO2 on NodeMCU)
#define DHT_PIN 2

// ---------------------------------------------------------------------------
// Publishing interval
// ---------------------------------------------------------------------------
// How often (in milliseconds) to read the sensor and publish to MQTT.
// DHT21 minimum sample rate is 0.5 Hz (one reading every 2 seconds).
#define PUBLISH_INTERVAL_MS 10000   // 10 seconds

// ---------------------------------------------------------------------------
// ASCOM Alpaca ObservingConditions server
// ---------------------------------------------------------------------------
// TCP port on which the Alpaca REST API is served (default: 11111)
#define ALPACA_PORT          11111

// Alpaca device index (almost always 0 for a single-device server)
#define ALPACA_DEVICE_NUMBER 0

// Human-readable name shown to Alpaca clients
#define ALPACA_DEVICE_NAME   "DHT21 Weather Station"

// Unique device GUID – generate once, do not change after first use
// You can create a new one at https://www.uuidgenerator.net/
#define ALPACA_DEVICE_UID    "550e8400-e29b-41d4-a716-446655440000"
