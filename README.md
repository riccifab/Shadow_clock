# Shadow Clock

Shadow Clock is a custom LED strip clock project built around an ESP8266 (Adafruit HUZZAH ESP8266) that displays the current time on a 60-LED circular strip. The project synchronizes time via Wi-Fi using NTP and uses a DS3231 RTC module as a backup when offline. Two Sharp distance sensors (via an ADS1115 ADC) detect proximity to activate the clock display. When no one cast a shadow, the LED strip gradually fades to a full white brightness.

## Issues
**Distance Sensors:** Two Sharp GP2Y0A21YK0F infrared distance sensors. These sensors perform poorly, should switch to the ultrasound proximity
## Renderings

At the beginning of this project, a rendering was created to visualize the final design. See the image below for a preview of the clock design:
![Clock v8](https://github.com/user-attachments/assets/3bf70655-fe4f-40c6-b2c7-97066f50ab22)

![bE-uhr-shadowplay_web](https://github.com/user-attachments/assets/4c8dc8cc-a111-4e5a-aa25-86409501e5c7)


## Features

- **Time Synchronization:**  
  Uses Wi-Fi to update time via an NTP server and writes it to a DS3231 RTC module for backup when offline.

- **LED Clock Display:**  
  Displays the current hour and minute on a 60-LED strip. The minute is indicated in blue and the hour in red (or magenta if both overlap). When no one is near, the LEDs gradually fade into full white.

- **Proximity Activation:**  
  Two Sharp distance sensors (via ADS1115 ADC channels) detect when a user is interact with the clock to activate the clock display.

- **Modular Software Architecture:**  
  Organized code into separate handler files for display, sensor calibration, and time synchronization.

## Hardware Components

- **ESP8266:** Adafruit HUZZAH ESP8266 for processing and Wi-Fi connectivity.
- **LED Strip:** 60 WS2812B addressable LEDs (1Meter).
- **RTC Module:** DS3231 for accurate timekeeping.
- **ADC:** ADS1115 for high-resolution sensor readings.
- **Distance Sensors:** Two Sharp GP2Y0A21YK0F infrared distance sensors.
- **Miscellaneous:** Appropriate power supply, wiring, and resistors (I2C pull-ups, etc.).

## Wiring & Setup

- **I2C Bus:**  
  The DS3231 and ADS1115 are connected to the ESP8266 via I2C. Ensure proper pull-up resistors (typically 4.7kΩ) are installed on the SDA and SCL lines.
  
- **LED Strip:**  
  Connect the data line of the LED strip to GPIO2 (D4 on some boards) of the ESP8266. Ensure your power supply can handle the current drawn by the LED strip.
  
- **Distance Sensors:**  
  The sensors are connected to separate channels on the ADS1115. Calibrate the sensor outputs to map ADC values to distance in centimeters.

## Software Structure

The project uses PlatformIO and is organized into the following files and directories:

```
Shadow_clock/code/
├── include/
│   ├── DisplayHandler.h       # LED display functions (clock and fade)
│   ├── SensorHandler.h        # Sensor calibration functions
│   └── TimeSyncHandler.h      # Wi-Fi/NTP time synchronization functions
├── src/
│   ├── main.cpp               # Main program loop
│   ├── DisplayHandler.cpp     # Implementation of display functions
│   ├── SensorHandler.cpp      # Implementation of sensor calibration
│   └── TimeSyncHandler.cpp    # Implementation of time synchronization
├── images/
│   ├── rendering1.png         # Rendering image 1
└── platformio.ini             # PlatformIO project configuration
```

## Installation

1. **Clone the Repository:**

   ```bash
   git clone https://github.com/riccifab/Shadow_clock.git
   cd Shadow_clock
   ```

2. **Open with PlatformIO:**  
   Open the project folder in VS Code with the PlatformIO extension installed.

3. **Configure Wi-Fi Credentials:**  
   Update `ssid` and `password` in `main.cpp`

4. **Build and Upload:**  
   Use PlatformIO commands to build and upload the firmware to your ESP8266:

   ```bash
   pio run --target upload
   ```

5. **Monitor Serial Output:**  
   Open the serial monitor to check debug output:

   ```bash
   pio device monitor
   ```

## Customization

- **Time Display:**  
  Modify `DisplayHandler.cpp` to change how the hour and minute indicators are rendered on the LED strip.
  
- **Sensor Calibration:**  
  Adjust the calibration curve in `SensorHandler.cpp` according to your measured sensor values.
  
- **Fade-In Effect:**  
  Change the fade-in duration in `main.cpp` by modifying the mapping parameters if needed.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Acknowledgements

- [FastLED](http://fastled.io/) for the LED library.
- [RTClib](https://github.com/adafruit/RTClib) for RTC functionality.
- [NTPClient](https://github.com/arduino-libraries/NTPClient) for time synchronization.
- [Shadowplay](http://www.breadedescalope.com/index.php/shadowplay-uhr-fuer-einen-salon) design inspiration.
