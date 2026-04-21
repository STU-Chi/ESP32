#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// === 設定區 ===
const char* ssid = "123";
const char* password = "22115103";
const char* mqtt_server = "broker.emqx.io";
const int mqtt_port = 8883;

// === MQTT 主題設定 ===
const char* topicLedControl = "myhome/esp32/led";
const char* topicButtonStatus = "myhome/esp32/button"; // 原始狀態回報
const String ROOM_ID = "UNDER_CAT_BATTLE_999"; 
String topicBtn = "morsegame/" + ROOM_ID + "/button";  // 遊戲用按鈕主題
String topicStatus = "morsegame/" + ROOM_ID + "/status"; // 在線狀態主題

const int ledPin = 22; 
const int buttonPin = 25; 

WiFiClientSecure espClient;
PubSubClient client(espClient);

// === 狀態變數 ===
int lastButtonState = HIGH;
unsigned long lastPingTime = 0;

// === WiFi 連線函式 ===
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

// === MQTT 接收訊息回呼 ===
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
      digitalWrite(ledPin, HIGH);
    } else if (messageTemp == "OFF") {
      digitalWrite(ledPin, LOW);
    }
  }
}

// === MQTT 重新連線函式 ===
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32_Morse_" + String(random(0xffff), HEX);
    
    // 加上 Last Will (遺囑)，斷線時自動發送 OFFLINE
    if (client.connect(clientId.c_str(), topicStatus.c_str(), 0, true, "OFFLINE")) {
      Serial.println("connected (Encrypted)");
      
      // 連線成功後發送 ONLINE 狀態
      client.publish(topicStatus.c_str(), "ONLINE", true); 
      
      // 訂閱主題
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
  pinMode(buttonPin, INPUT_PULLUP); 

  setup_wifi();

  // 忽略 SSL 憑證驗證
  espClient.setInsecure(); 
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // 1. 心跳包 (每2秒發送一次 PING)
  if (millis() - lastPingTime > 2000) {
    client.publish(topicStatus.c_str(), "PING");
    lastPingTime = millis();
  }

  // 2. 按鈕偵測與發送
  int currentButtonState = digitalRead(buttonPin);
  
  // 偵測按下瞬間 (HIGH 變 LOW)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    Serial.println("按鈕被按下");
    client.publish(topicBtn.c_str(), "DOWN");           // 遊戲用
    client.publish(topicButtonStatus, "按鈕被按下");       // 狀態監控用
    digitalWrite(ledPin, HIGH); 
    delay(50); // 簡單防彈跳
  } 
  // 偵測放開瞬間 (LOW 變 HIGH)
  else if (currentButtonState == HIGH && lastButtonState == LOW) {
    Serial.println("按鈕已放開");
    client.publish(topicButtonStatus, "按鈕已放開");
    digitalWrite(ledPin, LOW);
    delay(50); // 簡單防彈跳
  }
  
  lastButtonState = currentButtonState;
}