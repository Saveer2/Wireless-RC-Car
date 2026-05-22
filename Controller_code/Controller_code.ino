#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

uint8_t CAR_MAC[] = {0x04, 0x83, 0x08, 0x58, 0x00, 0x6C};

#define LEFT_VRY   35
#define LEFT_VRX   34
#define LEFT_SW    32
#define RIGHT_VRX  33
#define RIGHT_VRY  25
#define RIGHT_SW   26

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

typedef struct __attribute__((packed)) {
  int16_t throttle;
  int16_t steering;
  uint8_t turbo;
  uint8_t horn;
  uint8_t mode;
  uint8_t heartbeat;
} Packet;

Packet pkt;
esp_now_peer_info_t peer;

bool    connected   = false;
bool    sportMode   = false;
int     sigBars     = 0;
uint8_t hb          = 0;

unsigned long tSend = 0;
unsigned long tDisp = 0;

void onSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    connected = true;
    sigBars = min(5, sigBars + 1);
  } else {
    sigBars = max(0, sigBars - 1);
    if (sigBars == 0) connected = false;
  }
}

int16_t readAxis(int pin, bool invert) {
  int raw      = analogRead(pin);
  int centered = raw - 2048;
  if (abs(centered) < 180) return 0;
  int sign = centered > 0 ? 1 : -1;
  int mag  = map(abs(centered) - 180, 0, 1868, 0, 255);
  mag = constrain(mag, 0, 255);
  return invert ? -(sign * mag) : (sign * mag);
}

void showSplash() {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setCursor(6, 8);
  oled.print("RC REMOTE");
  oled.setTextSize(1);
  oled.setCursor(22, 32);
  oled.print("ESP-NOW v1.0");
  oled.setCursor(12, 48);
  oled.print("Starting...");
  oled.display();
  delay(2000);
}

void drawHUD() {
  oled.clearDisplay();

  oled.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  oled.setTextColor(SSD1306_BLACK);
  oled.setTextSize(1);
  oled.setCursor(2, 2);
  oled.print(sportMode ? "SPORT" : "SAFE ");
  oled.setCursor(40, 2);
  oled.print(connected ? "ONLINE" : "SEARCH");

  for (int i = 0; i < 5; i++) {
    int h = 3 + i;
    int x = 100 + i * 5;
    if (i < sigBars) oled.fillRect(x, 9 - h, 4, h, SSD1306_BLACK);
    else             oled.drawRect(x, 9 - h, 4, h, SSD1306_BLACK);
  }

  oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(2, 15);
  oled.print("T");
  oled.drawRect(12, 14, 80, 7, SSD1306_WHITE);
  oled.drawLine(52, 14, 52, 21, SSD1306_WHITE);
  if (pkt.throttle > 0) {
    oled.fillRect(52, 15, map(pkt.throttle, 0, 255, 0, 39), 5, SSD1306_WHITE);
  } else if (pkt.throttle < 0) {
    int w = map(-pkt.throttle, 0, 255, 0, 39);
    oled.fillRect(52 - w, 15, w, 5, SSD1306_WHITE);
  }
  oled.setCursor(95, 15);
  oled.print(pkt.throttle < 0 ? "REV" : "FWD");

  oled.setCursor(2, 26);
  oled.print("S");
  oled.drawRect(12, 25, 80, 7, SSD1306_WHITE);
  oled.drawLine(52, 25, 52, 32, SSD1306_WHITE);
  if (pkt.steering > 0) {
    oled.fillRect(52, 26, map(pkt.steering, 0, 255, 0, 39), 5, SSD1306_WHITE);
  } else if (pkt.steering < 0) {
    int w = map(-pkt.steering, 0, 255, 0, 39);
    oled.fillRect(52 - w, 26, w, 5, SSD1306_WHITE);
  }
  oled.setCursor(95, 26);
  oled.print(pkt.steering > 0 ? " >>" : pkt.steering < 0 ? "<<" : "CTR");

  oled.setCursor(2, 37);
  oled.print("T:"); oled.print(pkt.throttle);
  oled.setCursor(64, 37);
  oled.print("S:"); oled.print(pkt.steering);

  if (pkt.turbo) {
    oled.fillRoundRect(0, 50, 36, 11, 2, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(3, 52);
    oled.print("TURBO");
    oled.setTextColor(SSD1306_WHITE);
  }
  if (pkt.horn) {
    oled.fillRoundRect(40, 50, 28, 11, 2, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK);
    oled.setCursor(43, 52);
    oled.print("HORN");
    oled.setTextColor(SSD1306_WHITE);
  }

  if (hb % 2) oled.fillCircle(124, 58, 3, SSD1306_WHITE);
  else        oled.drawCircle(124, 58, 3, SSD1306_WHITE);

  if (!connected && (millis() / 400) % 2) {
    oled.setCursor(70, 52);
    oled.print("NO SIG!");
  }

  oled.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFT_SW,  INPUT_PULLUP);
  pinMode(RIGHT_SW, INPUT_PULLUP);

  analogReadResolution(12);

  Wire.begin(21, 22);
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
  } else {
    showSplash();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("Remote MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW FAILED!");
    while (true);
  }

  esp_now_register_send_cb(onSent);

  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, CAR_MAC, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);

  memset(&pkt, 0, sizeof(pkt));
  Serial.println("Remote ready!");
}

void loop() {
  unsigned long now = millis();

  pkt.throttle = readAxis(LEFT_VRY,  true);
  pkt.steering = readAxis(RIGHT_VRX, false);

  bool turbo = (digitalRead(RIGHT_SW) == LOW);
  pkt.turbo = turbo ? 1 : 0;
  if (turbo) {
    if (pkt.throttle > 0) pkt.throttle = min(255, pkt.throttle + 40);
    if (pkt.throttle < 0) pkt.throttle = max(-255, pkt.throttle - 40);
  }

  bool horn = (digitalRead(LEFT_SW) == LOW);
  pkt.horn = horn ? 1 : 0;

  pkt.mode = 0;
  pkt.heartbeat = hb++;

  if (now - tSend >= 20) {
    tSend = now;
    esp_now_send(CAR_MAC, (uint8_t*)&pkt, sizeof(pkt));
  }

  if (now - tDisp >= 100) {
    tDisp = now;
    drawHUD();
    Serial.printf("[THR %4d][STR %4d][TRB %d][HRN %d][SIG %d]\n",
      pkt.throttle, pkt.steering,
      pkt.turbo, pkt.horn, sigBars);
  }
}