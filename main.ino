#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>

// 핀 정의
#define CLK1_PIN    4   // Display1 CLK (현재 시각)
#define DIO1_PIN    3   // Display1 DIO
#define CLK2_PIN    6   // Display2 CLK (급수 시각)
#define DIO2_PIN    5   // Display2 DIO
#define CLK3_PIN    8   // Display3 CLK (경과 시간)
#define DIO3_PIN    7   // Display3 DIO

#define BUTTON_PIN  2   // 급수 기록용 버튼

// 객체 생성
RTC_DS3231 rtc;
TM1637Display dispNow(CLK1_PIN, DIO1_PIN);
TM1637Display dispLast(CLK2_PIN, DIO2_PIN);
TM1637Display dispElapsed(CLK3_PIN, DIO3_PIN);

volatile bool flagPressed = false;
uint32_t baseMillis;
uint32_t baseMinuteCount;      // 부팅 시점의 총 분
uint32_t lastWaterMinuteCount; // 마지막 급수 시각의 총 분

void onPress() {
  flagPressed = true;
}

void setup() {
  Wire.begin();
  rtc.begin();

  // 버튼 인터럽트
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onPress, FALLING);

  // 시간 초기화
  DateTime now = rtc.now();
  baseMinuteCount      = now.hour() * 60 + now.minute();
  lastWaterMinuteCount = baseMinuteCount;
  baseMillis           = millis();

  // 디스플레이 밝기
  dispNow.setBrightness(0x0f);
  dispLast.setBrightness(0x0f);
  dispElapsed.setBrightness(0x0f);
}

void loop() {
  // 가속 “1초 = 1분” 카운트
  uint32_t accelMin = baseMinuteCount + (millis() - baseMillis) / 1000;

  // 버튼 눌림 처리: 급수 시각 갱신
  if (flagPressed) {
    lastWaterMinuteCount = accelMin;
    flagPressed = false;
  }

  // 1) 현재 가속 시각
  {
    uint16_t h = (accelMin / 60) % 24;
    uint16_t m = accelMin % 60;
    uint16_t val = h * 100 + m;
    dispNow.showNumberDecEx(val, 0b01000000, true);
  }

  // 2) 마지막 급수 가속 시각
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
