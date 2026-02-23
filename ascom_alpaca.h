/*
 * ascom_alpaca.h
 *
 * ASCOM Alpaca ObservingConditions API (interface version 1) for ESP8266.
 *
 * Exposes the current temperature and humidity readings from the DHT21 sensor
 * as a standard ASCOM Alpaca ObservingConditions device so that astronomy
 * applications (e.g. N.I.N.A., Cartes du Ciel, Stellarium) can retrieve
 * live weather / observing-condition data from this device.
 *
 * Implemented endpoints
 * ─────────────────────
 * Management
 *   GET  /management/apiversions
 *   GET  /management/v1/description
 *   GET  /management/v1/configureddevices
 *
 * ObservingConditions device 0
 *   GET  /api/v1/observingconditions/0/connected
 *   PUT  /api/v1/observingconditions/0/connected
 *   GET  /api/v1/observingconditions/0/description
 *   GET  /api/v1/observingconditions/0/driverinfo
 *   GET  /api/v1/observingconditions/0/driverversion
 *   GET  /api/v1/observingconditions/0/interfaceversion
 *   GET  /api/v1/observingconditions/0/name
 *   GET  /api/v1/observingconditions/0/supportedactions
 *   GET  /api/v1/observingconditions/0/averageperiod
 *   PUT  /api/v1/observingconditions/0/averageperiod
 *   GET  /api/v1/observingconditions/0/cloudcover          (not implemented)
 *   GET  /api/v1/observingconditions/0/dewpoint            (calculated)
 *   GET  /api/v1/observingconditions/0/humidity
 *   GET  /api/v1/observingconditions/0/pressure            (not implemented)
 *   GET  /api/v1/observingconditions/0/rainrate            (not implemented)
 *   GET  /api/v1/observingconditions/0/skybrightness       (not implemented)
 *   GET  /api/v1/observingconditions/0/skyquality          (not implemented)
 *   GET  /api/v1/observingconditions/0/skytemperature      (not implemented)
 *   GET  /api/v1/observingconditions/0/starefwhm           (not implemented)
 *   GET  /api/v1/observingconditions/0/temperature
 *   GET  /api/v1/observingconditions/0/winddirection       (not implemented)
 *   GET  /api/v1/observingconditions/0/windgust            (not implemented)
 *   GET  /api/v1/observingconditions/0/windspeed           (not implemented)
 *   PUT  /api/v1/observingconditions/0/refresh
 *   GET  /api/v1/observingconditions/0/sensordescription
 *   GET  /api/v1/observingconditions/0/timesincelastupdate
 *
 * Alpaca UDP discovery (port 32227) is also supported.
 *
 * Sensor readings are provided by the main sketch via the globals:
 *   float         g_temperature   – last temperature reading in °C
 *   float         g_humidity      – last relative humidity reading in %
 *   unsigned long g_lastReadMs    – millis() timestamp of last successful read
 */

#pragma once

#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <math.h>

// Sensor readings shared with the main sketch (defined in ArduinoMQTTTest.ino)
extern float         g_temperature;
extern float         g_humidity;
extern unsigned long g_lastReadMs;

// ---------------------------------------------------------------------------
// Internal state
// ---------------------------------------------------------------------------
static ESP8266WebServer alpacaServer(ALPACA_PORT);
static WiFiUDP          alpacaUdp;
static uint32_t         g_serverTxID     = 0;
static bool             g_alpacaConnected = true;

// ASCOM Alpaca error codes
static const int ALPACA_OK               = 0;
static const int ALPACA_VALUE_NOT_SET    = 1026;  // 0x402 – value not yet available
static const int ALPACA_PROPERTY_NOT_IMPL = 1024;  // 0x400
static const int ALPACA_NOT_CONNECTED    = 1031;   // 0x407

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Magnus formula dew-point approximation
static float calcDewPoint(float tempC, float rhPct) {
  // Magnus formula constants for the temperature range -40 °C … +60 °C
  const float magnusA = 17.625f;   // dimensionless
  const float magnusB = 243.04f;   // °C
  float alpha = (magnusA * tempC / (magnusB + tempC)) + logf(rhPct / 100.0f);
  return (magnusB * alpha) / (magnusA - alpha);
}

// Extract ClientTransactionID from the current request (query-string or body)
static uint32_t clientTxID() {
  if (alpacaServer.hasArg("ClientTransactionID"))
    return (uint32_t)alpacaServer.arg("ClientTransactionID").toInt();
  return 0;
}

// Send a JSON response that includes a "Value" field.
// buf is sized for the worst-case: max float value (20 chars) +
// fixed JSON keys (~80 chars) + error message (up to ~64 chars).
static void sendValue(const String& valueJson,
                      int errNum = ALPACA_OK,
                      const char* errMsg = "") {
  char buf[512];
  snprintf(buf, sizeof(buf),
    "{\"Value\":%s,\"ClientTransactionID\":%u,\"ServerTransactionID\":%u,"
    "\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\"}",
    valueJson.c_str(), clientTxID(), ++g_serverTxID, errNum, errMsg);
  alpacaServer.send(200, F("application/json"), buf);
}

// Send a JSON response without a "Value" field (used by PUT methods)
static void sendNoValue(int errNum = ALPACA_OK, const char* errMsg = "") {
  char buf[192];
  snprintf(buf, sizeof(buf),
    "{\"ClientTransactionID\":%u,\"ServerTransactionID\":%u,"
    "\"ErrorNumber\":%d,\"ErrorMessage\":\"%s\"}",
    clientTxID(), ++g_serverTxID, errNum, errMsg);
  alpacaServer.send(200, F("application/json"), buf);
}

// Shorthand helpers
static void sendNotImplemented() {
  sendValue("0", ALPACA_PROPERTY_NOT_IMPL, "Property not implemented");
}

static void sendFloat(float v) {
  char vbuf[20];
  dtostrf(v, 1, 4, vbuf);
  sendValue(String(vbuf));
}

static void sendBool(bool v) {
  sendValue(v ? "true" : "false");
}

static void sendString(const char* s) {
  String js = "\"";
  js += s;
  js += "\"";
  sendValue(js);
}

// ---------------------------------------------------------------------------
// Management endpoints
// ---------------------------------------------------------------------------

static void handleGetApiVersions() {
  sendValue("[1]");
}

static void handleGetManagementDescription() {
  sendValue(
    "{\"ServerName\":\"ESP8266 Weather Station\","
    "\"Manufacturer\":\"DIY\","
    "\"ManufacturerVersion\":\"1.0\","
    "\"Location\":\"Local\"}"
  );
}

static void handleGetConfiguredDevices() {
  char val[256];
  snprintf(val, sizeof(val),
    "[{\"DeviceName\":\"%s\","
    "\"DeviceType\":\"ObservingConditions\","
    "\"DeviceNumber\":%d,"
    "\"UniqueID\":\"%s\"}]",
    ALPACA_DEVICE_NAME, ALPACA_DEVICE_NUMBER, ALPACA_DEVICE_UID);
  sendValue(val);
}

// ---------------------------------------------------------------------------
// Common device properties
// ---------------------------------------------------------------------------

static void handleGetConnected()     { sendBool(g_alpacaConnected); }
static void handleGetDescription()   { sendString("DHT21 Temperature and Humidity Sensor"); }
static void handleGetDriverInfo()    { sendString("ASCOM Alpaca ObservingConditions driver for DHT21 on ESP8266, v1.0"); }
static void handleGetDriverVersion() { sendString("1.0"); }
static void handleGetInterfaceVer()  { sendValue("1"); }
static void handleGetName()          { sendString(ALPACA_DEVICE_NAME); }
static void handleGetSupportedActions() { sendValue("[]"); }

static void handlePutConnected() {
  if (alpacaServer.hasArg("Connected")) {
    String val = alpacaServer.arg("Connected");
    val.toLowerCase();
    g_alpacaConnected = (val == "true");
  }
  sendNoValue();
}

// ---------------------------------------------------------------------------
// ObservingConditions properties – sensors available on DHT21
// ---------------------------------------------------------------------------

static void handleGetHumidity() {
  if (!g_alpacaConnected)
    return sendNoValue(ALPACA_NOT_CONNECTED, "Not connected");
  if (isnan(g_humidity))
    return sendValue("0", ALPACA_PROPERTY_NOT_IMPL, "Sensor read error");
  sendFloat(g_humidity);
}

static void handleGetTemperature() {
  if (!g_alpacaConnected)
    return sendNoValue(ALPACA_NOT_CONNECTED, "Not connected");
  if (isnan(g_temperature))
    return sendValue("0", ALPACA_PROPERTY_NOT_IMPL, "Sensor read error");
  sendFloat(g_temperature);
}

static void handleGetDewPoint() {
  if (!g_alpacaConnected)
    return sendNoValue(ALPACA_NOT_CONNECTED, "Not connected");
  if (isnan(g_temperature) || isnan(g_humidity))
    return sendValue("0", ALPACA_PROPERTY_NOT_IMPL, "Sensor read error");
  sendFloat(calcDewPoint(g_temperature, g_humidity));
}

// ---------------------------------------------------------------------------
// ObservingConditions properties – not available on this hardware
// ---------------------------------------------------------------------------

static void handleGetCloudCover()     { sendNotImplemented(); }
static void handleGetPressure()       { sendNotImplemented(); }
static void handleGetRainRate()       { sendNotImplemented(); }
static void handleGetSkyBrightness()  { sendNotImplemented(); }
static void handleGetSkyQuality()     { sendNotImplemented(); }
static void handleGetSkyTemperature() { sendNotImplemented(); }
static void handleGetStarFWHM()       { sendNotImplemented(); }
static void handleGetWindDirection()  { sendNotImplemented(); }
static void handleGetWindGust()       { sendNotImplemented(); }
static void handleGetWindSpeed()      { sendNotImplemented(); }

// ---------------------------------------------------------------------------
// ObservingConditions methods
// ---------------------------------------------------------------------------

static void handleGetAveragePeriod()  { sendFloat(0.0f); }
static void handlePutAveragePeriod()  { sendNoValue(); }  // 0 = no averaging

static void handlePutRefresh() {
  // Sensor is read automatically in the main loop; no manual refresh needed
  sendNoValue();
}

static void handleGetSensorDescription() {
  if (!alpacaServer.hasArg("SensorName"))
    return sendValue("\"\"", ALPACA_PROPERTY_NOT_IMPL, "SensorName parameter missing");

  String sn = alpacaServer.arg("SensorName");
  sn.toLowerCase();
  if (sn == "temperature")
    sendString("DHT21 temperature sensor");
  else if (sn == "humidity")
    sendString("DHT21 relative humidity sensor");
  else if (sn == "dewpoint")
    sendString("Dew point calculated from DHT21 temperature and humidity");
  else
    sendValue("\"\"", ALPACA_PROPERTY_NOT_IMPL, "Sensor not implemented");
}

static void handleGetTimeSinceLastUpdate() {
  if (!alpacaServer.hasArg("SensorName"))
    return sendValue("0", ALPACA_PROPERTY_NOT_IMPL, "SensorName parameter missing");

  String sn = alpacaServer.arg("SensorName");
  sn.toLowerCase();
  if (sn == "temperature" || sn == "humidity" || sn == "dewpoint") {
    if (g_lastReadMs == 0)
      return sendValue("0", ALPACA_VALUE_NOT_SET, "No sensor reading available yet");
    sendFloat((millis() - g_lastReadMs) / 1000.0f);
  } else {
    sendNotImplemented();
  }
}

// ---------------------------------------------------------------------------
// Alpaca UDP discovery  (RFC: Alpaca UDP port 32227)
// ---------------------------------------------------------------------------

static void handleAlpacaDiscovery() {
  int packetSize = alpacaUdp.parsePacket();
  if (packetSize <= 0) return;

  char buf[64];
  int len = alpacaUdp.read(buf, sizeof(buf) - 1);
  if (len <= 0) return;
  buf[len] = '\0';

  if (strncmp(buf, "alpacadiscovery1", 16) == 0) {
    char resp[48];
    snprintf(resp, sizeof(resp), "{\"AlpacaPort\":%d}", ALPACA_PORT);
    alpacaUdp.beginPacket(alpacaUdp.remoteIP(), alpacaUdp.remotePort());
    alpacaUdp.write(resp);
    alpacaUdp.endPacket();
  }
}

// ---------------------------------------------------------------------------
// Public API: call from setup() and loop()
// ---------------------------------------------------------------------------

void initAlpaca() {
  // Management routes
  alpacaServer.on("/management/apiversions",          HTTP_GET, handleGetApiVersions);
  alpacaServer.on("/management/v1/description",       HTTP_GET, handleGetManagementDescription);
  alpacaServer.on("/management/v1/configureddevices", HTTP_GET, handleGetConfiguredDevices);

  // Common device routes
  alpacaServer.on("/api/v1/observingconditions/0/connected",        HTTP_GET,  handleGetConnected);
  alpacaServer.on("/api/v1/observingconditions/0/connected",        HTTP_PUT,  handlePutConnected);
  alpacaServer.on("/api/v1/observingconditions/0/description",      HTTP_GET,  handleGetDescription);
  alpacaServer.on("/api/v1/observingconditions/0/driverinfo",       HTTP_GET,  handleGetDriverInfo);
  alpacaServer.on("/api/v1/observingconditions/0/driverversion",    HTTP_GET,  handleGetDriverVersion);
  alpacaServer.on("/api/v1/observingconditions/0/interfaceversion", HTTP_GET,  handleGetInterfaceVer);
  alpacaServer.on("/api/v1/observingconditions/0/name",             HTTP_GET,  handleGetName);
  alpacaServer.on("/api/v1/observingconditions/0/supportedactions", HTTP_GET,  handleGetSupportedActions);

  // ObservingConditions routes
  alpacaServer.on("/api/v1/observingconditions/0/averageperiod",       HTTP_GET, handleGetAveragePeriod);
  alpacaServer.on("/api/v1/observingconditions/0/averageperiod",       HTTP_PUT, handlePutAveragePeriod);
  alpacaServer.on("/api/v1/observingconditions/0/cloudcover",          HTTP_GET, handleGetCloudCover);
  alpacaServer.on("/api/v1/observingconditions/0/dewpoint",            HTTP_GET, handleGetDewPoint);
  alpacaServer.on("/api/v1/observingconditions/0/humidity",            HTTP_GET, handleGetHumidity);
  alpacaServer.on("/api/v1/observingconditions/0/pressure",            HTTP_GET, handleGetPressure);
  alpacaServer.on("/api/v1/observingconditions/0/rainrate",            HTTP_GET, handleGetRainRate);
  alpacaServer.on("/api/v1/observingconditions/0/skybrightness",       HTTP_GET, handleGetSkyBrightness);
  alpacaServer.on("/api/v1/observingconditions/0/skyquality",          HTTP_GET, handleGetSkyQuality);
  alpacaServer.on("/api/v1/observingconditions/0/skytemperature",      HTTP_GET, handleGetSkyTemperature);
  alpacaServer.on("/api/v1/observingconditions/0/starefwhm",           HTTP_GET, handleGetStarFWHM);
  alpacaServer.on("/api/v1/observingconditions/0/temperature",         HTTP_GET, handleGetTemperature);
  alpacaServer.on("/api/v1/observingconditions/0/winddirection",       HTTP_GET, handleGetWindDirection);
  alpacaServer.on("/api/v1/observingconditions/0/windgust",            HTTP_GET, handleGetWindGust);
  alpacaServer.on("/api/v1/observingconditions/0/windspeed",           HTTP_GET, handleGetWindSpeed);
  alpacaServer.on("/api/v1/observingconditions/0/refresh",             HTTP_PUT, handlePutRefresh);
  alpacaServer.on("/api/v1/observingconditions/0/sensordescription",   HTTP_GET, handleGetSensorDescription);
  alpacaServer.on("/api/v1/observingconditions/0/timesincelastupdate", HTTP_GET, handleGetTimeSinceLastUpdate);

  alpacaServer.begin();
  alpacaUdp.begin(32227);

  Serial.print(F("ASCOM Alpaca server listening on port "));
  Serial.println(ALPACA_PORT);
  Serial.println(F("Alpaca UDP discovery listening on port 32227"));
}

void handleAlpaca() {
  alpacaServer.handleClient();
  handleAlpacaDiscovery();
}
