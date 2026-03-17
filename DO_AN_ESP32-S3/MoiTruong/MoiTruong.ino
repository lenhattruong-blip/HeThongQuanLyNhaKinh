#include "DFRobot_EnvironmentalSensor.h"
#include <Wire.h>

#define I2C_SDA 7
#define I2C_SCL 6

// Khởi tạo cảm biến với chế độ I2C
// Địa chỉ mặc định thường là SEN0500_DEFAULT_DEVICE_ADDRESS
DFRobot_EnvironmentalSensor environment(SEN050X_DEFAULT_DEVICE_ADDRESS, &Wire);

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo I2C cho ESP32-S3
  Wire.begin(I2C_SDA, I2C_SCL);

  // Kiểm tra kết nối cảm biến
  while(environment.begin() != 0) {
    Serial.println(" Sensor initialize failed!!");
    delay(1000);
  }
  Serial.println(" Sensor initialize success!!");
}

void loop() {
  Serial.println("-------------------------------");
  
  // Đọc Nhiệt độ (Độ C)
  Serial.print("Temp: ");
  Serial.print(environment.getTemperature(TEMP_C));
  Serial.println(" ℃");
  
  // Đọc Độ ẩm (%)
  Serial.print("Humidity: ");
  Serial.print(environment.getHumidity());
  Serial.println(" %");
  
  // Cường độ tia cực tím (mw/cm2)
  Serial.print("Ultraviolet intensity: ");
  Serial.print(environment.getUltravioletIntensity());
  Serial.println(" mw/cm2");
  
  // Cường độ ánh sáng (lx)
  Serial.print("LuminousIntensity: ");
  Serial.print(environment.getLuminousIntensity());
  Serial.println(" lx");
  
  // Áp suất khí quyển (hpa)
  Serial.print("Atmospheric pressure: ");
  Serial.print(environment.getAtmospherePressure(HPA));
  Serial.println(" hpa");
  
  // Độ cao (m)
  Serial.print("Altitude: ");
  Serial.print(environment.getElevation());
  Serial.println(" m");
  
  Serial.println("-------------------------------");
  delay(1000); // Tăng delay một chút để dễ quan sát
}