#pragma once

void motorPurr() {
  // Hum: 60 ms ON / 120 ms OFF
  unsigned long interval = motorState ? 60UL : 120UL;
  if (millis() - lastMotorToggle >= interval) {
    motorState = !motorState;
    digitalWrite(MOTOR_PIN, motorState ? HIGH : LOW);
    lastMotorToggle = millis();
  }
}

void motorFear() {
  // Panik titreşimi: 25 ms ON/OFF
  if (millis() - lastMotorToggle >= 25UL) {
    motorState = !motorState;
    digitalWrite(MOTOR_PIN, motorState ? HIGH : LOW);
    lastMotorToggle = millis();
  }
}

void motorOff() {
  motorState = false;
  digitalWrite(MOTOR_PIN, LOW);
}

// HAPPY pink 
void aura_Happy() {
  breathePhase += 0.05f;
  if (breathePhase > TWO_PI) breathePhase -= TWO_PI;
  float t = (sinf(breathePhase) + 1.0f) * 0.5f;
  uint8_t b = (uint8_t)(40 + t * 160);
  CRGB c = CRGB(b, (uint8_t)(b * 0.24f), (uint8_t)(b * 0.47f));
  fill_solid(leds, NUM_LEDS, c);
  FastLED.show();
}

// NORMAL purple breath
void aura_Normal() {
  breathePhase += 0.02f;
  if (breathePhase > TWO_PI) breathePhase -= TWO_PI;
  float t = (sinf(breathePhase) + 1.0f) * 0.5f;
  uint8_t b = (uint8_t)(15 + t * 65);
  fill_solid(leds, NUM_LEDS, CRGB(b, (uint8_t)(b / 2), 0));
  FastLED.show();
}

// FEAR fast red strobe
void aura_Scared() {
  if (millis() - lastStrobeTime >= 75UL) {
    strobeOn = !strobeOn;
    fill_solid(leds, NUM_LEDS, strobeOn ? CRGB::Red : CRGB::Black);
    FastLED.show();
    lastStrobeTime = millis();
  }
}

//DAZZLED bright white pulse
void aura_Dazzled() {
  breathePhase += 0.08f;
  if (breathePhase > TWO_PI) breathePhase -= TWO_PI;
  float t = (sinf(breathePhase) + 1.0f) * 0.5f;
  uint8_t b = (uint8_t)(80 + t * 175);
  fill_solid(leds, NUM_LEDS, CRGB(b, b, b));
  FastLED.show();
}

// OFF
void aura_Off() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}
