#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>

// ---- TM1637 FND 핀 정의 (3개) ----
#define CLK1_PIN    4
#define DIO1_PIN    3
#define CLK2_PIN    6
#define DIO2_PIN    5
#define CLK3_PIN    8
#define DIO3_PIN    7

#define BUTTON_PIN    2   // 급수 버튼

// ---- 팬, 레버, RGB LED ----
#define LEVER_PIN     12  // 레버(팬 ON/OFF)
#define FAN_INA_PIN   A2  // 팬 INA (YUROBOT/L9110S)
#define FAN_INB_PIN   A3  // 팬 INB (YUROBOT/L9110S)

#define RGB_B_PIN     9
#define RGB_G_PIN     10
#define RGB_R_PIN     11

// ---- 객체 생성 ----
RTC_DS3231 rtc;
TM1637Display dispNow(CLK1_PIN, DIO1_PIN);
TM1637Display dispLast(CLK2_PIN, DIO2_PIN);
TM1637Display dispElapsed(CLK3_PIN, DIO3_PIN);

volatile bool flagPressed = false;
uint32_t baseMillis;
uint32_t baseMinuteCount;
uint32_t lastWaterMinuteCount;

// 팬 상태
bool isFanOn = false;

// ====== 함수 ======
void onPress() { flagPressed = true; }

// 팬 ON/OFF 제어
void setFanOn(bool on) {
  if (on) {
    digitalWrite(FAN_INA_PIN, HIGH);
    digitalWrite(FAN_INB_PIN, LOW);  // 정회전(ON)
    isFanOn = true;
  } else {
    digitalWrite(FAN_INA_PIN, LOW);
    digitalWrite(FAN_INB_PIN, LOW);  // 정지
    isFanOn = false;
  }
}

// 공통 캐소드 RGB LED 제어
void setFanLedColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
}

void setup() {
  Wire.begin();
  rtc.begin();

  dispNow.setBrightness(0x0f);
  dispLast.setBrightness(0x0f);
  dispElapsed.setBrightness(0x0f);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onPress, FALLING);

  pinMode(LEVER_PIN, INPUT_PULLUP);
  pinMode(FAN_INA_PIN, OUTPUT);
  pinMode(FAN_INB_PIN, OUTPUT);

  pinMode(RGB_R_PIN, OUTPUT);
  pinMode(RGB_G_PIN, OUTPUT);
  pinMode(RGB_B_PIN, OUTPUT);

  setFanOn(false);           // 초기 팬 OFF
  setFanLedColor(255,0,0);   // 팬 OFF 빨강

  DateTime now = rtc.now();
  baseMinuteCount      = now.hour() * 60 + now.minute();
  lastWaterMinuteCount = baseMinuteCount;
  baseMillis           = millis();
}

void loop() {
  // ---- 타이머 가속(1초 = 1분) ----
  uint32_t accelMin = baseMinuteCount + (millis() - baseMillis) / 1000;

  // ---- 버튼 급수 처리 ----
  if (flagPressed) {
    lastWaterMinuteCount = accelMin;
    flagPressed = false;
  }

  // ---- 레버 스위치로 팬 제어 (ON=팬ON, OFF=팬OFF) ----
  if (digitalRead(LEVER_PIN) == LOW) { // OFF(풀림)
    setFanOn(false);
    setFanLedColor(255,0,0);   // 팬 ON - 빨강
  } else { // ON(눌림)
    setFanOn(true);
    setFanLedColor(0,255,0);   // 팬 OFF - 초록
  }

  // ---- TM1637 3개 표시 ----
  // 1) 현재 가속 시각
  {
    uint16_t h = (accelMin / 60) % 24;
    uint16_t m = accelMin % 60;
    uint16_t val = h * 100 + m;
    dispNow.showNumberDecEx(val, 0b01000000, true);
  }

  // 2) 마지막 급수 시각
  {
    uint16_t h = (lastWaterMinuteCount / 60) % 24;
    uint16_t m = lastWaterMinuteCount % 60;
    uint16_t val = h * 100 + m;
    dispLast.showNumberDecEx(val, 0b01000000, true);
  }

  // 3) 경과 시간 (MM:00)
  {
    uint32_t diff = accelMin - lastWaterMinuteCount;
    if (diff > 9999) diff = 9999;
    uint16_t val = diff % 10000;
    dispElapsed.showNumberDec(val, false);
  }

  delay(200);
}
