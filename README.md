
## XIAO ESP32C3 – Low-Power Datacake Sketch

The `xiao_esp32c3_datacake/` folder contains a ready-to-flash Arduino sketch for the **Seeed Studio XIAO ESP32C3** that:

1. Wakes from deep sleep once per day.
2. Reads the water-level percentage from the Grove Water Level Sensor over I²C.
3. Connects to Wi-Fi and POSTs the reading to **[Datacake](https://datacake.co/)**.
4. Goes back to deep sleep for the remainder of the 24-hour cycle.

### Hardware

| Component | Notes |
|-----------|-------|
| Seeed Studio XIAO ESP32C3 | Target MCU |
| Grove Water Level Sensor | Connected via I²C (SDA/SCL) |

### Prerequisites

- Arduino IDE 2.x (or PlatformIO).
- **ESP32 board package** installed:  
  `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`  
  In Arduino IDE: *File → Preferences → Additional boards manager URLs*, then install **esp32 by Espressif Systems** from *Tools → Board Manager*.
- No extra libraries are needed beyond what ships with the ESP32 board package (`WiFi`, `HTTPClient`, `WiFiClientSecure`).

### Datacake Setup

1. Sign up at <https://datacake.co/> and create an **API device**.
2. Inside the device, add a numeric field with identifier **`WATER_LEVEL`** (type: *Float* or *Integer*).
3. Go to *Settings → API* and copy the **HTTP Ingestion URL** – it looks like:  
   `https://api.datacake.co/integrations/api/<device-token>/`

### Configuration

Open `xiao_esp32c3_datacake/xiao_esp32c3_datacake.ino` and edit the five constants at the top:

```cpp
#define WIFI_SSID        "YOUR_WIFI_SSID"
#define WIFI_PASSWORD    "YOUR_WIFI_PASSWORD"
#define DATACAKE_URL     "https://api.datacake.co/integrations/api/YOUR_DEVICE_TOKEN/"
#define SLEEP_DURATION_US  (24ULL * 60ULL * 60ULL * 1000000ULL)  // 24 hours
#define WIFI_TIMEOUT_MS  20000                                    // 20 seconds
```

### Flashing

1. In Arduino IDE select:  
   - Board: **XIAO_ESP32C3** (under *Seeed Studio XIAO ESP32 Series* or *ESP32C3 Dev Module*).
   - Port: your device's COM/ttyUSB port.
2. Click **Upload**.

### Power Behaviour

The sketch calls `esp_deep_sleep_start()` immediately after sending data (or after a failed Wi-Fi attempt). While in deep sleep the XIAO ESP32C3 draws ~43 µA, making it well-suited for battery operation.

---

## Grove Water Level Library
This tiny library is for read the current level from Grove Water Level Sensor.
Please note that the code behind this library is from [Seeed Studio - Grove Water Level Sensor](https://wiki.seeedstudio.com/Grove-Water-Level-Sensor/). I've only put the code into a library to reduce code in my Arduino project, and now I like to share this with you.

<p align="center" width="100%">
    <img width="33%" src="https://github.com/StefanDraeger/Grove-Water-Level-Sensor/blob/main/Grove_Water_Level_Sensor/documentation/Grove_Water_Level_Sensor.png">
</p>

###  Example

Here is an example, with initialize an Object of type WaterLevelSensor and call function with readPercentage.

    #include <waterlevelsensor.h>
    
    WaterLevelSensor sensor = WaterLevelSensor();
    
    void setup() {
      Serial.begin(9600);
    }
    
    void loop() {
      //lesen des Grove Wasser Level Sensors
      int waterLevel = sensor.readPercentage();
    
      //Ausgeben des ermittelten Wertes
      Serial.print(waterLevel);
      Serial.println("%");
      
      delay(25);
    }

The output is like the original one, with only the percentage of fill of cup.

<p align="center" width="100%">
    <img width="33%" src="https://github.com/StefanDraeger/Grove-Water-Level-Sensor/blob/main/Grove_Water_Level_Sensor/documentation/Output_Arduino_IDE_Water_Level_Sensor.png">
</p>

### Example with Grove LED Bar v2
At my blogpost [Arduino UNO R3 & Grove LED Bar v2](https://draeger-it.blog/arduino-uno-r3-grove-led-bar-v2/) you can find a example with the Grove LED Bar v2 device.

