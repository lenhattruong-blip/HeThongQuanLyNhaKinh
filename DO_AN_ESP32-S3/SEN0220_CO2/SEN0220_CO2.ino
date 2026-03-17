#include <Arduino.h>

// Định nghĩa chân UART cho ESP32-S3 (Bạn có thể đổi chân tùy theo nối dây thực tế)
#define RX_PIN 4 // Nối với TX của cảm biến
#define TX_PIN 5 // Nối với RX của cảm biến

// Khởi tạo UART1 cho cảm biến
HardwareSerial Co2Serial(1); 

// Mảng byte chứa lệnh yêu cầu đọc dữ liệu
const byte requestPacket[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
byte responsePacket[9];

// Hàm kiểm tra checksum
bool verifyChecksum(byte *packet) {
  // Kiểm tra 2 byte header
  if (packet[0] != 0xFF || packet[1] != 0x86) return false;
  
  byte checksum = 0;
  for (int i = 1; i < 8; i++) {
    checksum += packet[i];
  }
  checksum = 0xFF - checksum + 1;
  
  return packet[8] == checksum;
}

void setup() {
  // Khởi tạo Serial Monitor để xem kết quả trên máy tính
  Serial.begin(115200); 
  
  // Khởi tạo UART giao tiếp với cảm biến ở baudrate 9600
  Co2Serial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); 
  
  Serial.println("Đang khởi động cảm biến CO2 SEN0220...");
  Serial.println("Lưu ý: Thời gian làm nóng (Preheating Time) là 3 phút.");
}

void loop() {
  // 1. Xóa sạch buffer cũ
  while (Co2Serial.available()) {
    Co2Serial.read();
  }

  // 2. Gửi lệnh yêu cầu đọc dữ liệu
  Co2Serial.write(requestPacket, sizeof(requestPacket));
  
  // 3. Chờ một chút để dữ liệu truyền đi (hoặc cảm biến phản hồi)
  delay(100);

  // 4. Đọc và phân tích gói tin 9 bytes
  if (Co2Serial.available() >= 9) {
    Co2Serial.readBytes(responsePacket, 9);
    
    // ĐƯA PHẦN IN RAW DATA RA NGOÀI ĐỂ KIỂM TRA MỌI TÌNH HUỐNG
    Serial.print("Raw data nhận được: ");
    for(int i = 0; i < 9; i++) {
      if(responsePacket[i] < 0x10) Serial.print("0");
      Serial.print(responsePacket[i], HEX);
      Serial.print(" ");
    }
    Serial.println(); // Xuống dòng
    
    // Sau khi in ra xem thử, mới bắt đầu kiểm tra tính đúng đắn
    if (verifyChecksum(responsePacket)) {
      int co2_ppm = (responsePacket[2] << 8) | responsePacket[3];
      Serial.print("=> CO2 Hợp lệ = ");
      Serial.print(co2_ppm);
      Serial.println(" ppm\n");
    } else {
      Serial.println("=> Lỗi: Dữ liệu bị hỏng (Sai Checksum hoặc sai Header)!\n");
    }
  } else {
    Serial.println("Không nhận đủ phản hồi từ cảm biến.");
  }

  delay(1000); 
}