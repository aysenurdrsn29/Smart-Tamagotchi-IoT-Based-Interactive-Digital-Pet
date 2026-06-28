// ════════════════════════════════════════════════════════════════
//  Smart Tamagotchi  —  Tam Sürüm
//  MEF Üniversitesi EE308 — Proje
//
//  PIN TABLOSU:
//   TFT  CS=5  RST=4  DC=2  MOSI=23  SCK=18
//   HW-139 Touch  → GPIO 33
//   Ses Sensörü   → GPIO 32 (analog)
//   PIR HW-416A   → GPIO 27 (5V besle)
//   LDR           → GPIO 34 (10K pull-down GND'ye)
//   WS2812B DIN   → GPIO 13 (5V/VIN besle)
//   Vibrasyon     → GPIO 12 (2N2222 base, 1K direnç)
//   Boot butonu   → GPIO 0  (Mühendislik modu)
//   Ekstra buton  → GPIO 25 (Demo modu / Zorla uyku)
//
//  KÜTÜPHANELEr (Library Manager):
//   Adafruit GFX Library
//   Adafruit ST7735 and ST7789 Library
//   FastLED
// ════════════════════════════════════════════════════════════════

// ─── SİSTEM INCLUDE'LARI ─────────────────────────────────────────
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <FastLED.h>
#include <esp_sleep.h>
#include <WiFi.h>
#include <math.h>

// ─── PİN TANIMLARI ───────────────────────────────────────────────
#define TFT_CS      5
#define TFT_RST     4
#define TFT_DC      2
// MOSI=23, SCK=18 — Hardware SPI (otomatik)

#define TOUCH_PIN   33    // HW-139 dijital çıkış (HIGH = var)
#define SOUND_PIN   32    // Ses sensörü analog
#define PIR_PIN     27    // PIR dijital (HIGH = hareket)
#define LDR_PIN     34    // LDR analog, input-only
#define LED_PIN     13    // WS2812B
#define MOTOR_PIN   12    // 2N2222 base (1K direnç üstünden)
#define BOOT_BTN    0     // Mühendislik modu
#define DEMO_BTN    25    // Demo modu / zorla uyku

// ─── LED ─────────────────────────────────────────────────────────
#define NUM_LEDS      16
#define LED_BRIGHT    80

// ─── SENSÖR EŞİKLERİ ─────────────────────────────────────────────
#define SOUND_THRESHOLD     1000   // Baseline'dan ADC sapması
#define SOUND_MEMORY_MS     800   // Ses pikini bu süre tut (ms)
#define LDR_BRIGHT_ON       3700  // DAZZLED giriş eşiği
#define LDR_BRIGHT_OFF      3500  // DAZZLED çıkış eşiği (histerezis)

// ─── ZAMAN SABITLERI ─────────────────────────────────────────────
#define BORED_TIMEOUT_MS    300000UL   // 5 dakika
#define SLEEP_TIMEOUT_MS    900000UL   // 15 dakika
#define DEMO_SLEEP_MS       30000UL    // Demo: 30 saniye
#define SCARED_DURATION_MS  3000UL
#define HAPPY_DURATION_MS   6000UL
#define DAZZLED_DURATION_MS 4000UL
#define DEMO_BTN_LONG_MS    2000UL     // Uzun basma süresi
#define DEEP_SLEEP_DELAY_MS 60000UL    // Soft uyku -> deep sleep (1 dakika)
#define SOFT_SLEEP_ZZZ_MS   3000UL     // ZZZ animasyon yenileme

// ─── RGB565 RENKLER ──────────────────────────────────────────────
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

// ─── YÜZ MERKEZİ ─────────────────────────────────────────────────
#define CX  64
#define CY  68
#define CR  30

// ─── STATE MACHINE ────────────────────────────────────────────────
enum CatState : uint8_t {
  STATE_HAPPY   = 0,
  STATE_BORED   = 1,
  STATE_SCARED  = 2,
  STATE_DAZZLED = 3,
  STATE_SLEEP   = 4
};

// ─── RTC BELLEĞİ: deep sleep'ten sonra da korunur ────────────────
RTC_DATA_ATTR int      rtcTouchCount  = 0;
RTC_DATA_ATTR int      rtcScaredCount = 0;
RTC_DATA_ATTR int      rtcMotionCount = 0;
RTC_DATA_ATTR int      rtcWakeCount   = 0;
RTC_DATA_ATTR uint32_t rtcUptimeSec   = 0;

// ─── GLOBAL DEĞİŞKENLER ──────────────────────────────────────────
CatState currentState   = STATE_BORED;
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

const char* stateNames[]  = {"MUTLU","SIKGIN","KORKMUS","KAMASIK","UYKU"};
const char* stateColors[] = {"PEMBE","AMBER","KIRMIZI","BEYAZ","KAPALI"};

// ─── NESNELER ────────────────────────────────────────────────────
Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);
CRGB leds[NUM_LEDS];

// ─── LOCAL HEADER'LAR (global değişkenlerden sonra include edilmeli)
#include "aura.h"
#include "faces.h"

// ════════════════════════════════════════════════════════════════
//  YARDIMCI FONKSİYONLAR
// ════════════════════════════════════════════════════════════════

// Sesden tahmini dB hesapla (30–95 dB arası göreli ölçek)
float calcDB(int adcDelta) {
  if (adcDelta <= 0) return 30.0f;
  float db = 30.0f + 20.0f * log10f((float)adcDelta / 5.0f + 1.0f);
  return db > 95.0f ? 95.0f : db;
}

// ════════════════════════════════════════════════════════════════
//  DERİN UYKU
// ════════════════════════════════════════════════════════════════
void enterDeepSleep() {
  Serial.println("[SLEEP] Preparing for deep sleep...");

  // Uptime biriktir
  rtcUptimeSec += millis() / 1000;

  // Gün sonu özetini göster
  drawDailySummary();

  // LED'leri yavaşça söndür
  for (int b = FastLED.getBrightness(); b >= 0; b -= 4) {
    FastLED.setBrightness(b);
    FastLED.show();
    delay(20);
  }
  motorOff();

  delay(4000);           // Özeti 4 sn göster
  tft.fillScreen(C_BLACK);

  // PIR HIGH → uyan
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 1);

  Serial.printf("[SLEEP] RTC: dok=%d urk=%d har=%d uyan=%d uptime=%lus\n",
    rtcTouchCount, rtcScaredCount, rtcMotionCount,
    rtcWakeCount, (unsigned long)rtcUptimeSec);
  Serial.flush();

  esp_deep_sleep_start();
  // Buraya asla ulaşılmaz
}

// ════════════════════════════════════════════════════════════════
//  SETUP
// ════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n[BOOT] Smart Tamagotchi basliyor...");

  // Güç tasarrufu: radyoları kapat
  WiFi.mode(WIFI_OFF);
  btStop();

  // Pin modları
  pinMode(TOUCH_PIN, INPUT);
  pinMode(PIR_PIN,   INPUT);
  pinMode(BOOT_BTN,  INPUT_PULLUP);
  pinMode(DEMO_BTN,  INPUT_PULLUP);
  pinMode(MOTOR_PIN, OUTPUT);
  digitalWrite(MOTOR_PIN, LOW);

  // TFT başlat
  tft.initR(INITR_144GREENTAB);  // Sorun çıkarsa INITR_BLACKTAB dene
  tft.setRotation(1);            // 0/1/2/3 — ters gelirse değiştir
  tft.fillScreen(C_BLACK);

  // Açılış ekranı
  tft.setTextColor(C_CYAN); tft.setTextSize(2);
  tft.setCursor(10, 38); tft.print("Nyaaaa~");
  tft.setTextSize(1);
  tft.setTextColor(C_LGRAY);
  tft.setCursor(18, 66); tft.print("Tamagotchi v2");
  delay(1800);

  // FastLED başlat
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHT);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
  delay(2000);
  // ── Ses baseline kalibrasyonu ────────────────────────────────
  tft.fillScreen(C_BLACK);
  tft.setTextColor(C_PINK); tft.setTextSize(1);
  tft.setCursor(10, 55); tft.print("Kalibrasyon...");
  tft.setCursor(6, 70);  tft.print("Sessiz olun (1sn)");
  delay(1000);
  long _sum = 0;
  for (int i = 0; i < 200; i++) {
    _sum += analogRead(SOUND_PIN);
    delay(5);
  }
  soundBaseline = (int)(_sum / 200);
  Serial.printf("[CAL] Ses baseline: %d\n", soundBaseline);

  // ── Uyanma nedeni ─────────────────────────────────────────────
  esp_sleep_wakeup_cause_t wakeReason = esp_sleep_get_wakeup_cause();
  rtcWakeCount++;

  if (wakeReason == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[WAKE] PIR interrupt ile uyandim!");
    drawWakeupScreen();
    delay(3000);
    currentState = STATE_HAPPY;
  } else {
    Serial.println("[WAKE] Normal baslatma.");
    currentState = STATE_BORED;
  }

  // Demo butonu basılı başlatılırsa demo modu aç
  if (digitalRead(DEMO_BTN) == LOW) {
    demoMode = true;
    Serial.println("[DEMO] Demo modu ACIK — uyku suresi 30sn");
    tft.fillScreen(C_BLACK);
    tft.setTextColor(C_YELLOW); tft.setTextSize(1);
    tft.setCursor(14, 55); tft.print("DEMO MODU ACIK");
    tft.setCursor(8, 70);  tft.print("Uyku: 30 saniye");
    delay(1500);
  }

  lastInteractionTime = millis();
  stateEnteredTime    = millis();
  needsRedraw         = true;
  Serial.println("[BOOT] Hazir!");
}

// ════════════════════════════════════════════════════════════════
//  ANA DÖNGÜ
// ════════════════════════════════════════════════════════════════
void loop() {
  unsigned long now = millis();

  // ══ 1. SENSÖR OKUMA ════════════════════════════════════════════

  bool touchDetected = (digitalRead(TOUCH_PIN) == HIGH);
  bool pirDetected   = (digitalRead(PIR_PIN)   == HIGH);
  bool bootPressed   = (digitalRead(BOOT_BTN)  == LOW);
  bool demoPressed   = (digitalRead(DEMO_BTN)  == LOW);
  int  ldrValue      = analogRead(LDR_PIN);

   // Ses: 50 örnekten baseline'dan maksimum sapmayı bul (~10ms pencere)
  // MAX9814 gibi sensörler VCC/2 etrafında salınım yapar;
  // mutlak sapma (delta) her iki yönde de sesi doğru yakalar.
  int maxDelta = 0;
  for (int i = 0; i < 50; i++) {
    int s = analogRead(SOUND_PIN);
    int currentDelta = abs(s - soundBaseline);
    if (currentDelta > maxDelta) maxDelta = currentDelta;
    delayMicroseconds(200);
  }
  // Piki SOUND_MEMORY_MS boyunca hatırla
  if (maxDelta > soundPeak) {
    soundPeak     = maxDelta;
    lastSoundTime = now;
  }
  if (now - lastSoundTime > SOUND_MEMORY_MS) {
    soundPeak = 0;
  }
  int soundDelta = soundPeak;   // Doğrudan delta — baseline'dan sapma
  int soundValue = soundPeak;
  soundDB = calcDB(soundDelta);

  // ══ 2. ETKİLEŞİM ZAMANI ═══════════════════════════════════════

  if (touchDetected || pirDetected) {
    if (pirDetected && !touchDetected) {
      // PIR'i fazla saymamak için: önceki hareketten en az 2sn geçmeli
      static unsigned long lastPIRcount = 0;
      if (now - lastPIRcount > 2000) {
        rtcMotionCount++;
        lastPIRcount = now;
      }
    }
    lastInteractionTime = now;
  }

  // ══ 3. BOOT BUTONU: Mühendislik Modu ══════════════════════════

  if (bootPressed && (now - lastEngDebounce >= 500UL)) {
    engMode = !engMode;
    lastEngDebounce = now;
    needsRedraw = true;
    Serial.printf("[ENG] Muhendislik modu: %s\n", engMode ? "ACIK" : "KAPALI");
  }

  // ══ 4. DEMO BUTONU: Kısa=Demo Toggle / Uzun=Zorla Uyku ════════

  if (demoPressed) {
    if (!demoBtnActive) {
      demoBtnActive    = true;
      demoBtnPressStart = now;
    }
    // Uzun basma: anında uyku
    if ((now - demoBtnPressStart) >= DEMO_BTN_LONG_MS) {
      Serial.println("[DEMO] Zorla uyku tetiklendi.");
      demoBtnActive = false;
      enterDeepSleep();
    }
  } else {
    if (demoBtnActive) {
      // Kısa bırakma: demo mode toggle
      if ((now - demoBtnPressStart) < DEMO_BTN_LONG_MS) {
        demoMode = !demoMode;
        needsRedraw = true;
        Serial.printf("[DEMO] Demo modu: %s\n", demoMode ? "ACIK" : "KAPALI");
      }
      demoBtnActive = false;
    }
  }

  // ══ 5. MÜHENDİSLİK MODU EKRANI ════════════════════════════════

  if (engMode) {
    if (needsRedraw || (now - lastDemoUpdate >= 400UL)) {
      drawEngineeringMode(soundValue, ldrValue, pirDetected,
                          touchDetected, soundDB);
      lastDemoUpdate = now;
      needsRedraw    = false;
    }
    fill_solid(leds, NUM_LEDS, CRGB(0, 0, 25));
    FastLED.show();
    motorOff();
    return;
  }

  // ══ 6. STATE MACHINE GEÇİŞLERİ ════════════════════════════════

  CatState newState = currentState;
  unsigned long sinceInteraction = now - lastInteractionTime;
  unsigned long sinceStateChange = now - stateEnteredTime;
  unsigned long sleepLimit = demoMode ? DEMO_SLEEP_MS : SLEEP_TIMEOUT_MS;

  // Öncelik sırası: Dokunma > Ses > Işık > Uyku > Sıkılma > Süre
  if (touchDetected) {
    newState = STATE_HAPPY;

  } else if (soundDelta > SOUND_THRESHOLD) {
    newState = STATE_SCARED;

  } else if (ldrValue > LDR_BRIGHT_ON) {
    // Histerezis: giriş için ON eşiği
    newState = STATE_DAZZLED;

  } else if (!pirDetected && sinceInteraction >= sleepLimit) {
    newState = STATE_SLEEP;

  } else if (sinceInteraction >= BORED_TIMEOUT_MS) {
    newState = STATE_BORED;

  } else {
    // State süresi dolunca çıkış
    switch (currentState) {
      case STATE_SCARED:
        if (sinceStateChange >= SCARED_DURATION_MS)
          newState = STATE_BORED;
        break;
      case STATE_HAPPY:
        if (sinceStateChange >= HAPPY_DURATION_MS)
          newState = STATE_BORED;
        break;
      case STATE_DAZZLED:
        // Histerezis: çıkış için OFF eşiği (daha düşük)
        if (sinceStateChange >= DAZZLED_DURATION_MS &&
            ldrValue <= LDR_BRIGHT_OFF)
          newState = STATE_BORED;
        break;
      default:
        break;
    }
  }

  // State geçişi
  if (newState != currentState) {
    Serial.printf("[STATE] %s -> %s\n",
      stateNames[currentState], stateNames[newState]);

    // İstatistik say
    if (newState == STATE_HAPPY)  rtcTouchCount++;
    if (newState == STATE_SCARED) rtcScaredCount++;

    currentState     = newState;
    stateEnteredTime = now;
    breathePhase     = 0.0f;
    eyesOpen         = true;
    lastBlinkTime    = now;
    needsRedraw      = true;
  }

  // ══ 7. İKİ AŞAMALI UYKU ══════════════════════════════
  //  Aş.1: Yumusak uyku -- uyku yüzü göster, PIR ile uyanilabilir
  //  Aş.2: 1 dakika sonra --> deep sleep

  if (currentState == STATE_SLEEP) {
    if (pirDetected || touchDetected) {
      Serial.println("[SLEEP] Hareket algilandi, uyaniyorum!");
      currentState        = STATE_BORED;
      stateEnteredTime    = now;
      lastInteractionTime = now;
      needsRedraw         = true;
      breathePhase        = 0.0f;
      eyesOpen            = true;
      lastBlinkTime       = now;
      FastLED.setBrightness(LED_BRIGHT);
    } else {
      if (needsRedraw || (now - lastBlinkTime >= SOFT_SLEEP_ZZZ_MS)) {
        drawSleepFace(CX, CY);
        lastBlinkTime = now;
        needsRedraw   = false;
      }
      aura_Off();
      motorOff();
      if (sinceStateChange >= DEEP_SLEEP_DELAY_MS) {
        enterDeepSleep();
      }
    }
    delay(8);
    return;
  }

  // ══ 8. ANİMASYON FRAME KONTROLÜ ════════════════════════════════

  switch (currentState) {

    case STATE_BORED:
      // Kırpma: 4sn'de bir, 180ms kapalı
      if (eyesOpen && (now - lastBlinkTime >= 4000UL)) {
        eyesOpen = false; lastBlinkTime = now; needsRedraw = true;
      } else if (!eyesOpen && (now - lastBlinkTime >= 180UL)) {
        eyesOpen = true;  needsRedraw = true;
      }
      break;

    case STATE_SCARED:
      // 100ms'de bir sarsılma
      if (now - lastShakeTime >= 100UL) {
        shakeDir    = !shakeDir;
        shakeOffset = shakeDir ? 3 : -3;
        lastShakeTime = now;
        needsRedraw = true;
      }
      break;

    case STATE_HAPPY:
      // 700ms'de bir sparkle değişimi
      if (now - lastSparkleTime >= 700UL) {
        sparkleFrame = (sparkleFrame + 1) % 4;
        lastSparkleTime = now;
        needsRedraw = true;
      }
      break;

    case STATE_DAZZLED:
      // 400ms'de bir ter damlası animasyonu
      if (now - lastSparkleTime >= 400UL) {
        sweepFrame++;
        lastSparkleTime = now;
        needsRedraw = true;
      }
      break;

    default:
      break;
  }

  // ══ 9. EKRAN ÇIZIMI ════════════════════════════════════════════

  if (needsRedraw) {
    switch (currentState) {
      case STATE_HAPPY:
        drawHappyFace(CX, CY);
        break;
      case STATE_BORED:
        drawBoredFace(CX, CY, !eyesOpen);
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

  // ══ 10. LED AURASI ═════════════════════════════════════════════

  switch (currentState) {
    case STATE_HAPPY:   aura_Happy();   break;
    case STATE_BORED:   aura_Bored();   break;
    case STATE_SCARED:  aura_Scared();  break;
    case STATE_DAZZLED: aura_Dazzled(); break;
    default:            aura_Off();     break;
  }

  // ══ 11. MOTOR ══════════════════════════════════════════════════

  switch (currentState) {
    case STATE_HAPPY:  motorPurr(); break;
    case STATE_SCARED: motorFear(); break;
    default:           motorOff();  break;
  }

  // ══ 12. SERIAL DEBUG (3sn'de bir) ═════════════════════════════

  static unsigned long lastDebug = 0;
  if (now - lastDebug >= 3000UL) {
    lastDebug = now;
    Serial.printf(
      "[DBG] %-8s SES=%4d(~%2ddB) LDR=%4d PIR=%d DOK=%d "
      "DEMO=%s Uptime=%lus\n",
      stateNames[currentState],
      soundValue, (int)soundDB,
      ldrValue, pirDetected, touchDetected,
      demoMode ? "ON" : "OFF",
      now / 1000
    );
  }

  delay(8);   // Watchdog besleme
}