#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h> // 新增 MQTT 程式庫

// === 網路與 MQTT 設定 ===
const char* ssid = "YOUR_WIFI_SSID";           // 替換成您的 WiFi 帳號
const char* password = "YOUR_WIFI_PASSWORD";   // 替換成您的 WiFi 密碼
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 8883;                    // 8883 是 MQTTS (加密 MQTT) 通訊埠

// === 硬體腳位設定 ===
const int ledPin = 22; 
const int buttonPin = 25; 

// === MQTT 主題設定 ===
const char* topicLedControl = "myhome/esp32/led"; // Led燈 的 MQTT 路徑，可替換成客製化名稱

const char* topicButtonStatus = "myhome/esp32/button"; // 按鈕 的 MQTT 路徑，可替換成客製化名稱

// 使用 WiFiClientSecure 進行加密連線
WiFiClientSecure espClient;
PubSubClient client(espClient);

// 記錄按鈕狀態以避免重複發送
int lastButtonState = HIGH; 

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// 接收到 MQTT 訊息時的回呼函式
void callback(char* topic, byte* payload, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)payload[i];
  }
  
  Serial.print("收到訊息 [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(messageTemp);

  // 判斷是否為控制 LED 的指令
  if (String(topic) == topicLedControl) {
    if (messageTemp == "ON") {
      digitalWrite(ledPin, HIGH); // 打開 LED
    } else if (messageTemp == "OFF") {
      digitalWrite(ledPin, LOW);  // 關閉 LED
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // 隨機產生 Client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected (Encrypted)");
      // 連線後訂閱 LED 控制主題
      client.subscribe(topicLedControl);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); // 啟用內部上拉電阻

  setup_wifi();

  // 忽略 SSL 憑證驗證 (適合簡單測試，若要用於正式產品應載入 Root CA 憑證)
  espClient.setInsecure(); 
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 讀取按鈕狀態 (Pin 0 預設為 HIGH，按下時為 LOW)
  int currentButtonState = digitalRead(buttonPin);
  
  // 如果狀態改變了才發送訊息 (簡單的防彈跳邏輯)
  if (currentButtonState != lastButtonState) {
    delay(50); // 防彈跳延遲
    currentButtonState = digitalRead(buttonPin); // 再次確認
    
    if (currentButtonState != lastButtonState) {
      if (currentButtonState == LOW) {
        Serial.println("按鈕被按下");
        client.publish(topicButtonStatus, "已按下 (PRESSED)");
      } else {
        Serial.println("按鈕已放開");
        client.publish(topicButtonStatus, "已放開 (RELEASED)");
      }
      lastButtonState = currentButtonState;
    }
  }
}