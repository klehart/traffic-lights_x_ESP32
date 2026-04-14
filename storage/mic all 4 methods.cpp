#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 27
#define NUM_LEDS 120
#define MIC_PIN 34

#define PULSE_DURATION 150   // Dauer des Pulses in ms
#define SENSITIVITY 1.5      // Empfindlichkeit
#define DEBOUNCE_TIME 250    // Mindestabstand zwischen Events
#define SMOOTHING 0.99       // Hintergrundanpassung (0.99 = sehr langsam)

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
uint32_t currentColor = strip.Color(119, 200, 65);

class SoundReactiveLEDs {
public:
  SoundReactiveLEDs(uint8_t micPin, Adafruit_NeoPixel& ledStrip)
    : _micPin(micPin), _strip(ledStrip) {}

  void begin() {
    _backgroundLevel = analogRead(_micPin);
    _backgroundLevel = abs(_backgroundLevel - 2048);
    _backgroundLevelSmooth = _backgroundLevel;
    _lastEventTime = millis();
    randomSeed(analogRead(0));
  }

  void update() {
    int micValue = analogRead(_micPin);
    micValue = abs(micValue - 2048);

    unsigned long currentMillis = millis();

    _backgroundLevelSmooth = (_backgroundLevelSmooth * SMOOTHING) + (micValue * (1.0 - SMOOTHING));

    bool bassDetected = micValue > _backgroundLevelSmooth * SENSITIVITY;

    if (bassDetected && !_bassWasActive && (currentMillis - _lastEventTime > DEBOUNCE_TIME)) {
      if (_mode != NONE) {
        triggerEvent();
      }
      _lastEventTime = currentMillis;
    }

    _bassWasActive = bassDetected;

    if (_bassActive && (currentMillis - _bassStartTime > PULSE_DURATION)) {
      _bassActive = false;
      _strip.clear();
      _strip.show();
    }
  }

  void soundReactiveAll() {
    _mode = ALL;
  }

  void soundInOrder() {
    _mode = ORDER;
  }

  void soundRandomGroup() {
    _mode = RANDOM_GROUP;
  }

  void soundStackGroups() {
    _mode = STACK;
  }

private:
  enum Mode { NONE, ALL, ORDER, RANDOM_GROUP, STACK };

  uint8_t _micPin;
  Adafruit_NeoPixel& _strip;
  Mode _mode = NONE;

  float _backgroundLevel = 0;
  float _backgroundLevelSmooth = 0;
  bool _bassWasActive = false;
  bool _bassActive = false;
  unsigned long _bassStartTime = 0;
  unsigned long _lastEventTime = 0;
  int _eventCounter = 0;
  int _stackCounter = 0;

  void triggerEvent() {
    switch (_mode) {
      case ALL:
        pulseAll();
        break;
      case ORDER:
        toggleGroupInOrder();
        break;
      case RANDOM_GROUP:
        pulseRandomGroup();
        break;
      case STACK:
        stackGroups();
        break;
      default:
        break;
    }
  }

  void pulseAll() {
    _bassActive = true;
    _bassStartTime = millis();
    setAllLeds(currentColor);
    _strip.show();
  }

  void toggleGroupInOrder() {
    _bassActive = true;
    _bassStartTime = millis();
    _strip.clear();
    int group = _eventCounter % 3;
    setGroupLeds(group, currentColor);
    _strip.show();
    _eventCounter++;
  }

  void pulseRandomGroup() {
    _bassActive = true;
    _bassStartTime = millis();
    _strip.clear();
    int group = random(0, 3);
    setGroupLeds(group, currentColor);
    _strip.show();
  }

  void stackGroups() {
    int action = _stackCounter % 6;
    int group = action % 3;

    if (action < 3) {
      setGroupLeds(group, currentColor);
    } else {
      clearGroupLeds(group);
    }

    _strip.show();
    _stackCounter++;
  }

  void setAllLeds(uint32_t color) {
    for (int i = 0; i < _strip.numPixels(); i++) {
      _strip.setPixelColor(i, color);
    }
  }

  void setGroupLeds(int group, uint32_t color) {
    int start = group * 40;
    int end = start + 40;
    for (int i = start; i < end; i++) {
      _strip.setPixelColor(i, color);
    }
  }

  void clearGroupLeds(int group) {
    int start = group * 40;
    int end = start + 40;
    for (int i = start; i < end; i++) {
      _strip.setPixelColor(i, 0);
    }
  }
};

SoundReactiveLEDs leds(MIC_PIN, strip);

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.show();
  strip.setBrightness(255);

  delay(500);
  leds.begin();

  // Hier Modus wählen:
  leds.soundReactiveAll();
  leds.soundInOrder();
  leds.soundRandomGroup();
  leds.soundStackGroups();
}

void loop() {
  leds.update();
  delay(10);
}
