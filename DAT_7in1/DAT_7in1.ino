
#include <ModbusMaster.h>

// Định nghĩa chân ESP32 giao tiếp với module RS485
#define RX_PIN 18 // Nối với chân TXD của module RS485
#define TX_PIN 17 // Nối với chân RXD của module RS485

// Khởi tạo đối tượng Modbus
ModbusMaster node;

void setup() {
  // Bật Serial monitor để xem kết quả trên máy tính
  Serial.begin(115200);
  
  // Khởi tạo Serial2 cho giao tiếp RS485 (Baudrate 9600, 8N1 - giống file Python)
  Serial2.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  // Thiết lập ID của cảm biến (Slave = 1 theo file Python)
  node.begin(1, Serial2);
  
  Serial.println("Dang khoi tao he thong doc cam bien dat 7 in 1...");
  delay(2000);
}

void loop() {
  uint8_t result;
  
  // Đọc 8 thanh ghi (REG_COUNT = 8) bắt đầu từ địa chỉ 0x0000 (REG_START = 0x0000)
  // Hàm này tự động dùng Function Code 3
  result = node.readHoldingRegisters(0x0000, 8);

  if (result == node.ku8MBSuccess) {
    // Lấy dữ liệu và áp dụng hệ số (scale) y hệt mảng FIELDS trong file .py
    float temp_C   = node.getResponseBuffer(0) * 0.1;
    float hum_pct  = node.getResponseBuffer(1) * 0.1;
    float ec_uS_cm = node.getResponseBuffer(2) * 1.0;
    float pH       = node.getResponseBuffer(3) * 0.01;
    float N_mgkg   = node.getResponseBuffer(4) * 1.0;
    float P_mgkg   = node.getResponseBuffer(5) * 1.0;
    float K_mgkg   = node.getResponseBuffer(6) * 1.0;
    float salt_mgL = node.getResponseBuffer(7) * 1.0;

    // In kết quả ra Serial Monitor
    Serial.println("===== KET QUA DO =====");
    Serial.print("Nhiet do  : "); Serial.print(temp_C, 1); Serial.println(" *C");
    Serial.print("Do am     : "); Serial.print(hum_pct, 1); Serial.println(" %");
    Serial.print("EC        : "); Serial.print(ec_uS_cm, 0); Serial.println(" uS/cm");
    Serial.print("pH        : "); Serial.print(pH, 2); Serial.println("");
    Serial.print("Nito (N)  : "); Serial.print(N_mgkg, 0); Serial.println(" mg/kg");
    Serial.print("Photpho(P): "); Serial.print(P_mgkg, 0); Serial.println(" mg/kg");
    Serial.print("Kali (K)  : "); Serial.print(K_mgkg, 0); Serial.println(" mg/kg");
    Serial.print("Do man    : "); Serial.print(salt_mgL, 0); Serial.println(" mg/L");
    Serial.println("======================\n");
  } else {
    // In ra mã lỗi nếu không đọc được (ví dụ: E2 là lỗi timeout do mất kết nối)
    Serial.print("Loi ket noi Modbus. Ma loi: 0x");
    Serial.println(result, HEX);
  }

  // Chờ 2 giây trước khi đọc lại lần tiếp theo
  delay(2000);
}