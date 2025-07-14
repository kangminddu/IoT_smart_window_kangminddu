#include <DHT.h>
#include <SoftwareSerial.h>

// --- 핀 정의 ---
#define IN1 11        // 스텝모터 핀 연결 변수
#define IN2 10
#define IN3 9
#define IN4 8

#define LIMIT_PIN1 12    // 리미트 스위치 (열림)
#define LIMIT_PIN2 13    // 리미트 스위치 (닫힘)

#define RAIN_SENSOR A0 // 레인센서
#define GAS_SENSOR A1 // 가스센서
#define CDS_SENSOR A3 // 조도센서
#define DHT_PIN 2 // 온습도 센서
#define DHT_TYPE DHT11 

#define LIMIT_RAIN_VALUE 500   // 이 값을 넘어가면 창문이 닫힘
#define LIMIT_GAS_VALUE 500    // 이 값을 넘어가면 창문이 닫힘
#define CDS_VALUE_LIMIT 850    // 이 값보다 떨어지면 창문이 닫힘
#define STEP_DELAY 2          
#define STEP_COUNT 4096        // 모터의 돌아가는 횟수 지정
#define SENSOR_CHECK_PERIOD 2000


SoftwareSerial soft(3, 4); // 블루투스 모듈 사용 핀
DHT dht(DHT_PIN, DHT_TYPE);

int window_control = 0;
int window_status = 0;
unsigned long last_sensor_check = 0;

void setup() {
  Serial.begin(9600);
  soft.begin(9600);
  dht.begin();

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(LIMIT_PIN1, INPUT_PULLUP);
  pinMode(LIMIT_PIN2, INPUT_PULLUP);

  pinMode(RAIN_SENSOR, INPUT);
  pinMode(GAS_SENSOR, INPUT);
  pinMode(CDS_SENSOR, INPUT);

  Serial.println("Smart Window System Ready.");
}

void loop() {
  // 블루투스 명령 처리
  if (soft.available()) {
    char cmd = soft.read();
    if (cmd == 'a') {
      window_control = 1;
      Serial.println("자동 모드 전환");
    }
    else if (cmd == 'm') {
      window_control = 0;
      Serial.println("수동 모드 전환");
    }
    else if (cmd == 'o') {
      openWindow();
    }
    else if (cmd == 'c') {
      closeWindow();
    }
  }

  // 센서 체크 주기 도달 시
  if (millis() - last_sensor_check > SENSOR_CHECK_PERIOD) {
    last_sensor_check = millis();

    int rain = analogRead(RAIN_SENSOR);
    int gas = analogRead(GAS_SENSOR);
    int cds = analogRead(CDS_SENSOR);
    float temp = dht.readTemperature();
    float humid = dht.readHumidity();

    if (isnan(temp) || isnan(humid)) {
      Serial.println("온습도 센서 오류");
      return;
    }

    // 센서 데이터 문자열 전송
    String data = "T:" + String((int)temp) +
                  " H:" + String((int)humid) +
                  " R:" + String(rain) +
                  " G:" + String(gas) +
                  " L:" + String(cds);

    soft.println(data);
    Serial.println(data);

    // 자동 모드일 경우에만 창문 제어
    if (window_control == 1) {
      if (rain < LIMIT_RAIN_VALUE || gas > LIMIT_GAS_VALUE || cds > CDS_VALUE_LIMIT) {
        closeWindow();
      } else {
        openWindow();
      }
    }
  }
}

void openWindow() {
  if (digitalRead(LIMIT_PIN1) == LOW) {
    Serial.println("이미 열려 있음");
    return;
  }

  Serial.println("창문 여는 중...");
  for (int i = 0; i < STEP_COUNT; i++) {
    if (digitalRead(LIMIT_PIN1) == LOW) break;
    stepForward(i % 8);
    delay(STEP_DELAY);
  }
  setCoils(0, 0, 0, 0);
  window_status = 1;
}

void closeWindow() {
  if (digitalRead(LIMIT_PIN2) == LOW) {
    Serial.println("이미 닫혀 있음");
    return;
  }

  Serial.println("창문 닫는 중...");
  for (int i = 0; i < STEP_COUNT; i++) {
    if (digitalRead(LIMIT_PIN2) == LOW) break;
    stepBackward(i % 8);
    delay(STEP_DELAY);
  }
  setCoils(0, 0, 0, 0);
  window_status = 2;
}

void stepForward(int step) {
  switch (step) {
    case 0: setCoils(1, 0, 0, 0); break;
    case 1: setCoils(1, 1, 0, 0); break;
    case 2: setCoils(0, 1, 0, 0); break;
    case 3: setCoils(0, 1, 1, 0); break;
    case 4: setCoils(0, 0, 1, 0); break;
    case 5: setCoils(0, 0, 1, 1); break;
    case 6: setCoils(0, 0, 0, 1); break;
    case 7: setCoils(1, 0, 0, 1); break;
  }
}

void stepBackward(int step) {
  switch (step) {
    case 0: setCoils(1, 0, 0, 1); break;
    case 1: setCoils(0, 0, 0, 1); break;
    case 2: setCoils(0, 0, 1, 1); break;
    case 3: setCoils(0, 0, 1, 0); break;
    case 4: setCoils(0, 1, 1, 0); break;
    case 5: setCoils(0, 1, 0, 0); break;
    case 6: setCoils(1, 1, 0, 0); break;
    case 7: setCoils(1, 0, 0, 0); break;
  }
}

void setCoils(int a, int b, int c, int d) {
  digitalWrite(IN1, a);
  digitalWrite(IN2, b);
  digitalWrite(IN3, c);
  digitalWrite(IN4, d);
}
