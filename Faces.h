//  face center CX=64, CY=68, radius CR=30

#pragma once

//Bold line 2 pixels

void thickLine(int x0,int y0,int x1,int y1,uint16_t c) {
  tft.drawLine(x0, y0,   x1, y1,   c);
  tft.drawLine(x0, y0+1, x1, y1+1, c);
}


// PLATFORM: head ears nose  whiskers
void drawBase(int cx, int cy, uint16_t bg = C_BLACK) {
  tft.fillScreen(bg);

  // Kafa
  tft.fillCircle(cx, cy, CR, C_WHITE);
  tft.drawCircle(cx, cy, CR,     C_LGRAY);
  tft.drawCircle(cx, cy, CR + 1, C_LGRAY);

  // Left ears
  tft.fillTriangle(cx-CR-2,cy-12, cx-14,cy-50, cx-6, cy-20, C_WHITE);
  tft.drawLine(cx-CR-2,cy-12, cx-14,cy-50, C_LGRAY);
  tft.drawLine(cx-14,  cy-50, cx-6, cy-20, C_LGRAY);
  tft.fillTriangle(cx-CR+2,cy-14, cx-14,cy-44, cx-8, cy-22, C_PINK);

  // right ears
  tft.fillTriangle(cx+6,cy-20, cx+14,cy-50, cx+CR+2,cy-12, C_WHITE);
  tft.drawLine(cx+6,  cy-20, cx+14,cy-50, C_LGRAY);
  tft.drawLine(cx+14, cy-50, cx+CR+2,cy-12, C_LGRAY);
  tft.fillTriangle(cx+8,cy-22, cx+14,cy-44, cx+CR-2,cy-14, C_PINK);

  // nose 
  tft.fillTriangle(cx-3,cy+5, cx+3,cy+5, cx,cy+1, C_PINK);

  // Whiskers 
  tft.drawLine(cx-28,cy+4,  cx-10,cy+5,  C_DGRAY);
  tft.drawLine(cx-28,cy+10, cx-10,cy+8,  C_DGRAY);
  tft.drawLine(cx+10,cy+5,  cx+28,cy+4,  C_DGRAY);
  tft.drawLine(cx+10,cy+8,  cx+28,cy+10, C_DGRAY);
}

// Happy face (^w^)

void drawHappyFace(int cx, int cy) {
  drawBase(cx, cy, C_BLACK);

  // Closed happy eyes — rendered as thick inverted-V
  for (int t = 0; t < 3; t++) {
    tft.drawLine(cx-16,cy-11+t, cx-10,cy-17+t, C_BLACK);
    tft.drawLine(cx-10,cy-17+t, cx-4, cy-11+t, C_BLACK);
    tft.drawLine(cx+4, cy-11+t, cx+10,cy-17+t, C_BLACK);
    tft.drawLine(cx+10,cy-17+t, cx+16,cy-11+t, C_BLACK);
  }

  // Blushing cheeks effect
  for (int r = 4; r < 7; r++) {
    tft.drawCircle(cx-20, cy+8, r, C_ORANGE);
    tft.drawCircle(cx+20, cy+8, r, C_ORANGE);
  }

  // Wide W-shaped smile
  for (int t = 0; t < 2; t++) {
    tft.drawLine(cx-14,cy+10+t, cx-7, cy+17+t, C_BLACK);
    tft.drawLine(cx-7, cy+17+t, cx,   cy+13+t, C_BLACK);
    tft.drawLine(cx,   cy+13+t, cx+7, cy+17+t, C_BLACK);
    tft.drawLine(cx+7, cy+17+t, cx+14,cy+10+t, C_BLACK);
  }

  // Rotating sparkle animation 
  static const int8_t spX[] = {-40, 40, -35, 38};
  static const int8_t spY[] = {-35, -38, -42, -30};
  for (int s = 0; s < 4; s++) {
    int px = cx + spX[s];
    int py = cy + spY[s];
    uint8_t a = (sparkleFrame == s) ? 4 : 2;
    tft.drawLine(px-a, py,   px+a, py,   C_YELLOW);
    tft.drawLine(px,   py-a, px,   py+a, C_YELLOW);
    if (sparkleFrame == s) {
      tft.drawPixel(px-2, py-2, C_YELLOW);
      tft.drawPixel(px+2, py+2, C_YELLOW);
      tft.drawPixel(px+2, py-2, C_YELLOW);
      tft.drawPixel(px-2, py+2, C_YELLOW);
    }
  }
}


//NORMAL / CALM FACE (=^.^=)

void drawNormalFace(int cx, int cy, bool blink) {
  drawBase(cx, cy, C_BLACK);

  if (blink) {
    // Winking: Cute horizontal lines
    thickLine(cx-16, cy-10, cx-6, cy-10, C_BLACK);
    thickLine(cx+6,  cy-10, cx+16, cy-10, C_BLACK);
  } else {
    // Sweet, big round eyes
    tft.fillCircle(cx-12, cy-11, 5, C_BLACK);
    tft.fillCircle(cx+12, cy-11, 5, C_BLACK);
    // shiny eyes
    tft.drawPixel(cx-13, cy-12, C_WHITE);
    tft.drawPixel(cx+11, cy-12, C_WHITE);
  }

  // cute little mouth  'w' 
  tft.drawLine(cx-8, cy+11, cx-4, cy+14, C_BLACK);
  tft.drawLine(cx-4, cy+14, cx,   cy+11, C_BLACK);
  tft.drawLine(cx,   cy+11, cx+4, cy+14, C_BLACK);
  tft.drawLine(cx+4, cy+14, cx+8, cy+11, C_BLACK);
}


// Scared face  (O_O) 

void drawScaredFace(int cx, int cy) {
  drawBase(cx, cy, C_BLACK);

  // bigg eyes
  auto drawEye = [&](int ex, int ey) {
    tft.fillCircle(ex, ey, 9, C_WHITE);
    tft.drawCircle(ex, ey, 9, C_BLACK);
    tft.drawCircle(ex, ey, 8, C_BLACK);
    tft.fillCircle(ex, ey, 4, C_BLACK);        // big pupil
    tft.fillCircle(ex-2, ey-2, 1, C_WHITE);    // shine
  };
  drawEye(cx-11, cy-11);
  drawEye(cx+11, cy-11);

  //Open mouth of panic
  tft.fillCircle(cx, cy+14, 5, C_BLACK);
  tft.drawCircle(cx, cy+14, 6, C_BLACK);

  // Titreme çizgileri
  tft.drawLine(cx-34,cy-18, cx-28,cy-12, C_LGRAY);
  tft.drawLine(cx-36,cy-12, cx-30,cy-6,  C_LGRAY);
  tft.drawLine(cx+28,cy-18, cx+34,cy-12, C_LGRAY);
  tft.drawLine(cx+30,cy-12, cx+36,cy-6,  C_LGRAY);
}

// Dazzled face  (-.-)  sweat  shine

void drawDazzledFace(int cx, int cy) {
  drawBase(cx, cy, C_BLACK);

// Narrowed eyes
  for (int t = 0; t < 3; t++) {
    tft.drawLine(cx-16,cy-12+t, cx-4, cy-15+t, C_BLACK);
    tft.drawLine(cx+4, cy-15+t, cx+16,cy-12+t, C_BLACK);
  }

  // surprised mouth
  tft.drawCircle(cx, cy+14, 3, C_BLACK);

  //Moving sweat droplet (sweepFrame)
  int dy = (int)(sweepFrame % 6) * 2;
  tft.fillCircle  (cx+25, cy-14+dy, 4, C_BLUE);
  tft.fillTriangle(cx+22, cy-18+dy, cx+28,cy-18+dy, cx+25,cy-26+dy, C_BLUE);

  // Corner shines
  tft.drawLine(cx-38,cy-44, cx-33,cy-39, C_YELLOW);
  tft.drawLine(cx-33,cy-44, cx-38,cy-39, C_YELLOW);
  tft.drawLine(cx+33,cy-44, cx+38,cy-39, C_YELLOW);
  tft.drawLine(cx+38,cy-44, cx+33,cy-39, C_YELLOW);
  // Forehead shine
  tft.fillCircle(cx-15, cy-18, 2, C_WHITE);
}


// sleepy face  (-.-)zzZ

void drawSleepFace(int cx, int cy) {
  tft.fillScreen(C_DKBLUE);

  tft.fillCircle(cx, cy, CR, C_WHITE);
  tft.drawCircle(cx, cy, CR,     C_LGRAY);
  tft.drawCircle(cx, cy, CR + 1, C_LGRAY);

  // ears
  tft.fillTriangle(cx-CR-2,cy-12, cx-14,cy-50, cx-6,cy-20, C_WHITE);
  tft.fillTriangle(cx-CR+2,cy-14, cx-14,cy-44, cx-8,cy-22, C_PINK);
  tft.fillTriangle(cx+6,cy-20, cx+14,cy-50, cx+CR+2,cy-12, C_WHITE);
  tft.fillTriangle(cx+8,cy-22, cx+14,cy-44, cx+CR-2,cy-14, C_PINK);

  // nose tingles
  tft.fillTriangle(cx-3,cy+5, cx+3,cy+5, cx,cy+1, C_PINK);
  tft.drawLine(cx-28,cy+4,  cx-10,cy+5,  C_DGRAY);
  tft.drawLine(cx-28,cy+10, cx-10,cy+8,  C_DGRAY);
  tft.drawLine(cx+10,cy+5,  cx+28,cy+4,  C_DGRAY);
  tft.drawLine(cx+10,cy+8,  cx+28,cy+10, C_DGRAY);

  // closed eyes
  for (int t = 0; t < 2; t++) {
    tft.drawLine(cx-16,cy-11+t, cx-10,cy-15+t, C_BLACK);
    tft.drawLine(cx-10,cy-15+t, cx-4, cy-11+t, C_BLACK);
    tft.drawLine(cx+4, cy-11+t, cx+10,cy-15+t, C_BLACK);
    tft.drawLine(cx+10,cy-15+t, cx+16,cy-11+t, C_BLACK);
  }

  // mouth
  tft.drawLine(cx-4, cy+13, cx+4, cy+13, C_BLACK);

  // ZZZ 
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(cx+20, cy-32); tft.print("z");
  tft.setTextSize(2);
  tft.setCursor(cx+26, cy-44); tft.print("Z");
  tft.setTextSize(1);
  tft.setCursor(cx+40, cy-55); tft.print("Z");
}


// END OF DAY SUMMARY

void drawDailySummary() {
  tft.fillScreen(C_BLACK);

  // Hearder
  tft.setTextColor(C_CYAN); tft.setTextSize(1);
  tft.setCursor(14, 4);
  tft.print("= GUN SONU =");
  tft.drawLine(0, 15, 128, 15, C_LGRAY);

  // Statistics
  tft.setTextColor(C_PINK);   tft.setCursor(4, 22); tft.print("Touch  : ");
  tft.setTextColor(C_WHITE);  tft.print(rtcTouchCount);

  tft.setTextColor(C_RED);    tft.setCursor(4, 36); tft.print("Fear  : ");
  tft.setTextColor(C_WHITE);  tft.print(rtcScaredCount);

  tft.setTextColor(C_GREEN);  tft.setCursor(4, 50); tft.print("Move: ");
  tft.setTextColor(C_WHITE);  tft.print(rtcMotionCount);

  tft.setTextColor(C_YELLOW); tft.setCursor(4, 64); tft.print("Wake : ");
  tft.setTextColor(C_WHITE);  tft.print(rtcWakeCount);

  tft.setTextColor(C_LGRAY);  tft.setCursor(4, 78); tft.print("Uptime : ");
  tft.setTextColor(C_WHITE);
  uint32_t tot = rtcUptimeSec + millis() / 1000;
  if (tot < 60) { tft.print(tot); tft.print("s"); }
  else          { tft.print(tot/60); tft.print("dk "); tft.print(tot%60); tft.print("s"); }

  // Mini bar chart 
  tft.drawLine(0, 89, 128, 89, C_LGRAY);
  int total = rtcTouchCount + rtcScaredCount + rtcMotionCount;
  if (total > 0) {
    int w1 = (rtcTouchCount  * 118) / total;
    int w2 = (rtcScaredCount * 118) / total;
    int w3 = (rtcMotionCount * 118) / total;
    tft.fillRect(5,          92, w1, 9, C_PINK);
    tft.fillRect(5 + w1,     92, w2, 9, C_RED);
    tft.fillRect(5 + w1 + w2,92, w3, 9, C_GREEN);
    tft.drawRect(5, 92, 118, 9, C_LGRAY);
  }

  // sleep message
  tft.setTextColor(0x8410); tft.setCursor(10, 105);
  tft.print("(=^.^=) zZz...");
  tft.setTextColor(C_DGRAY); tft.setCursor(2, 116);
  tft.print("Wake me up with PIR!");
}


// Wake up sreen (after deep sleep)

void drawWakeupScreen() {
  tft.fillScreen(C_BLACK);

  tft.setTextColor(C_YELLOW); tft.setTextSize(1);
  tft.setCursor(16, 8);  tft.print("I'm backk!");

  tft.setTextColor(C_WHITE);
  tft.setCursor(28, 24); tft.print("(^-^)/");

  tft.drawLine(0, 36, 128, 36, C_LGRAY);

  tft.setTextColor(C_LGRAY);
  tft.setCursor(4, 44); tft.print("Sleep : ");
  tft.setTextColor(C_WHITE); tft.println(rtcWakeCount);

  tft.setTextColor(C_LGRAY);
  tft.setCursor(4, 58); tft.print("Touch  : ");
  tft.setTextColor(C_PINK); tft.println(rtcTouchCount);

  tft.setTextColor(C_LGRAY);
  tft.setCursor(4, 72); tft.print("Scared  : ");
  tft.setTextColor(C_RED); tft.println(rtcScaredCount);

  tft.setTextColor(C_LGRAY);
  tft.setCursor(4, 86); tft.print("Movement: ");
  tft.setTextColor(C_GREEN); tft.println(rtcMotionCount);

  tft.setTextColor(C_LGRAY);
  tft.setCursor(4,100); tft.print("Uptime : ");
  tft.setTextColor(C_WHITE);
  uint32_t tot = rtcUptimeSec;
  if (tot < 60) { tft.print(tot); tft.print("s"); }
  else          { tft.print(tot/60); tft.print("minute"); }
}

// Diary mode
void drawDiaryMode(int sound, int ldr, bool pir, bool touch, float db) {
  tft.fillScreen(C_BLACK);

  tft.setTextColor(C_GREEN); tft.setTextSize(1);
  tft.setCursor(4, 3); tft.print("DIARY");

  tft.setTextColor(C_WHITE);

  // Sound + dB
  tft.setCursor(4, 17); tft.print("Sound   : ");
  tft.print(sound);
  tft.print(" ~");
  tft.print((int)db);
  tft.print("dB");
  if (abs(sound - soundBaseline) > SOUND_THRESHOLD) {
    tft.setTextColor(C_RED); tft.print("!"); tft.setTextColor(C_WHITE);
  }

  // LDR
  tft.setCursor(4, 30); tft.print("LDR   : "); tft.print(ldr);
  if (ldr > LDR_BRIGHT_ON) {
    tft.setTextColor(C_YELLOW); tft.print("!"); tft.setTextColor(C_WHITE);
  }

  // PIR
  tft.setCursor(4, 43); tft.print("PIR   : ");
  tft.setTextColor(pir ? C_GREEN : C_DGRAY);
  tft.print(pir ? "Movement" : "Silence");
  tft.setTextColor(C_WHITE);

  // Touch
  tft.setCursor(4, 56); tft.print("Touch : ");
  tft.setTextColor(touch ? C_GREEN : C_DGRAY);
  tft.print(touch ? "VAR" : "YOK");
  tft.setTextColor(C_WHITE);

  // Situation
  tft.setCursor(4, 69); tft.print("Statue : ");
  tft.setTextColor(C_CYAN); tft.print(stateNames[currentState]);
  tft.setTextColor(C_WHITE);

  // Baseline + uptime
  tft.setCursor(4, 82); tft.print("BASE  : "); tft.print(soundBaseline);
  tft.setCursor(4, 95); tft.print("UPTIME: ");
  tft.print(millis() / 1000); tft.print("s");

  // RTC summary
  tft.setTextColor(C_DGRAY);
  tft.setCursor(4,108);
  tft.print("D:"); tft.print(rtcTouchCount);
  tft.print(" K:"); tft.print(rtcScaredCount);
  tft.print(" H:"); tft.print(rtcMotionCount);
  tft.print(" U:"); tft.print(rtcWakeCount);

  // Demo mode
  tft.setCursor(4, 121);
  tft.setTextColor(demoMode ? C_YELLOW : C_DGRAY);
  tft.print(demoMode ? "DEMO:On(30s)" : "DEMO:Off");
}
