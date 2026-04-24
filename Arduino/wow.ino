#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// --- Wi-Fi 與 MQTT 設定 ---
const char* ssid = "123";         // 請填入你的 WiFi 名稱
const char* password = "22115103"; // 請填入你的 WiFi 密碼
const char* mqtt_server = "broker.emqx.io";  // 使用 EMQX 免費公開伺服器
const int mqtt_port = 8883;                  // 8883 為加密的 MQTTS Port
const char* mqtt_topic = "my_esp32/sensor/distance_22115103"; // 建議將 12345 改成隨機數字避免與他人衝突

// --- 腳位設定 ---
const int trigPin = 23;
const int echoPin = 22;

WiFiClientSecure espClient;
PubSubClient client(espClient);

long duration;
int distance;

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
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // 建立隨機的 Client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
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
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  setup_wifi();
  
  // 設定不驗證伺服器憑證 (僅供測試方便，若用於正式產品建議載入 Root CA)
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // --- 讀取超音波感測器 ---
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  // 計算距離 (公分)
  distance = duration * 0.034 / 2;
  
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // --- 發布數據至 MQTT ---
  String payload = String(distance);
  client.publish(mqtt_topic, payload.c_str());

  delay(1000); // 每秒更新一次
}