#include <WiFi.h>
#include <PubSubClient.h>
#define RX_PIN 16
#define TX_PIN 17
#define BAUD_RATE 115200
String dataBuffer = ""; // Biến lưu dữ liệu tạm thời
HardwareSerial MySerial(1);

// Thông tin WiFi
const char* WIFI_SSID = "PIFLab_M5";
const char* WIFI_PASSWORD = "khonghoisaocopass";

// Thông tin MQTT Broker
const char* MQTT_SERVER = "broker.hivemq.com";  // Broker 
const int MQTT_PORT = 1883;
const char* MQTT_TOPIC_PUB = "cuong151";
const char* MQTT_TOPIC_SUB = "receive";

// Khai báo client
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Biến thời gian
unsigned long previousWiFiCheckTime = 0;
unsigned long previousPublishTime = 0;

// Kết nối WiFi
void connectWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
    if (millis() - previousWiFiCheckTime > 10000) {
      Serial.println("\nWiFi failed, retrying...");
      return;
    }
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());
}

// Callback nhận tin nhắn
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Kết nối MQTT
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Connected to MQTT");
      mqttClient.subscribe(MQTT_TOPIC_SUB);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Retry in 2 seconds...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(15);
  
  // Cấu hình UART1 (TX = GPIO17, RX = GPIO16)
  MySerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // Baudrate, 8 data bits, no parity, 1 stop bit
  Serial.println("UART1 initialized.");
}

bool isValidData(const String& data) {
  // Kiểm tra dữ liệu có bắt đầu bằng "+UUDF:" không
  if (!data.startsWith("+UUDF:")) {
    return false;
  }

  // Kiểm tra các trường cơ bản bằng cách đếm dấu phẩy
  int commaCount = 0;
  for (char c : data) {
    if (c == ',') {
      commaCount++;
    }
  }
  // Giả định một gói dữ liệu hợp lệ phải có ít nhất 9 dấu phẩy
  return commaCount >= 9;
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Đọc dữ liệu từ MySerial
  while (MySerial.available()) {
    char c = MySerial.read();
    if (c == '\n') { // Khi gặp ký tự kết thúc '\n'
      Serial.println("Received complete data: " + dataBuffer);
      if (isValidData(dataBuffer)) {
        Serial.println("Data is valid: " + dataBuffer);
    unsigned long currentMillis = millis();
    if (currentMillis - previousPublishTime > 2000) {
    previousPublishTime = currentMillis;
    if (dataBuffer.length() > 0 ) { // Chỉ gửi nếu có dữ liệu đầy đủ và hợp lệ
      mqttClient.publish(MQTT_TOPIC_PUB, dataBuffer.c_str()); // Gửi chuỗi data qua MQTT
      Serial.println("Sent valid data via MQTT: " + dataBuffer);
      dataBuffer = ""; // Xóa buffer sau khi gửi
    } else {
      Serial.println("No valid data to send via MQTT.\n");
    }
      } else {
        Serial.println("Invalid data format, discarding: " + dataBuffer);
      }
  }
      dataBuffer = ""; // Xóa buffer sau khi xử lý
    } else {
      dataBuffer += c; // Ghép ký tự vào buffer
    }
    delay(10);
  }
}
