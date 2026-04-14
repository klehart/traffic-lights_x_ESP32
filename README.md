# 🚦 Jura Traffic Light Control (ESP32)

I built this project for our local youth club ("Jugendraum", or "Jura" for short). The entire hardware setup is mounted inside a real, physical traffic light enclosure! It uses an ESP32 to provide a web interface for manual control and a sound-reactive mode using a microphone.

![Project Image](aaa.png)

## ⚠️ Disclaimer
**I am fully aware that this code is messy.** It was built to get the job done and is a collection of various experimental features. It's not a masterpiece of software architecture, but it works and makes the lights look cool in our club!

## 🛠 Features
- **Dual Mode (WiFi vs. Sound):** Toggle between **WiFi/Server Mode** and **Sound-Reactive Mode** via a physical toggle switch. *Why the switch?* I suspect the ESP32 gets a bit overloaded trying to run the web server and process audio data simultaneously, so separating the modes keeps things running smoothly.
- **Web Interface:** Control brightness, colors, and animations from any browser.
- **Sound-Reactive:** Uses a microphone (MAX4466) to pulse the LEDs to the beat/bass.
- **Auto-Cycle:** Automatically cycles through different animations every 40 seconds unless a manual mode is selected.
- **Animations:** Includes LSD/Rainbow, Running Light, Sparkle, Strobe, Roulette, and a classic Traffic Light simulation.

## 🔌 Hardware Setup
- **Microcontroller:** ESP32
- **LEDs:** 120 x WS2812B (divided into 3 groups of 40) on `GPIO 27`
- **Microphone:** Analog Input on `GPIO 34`
- **Toggle Switch:** Connected to `GPIO 32` (using internal pull-up)

## 🚀 Quick Start
1. Open `main.cpp` and update the `ssid` and `password` variables with your WiFi credentials.
2. Install the `Adafruit_NeoPixel` library in your Arduino IDE or PlatformIO.
3. Flash the code to your ESP32.
4. **Operation:**
   - **Switch UP:** Connects to WiFi. Access the control panel via the ESP32's IP address.
   - **Switch DOWN:** Offline mode. The web server shuts down and the light reacts to sound/music.

## 📜 License
This project is provided "as is" under the **MIT License**. This means you can do whatever you want with the code (even if it's messy), as long as you include the original copyright and license notice. See the [LICENSE](LICENSE) file for details.
