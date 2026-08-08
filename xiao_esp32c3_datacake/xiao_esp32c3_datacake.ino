/**
 * xiao_esp32c3_datacake.ino
 *
 * Low-power water-level monitor for Seeed Studio XIAO ESP32C3.
 *
 * Flow (runs once per deep-sleep wake-up):
 *   1. Read water-level percentage from Grove Water Level Sensor (I²C).
 *   2. Connect to Wi-Fi.
 *   3. Send the value to Datacake via HTTP POST.
 *   4. Deep sleep for SLEEP_DURATION_US microseconds (default: 24 h).
 *
 * ── Configuration ────────────────────────────────────────────────────────────
 * Edit the five constants in the "User Configuration" section below, then
 * flash the sketch.  No other changes are required.
 *
 * Datacake HTTP API quick-start:
 *   1. Create a free account at https://datacake.co/ and add an "API device".
 *   2. Copy the device's HTTP endpoint URL (Settings → API) and paste it into
 *      DATACAKE_URL below.  It looks like:
 *        https://api.datacake.co/integrations/api/<your-device-token>/
 *   3. Create a field named "WATER_LEVEL" (type: Float/Integer) on the device.
 *      The JSON key in the payload matches this field identifier.
 * ─────────────────────────────────────────────────────────────────────────────
 */

// ── User Configuration ────────────────────────────────────────────────────────
#define WIFI_SSID        "YOUR_WIFI_SSID"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"

// Full Datacake HTTP ingestion URL for your device, e.g.:
//   "https://api.datacake.co/integrations/api/<device-token>/"
#define DATACAKE_URL     "https://api.datacake.co/integrations/api/YOUR_DEVICE_TOKEN/"

// How long to sleep between readings (microseconds).  Default = 24 hours.
#define SLEEP_DURATION_US  (24ULL * 60ULL * 60ULL * 1000000ULL)

// Maximum time to wait for a Wi-Fi connection (milliseconds).
#define WIFI_TIMEOUT_MS  20000
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ── Grove Water Level Sensor constants (from waterlevelsensor.cpp) ────────────
#define NO_TOUCH            0xFE
#define THRESHOLD           100
#define ATTINY1_HIGH_ADDR   0x78
#define ATTINY2_LOW_ADDR    0x77

/**
 * Read one water-level sample from the Grove Water Level Sensor.
 * Returns a percentage in the range [0, 100] (multiples of 5).
 * Replicates the logic in Grove_Water_Level_Sensor/waterlevelsensor.cpp so
 * the sketch is self-contained and does not require modifying the library.
 */
int readWaterLevelPercent() {
  const int sensorvalue_min = 250;
  const int sensorvalue_max = 255;
  unsigned char low_data[8]  = {0};
  unsigned char high_data[12] = {0};

  // --- low section (ATtiny 2, 8 pads) ---
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  unsigned long t = millis();
  while (Wire.available() < 8) {
    if (millis() - t > 500) break;  // timeout guard
  }
  for (int i = 0; i < 8; i++) { low_data[i] = Wire.read(); }

  // --- high section (ATtiny 1, 12 pads) ---
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  t = millis();
  while (Wire.available() < 12) {
    if (millis() - t > 500) break;  // timeout guard
  }
  for (int i = 0; i < 12; i++) { high_data[i] = Wire.read(); }

  delay(10);

  // Count touched sections
  uint32_t touch_val    = 0;
  uint8_t  trig_section = 0;

  for (int i = 0; i < 8;  i++) { if (low_data[i]  > THRESHOLD) touch_val |= (uint32_t)1 << i; }
  for (int i = 0; i < 12; i++) { if (high_data[i]  > THRESHOLD) touch_val |= (uint32_t)1 << (8 + i); }

  while (touch_val & 0x01) { trig_section++; touch_val >>= 1; }

  return trig_section * 5;
}

/**
 * Block until Wi-Fi is connected or WIFI_TIMEOUT_MS elapses.
 * Returns true on success, false on timeout.
 */
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_TIMEOUT_MS) {
      Serial.println("\nWi-Fi timeout.");
      return false;
    }
    delay(250);
    Serial.print('.');
  }
  Serial.println("\nConnected. IP: " + WiFi.localIP().toString());
  return true;
}

/**
 * POST the water-level percentage to Datacake.
 * Returns true if the server responded with HTTP 2xx.
 */
bool sendToDatacake(int waterLevelPct) {
  WiFiClientSecure client;
  client.setInsecure();  // skip certificate validation; replace with a CA cert for production

  HTTPClient http;
  if (!http.begin(client, DATACAKE_URL)) {
    Serial.println("HTTPClient begin failed.");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  // Datacake HTTP API expects:  {"FIELD_ID": value}
  String payload = "{\"WATER_LEVEL\":" + String(waterLevelPct) + "}";
  Serial.println("Payload: " + payload);

  int httpCode = http.POST(payload);
  http.end();

  if (httpCode >= 200 && httpCode < 300) {
    Serial.printf("Datacake response: %d\n", httpCode);
    return true;
  }

  Serial.printf("Datacake POST failed, HTTP code: %d\n", httpCode);
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(100);  // let the serial port settle

  // ── 1. Measure water level ─────────────────────────────────────────────────
  Wire.begin();
  int waterLevel = readWaterLevelPercent();
  Serial.printf("Water level: %d%%\n", waterLevel);

  // ── 2. Connect to Wi-Fi ────────────────────────────────────────────────────
  bool wifiOk = connectWiFi();

  // ── 3. Send to Datacake ────────────────────────────────────────────────────
  if (wifiOk) {
    bool sent = sendToDatacake(waterLevel);
    if (!sent) {
      Serial.println("Failed to send data; going to sleep anyway.");
    }
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  } else {
    Serial.println("Skipping Datacake upload (no Wi-Fi).");
  }

  // ── 4. Deep sleep ──────────────────────────────────────────────────────────
  Serial.printf("Sleeping for %.1f hours...\n", SLEEP_DURATION_US / 3600000000.0);
  Serial.flush();
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION_US);
  esp_deep_sleep_start();
}

void loop() {
  // Never reached – the device always wakes into setup() after deep sleep.
}
