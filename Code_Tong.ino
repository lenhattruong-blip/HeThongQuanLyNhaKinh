#include "DFRobot_EnvironmentalSensor.h"
#include <Wire.h>
#include <ModbusMaster.h>
#include <Arduino.h>

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

// ================= KHỞI TẠO ĐỐI TƯỢNG =================
DFRobot_EnvironmentalSensor environment(SEN050X_DEFAULT_DEVICE_ADDRESS, &Wire);
ModbusMaster node;
HardwareSerial Co2Serial(1);

// Mảng byte chứa lệnh yêu cầu đọc dữ liệu CO2
const byte requestPacket[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte responsePacket[9];

// Hàm kiểm tra checksum của CO2
bool verifyChecksum(byte *packet) {
  if (packet[0] != 0xFF || packet[1] != 0x86) return false;
  byte checksum = 0;
  for (int i = 1; i < 8; i++) checksum += packet[i];
  checksum = 0xFF - checksum + 1;
  return packet[8] == checksum;
}

void setup() {
  Serial.begin(115200);
  
  // 1. Khởi tạo Cảm biến môi trường (I2C)
  Serial.println("Dang khoi tao cam bien moi truong...");
  Wire.begin(I2C_SDA, I2C_SCL);
  while(environment.begin() != 0) {
    Serial.println(" -> Loi: Khong tim thay cam bien moi truong!!");
    delay(1000);
  }
  Serial.println(" -> Khoi tao cam bien moi truong THANH CONG!");

  // 2. Khởi tạo Cảm biến đất 7 in 1 (RS485 - Serial2)
  Serial.println("Dang khoi tao he thong doc cam bien dat 7 in 1...");
  Serial2.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  node.begin(1, Serial2);
  Serial.println(" -> Khoi tao Modbus THANH CONG!");

  // 3. Khởi tạo Cảm biến CO2 (Serial1)
  Serial.println("Dang khoi dong cam bien CO2 SEN0220...");
  Co2Serial.begin(9600, SERIAL_8N1, CO2_RX, CO2_TX);
  Serial.println(" -> Khoi tao CO2 THANH CONG! (Luu y: Can 3 phut lam nong)");
  
  delay(2000);
}

void loop() {
  Serial.println("\n================= NHÀ KÍNH - DỮ LIỆU TỔNG HỢP =================");

  // ---------------- [1] ĐỌC CẢM BIẾN MÔI TRƯỜNG (I2C) ----------------
  Serial.println("[1] Thong so Moi truong khong khi:");
  Serial.print(" - Nhiet do  : "); Serial.print(environment.getTemperature(TEMP_C)); Serial.println(" ℃");
  Serial.print(" - Do am     : "); Serial.print(environment.getHumidity()); Serial.println(" %");
  Serial.print(" - Tia UV    : "); Serial.print(environment.getUltravioletIntensity()); Serial.println(" mw/cm2");
  Serial.print(" - Anh sang  : "); Serial.print(environment.getLuminousIntensity()); Serial.println(" lx");
  Serial.print(" - Ap suat   : "); Serial.print(environment.getAtmospherePressure(HPA)); Serial.println(" hpa");
  Serial.print(" - Do cao    : "); Serial.print(environment.getElevation()); Serial.println(" m");

  // ---------------- [2] ĐỌC CẢM BIẾN CO2 (UART) ----------------
  Serial.println("\n[2] Thong so CO2:");
  // Xóa sạch buffer cũ trước khi gửi lệnh mới
  while (Co2Serial.available()) {
    Co2Serial.read();
  }
  
  // Gửi lệnh yêu cầu đọc
  Co2Serial.write(requestPacket, sizeof(requestPacket));
  delay(100); // Chờ cảm biến phản hồi

  if (Co2Serial.available() >= 9) {
    Co2Serial.readBytes(responsePacket, 9);
    if (verifyChecksum(responsePacket)) {
      int co2_ppm = (responsePacket[2] << 8) | responsePacket[3];
      Serial.print(" - Nong do CO2: "); Serial.print(co2_ppm); Serial.println(" ppm");
    } else {
      Serial.println(" - Loi: Du lieu CO2 bi hong (Sai Checksum)!");
    }
  } else {
    Serial.println(" - Loi: Khong nhan du phan hoi tu cam bien CO2.");
  }

  // ---------------- [3] ĐỌC CẢM BIẾN ĐẤT 7 IN 1 (MODBUS) ----------------
  Serial.println("\n[3] Thong so Dat (7 in 1):");
  uint8_t result = node.readHoldingRegisters(0x0000, 8);
  
  if (result == node.ku8MBSuccess) {
    // Đổ dữ liệu vào các biến float (Rất hữu ích để xử lý logic tưới tiêu sau này)
    float temp_C   = node.getResponseBuffer(0) * 0.1;
    float hum_pct  = node.getResponseBuffer(1) * 0.1;
    float ec_uS_cm = node.getResponseBuffer(2) * 1.0;
    float pH       = node.getResponseBuffer(3) * 0.01;
    float N_mgkg   = node.getResponseBuffer(4) * 1.0;
    float P_mgkg   = node.getResponseBuffer(5) * 1.0;
    float K_mgkg   = node.getResponseBuffer(6) * 1.0;
    float salt_mgL = node.getResponseBuffer(7) * 1.0;

    Serial.print(" - Nhiet do  : "); Serial.print(temp_C, 1); Serial.println(" ℃");
    Serial.print(" - Do am     : "); Serial.print(hum_pct, 1); Serial.println(" %");
    Serial.print(" - EC        : "); Serial.print(ec_uS_cm, 0); Serial.println(" uS/cm");
    Serial.print(" - pH        : "); Serial.print(pH, 2); Serial.println("");
    Serial.print(" - Nito (N)  : "); Serial.print(N_mgkg, 0); Serial.println(" mg/kg");
    Serial.print(" - Photpho(P): "); Serial.print(P_mgkg, 0); Serial.println(" mg/kg");
    Serial.print(" - Kali (K)  : "); Serial.print(K_mgkg, 0); Serial.println(" mg/kg");
    Serial.print(" - Do man    : "); Serial.print(salt_mgL, 0); Serial.println(" mg/L");
  } else {
    Serial.print(" - Loi ket noi Modbus. Ma loi: 0x");
    Serial.println(result, HEX);
  }

  Serial.println("===============================================================");
  
  // Chờ 5 giây trước khi cập nhật toàn bộ dữ liệu mới
  delay(5000); 
}