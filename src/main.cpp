#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <Esp.h>

#define LED_PIN 27
#define MIC_PIN 34
#define NUM_LEDS 120
#define SHAKE_LENGTH 10
#define SWITCH_PIN 32  // dein Kippschalter‑Pin (INPUT_PULLUP)

#define PULSE_DURATION 150 // Dauer des Pulses in ms
float sensitivity = 1.85; // Empfindlichkeit als globale Variable
#define DEBOUNCE_TIME 250  // Mindestabstand zwischen Events
#define SMOOTHING 0.99     // Hintergrundanpassung (0.99 = sehr langsam)


bool soundMode = false;  // aktuell im Sound‑Modus?
bool lastSwitch = HIGH;  // letzter gemessener Zustand
bool manualEffectActive = false; // Gibt an, ob ein manueller Effekt aktiv ist

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const char *ssid = "FRITZ!Box Fon WLAN 7490";
const char *password = "88677692662852536431";

// Variablen, um den Zustand des Effekts zu speichern
bool animationActive = false;
bool ledOffRequested = false;
bool runningLightActive = false;
bool shakeLightActive = false;
bool sparkleActive = false;
bool classicAmpelActive = false;
bool stroboActive = false;
bool rouletteActive = false;

int runningLightLength = 1;
int runningLightDelay = 50;
int shakeLightLength = SHAKE_LENGTH;
int shakeLightDelay = 50;
int rainbowDelay = 20;
int sparkleDelay = 100;
int stroboDelay = 70;
WebServer server(80);
int brightness = 255;
uint32_t currentColor = strip.Color(119, 200, 65);

unsigned long previousMillis = 0;
int rainbowStep = 0;
int shakePosition = -SHAKE_LENGTH;

// Neue globale Variablen für Roulette
unsigned long rouletteLastSwitch = 0;
int rouletteCurrentGroup = -1;

// Globale Variable für den Timer
unsigned long lastEffectChange = 0;

void stopAllEffects()
{
  animationActive = false;
  ledOffRequested = false;
  runningLightActive = false;
  shakeLightActive = false;
  sparkleActive = false;
  classicAmpelActive = false;
  stroboActive = false;
  rouletteActive = false;
  strip.clear();
  strip.show();
}

class SoundReactiveLEDs
{
public:
  SoundReactiveLEDs(uint8_t micPin, Adafruit_NeoPixel &ledStrip)
      : _micPin(micPin), _strip(ledStrip) {}

  void begin()
  {
    _backgroundLevel = analogRead(_micPin);
    _backgroundLevel = abs(_backgroundLevel - 2048);
    _backgroundLevelSmooth = _backgroundLevel;
    _lastEventTime = millis();
    randomSeed(analogRead(0));
  }

  void update()
  {
    int micValue = analogRead(_micPin);
    micValue = abs(micValue - 2048);

    unsigned long currentMillis = millis();

    _backgroundLevelSmooth = (_backgroundLevelSmooth * SMOOTHING) + (micValue * (1.0 - SMOOTHING));

    bool bassDetected = micValue > _backgroundLevelSmooth * sensitivity;

    if (bassDetected && !_bassWasActive && (currentMillis - _lastEventTime > DEBOUNCE_TIME))
    {
      if (_mode != NONE)
      {
        triggerEvent();
      }
      _lastEventTime = currentMillis;
    }

    _bassWasActive = bassDetected;

    if (_bassActive && (currentMillis - _bassStartTime > PULSE_DURATION))
    {
      _bassActive = false;
      _strip.clear();
      _strip.show();
    }
  }


  void soundRandomGroup()
  {
    _mode = RANDOM_GROUP;
  }


private:
  enum Mode
  {
    NONE,
    RANDOM_GROUP,
  };

  uint8_t _micPin;
  Adafruit_NeoPixel &_strip;
  Mode _mode = NONE;

  float _backgroundLevel = 0;
  float _backgroundLevelSmooth = 0;
  bool _bassWasActive = false;
  bool _bassActive = false;
  unsigned long _bassStartTime = 0;
  unsigned long _lastEventTime = 0;
  int _eventCounter = 0;
  int _stackCounter = 0;

  void triggerEvent()
  {
    switch (_mode)
    {
    case RANDOM_GROUP:
      pulseRandomGroup();
      break;
    default:
      break;
    }
  }

  void pulseRandomGroup()
  {
    _bassActive = true;
    _bassStartTime = millis();
    _strip.clear();
    int group = random(0, 3);
    setGroupLeds(group, currentColor);
    _strip.show();
  }


  void setAllLeds(uint32_t color)
  {
    for (int i = 0; i < _strip.numPixels(); i++)
    {
      _strip.setPixelColor(i, color);
    }
  }

  void setGroupLeds(int group, uint32_t color)
  {
    int start = group * 40;
    int end = start + 40;
    for (int i = start; i < end; i++)
    {
      _strip.setPixelColor(i, color);
    }
  }

  void clearGroupLeds(int group)
  {
    int start = group * 40;
    int end = start + 40;
    for (int i = start; i < end; i++)
    {
      _strip.setPixelColor(i, 0);
    }
  }
};
SoundReactiveLEDs leds(MIC_PIN, strip);

void applyColor()
{
  strip.setBrightness(brightness);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    strip.setPixelColor(i, currentColor);
  }
  strip.show();
}

void applyColor2nd(uint8_t red, uint8_t green, uint8_t blue)
{
  uint32_t tempColor = strip.Color(red, green, blue);
  strip.setBrightness(brightness);
  for (int i = 0; i < NUM_LEDS; i++)
  {
    strip.setPixelColor(i, tempColor);
  }
  strip.show();
}

void ledOn() {
  stopAllEffects();
  manualEffectActive = true; // Manuell gestarteter Effekt
  applyColor();
  server.send(204, "text/plain", "");
}

void handleLedOff()
{
  ledOffRequested = true;
  server.send(204, "text/plain", "");
}

void ledOffStep()
{
  if (ledOffRequested)
  {
    strip.clear();
    strip.show();
    manualEffectActive = true;
    ledOffRequested = false;
  }
}

uint32_t Wheel(byte WheelPos)
{
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85)
  {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170)
  {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbowCycleStep()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= rainbowDelay)
  {
    previousMillis = currentMillis;
    for (int i = 0; i < strip.numPixels(); i++)
    {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbowStep) & 255));
    }
    strip.show();
    rainbowStep++;
    if (rainbowStep >= 256)
    {
      rainbowStep = 0;
    }
  }
}

void startRainbowAnimation()
{
  stopAllEffects();
  animationActive = true;
}

void stopAnimation()
{
  animationActive = false;
  strip.clear();
  strip.show();
  server.send(204, "text/plain", "");
}

void handleStartRainbow()
{
  startRainbowAnimation();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopAnimation()
{
  stopAnimation();
}

void setRainbowDelay()
{
  if (server.hasArg("delay"))
  {
    rainbowDelay = server.arg("delay").toInt();
    if (rainbowDelay < 1)
      rainbowDelay = 1;
  }
  server.send(204, "text/plain", "");
}

void setBrightness()
{
  if (server.hasArg("value"))
  {
    brightness = server.arg("value").toInt();
    applyColor();
  }
  server.send(204, "text/plain", "");
}

void setColor()
{
  if (server.hasArg("color"))
  {
    String hexColor = server.arg("color");
    long number = strtol(hexColor.c_str(), NULL, 16);
    int r = (number >> 16) & 0xFF;
    int g = (number >> 8) & 0xFF;
    int b = number & 0xFF;
    currentColor = strip.Color(r, g, b);
    applyColor();
  }
  server.send(204, "text/plain", "");
}

void setSensitivity() {
  if (server.hasArg("value")) {
    float newSensitivity = server.arg("value").toFloat();
    if (newSensitivity > 0.1 && newSensitivity < 10.0) { // Begrenzung der Werte
      sensitivity = newSensitivity;
    }
  }
  server.send(204, "text/plain", ""); // Keine Antwort nötig
}

void runningLightStep()
{
  if (runningLightActive)
  {
    for (int i = 0; i < NUM_LEDS; i++)
    {
      if (i < runningLightLength)
      {
        strip.setPixelColor(i, currentColor);
      }
      else
      {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
      }
    }
    strip.show();

    if (runningLightLength >= NUM_LEDS)
    {
      for (int i = 0; i < NUM_LEDS; i++)
      {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
        strip.show();
        delay(runningLightDelay);
      }
      runningLightLength = 0;
    }
    else
    {
      runningLightLength++;
      delay(runningLightDelay);
    }
  }
}

void startRunningLight()
{
  stopAllEffects();
  runningLightActive = true;
}

void stopRunningLight()
{
  runningLightActive = false;
  strip.clear();
  strip.show();
  server.send(204, "text/plain", "");
}

void handleStartRunningLight()
{
  startRunningLight();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopRunningLight()
{
  stopRunningLight();
}

void setRunningLightDelay()
{
  if (server.hasArg("delay"))
  {
    runningLightDelay = server.arg("delay").toInt();
    if (runningLightDelay < 1)
      runningLightDelay = 1;
  }
  server.send(204, "text/plain", "");
}

void shakeLightStep()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }

  for (int i = shakePosition; i < shakePosition + shakeLightLength; i++)
  {
    if (i >= 0 && i < NUM_LEDS)
    {
      strip.setPixelColor(i, currentColor);
    }
  }

  strip.show();

  shakePosition++;
  if (shakePosition >= NUM_LEDS)
  {
    shakePosition = -shakeLightLength;
  }

  delay(shakeLightDelay);
}

void startShakeLight()
{
  stopAllEffects();
  shakeLightActive = true;
}

void stopShakeLight()
{
  shakeLightActive = false;
  strip.clear();
  strip.show();
}

void handleStartShakeLight()
{
  startShakeLight();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopShakeLight()
{
  stopShakeLight();
  server.send(204, "text/plain", "");
}

void setShakeLightLength()
{
  if (server.hasArg("length"))
  {
    shakeLightLength = server.arg("length").toInt();
    if (shakeLightLength < 1)
      shakeLightLength = 1;
    if (shakeLightLength > NUM_LEDS)
      shakeLightLength = NUM_LEDS;
  }
  server.send(204, "text/plain", "");
}

void setShakeLightDelay()
{
  if (server.hasArg("delay"))
  {
    shakeLightDelay = server.arg("delay").toInt();
    if (shakeLightDelay < 1)
      shakeLightDelay = 1;
  }
  server.send(204, "text/plain", "");
}

void sparkle()
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (random(10) < 2)
    {
      int sparkleBrightness = random(0, 256);
      strip.setPixelColor(i, currentColor);
    }
    else
    {
      strip.setPixelColor(i, strip.Color(0, 0, 0));
    }
  }
  strip.show();
  delay(sparkleDelay);
}

void startSparkle()
{
  stopAllEffects();
  sparkleActive = true;
}

void stopSparkle()
{
  sparkleActive = false;
  strip.clear();
  strip.show();
}

void handleStartSparkle()
{
  startSparkle();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopSparkle()
{
  stopSparkle();
  server.send(204, "text/plain", "");
}

void setSparkleDelay()
{
  if (server.hasArg("delay"))
  {
    sparkleDelay = server.arg("delay").toInt();
    if (sparkleDelay < 1)
      sparkleDelay = 1;
  }
  server.send(204, "text/plain", "");
}

void classicAmpelStep()
{
  static unsigned long lastPhaseChange = 0;
  static int phase = 0;                                     // 0: Rot, 1: Rot-Gelb, 2: Grün, 3: Gelb
  static unsigned long phaseDuration = random(4000, 10001); // Zufällige Dauer für die aktuelle Phase

  if (!classicAmpelActive)
    return;

  unsigned long currentMillis = millis();

  if (currentMillis - lastPhaseChange >= phaseDuration)
  {
    // Wechsel zur nächsten Phase
    phase = (phase + 1) % 4; // Zyklischer Wechsel zwischen 0, 1, 2, 3
    lastPhaseChange = currentMillis;
    phaseDuration = random(4000, 10001); // Neue zufällige Dauer für die nächste Phase

    strip.clear();

    switch (phase)
    {
    case 0: // Rot
      for (int i = 0; i < NUM_LEDS / 3; i++)
      {
        strip.setPixelColor(i, currentColor);
      }
      break;

    case 1: // Rot-Gelb
      for (int i = 0; i < 2 * NUM_LEDS / 3; i++)
      {
        if (i < NUM_LEDS / 3)
          strip.setPixelColor(i, currentColor); // Rot
        else
          strip.setPixelColor(i, currentColor); // Gelb
      }
      break;

    case 2: // Grün
      for (int i = 2 * NUM_LEDS / 3; i < NUM_LEDS; i++)
      {
        strip.setPixelColor(i, currentColor);
      }
      break;

    case 3: // Gelb
      for (int i = NUM_LEDS / 3; i < 2 * NUM_LEDS / 3; i++)
      {
        strip.setPixelColor(i, currentColor);
      }
      break;
    }

    strip.show();
  }
}

void startClassicAmpel()
{
  stopAllEffects();
  classicAmpelActive = true;
}

void stopClassicAmpel()
{
  classicAmpelActive = false;
  strip.clear();
  strip.show();
  server.send(204, "text/plain", "");
}

void handleStartClassicAmpel()
{
  startClassicAmpel();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopClassicAmpel()
{
  stopClassicAmpel();
  server.send(204, "text/plain", "");
}

void handleRestart()
{
  server.send(200, "text/plain", "ESP wird neu gestartet...");
  ESP.restart(); // Neustart des ESP
}

void stroboAll()
{
  static unsigned long lastUpdateTime = 0; // Zeitstempel für den letzten Schritt
  static bool isOn = false;                // Zustand (an/aus)

  if (!stroboActive) // Effekt nur ausführen, wenn aktiv
    return;

  unsigned long currentMillis = millis();

  if (currentMillis - lastUpdateTime >= stroboDelay)
  {
    lastUpdateTime = currentMillis;

    if (isOn)
    {
      // Alle LEDs ausschalten
      strip.clear();
    }
    else
    {
      // Alle LEDs einschalten
      for (int j = 0; j < NUM_LEDS; j++)
      {
        strip.setPixelColor(j, currentColor);
      }
    }

    strip.show();
    isOn = !isOn; // Zustand umschalten
  }
}

void setStroboDelay()
{
  if (server.hasArg("delay"))
  {
    stroboDelay = server.arg("delay").toInt();
    if (stroboDelay < 1)
      stroboDelay = 1; // Mindestwert
  }
  server.send(204, "text/plain", "");
}

void handleStroboAll()
{
  int delay = 100; // Standardwert

  if (server.hasArg("delay"))
  {
    delay = server.arg("delay").toInt();
    if (delay < 10)
      delay = 10;
  }

  stroboAll();
  server.send(204, "text/plain", "");
}

void startStrobo()
{
  stopAllEffects(); // Stoppt alle anderen Effekte
  stroboActive = true;
}

void stopStrobo()
{
  stroboActive = false;
  strip.clear();
  strip.show();
  server.send(204, "text/plain", "");
}

void handleStartStrobo()
{
  startStrobo();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopStrobo()
{
  stopStrobo();
}

// Angepasste Roulette-Logik (nicht-blockierend)
void rouletteStep()
{
  static unsigned long lastUpdateTime = 0; // Zeitstempel für den letzten Schritt
  static int step = 0;                     // Aktueller Schritt
  static bool isOn = false;                // Zustand (an/aus)
  static int newGrouprand = -1;            // Zufällige Gruppenauswahl

  if (millis() - lastUpdateTime >= 120) // 120 ms Verzögerung zwischen den Schritten
  {
    lastUpdateTime = millis(); // Zeitstempel aktualisieren

    if (step < 85) // Während der ersten 85 Schritte
    {
      if (newGrouprand == -1 || !isOn) // Neue Gruppe auswählen, wenn LEDs ausgeschaltet sind
      {
        newGrouprand = random(0, 3); // Zufällige Gruppenauswahl
      }

      strip.clear(); // LEDs ausschalten

      if (isOn) // LEDs einschalten
      {
        if (newGrouprand == 0) // Gruppe 1
        {
          for (int i = 0; i < NUM_LEDS / 3; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }
        else if (newGrouprand == 1) // Gruppe 2
        {
          for (int i = NUM_LEDS / 3; i < 2 * NUM_LEDS / 3; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }
        else // Gruppe 3
        {
          for (int i = 2 * NUM_LEDS / 3; i < NUM_LEDS; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }
      }

      strip.show(); // Änderungen anzeigen
      isOn = !isOn; // Zustand umschalten (an/aus)
      step++;       // Nächster Schritt
    }
    else // Nach den 85 Schritten
    {
      static unsigned long finalGroupDelayStart = 0;

      if (finalGroupDelayStart == 0) // Startzeit für die lange Pause setzen
      {
        finalGroupDelayStart = millis();
        newGrouprand = random(0, 3); // Zufällige Gruppe für die lange Pause auswählen
        strip.clear();

        if (newGrouprand == 0) // Gruppe 1
        {
          for (int i = 0; i < NUM_LEDS / 3; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }
        else if (newGrouprand == 1) // Gruppe 2
        {
          for (int i = NUM_LEDS / 3; i < 2 * NUM_LEDS / 3; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }
        else // Gruppe 3
        {
          for (int i = 2 * NUM_LEDS / 3; i < NUM_LEDS; i++)
          {
            strip.setPixelColor(i, currentColor);
          }
        }

        strip.show(); // Änderungen anzeigen
      }

      // 30 Sekunden Pause zwischen den Gruppenschaltungen
      if (millis() - finalGroupDelayStart >= 30000)
      {
        step = 0;                 // Zurücksetzen für den nächsten Durchlauf
        finalGroupDelayStart = 0; // Pause zurücksetzen
      }
    }
  }
}

// Startet Roulette
void startRoulette()
{
  stopAllEffects();
  rouletteActive = true;
  rouletteLastSwitch = millis();
}

void stopRoulette()
{
  rouletteActive = false;
  strip.clear();
  strip.show();
  server.send(204, "text/plain", "");
}

void handleStartRoulette()
{
  startRoulette();
  manualEffectActive = true;
  server.send(204, "text/plain", "");
}

void handleStopRoulette()
{
  stopRoulette();
}

void cycleEffects() {
  static int currentEffect = 0;

  unsigned long currentMillis = millis();

  // Effektwechsel nur, wenn kein manueller Effekt aktiv ist
  if (!manualEffectActive && (currentMillis - lastEffectChange >= 40000)) {
    // Zeitstempel aktualisieren
    lastEffectChange = currentMillis;

    // Stoppe den aktuellen Effekt
    stopAllEffects();

    // Effekt basierend auf der aktuellen Position starten
    switch (currentEffect) {
      case 0:
        startRunningLight();
        break;
      case 1:
        startRainbowAnimation();
        break;
      case 2:
        startShakeLight();
        break;
      case 3:
        startSparkle();
        break;
      case 4:
        startClassicAmpel();
        break;
      case 5:
        startStrobo();
        break;
      case 6:
        ledOn(); // LEDs einschalten
        break;
    }

    // Zum nächsten Effekt wechseln
    currentEffect = (currentEffect + 1) % 7; // Es gibt 7 Effekte
  }
}

void handleRoot()
{
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="de">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Ampel Jura Steuerung</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #1e1e1e;
      color: #ffffff;
      text-align: center;
      padding-top: 50px;
    }
    .container {
      background-color: #2c2c2c;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 20px rgba(0,0,0,0.5);
      display: inline-block;
    }
    button {
      padding: 10px 20px;
      margin: 10px;
      border: none;
      border-radius: 5px;
      background-color: #28a745;
      color: white;
      font-size: 16px;
      cursor: pointer;
    }
    button:hover {
      background-color: #218838;
    }
    #brightnessSlider {
      width: 80%;
      appearance: none;
      height: 10px;
      border-radius: 5px;
      background: linear-gradient(to right, #000000, #ffffff);
      outline: none;
      cursor: pointer;
    }
    input[type="color"] {
      width: 100px;
      height: 100px;
      border: none;
      cursor: pointer;
    }
    label {
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Ampel Steuerung</h1>
    <br><br>
    <h3>Für Soundmodus den Schalter unten an der Ampel umlegen</h3><br>
    <label for="sensitivity">Empfindlichkeit Soundmodus:</label><br>
    <p>Empfindlichkeit setzen, dann wieder Schalter umlegen</p><br>
    <input type="number" id="sensitivity" min="0.1" max="10.0" step="0.1" value="2.0">
    <button onclick="setSensitivity()">Empfindlichkeit setzen</button>
    <br><br>
    <br><br>
    <h3>Effekte:</h3><br>
    <button onclick="sendRequest('/start_running_light')"> Lauflicht Starten</button>
    <button onclick="sendRequest('/stop_running_light')">Lauflicht Stoppen</button>
    <br><br>
    <label for="runningLightDelay">Verzögerung des Lauflichts (ms):</label><br>
    <input type="number" id="runningLightDelay" min="1" value="50">
    <button onclick="setRunningLightDelay()">Verzögerung setzen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_rainbow')">LSD Ampel Starten</button>
    <button onclick="sendRequest('/stop_animation')">LSD Ampel Stoppen</button>
    <br><br>
    <label for="rainbowDelay">Verzögerung des Rainbow-Effekts (ms):</label><br>
    <input type="number" id="rainbowDelay" min="1" value="20">
    <button onclick="setRainbowDelay()">Verzögerung setzen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_shake_light')">Gruppen Lauflicht Starten</button>
    <button onclick="sendRequest('/stop_shake_light')">Gruppen Lauflicht Stoppen</button>
    <br><br>
    <label for="shakeLightLength">Länge des Shake-Lichts (max: 120):</label><br>
    <input type="number" id="shakeLightLength" min="1" max="120" value="10">
    <button onclick="setShakeLightLength()">Länge setzen</button>
    <br><br>
    <label for="shakeLightDelay">Verzögerung des Shake-Lichts (ms):</label><br>
    <input type="number" id="shakeLightDelay" min="1" value="50">
    <button onclick="setShakeLightDelay()">Verzögerung setzen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_sparkle')">Sparkle Starten</button>
    <button onclick="sendRequest('/stop_sparkle')">Sparkle Stoppen</button>
    <br><br>
    <label for="sparkleDelay">Verzögerung des Sparkle-Effekts (ms):</label><br>
    <input type="number" id="sparkleDelay" min="1" value="100">
    <button onclick="setSparkleDelay()">Verzögerung setzen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_classic_ampel')">Klassische Ampel Starten</button>
    <button onclick="sendRequest('/stop_classic_ampel')">Klassische Ampel Stoppen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_strobo')">Strobo Starten</button>
    <button onclick="sendRequest('/stop_strobo')">Strobo Stoppen</button>
    <br><br>
    <label for="stroboDelay">Verzögerung des Strobo-Effekts (ms):</label><br>
    <input type="number" id="stroboDelay" min="1" value="70">
    <button onclick="setStroboDelay()">Verzögerung setzen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/start_roulette')">Roulette Starten</button>
    <button onclick="sendRequest('/stop_roulette')">Roulette Stoppen</button>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/led_on')">LEDs Einschalten</button>
    <button onclick="sendRequest('/led_off')">LEDs Ausschalten</button>
    <br><br>
    <br><br>
    <label for="brightnessSlider">Helligkeit einstellen:</label><br>
    <input type="range" id="brightnessSlider" min="0" max="255" value="255" oninput="updateBrightness(this.value)">
    <span id="brightnessValue">100%</span>
    <br><br>
    <br><br>
    <label for="colorPicker">Farbe wählen:</label><br>
    <input type="color" id="colorPicker" value="#ffffff" oninput="updateColor(this.value)">
    <span id="colorValue">#FFFFFF</span>
    <br><br>
    <br><br>
    <button onclick="sendRequest('/restart')">ESP Neustarten</button>
    <br><br>
    <br><br>
    <br><br>
  </div>

  <script>
    function sendRequest(endpoint) {
      fetch(endpoint).then(response => {
        if (!response.ok) {
          console.error('Fehler bei der Anfrage:', response.statusText);
        }
      });
    }

    function updateBrightness(value) {
      document.getElementById('brightnessValue').textContent = Math.round(value / 255 * 100) + '%';
      fetch('/set_brightness?value=' + value);
    }

    function updateColor(value) {
      document.getElementById('colorValue').textContent = value.toUpperCase();
      fetch('/set_color?color=' + value.substring(1));
    }

    function setRunningLightDelay() {
      const delay = document.getElementById('runningLightDelay').value;
      fetch('/set_running_light_delay?delay=' + delay);
    }

    function setRainbowDelay() {
      const delay = document.getElementById('rainbowDelay').value;
      fetch('/set_rainbow_delay?delay=' + delay);
    }

    function setShakeLightLength() {
      const length = document.getElementById('shakeLightLength').value;
      fetch('/set_shake_light_length?length=' + length);
    }

    function setShakeLightDelay() {
      const delay = document.getElementById('shakeLightDelay').value;
      fetch('/set_shake_light_delay?delay=' + delay);
    }

    function setSparkleDelay() {
      const delay = document.getElementById('sparkleDelay').value;
      fetch('/set_sparkle_delay?delay=' + delay);
    }

    function getStroboDelay() {
      return document.getElementById('stroboDelay').value;
    }

    function getStroboCount() {
      return document.getElementById('stroboCount').value;
    }

    function setStroboDelay() {
      const delay = document.getElementById('stroboDelay').value;
      fetch('/set_strobo_delay?delay=' + delay).then(response => {
        if (!response.ok) {
          console.error('Fehler beim Setzen der Strobo-Verzögerung:', response.statusText);
        }
      });
    }

    function setSensitivity() {
      const sensitivity = document.getElementById('sensitivity').value;
      fetch('/set_sensitivity?value=' + sensitivity);
    }
  </script>
</body>
</html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void startServer() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  server.on("/", handleRoot);

  server.on("/led_on", ledOn);
  server.on("/led_off", handleLedOff);

  server.on("/set_brightness", setBrightness);

  server.on("/set_color", setColor);

  server.on("/start_rainbow", handleStartRainbow);
  server.on("/stop_animation", handleStopAnimation);
  server.on("/set_rainbow_delay", setRainbowDelay);

  server.on("/start_running_light", handleStartRunningLight);
  server.on("/stop_running_light", handleStopRunningLight);
  server.on("/set_running_light_delay", setRunningLightDelay);

  server.on("/start_shake_light", handleStartShakeLight);
  server.on("/stop_shake_light", handleStopShakeLight);
  server.on("/set_shake_light_length", setShakeLightLength);
  server.on("/set_shake_light_delay", setShakeLightDelay);

  server.on("/start_sparkle", handleStartSparkle);
  server.on("/stop_sparkle", handleStopSparkle);
  server.on("/set_sparkle_delay", setSparkleDelay);

  server.on("/start_classic_ampel", handleStartClassicAmpel);
  server.on("/stop_classic_ampel", handleStopClassicAmpel);

  server.on("/restart", handleRestart);

  server.on("/strobo_all", handleStroboAll);
  server.on("/set_strobo_delay", setStroboDelay);
  server.on("/start_strobo", handleStartStrobo);
  server.on("/stop_strobo", handleStopStrobo);

  server.on("/start_roulette", handleStartRoulette);
  server.on("/stop_roulette", handleStopRoulette);

  server.on("/set_sensitivity", setSensitivity);

  server.begin();
  Serial.println("Webserver gestartet");

  // Reset der Zeit für cycleEffects
  lastEffectChange = millis(); // Setze den Timer zurück

  applyColor2nd(0, 255, 0);
  delay(3000);
  applyColor();
  delay(1000);
}

void setup()
{
  Serial.begin(9600);
  strip.begin();
  leds.begin();
  applyColor();
  delay(10);


  pinMode(SWITCH_PIN, INPUT_PULLUP);

  // initialen Modus setzen
  lastSwitch = digitalRead(SWITCH_PIN);
  if (lastSwitch == LOW) {
    // direkt in Sound‑Mode starten
    stopAllEffects();
    WiFi.disconnect(true);
    leds.soundRandomGroup();
    soundMode = true;
  } else {
    // direkt im Server‑Mode starten
    startServer();
  }
}

void loop() {
  // 1) Kippschalter einlesen
  int curSwitch = digitalRead(SWITCH_PIN);

  // 2) Bei Positionswechsel Modus wechseln
  if (curSwitch != lastSwitch) {
    delay(50);  // kleines Debounce
    curSwitch = digitalRead(SWITCH_PIN);
    if (curSwitch != lastSwitch) {
      if (curSwitch == LOW) {
        // → Sound‑Mode
        stopAllEffects();
        WiFi.disconnect(true);
        applyColor();
        delay(30);
        applyColor();
        soundMode = true;
        leds.soundRandomGroup();
      } else {
        // → Server‑Mode
        stopAllEffects();
        startServer();  // WiFi.begin + server.begin
        soundMode = false;
      }
      lastSwitch = curSwitch;
    }
  }

  // 3) den aktiven Modus ausführen
  if (soundMode) {
    // Nur Sound‑Reactive
    leds.update();
    delay(10);  // kleine Pause für die Sound‑Reactive LEDs
  } else {
    // Nur Web‑Server + UI‑Effekte
    server.handleClient();  // nur nötig, wenn du den Sync‑WebServer nutzt
    if (animationActive) rainbowCycleStep();
    if (runningLightActive) runningLightStep();
    if (shakeLightActive) shakeLightStep();
    if (sparkleActive) sparkle();
    if (classicAmpelActive) classicAmpelStep();
    if (stroboActive) stroboAll();
    if (rouletteActive) rouletteStep();
    ledOffStep();

    // `cycleEffects()` unabhängig von anderen Effekten weiterlaufen lassen
    cycleEffects();
  }
}