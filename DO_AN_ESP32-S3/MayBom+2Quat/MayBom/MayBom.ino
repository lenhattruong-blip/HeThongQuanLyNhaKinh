// Khai báo các chân GPIO điều khiển
const int pumpPin = 21;  // Chân điều khiển Bơm (Relay 1)
const int fan1Pin = 47;  // Chân điều khiển Quạt 1 (Relay 2)
const int fan2Pin = 48;  // Chân điều khiển Quạt 2 (Relay 3)

void setup() {
  Serial.begin(115200);
  
  // Vì mạch đang ở chế độ H, ta xuất LOW ngay từ đầu để tắt tất cả khi khởi động
  digitalWrite(pumpPin, LOW); 
  digitalWrite(fan1Pin, LOW); 
  digitalWrite(fan2Pin, LOW); 
  
  // Cấu hình các chân là OUTPUT
  pinMode(pumpPin, OUTPUT);
  pinMode(fan1Pin, OUTPUT);
  pinMode(fan2Pin, OUTPUT);
  
  Serial.println("--- HE THONG NHA KINH DA SAN SANG ---");
  Serial.println("Danh sach lenh dieu khien:");
  Serial.println(" Phim '1': BAT Bom   | Phim '0': TAT Bom");
  Serial.println(" Phim '2': BAT Quat 1| Phim '3': TAT Quat 1");
  Serial.println(" Phim '4': BAT Quat 2| Phim '5': TAT Quat 2");
  Serial.println(" Phim '9': BAT TAT CA| Phim '8': TAT TAT CA");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read(); 
    
    switch (command) {
      // Điều khiển Bơm
      case '1':
        digitalWrite(pumpPin, HIGH); 
        Serial.println("-> BOM: ON");
        break;
      case '0':
        digitalWrite(pumpPin, LOW); 
        Serial.println("-> BOM: OFF");
        break;

      // Điều khiển Quạt 1
      case '2':
        digitalWrite(fan1Pin, HIGH); 
        Serial.println("-> QUAT 1: ON");
        break;
      case '3':
        digitalWrite(fan1Pin, LOW); 
        Serial.println("-> QUAT 1: OFF");
        break;

      // Điều khiển Quạt 2
      case '4':
        digitalWrite(fan2Pin, HIGH); 
        Serial.println("-> QUAT 2: ON");
        break;
      case '5':
        digitalWrite(fan2Pin, LOW); 
        Serial.println("-> QUAT 2: OFF");
        break;

      // Bật/Tắt đồng loạt
      case '9':
        digitalWrite(pumpPin, HIGH);
        digitalWrite(fan1Pin, HIGH);
        digitalWrite(fan2Pin, HIGH);
        Serial.println("-> DA BAT TOAN BO THIET BI!");
        break;
      case '8':
        digitalWrite(pumpPin, LOW);
        digitalWrite(fan1Pin, LOW);
        digitalWrite(fan2Pin, LOW);
        Serial.println("-> DA TAT TOAN BO THIET BI!");
        break;
    }
  }
}