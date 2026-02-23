# ArduinoMQTTTest

An Arduino sketch for the **ESP8266** that reads **temperature** and **humidity**
from a **DHT21 (AM2301)** sensor, publishes the values to an **MQTT broker**
over WiFi, and exposes them as an **ASCOM Alpaca ObservingConditions** device
so that astronomy software can retrieve live weather / observing-condition data.

---

## Features

- Connects to a WiFi network using the ESP8266 built-in WiFi stack
- Reads temperature (°C) and humidity (%) from a DHT21 sensor
- Publishes readings to configurable MQTT topics with the *retain* flag set
- Supports optional MQTT username / password authentication
- Reconnects automatically to WiFi and MQTT if the connection is lost
- Configurable publish interval (default: every 10 seconds)
- **ASCOM Alpaca ObservingConditions REST API** on TCP port 11111
  - Temperature, humidity, and calculated dew point via ASCOM-standard JSON endpoints
  - Alpaca UDP auto-discovery on port 32227

---

## Hardware

| Component | Notes |
|-----------|-------|
| ESP8266 board | NodeMCU v2/v3, Wemos D1 mini, or any ESP8266 variant |
| DHT21 / AM2301 sensor | 3-pin or 4-pin package |

### Wiring

```
DHT21 pin   →   ESP8266 pin
─────────────────────────────
VCC         →   3.3 V (or 5 V – check your module)
GND         →   GND
DATA        →   D4 (GPIO 2)
```

> A 4.7 kΩ–10 kΩ pull-up resistor between DATA and VCC is recommended by the
> DHT21 datasheet, though many breakout boards include one already.

---

## Software dependencies

Install all libraries via the **Arduino IDE Library Manager**
(*Sketch → Include Library → Manage Libraries…*):

| Library | Author | Purpose |
|---------|--------|---------|
| **ESP8266WiFi** | ESP8266 community | Bundled with the ESP8266 Arduino core |
| **PubSubClient** | Nick O'Leary | MQTT client |
| **DHT sensor library** | Adafruit | DHT21 driver |
| **Adafruit Unified Sensor** | Adafruit | Required by DHT library |

> `ESP8266WebServer` and `WiFiUdp` used by the Alpaca server are both bundled
> with the ESP8266 Arduino core – no extra installation is needed.

### ESP8266 Arduino core

If you have not installed the ESP8266 core yet, add the following URL to
*File → Preferences → Additional Boards Manager URLs* and then install
**"esp8266 by ESP8266 Community"** in the Boards Manager:

```
https://arduino.esp8266.com/stable/package_esp8266com_index.json
```

---

## Configuration

Open **`config.h`** and update the values before flashing:

```cpp
// WiFi credentials
#define WIFI_SSID       "your-wifi-ssid"
#define WIFI_PASSWORD   "your-wifi-password"

// MQTT broker
#define MQTT_SERVER     "192.168.1.100"   // IP or hostname
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "esp8266-dht21"
#define MQTT_USER       ""                // leave empty if no auth
#define MQTT_PASSWORD   ""

// MQTT topics
#define MQTT_TOPIC_TEMPERATURE  "home/sensor/temperature"
#define MQTT_TOPIC_HUMIDITY     "home/sensor/humidity"

// Sensor GPIO pin (D4 = GPIO2 on NodeMCU)
#define DHT_PIN 2

// Publish interval in milliseconds
#define PUBLISH_INTERVAL_MS 10000

// ASCOM Alpaca server
#define ALPACA_PORT          11111
#define ALPACA_DEVICE_NUMBER 0
#define ALPACA_DEVICE_NAME   "DHT21 Weather Station"
#define ALPACA_DEVICE_UID    "550e8400-e29b-41d4-a716-446655440000"  // change this!
```

> ⚠️ Do **not** commit real WiFi passwords or MQTT credentials to version control.
>
> Generate a unique `ALPACA_DEVICE_UID` at <https://www.uuidgenerator.net/> and
> keep it stable once deployed so that Alpaca clients can remember the device.

---

## Flashing

1. Open `ArduinoMQTTTest.ino` in the Arduino IDE.
2. Select your board under *Tools → Board → ESP8266 Boards*.
3. Select the correct COM/serial port under *Tools → Port*.
4. Click **Upload**.
5. Open the **Serial Monitor** at **115200 baud** to watch status messages.

---

## MQTT messages

Once running the device publishes two retained messages every
`PUBLISH_INTERVAL_MS` milliseconds:

| Topic | Example payload |
|-------|-----------------|
| `home/sensor/temperature` | `23.40` |
| `home/sensor/humidity`    | `55.20` |

You can subscribe to these topics with any MQTT client, for example:

```bash
mosquitto_sub -h 192.168.1.100 -t "home/sensor/#" -v
```

---

## ASCOM Alpaca API

The device runs a standard **ASCOM Alpaca ObservingConditions** server on
`http://<device-ip>:11111`.

### Management endpoints

| Method | Path | Description |
|--------|------|-------------|
| GET | `/management/apiversions` | Supported Alpaca API versions |
| GET | `/management/v1/description` | Server description |
| GET | `/management/v1/configureddevices` | List of Alpaca devices |

### ObservingConditions endpoints (device 0)

| Method | Path | Notes |
|--------|------|-------|
| GET | `/api/v1/observingconditions/0/connected` | Connection state |
| PUT | `/api/v1/observingconditions/0/connected` | Set connection state |
| GET | `/api/v1/observingconditions/0/temperature` | °C from DHT21 |
| GET | `/api/v1/observingconditions/0/humidity` | %RH from DHT21 |
| GET | `/api/v1/observingconditions/0/dewpoint` | °C, calculated |
| GET | `/api/v1/observingconditions/0/averageperiod` | Always 0 |
| PUT | `/api/v1/observingconditions/0/refresh` | No-op |
| GET | `/api/v1/observingconditions/0/sensordescription` | Sensor description |
| GET | `/api/v1/observingconditions/0/timesincelastupdate` | Seconds since last read |
| GET | `/api/v1/observingconditions/0/cloudcover` | Not implemented (1024) |
| GET | `/api/v1/observingconditions/0/pressure` | Not implemented (1024) |
| GET | `/api/v1/observingconditions/0/rainrate` | Not implemented (1024) |
| GET | `/api/v1/observingconditions/0/winddirection` | Not implemented (1024) |
| GET | `/api/v1/observingconditions/0/windgust` | Not implemented (1024) |
| GET | `/api/v1/observingconditions/0/windspeed` | Not implemented (1024) |

All responses follow the standard Alpaca JSON envelope:

```json
{
  "Value": 23.4,
  "ClientTransactionID": 1,
  "ServerTransactionID": 42,
  "ErrorNumber": 0,
  "ErrorMessage": ""
}
```

### Auto-discovery

The device listens for Alpaca UDP discovery packets on port **32227**.
Alpaca-aware clients (e.g. N.I.N.A.) will find it automatically on the
local network without any manual IP configuration.

### Quick test

```bash
curl http://<device-ip>:11111/api/v1/observingconditions/0/temperature
curl http://<device-ip>:11111/api/v1/observingconditions/0/humidity
curl http://<device-ip>:11111/api/v1/observingconditions/0/dewpoint
```
