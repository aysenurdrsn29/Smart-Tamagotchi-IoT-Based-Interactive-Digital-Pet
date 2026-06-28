// system include
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <FastLED.h>
#include <esp_sleep.h>
#include <WiFi.h>
#include <math.h>

// pin configuration
#define TFT_CS      5
#define TFT_RST     4
#define TFT_DC      2
// MOSI=23, SCK=18 are default Hardware SPI pins 

#define TOUCH_PIN   33      // Digital IN (HIGH = touched) 
#define SOUND_PIN   32    // Analog IN sound sensor
#define PIR_PIN     27       // PIR digital (HIGH = motiın)
#define LDR_PIN     34    // LDR analog, input-only
#define LED_PIN     13    // WS2812B Data IN 
#define MOTOR_PIN   12    // Haptic feedback PWM 
#define BOOT_BTN    0     // Diary mode
#define DEMO_BTN    25    // External push button 

//LED 
#define NUM_LEDS      16
#define LED_BRIGHT    80

// sensor thresholds
#define SOUND_THRESHOLD     1600   // ADC deviation from baseline 
#define SOUND_MEMORY_MS     800   // Peak hold duration s
#define LDR_BRIGHT_ON       4000  // Entry threshold for Dazzled 
#define LDR_BRIGHT_OFF      3500  // Exit threshold (hysteresis) 
#define LDR_DARK_THRESHOLD  1500 // Sleep trigger threshold 

// time variables
#define NORMAL_TIMEOUT_MS    300000UL   // 5 minute
#define SLEEP_TIMEOUT_MS    900000UL   // 15 minute
#define DEMO_SLEEP_MS       30000UL    // Demo: 30 second
#define SCARED_DURATION_MS  3000UL
#define HAPPY_DURATION_MS   6000UL
#define DAZZLED_DURATION_MS 4000UL
#define DEMO_BTN_LONG_MS    2000UL
#define LDR_DARK_DELAY_MS     300UL    // Darkness instant sleep phase (0.3 sec)
#define LDR_DARK_DELAY_DEMO   200UL    // Demo: 0.2sn
#define DEEP_SLEEP_DELAY_MS 10000UL    // deep sleep (10s)
#define SOFT_SLEEP_ZZZ_MS   3000UL   // ZZZ animation refresh

//RGB565 colors
#define C_BLACK   0x0000
#define C_WHITE   0xFFFF
#define C_LGRAY   0xC618
#define C_DGRAY   0x7BEF
#define C_YELLOW  0xFFE0
#define C_PINK    0xFB56
#define C_BLUE    0x001F
#define C_CYAN    0x07FF
#define C_DKBLUE  0x000C
#define C_ORANGE  0xFD00
#define C_RED     0xF800
#define C_GREEN   0x07E0

// face center
#define CX  64
#define CY  68
#define CR  30

// state machine
enum CatState : uint8_t {
  STATE_HAPPY   = 0,
  STATE_NORMAL   = 1,
  STATE_SCARED  = 2,
  STATE_DAZZLED = 3,
  STATE_SLEEP   = 4
};

// RTC memory survives deep sleep 
RTC_DATA_ATTR int      rtcTouchCount  = 0;
RTC_DATA_ATTR int      rtcScaredCount = 0;
RTC_DATA_ATTR int      rtcMotionCount = 0;
RTC_DATA_ATTR int      rtcWakeCount   = 0;
RTC_DATA_ATTR uint32_t rtcUptimeSec   = 0;

// GLOBAL VARIABLES
CatState currentState   = STATE_NORMAL;
CatState prevDrawnState = (CatState)99;

int           soundBaseline = 0;
int           soundPeak     = 0;
unsigned long lastSoundTime = 0;
float         soundDB       = 30.0f;

unsigned long lastInteractionTime = 0;
unsigned long stateEnteredTime    = 0;
unsigned long lastBlinkTime       = 0;
unsigned long lastShakeTime       = 0;
unsigned long lastSparkleTime     = 0;
unsigned long lastStrobeTime      = 0;
unsigned long lastMotorToggle     = 0;
unsigned long lastEngDebounce     = 0;
unsigned long lastDemoUpdate      = 0;
unsigned long demoBtnPressStart   = 0;
unsigned long ldrDarkSince = 0;

bool    eyesOpen       = true;
bool    shakeDir       = true;
bool    strobeOn       = false;
bool    motorState     = false;
bool    engMode        = false;
bool    demoMode       = false;
bool    needsRedraw    = true;
bool    demoBtnActive  = false;

int     shakeOffset  = 0;
uint8_t sparkleFrame = 0;
uint8_t sweepFrame   = 0;
float   breathePhase = 0.0f;

const char* stateNames[]  = {"HAPPY","NORMAL","SCARED","DAZZLED","SLEEP"};
const char* stateColors[] = {"PINK","AMBER","RED","WHITE","BLACK"};

// OBJECTS
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
CRGB leds[NUM_LEDS];

// LOCAL HEADER's Must follow globals 
#include "aura.h"
#include "faces.h"

//  functions

// Calculate approximate decibels 30-95 dB scale
float calcDB(int adcDelta) {
  if (adcDelta <= 0) return 30.0f;
  float db = 30.0f + 20.0f * log10f((float)adcDelta / 5.0f + 1.0f);
  return db > 95.0f ? 95.0f : db;
}


//  Deep sleep

void enterDeepSleep() {
  Serial.println("[SLEEP] Preparing for deep sleep...");

  // Accumulate uptime
  rtcUptimeSec += millis() / 1000;

  // Show daily summary
  drawDailySummary();

  // Slowly turn off the LEDs
  for (int b = FastLED.getBrightness(); b >= 0; b -= 4) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(20);
  }
  motorOff();

  delay(4000);           // Show summary for 4 seconds
  tft.fillScreen(C_BLACK);

  // PIR high → wake up
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 1);

  Serial.printf("[SLEEP] RTC: touch=%d fear=%d mov=%d wake=%d uptime=%lus\n",
    rtcTouchCount, rtcScaredCount, rtcMotionCount,
    rtcWakeCount, (unsigned long)rtcUptimeSec);
  Serial.flush();

  esp_deep_sleep_start();
  // This place is never reached
}


//  SETUP

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[BOOT] Smart Tamagotchi starting...");

  // Power saving
  WiFi.mode(WIFI_OFF);
  btStop();

  // Pin modes
  pinMode(TOUCH_PIN, INPUT);
  pinMode(PIR_PIN,   INPUT);
  pinMode(BOOT_BTN,  INPUT_PULLUP);
  pinMode(DEMO_BTN,  INPUT_PULLUP);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // TFT start
  tft.initR(INITR_144GREENTAB);  
  tft.setRotation(1);            // 0/1/2/3  if it goes wrong, replace it
  tft.fillScreen(C_BLACK);

  // display start
  tft.setTextColor(C_CYAN); tft.setTextSize(2);
  tft.setCursor(10, 38); tft.print("Nyaaaa~");
  tft.setTextSize(1);
  tft.setTextColor(C_LGRAY);
  tft.setCursor(18, 66); tft.print("Tamagotchi v2");
  delay(1800);

  // FastLED 
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHT);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(2000);
  // sound baseline calibration
  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_PINK); tft.setTextSize(1);
  tft.setCursor(10, 55); tft.print("Kalibration...");
  tft.setCursor(6, 70);  tft.print("Be quiet  (1s)");
  delay(1000);
  long _sum = 0;
  for (int i = 0; i < 200; i++) {
    _sum += analogRead(SOUND_PIN);
    delay(5);
  }
  soundBaseline = (int)(_sum / 200);
  Serial.printf("[CAL] Sound baseline: %d\n", soundBaseline);

  // reason of waking up
  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  rtcWakeCount++;

  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[WAKE] I woke up to a PIR interrupt!");
    drawWakeupScreen();
    delay(3000);
    currentState = STATE_HAPPY;
  } else {
    Serial.println("[WAKE] Normal starting.");
    currentState = STATE_NORMAL;
  }

  // demo button pressed demo mode
  if (digitalRead(DEMO_BTN) == LOW) {
    demoMode = true;
    Serial.println("[DEMO] Demo mode On — Sleep duration 30s");
    tft.fillScreen(C_BLACK);
    tft.setTextColor(C_YELLOW); tft.setTextSize(1);
    tft.setCursor(14, 55); tft.print("DEMO MODE ON");
    tft.setCursor(8, 70);  tft.print("Sleep: 30 secons");
    delay(1500);
  }

  lastInteractionTime = millis();
  stateEnteredTime    = millis();
  needsRedraw         = true;
  Serial.println("[BOOT] Ready!");
}

// main loop
void loop() {
  unsigned long now = millis();

  // sensor reading

  bool touchDetected = (digitalRead(TOUCH_PIN) == HIGH);
  bool pirDetected   = (digitalRead(PIR_PIN)   == HIGH);
  bool bootPressed   = (digitalRead(BOOT_BTN)  == LOW);
  bool demoPressed   = (digitalRead(DEMO_BTN)  == LOW);
  int  ldrValue      = analogRead(LDR_PIN);

   // Audio: Find the maximum deviation from baseline from 50 samples (~10ms window)
  // Sensors like the MAX9814 oscillate around VCC/2;
  // Absolute deviation (delta) accurately captures sound in both directions
  int maxDelta = 0;
  for (int i = 0; i < 50; i++) {
    int s = analogRead(SOUND_PIN);
    int currentDelta = abs(s - soundBaseline);
    if (currentDelta > maxDelta) maxDelta = currentDelta;
    delayMicroseconds(200);
  }
//remember peak
  if (maxDelta > soundPeak) {
    soundPeak     = maxDelta;
    lastSoundTime = now;
  }
  if (now - lastSoundTime > SOUND_MEMORY_MS) {
    soundPeak = 0;
  }
  int soundDelta = soundPeak;   
  int soundValue = soundPeak;
  soundDB = calcDB(soundDelta);

  //Interaction time

  if (touchDetected || pirDetected) {
    if (pirDetected && !touchDetected) {
      //To avoid counting the PIR too much: at least 2 seconds should pass since the previous movement
      static unsigned long lastPIRcount = 0;
      if (now - lastPIRcount > 2000) {
        rtcMotionCount++;
        lastPIRcount = now;
      }
    }
    lastInteractionTime = now;
  }

  //Boot button

  if (bootPressed && (now - lastEngDebounce >= 500UL)) {
    engMode = !engMode;
    lastEngDebounce = now;
    needsRedraw = true;
    Serial.printf("[ENG] Diary: %s\n", engMode ? "On" : "Off");
  }

  //Demo button 

  if (demoPressed) {
    if (!demoBtnActive) {
      demoBtnActive    = true;
      demoBtnPressStart = now;
    }
    // long = sleep by force
    if ((now - demoBtnPressStart) >= DEMO_BTN_LONG_MS) {
      Serial.println("[DEMO] Forced sleep was triggered.");
      demoBtnActive = false;
      enterDeepSleep();
    }
  } else {
    if (demoBtnActive) {
      // short = Demo Toggle  
      if ((now - demoBtnPressStart) < DEMO_BTN_LONG_MS) {
        demoMode = !demoMode;
        needsRedraw = true;
        Serial.printf("[DEMO] Demo mode: %s\n", demoMode ?  "On" : "Off");
      }
      demoBtnActive = false;
    }
  }

  // Diary

  if (engMode) {
    if (needsRedraw || (now - lastDemoUpdate >= 400UL)) {
      drawDiaryMode(soundValue, ldrValue, pirDetected,
                          touchDetected, soundDB);
      lastDemoUpdate = now;
      needsRedraw    = false;
    }
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 25));
    FastLED.show();
    motorOff();
    return;
  }

  // State machine

  CatState newState = currentState;
  unsigned long sinceInteraction = now - lastInteractionTime;
  unsigned long sinceStateChange = now - stateEnteredTime;
  unsigned long sleepLimit = demoMode ? DEMO_SLEEP_MS : SLEEP_TIMEOUT_MS;

  // Priority order: Touch > Sound > Light > Sleep > Normal > Time
  if (touchDetected) {
    newState = STATE_HAPPY;

  } else if (soundDelta > SOUND_THRESHOLD) {
    newState = STATE_SCARED;

  } else if (ldrValue > LDR_BRIGHT_ON) {
    // Hysteresis: ON threshold for entry
    newState = STATE_DAZZLED;

  } else if (ldrValue < LDR_DARK_THRESHOLD) { 
    newState = STATE_SLEEP;

  } else if (!pirDetected && sinceInteraction >= sleepLimit) {
    newState = STATE_SLEEP;

  } else if (sinceInteraction >= NORMAL_TIMEOUT_MS) {
    newState = STATE_NORMAL;

  } else {
    // Exit when the state period expires.
    switch (currentState) {
      case STATE_SCARED:
        if (sinceStateChange >= SCARED_DURATION_MS)
          newState = STATE_NORMAL;
        break;
      case STATE_HAPPY:
        if (sinceStateChange >= HAPPY_DURATION_MS)
          newState = STATE_NORMAL;
        break;
      case STATE_DAZZLED:
        // Hysteresis: OFF threshold (lower) for output
        if (sinceStateChange >= DAZZLED_DURATION_MS &&
            ldrValue <= LDR_BRIGHT_OFF)
          newState = STATE_NORMAL;
        break;
      default:
        break;
    }
  }

  //state transition
  if (newState != currentState) {
    Serial.printf("[STATE] %s -> %s\n",
      stateNames[currentState], stateNames[newState]);

    //Count statistics
    if (newState == STATE_HAPPY)  rtcTouchCount++;
    if (newState == STATE_SCARED) rtcScaredCount++;

    currentState     = newState;
    stateEnteredTime = now;
    breathePhase     = 0.0f;
    eyesOpen         = true;
    lastBlinkTime    = now;
    needsRedraw      = true;
  }

  // Deep sleep

// Two-Stage Sleep
// Stage 1: Show face when sleeping. Awakened by PIR or touch.
// Stage 2: DEEP_SLEEP_DELAY_MS then deep sleep. 
// Light changes do not awaken (LDR oscillates around the threshold,
/ // turns NORMAL without showing face when sleeping).


  if (currentState == STATE_SLEEP) {

    if (touchDetected || pirDetected) {
      Serial.println("[SLEEP] Awakening was triggered.");
      currentState        = STATE_NORMAL;
      stateEnteredTime    = now;
      lastInteractionTime = now;
      ldrDarkSince        = 0;
      needsRedraw         = true;
      breathePhase        = 0.0f;
      eyesOpen            = true;
      lastBlinkTime       = now;
      FastLED.setBrightness(LED_BRIGHT);
    } else {
      //Draw the sleeping face (refresh with a ZZZ on first login or in SOFT_SLEEP_ZZZ_MS)
      if (needsRedraw || (now - lastBlinkTime >= SOFT_SLEEP_ZZZ_MS)) {
        drawSleepFace(CX, CY);
        lastBlinkTime = now;
        needsRedraw   = false;
      }
      aura_Night();
      motorOff();
      if (sinceStateChange >= DEEP_SLEEP_DELAY_MS) {
        enterDeepSleep();
      }
    }
    delay(8);
    return;
  }


  //Animation control

  switch (currentState) {

    case STATE_NORMAL:
      // Cropping: Every 4 seconds, 180ms off
      if (eyesOpen && (now - lastBlinkTime >= 4000UL)) {
        eyesOpen = false; lastBlinkTime = now; needsRedraw = true;
      } else if (!eyesOpen && (now - lastBlinkTime >= 180UL)) {
        eyesOpen = true;  needsRedraw = true;
      }
      break;

    case STATE_SCARED:
      // A jolt every 100ms
      if (now - lastShakeTime >= 100UL) {
        shakeDir    = !shakeDir;
        shakeOffset = shakeDir ? 3 : -3;
        lastShakeTime = now;
        needsRedraw = true;
      }
      break;

    case STATE_HAPPY:
      // A sparkle change every 700ms
      if (now - lastSparkleTime >= 700UL) {
        sparkleFrame = (sparkleFrame + 1) % 4;
        lastSparkleTime = now;
        needsRedraw = true;
      }
      break;

    case STATE_DAZZLED:
      // 
      if (now - lastSparkleTime >= 400UL) {
        sweepFrame++;
        lastSparkleTime = now;
        needsRedraw = true;
      }
      break;

    default:
      break;
  }

  //Display

  if (needsRedraw) {
    switch (currentState) {
      case STATE_HAPPY:
        drawHappyFace(CX, CY);
        break;
      case STATE_NORMAL:
        drawNormalFace(CX, CY, !eyesOpen);
        break;
      case STATE_SCARED:
        drawScaredFace(CX + shakeOffset, CY);
        break;
      case STATE_DAZZLED:
        drawDazzledFace(CX, CY);
        break;
      default:
        break;
    }
    needsRedraw    = false;
    prevDrawnState = currentState;
  }

  // LED aura

  switch (currentState) {
    case STATE_HAPPY:   aura_Happy();   break;
    case STATE_NORMAL:   aura_Normal();   break;
    case STATE_SCARED:  aura_Scared();  break;
    case STATE_DAZZLED: aura_Dazzled(); break;
    default:            aura_Off();     break;
  }

  //Motor 
  switch (currentState) {
    case STATE_HAPPY:  motorPurr(); break;
    case STATE_SCARED: motorFear(); break;
    default:           motorOff();  break;
  }

  // Serial debug

  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 3000UL) {
    lastDebug = now;
    Serial.printf(
      "[DBG] %-8s SOUND=%4d(~%2ddB) LDR=%4d PIR=%d TOUCH=%d "
      "DEMO=%s Uptime=%lus\n",
      stateNames[currentState],
      soundValue, (int)soundDB,
      ldrValue, pirDetected, touchDetected,
      demoMode ? "ON" : "OFF",
      now / 1000
    );
  }

  delay(8);   // Hardware watchdog yield 
}
