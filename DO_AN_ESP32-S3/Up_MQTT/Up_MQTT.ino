#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DFRobot_EnvironmentalSensor.h"
#include <Wire.h>
#include <ModbusMaster.h>
#include <time.h> 

// ================= CẤU HÌNH CHÂN =================
// 1. Cảm biến Môi trường (I2C)
#define I2C_SDA 7
#define I2C_SCL 6

// 2. Cảm biến Đất 7 trong 1 (Modbus RS485 - UART2)
#define RS485_RX 18 
#define RS485_TX 17 

// 3. Cảm biến CO2 SEN0220 (UART1)
#define CO2_RX 4 
#define CO2_TX 5 

// 4. Máy bơm, quạt
const int pumpPin = 21;  // Chân điều khiển Bơm (Relay 1)
const int fan1Pin = 47;  // Chân điều khiển Quạt 1 (Relay 2)
const int fan2Pin = 48;  // Chân điều khiển Quạt 2 (Relay 3)

// ================= THÔNG TIN WIFI & MQTT =================
const char* ssid = "Huflit-CB-NV";
const char* password = "huflitcbnv@123";

const char* mqtt_server = "90f058c4d3f740118b63619bae0480d8.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_user = "hethongnhakinh";
const char* mqtt_pass = "Htnk123456@";
const char* mqtt_topic = "nhakinh/khuvuc1/dulieu"; 
const char* mqtt_topic_cmd = "nhakinh/khuvuc1/dieukhien";

// ================= CẤU HÌNH ĐỒNG BỘ THỜI GIAN (NTP) =================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600; // GMT+7
const int   daylightOffset_sec = 0;

WiFiClientSecure espClient; 
PubSubClient client(espClient);

// ================= KHỞI TẠO ĐỐI TƯỢNG =================
DFRobot_EnvironmentalSensor environment(SEN050X_DEFAULT_DEVICE_ADDRESS, &Wire);
ModbusMaster node;
HardwareSerial Co2Serial(1);

const byte requestPacket[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

// ================= CẤU TRÚC DỮ LIỆU & MUTEX =================
// Đã bổ sung ĐẦY ĐỦ các biến bị thiếu
struct SensorData {
  float airTemp, airHum, uv, light, pressure, elevation; // Môi trường
  int co2_ppm;                                           // CO2
  float soilTemp, soilHum, ec, pH, N, P, K, salinity;    // Đất
  
  bool isSoilValid;
  bool isCo2Valid;
};

SensorData myData;
SemaphoreHandle_t dataMutex;

// ================= CÁC HÀM PHỤ TRỢ =================
bool verifyChecksum(byte *packet) {
  if (packet[0] != 0xFF || packet[1] != 0x86) return false;
  byte checksum = 0;
  for (int i = 1; i < 8; i++) checksum += packet[i];
  checksum = 0xFF - checksum + 1;
  return packet[8] == checksum;
}

void initTime() {
  // Cấu hình thông số múi giờ và server NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  int retryCount = 0;
  
  Serial.print("Dang dong bo thoi gian NTP");
  
  // Vòng lặp thử lấy thời gian tối đa 10 lần, mỗi lần cách nhau 1 giây
  while (!getLocalTime(&timeinfo) && retryCount < 10) {
    Serial.print(".");
    delay(1000);
    retryCount++;
  }
  
  if (retryCount == 10) {
    Serial.println("\n -> Loi: Khong the dong bo thoi gian NTP (Kiem tra lai mang WiFi)!");
  } else {
    Serial.println("\n -> Dong bo thoi gian NTP THANH CONG!");
  }
}

String getTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return "N/A";
  char timeStringBuff[50];
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M:%S %d-%m-%Y", &timeinfo);
  return String(timeStringBuff);
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Dang ket noi MQTTS (TLS)...");
    String clientId = "ESP32S3-NhaKinh-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" Thanh cong!");
      client.subscribe(mqtt_topic_cmd);
    } else {
      Serial.print(" That bai, ma loi: ");
      Serial.print(client.state());
      Serial.println(" Thu lai sau 5s");
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

// Hàm tự động chạy khi có lệnh từ App gửi xuống
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 1. Đọc tin nhắn từ App
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("=> Nhan lenh tu App: "); Serial.println(message);

  // 2. Phân tích chuỗi JSON (Ví dụ: {"thietbi":"bom", "trangthai":"ON"})
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);

  if (!error) {
    String thietbi = doc["thietbi"];
    String trangthai = doc["trangthai"];

    // 3. Logic bật tắt (HIGH = Bật, LOW = Tắt theo đúng mạch của bạn)
    int pinState = (trangthai == "ON") ? HIGH : LOW; 

    // 4. Xuất tín hiệu ra chân tương ứng
    if (thietbi == "bom") {
      digitalWrite(pumpPin, pinState);
      Serial.println("   -> Da dieu khien BOM");
    } 
    else if (thietbi == "quat1") {
      digitalWrite(fan1Pin, pinState);
      Serial.println("   -> Da dieu khien QUAT 1");
    } 
    else if (thietbi == "quat2") {
      digitalWrite(fan2Pin, pinState);
      Serial.println("   -> Da dieu khien QUAT 2");
    }
  } else {
    Serial.println("   -> Loi: Sai dinh dang JSON dieu khien!");
  }
}
// ================= CÁC HÀM TASKS (CHẠY SONG SONG) =================

// TASK 1: Đọc cảm biến môi trường I2C
void TaskReadEnv(void *pvParameters) {
  for (;;) {
    float t = environment.getTemperature(TEMP_C);
    float h = environment.getHumidity();
    float uv = environment.getUltravioletIntensity();
    float l = environment.getLuminousIntensity();
    float p = environment.getAtmospherePressure(HPA);
    float e = environment.getElevation();

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      myData.airTemp = t; myData.airHum = h; 
      myData.uv = uv; myData.light = l;
      myData.pressure = p; myData.elevation = e;
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// TASK 2: Đọc cảm biến CO2 UART
void TaskReadCO2(void *pvParameters) {
  for (;;) {
    while (Co2Serial.available()) Co2Serial.read(); // Xóa buffer
    Co2Serial.write(requestPacket, sizeof(requestPacket));
    vTaskDelay(pdMS_TO_TICKS(100)); // Chờ phản hồi

    bool valid = false;
    int ppm = 0;
    
    if (Co2Serial.available() >= 9) {
      byte resp[9];
      Co2Serial.readBytes(resp, 9);
      if (verifyChecksum(resp)) {
        ppm = (resp[2] << 8) | resp[3];
        valid = true;
      }
    }

    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      myData.co2_ppm = ppm;
      myData.isCo2Valid = valid;
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// TASK 3: Đọc cảm biến đất RS485
void TaskReadSoil(void *pvParameters) {
  for (;;) {
    uint8_t result = node.readHoldingRegisters(0x0000, 8);
    if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
      if (result == node.ku8MBSuccess) {
        myData.soilTemp = node.getResponseBuffer(0) * 0.1;
        myData.soilHum  = node.getResponseBuffer(1) * 0.1;
        myData.ec       = node.getResponseBuffer(2) * 1.0;
        myData.pH       = node.getResponseBuffer(3) * 0.01;
        myData.N        = node.getResponseBuffer(4) * 1.0;
        myData.P        = node.getResponseBuffer(5) * 1.0;
        myData.K        = node.getResponseBuffer(6) * 1.0;
        myData.salinity = node.getResponseBuffer(7) * 1.0;
        myData.isSoilValid = true;
      } else {
        myData.isSoilValid = false;
      }
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

// TASK 4: Gửi dữ liệu lên MQTT
void TaskSendMQTT(void *pvParameters) {
  unsigned long lastMsgTime = 0;
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!client.connected()) reconnectMQTT();
      client.loop(); // Giữ kết nối

      unsigned long now = millis();
      if (now - lastMsgTime >= 5000) { 
        lastMsgTime = now;
        SensorData tempData;
        
        // Copy dữ liệu an toàn từ Mutex
        if (xSemaphoreTake(dataMutex, portMAX_DELAY)) {
          tempData = myData;
          xSemaphoreGive(dataMutex);
        }

        // Tạo chuỗi JSON
        JsonDocument doc; 
        doc["timestamp"] = getTimeString();
        
        JsonObject env = doc.createNestedObject("environment");
        env["temp"] = tempData.airTemp;
        env["hum"] = tempData.airHum;
        env["uv"] = tempData.uv;
        env["light"] = tempData.light;
        env["pressure"] = tempData.pressure;
        env["elevation"] = tempData.elevation;

        JsonObject co2 = doc.createNestedObject("co2");
        if (tempData.isCo2Valid) {
          co2["ppm"] = tempData.co2_ppm;
        } else {
          co2["ppm"] = "Error"; // Hoặc truyền null tùy cấu hình DB của bạn
        }

        JsonObject soil = doc.createNestedObject("soil");
        if (tempData.isSoilValid) {
          soil["temp"] = tempData.soilTemp;
          soil["hum"] = tempData.soilHum;
          soil["ec"] = tempData.ec;
          soil["pH"] = tempData.pH;
          soil["N"] = tempData.N;
          soil["P"] = tempData.P;
          soil["K"] = tempData.K;
          soil["salinity"] = tempData.salinity;
        } else {
          soil["status"] = "Modbus Error";
        }

        // Tăng buffer để chứa mảng JSON lớn
        char msgBuffer[1024]; 
        serializeJson(doc, msgBuffer);
        
        if (client.publish(mqtt_topic, msgBuffer)) {
          Serial.print("=> Gui MQTT Thanh cong: ");
        } else {
          Serial.print("=> Gui MQTT THAT BAI: ");
        }
        Serial.println(msgBuffer);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================= SETUP & LOOP =================
void setup() {
  Serial.begin(115200);

  pinMode(pumpPin, OUTPUT);
  pinMode(fan1Pin, OUTPUT);
  pinMode(fan2Pin, OUTPUT);
  // Tắt tất cả khi khởi động [cite: 112, 113]
  digitalWrite(pumpPin, LOW);
  digitalWrite(fan1Pin, LOW); 
  digitalWrite(fan2Pin, LOW);
  // 1. Kết nối WiFi
  WiFi.begin(ssid, password);
  Serial.print("Dang ket noi WiFi.");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected!");

  // 2. Đồng bộ thời gian
  initTime();

  // 3. Cấu hình MQTT Bảo mật
  espClient.setInsecure(); 
  client.setServer(mqtt_server, mqtt_port);
  client.setBufferSize(1024); // Mở rộng bộ đệm MQTT (Rất quan trọng vì JSON giờ rất dài)
  client.setCallback(mqttCallback);
  // 4. Khởi tạo phần cứng cảm biến
  Wire.begin(I2C_SDA, I2C_SCL);
  environment.begin();
  
  Serial2.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  node.begin(1, Serial2);
  
  Co2Serial.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);
  Serial.println("Khoi tao phan cung xong! (Luu y CO2 can 3 phut lam nong)");

  // 5. Khởi tạo Tasks FreeRTOS
  dataMutex = xSemaphoreCreateMutex();
  if (dataMutex != NULL) {
    xTaskCreatePinnedToCore(TaskReadEnv,  "ReadEnv",  4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskReadCO2,  "ReadCO2",  4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskReadSoil, "ReadSoil", 4096, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(TaskSendMQTT, "SendMQTT", 8192, NULL, 2, NULL, 0); // Task gửi MQTT cần Stack lớn hơn
  }
  
}

void loop() {
  // FreeRTOS xử lý tất cả ở Task, Loop chỉ cần nghỉ ngơi.
  vTaskDelay(pdMS_TO_TICKS(1000)); 
}