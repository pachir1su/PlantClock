#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>
#include <LiquidCrystal_I2C.h>

// FND (2개만 사용)
#define CLK2_PIN    6
#define DIO2_PIN    5
#define CLK3_PIN    8
#define DIO3_PIN    7

#define BUTTON_PIN    2   // 급수 버튼

// 팬, 레버, RGB LED
#define LEVER_PIN     12  // 레버(팬 ON/OFF)
#define FAN_INA_PIN   A2  // 팬 INA (YUROBOT/L9110S)
#define FAN_INB_PIN   A3  // 팬 INB (YUROBOT/L9110S)
#define RGB_B_PIN     9
#define RGB_G_PIN     10
#define RGB_R_PIN     11

// LCD, 센서
#define SOIL_PIN      A0   // 토양 습도 센서 (A0, 그대로!)
LiquidCrystal_I2C lcd(0x20, 16, 2);

// ---- 알림 장치 ----
#define VIB_MOTOR_PIN  3    // 진동 모터 (D3)
#define ALERT_LED_PIN  4    // LED (D4)
#define BUZZER_PIN     A1   // 부저 (A1, 디지털 출력!)

RTC_DS3231 rtc;
TM1637Display dispLast(CLK2_PIN, DIO2_PIN);
TM1637Display dispElapsed(CLK3_PIN, DIO3_PIN);

volatile bool flagPressed = false;
uint32_t baseMillis;
uint32_t baseMinuteCount;
uint32_t lastWaterMinuteCount;

bool isFanOn = false;

// ---- 사용자 함수 ----
void onPress() { flagPressed = true; }
void setFanOn(bool on) {
  if (on) {
    digitalWrite(FAN_INA_PIN, HIGH);
    digitalWrite(FAN_INB_PIN, LOW);
    isFanOn = true;
  } else {
    digitalWrite(FAN_INA_PIN, LOW);
    digitalWrite(FAN_INB_PIN, LOW);
    isFanOn = false;
  }
}
void setFanLedColor(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
}

void setup() {
  Wire.begin();
  rtc.begin();
  lcd.init();
  lcd.backlight();

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

  pinMode(VIB_MOTOR_PIN, OUTPUT);
  pinMode(ALERT_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  setFanOn(false);
  setFanLedColor(255,0,0);

  DateTime now = rtc.now();
  baseMinuteCount      = now.hour() * 60 + now.minute();
  lastWaterMinuteCount = baseMinuteCount;
  baseMillis           = millis();
}

void loop() {
  uint32_t accelMin = baseMinuteCount + (millis() - baseMillis) / 1000;

  if (flagPressed) {
    lastWaterMinuteCount = accelMin;
    flagPressed = false;
  }

  // 팬 제어
  if (digitalRead(LEVER_PIN) == LOW) {
    setFanOn(false);
    setFanLedColor(255,0,0);
  } else {
    setFanOn(true);
    setFanLedColor(0,255,0);
  }

  // FND 2개 표시
  uint16_t last_h = (lastWaterMinuteCount / 60) % 24;
  uint16_t last_m = lastWaterMinuteCount % 60;
  uint16_t last_val = last_h * 100 + last_m;
  dispLast.showNumberDecEx(last_val, 0b01000000, true);

  uint32_t diff = accelMin - lastWaterMinuteCount;
  if (diff > 9999) diff = 9999;
  uint16_t elapsed_val = diff % 10000;
  dispElapsed.showNumberDec(elapsed_val, false);

  // 토양 습도 LCD 표시 (A0)
  int soilVal = analogRead(SOIL_PIN);
  lcd.setCursor(0,0);
  lcd.print("Soil:");
  lcd.print(soilVal);
  lcd.print("    ");

  // LCD 현재 시각 표시
  lcd.setCursor(0,1);
  uint16_t h = (accelMin / 60) % 24;
  uint16_t m = accelMin % 60;
  lcd.print("Time:");
  if (h < 10) lcd.print("0");
  lcd.print(h);
  lcd.print(":");
  if (m < 10) lcd.print("0");
  lcd.print(m);
  lcd.print("   ");

  // ------ 30초(30분)가 지나면 LED 깜빡 + 진동 + 부저 ------
  if (diff >= 30) {
    // LED 깜빡이기 (0.2초 ON, 0.2초 OFF)
    digitalWrite(ALERT_LED_PIN, HIGH);
    digitalWrite(VIB_MOTOR_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(200);

    digitalWrite(ALERT_LED_PIN, LOW);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  } else {
    digitalWrite(ALERT_LED_PIN, LOW);
    digitalWrite(VIB_MOTOR_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}
