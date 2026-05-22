#include <esp_now.h>
#include <WiFi.h>

#define R_ENA  5
#define R_IN1  18
#define R_IN2  19
#define R_ENB  23
#define R_IN3  25
#define R_IN4  26

#define L_ENA  27
#define L_IN1  32
#define L_IN2  33
#define L_ENB  21
#define L_IN3  22
#define L_IN4  4

typedef struct __attribute__((packed)) {
  int16_t throttle;
  int16_t steering;
  uint8_t turbo;
  uint8_t horn;
  uint8_t mode;
  uint8_t heartbeat;
} Packet;

volatile Packet rxPkt;
volatile bool fresh = false;
unsigned long lastRx = 0;
bool sigLost = false;
bool everReceived = false;
Packet pkt;

void onReceive(const esp_now_recv_info* info, const uint8_t* data, int len) {
  if (len == sizeof(Packet)) {
    memcpy((void*)&rxPkt, data, len);
    fresh        = true;
    lastRx       = millis();
    sigLost      = false;
    everReceived = true;
  }
}

void rightMotors(int spd) {
  if (abs(spd) < 30) spd = 0;
  spd = constrain(spd, -255, 255);
  digitalWrite(R_IN1, spd > 0 ? HIGH : LOW);
  digitalWrite(R_IN2, spd < 0 ? HIGH : LOW);
  ledcWrite(R_ENA, abs(spd));
  digitalWrite(R_IN3, spd > 0 ? HIGH : LOW);
  digitalWrite(R_IN4, spd < 0 ? HIGH : LOW);
  ledcWrite(R_ENB, abs(spd));
}

void leftMotors(int spd) {
  if (abs(spd) < 30) spd = 0;
  spd = constrain(spd, -255, 255);
  digitalWrite(L_IN1, spd > 0 ? HIGH : LOW);
  digitalWrite(L_IN2, spd < 0 ? HIGH : LOW);
  ledcWrite(L_ENA, abs(spd));
  digitalWrite(L_IN3, spd > 0 ? HIGH : LOW);
  digitalWrite(L_IN4, spd < 0 ? HIGH : LOW);
  ledcWrite(L_ENB, abs(spd));
}

void stopAll() {
  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(R_IN3, LOW); digitalWrite(R_IN4, LOW);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);
  ledcWrite(R_ENA, 0); ledcWrite(R_ENB, 0);
  ledcWrite(L_ENA, 0); ledcWrite(L_ENB, 0);
}

void drive(int thr, int str) {
  int L = thr + str;
  int R = thr - str;
  int mx = max(abs(L), abs(R));
  if (mx > 255) {
    L = L * 255 / mx;
    R = R * 255 / mx;
  }
  leftMotors(L);
  rightMotors(R);
}

void setup() {
  Serial.begin(115200);

  pinMode(R_IN1, OUTPUT); pinMode(R_IN2, OUTPUT);
  pinMode(R_IN3, OUTPUT); pinMode(R_IN4, OUTPUT);
  pinMode(L_IN1, OUTPUT); pinMode(L_IN2, OUTPUT);
  pinMode(L_IN3, OUTPUT); pinMode(L_IN4, OUTPUT);

  digitalWrite(R_IN1, LOW); digitalWrite(R_IN2, LOW);
  digitalWrite(R_IN3, LOW); digitalWrite(R_IN4, LOW);
  digitalWrite(L_IN1, LOW); digitalWrite(L_IN2, LOW);
  digitalWrite(L_IN3, LOW); digitalWrite(L_IN4, LOW);

  ledcAttach(R_ENA, 1000, 8);
  ledcAttach(R_ENB, 1000, 8);
  ledcAttach(L_ENA, 1000, 8);
  ledcAttach(L_ENB, 1000, 8);
  ledcWrite(R_ENA, 0); ledcWrite(R_ENB, 0);
  ledcWrite(L_ENA, 0); ledcWrite(L_ENB, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  delay(100);

  Serial.println("\n==============================");
  Serial.print("  CAR MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("  Paste into remote CAR_MAC[]");
  Serial.println("==============================\n");

  esp_now_init();
  esp_now_register_recv_cb(onReceive);

  Serial.println("Car ready — waiting for remote...");
}

void loop() {
  unsigned long now = millis();

  if (!everReceived) {
    stopAll();
    return;
  }

  if (now - lastRx > 500) {
    if (!sigLost) {
      sigLost = true;
      stopAll();
      Serial.println("SIGNAL LOST — car stopped!");
    }
    return;
  }

  if (fresh) {
    fresh = false;
    memcpy(&pkt, (void*)&rxPkt, sizeof(pkt));
    drive(pkt.throttle, pkt.steering);
    Serial.printf("T:%4d S:%4d TRB:%d HRN:%d HB:%d\n",
      pkt.throttle, pkt.steering,
      pkt.turbo, pkt.horn, pkt.heartbeat);
  }
}