// Libraries
#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <SoftwareSerial.h>  // Thêm thư viện SoftwareSerial
// Constants
#define DHTPIN A3     // Pin của DHT
#define DHTTYPE DHT22 // DHT 22 (AM2302)
DHT dht(DHTPIN, DHTTYPE); // Initialize DHT sensor for normal 16mhz Arduino

#define SOIL_MOISTURE_PIN A0 // Soil moisture sensor connected to analog pin A0
#define MQ7_PIN A2 // MQ-7 sensor connected to analog pin A2
#define RAIN_SENSOR_PIN A1 // Cảm biến mưa kết nối với A1

// SoftwareSerial definition (sử dụng chân 10 và 11 cho RX và TX)
SoftwareSerial mySerial(10, 11); // RX, TX


// Variables
float h;  // Lưu trữ giá trị độ ẩm
float t;  // Lưu trữ giá trị nhiệt độ
int soilMoistureValue; // Lưu trữ giá trị độ ẩm đất
int mq7Value; // Lưu trữ giá trị cảm biến MQ-7
int rainValue; // Lưu trữ giá trị cảm biến mưa

// BH1750 object
BH1750 lightMeter;

// Timing variables
unsigned long previousMillis = 0;
const long interval = 120000; // 120 giây (120000 ms) = 2 phút

unsigned long previousMillis1 = 0;
const long interval1 = 1000; // 120 giây (120000 ms) = 2 phút

float tempSum = 0, humSum = 0, luxSum = 0, coSum = 0;
int soilMoistureSum = 0, rainSum = 0;
int count = 1; // Đếm số lần đo

void setup() {
    Serial.begin(9600);

    mySerial.begin(9600); // Giao tiếp với ESP32 qua SoftwareSerial

    Serial.println("Temperature, Humidity, Soil Moisture, Light, Gas, and Rain Sensor Test");
    dht.begin();

    pinMode(SOIL_MOISTURE_PIN, INPUT); // Set soil moisture pin as input
    pinMode(MQ7_PIN, INPUT); // Set MQ-7 pin as input
    pinMode(RAIN_SENSOR_PIN, INPUT); // Set Rain sensor pin as input

    Wire.begin();
    if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("BH1750 Advanced begin");
    } else {
        Serial.println("Error initializing BH1750");
    }
}

void loop() {
    unsigned long currentMillis = millis();
    // Đọc dữ liệu và lưu vào các biến h (độ ẩm) và t (nhiệt độ)
    h = dht.readHumidity();
    t = dht.readTemperature();

    // Kiểm tra nếu việc đọc dữ liệu thất bại và thoát sớm (thử lại)
    if (isnan(h) || isnan(t)){
      Serial.println("Failed to read from DHT sensor!");
      t = -1;
      h = -1;
    }

    // Đọc giá trị độ ẩm đất
    soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
    // Chuyển đổi giá trị analog thành phần trăm (0-100)
    // int soilMoisturePercent = map(soilMoistureValue, 0, 1023, 0, 100);
    // int soilMoistureTruePercent = 100 - soilMoisturePercent;
    int soilMoistureTruePercent;
    if (soilMoistureValue == 0 || soilMoistureValue == 1023) {
        Serial.println("Lỗi: Không đọc được cảm biến độ ẩm đất!");
        soilMoistureTruePercent = -1;
    } else {
        int soilMoisturePercent = map(soilMoistureValue, 0, 1023, 0, 100);
        soilMoistureTruePercent = 100 - soilMoisturePercent;
    }

    // Đọc mức độ ánh sáng
    float lux = lightMeter.readLightLevel();
    // Kiểm tra nếu việc đọc ánh sáng thất bại
    if (lux < 0) {
        Serial.println("Failed to read from BH1750 sensor!");
        // mySerial.println("Failed to read from BH1750 sensor!");
        // return;
        lux = -1;
    }

    // Đọc giá trị từ cảm biến MQ-7
    mq7Value = analogRead(MQ7_PIN);
    // float co_ppm = mq7Value / 10.0;
    float co_ppm;
    if (mq7Value < 10) {  // Nếu giá trị rất thấp, có thể cảm biến chưa đủ nóng hoặc bị lỗi
      Serial.println("Lỗi: Không đọc được cảm biến MQ-7!");
      co_ppm = -1;
    } else {
      co_ppm = mq7Value;  // Quy đổi ra ppm
    }

    // Đọc giá trị từ cảm biến mưa
    rainValue = analogRead(RAIN_SENSOR_PIN);
    // Giả sử nếu giá trị cảm biến dưới một mức nào đó (chẳng hạn 500), có thể coi là có mưa
    // bool isRaining = rainValue < 500;
    bool isRaining;
    if (rainValue < 10 || rainValue > 1023) {  // Nếu giá trị bất thường
        Serial.println("Lỗi: Không đọc được cảm biến mưa!");
        isRaining = -1;  // Đánh dấu lỗi
    } else {
        isRaining = (rainValue < 500) ? 1 : 0;
    }

    // Đo cảm biến mỗi 120 giây - 2 phút
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      count++; // Tăng bộ đếm sau mỗi lần đo
      if(count > 30) {
        count = 1;
      }

      // Gửi kết quả ngay sau mỗi 20 giây
      String data = String(count) + "," + String(h) + "," + String(t) + "," + String(soilMoistureTruePercent) + "," + String(lux) + "," + String(co_ppm) + "," + String(isRaining ? 1 : 0);
      mySerial.println(data);  // Gửi dữ liệu cho ESP32
      Serial.println(data);  // Gửi dữ liệu cho ESP32

      // delay(1000);  // Thời gian chờ để không gửi quá nhanh
    }
    else if(currentMillis - previousMillis1 >= interval1){
      previousMillis1 = currentMillis;
      // String data =  "40,-1," + String(t) + "," + String(soilMoistureTruePercent) + "," + String(lux) + "," + String(co_ppm) + "," + String(isRaining ? 1 : 0);
      String data =  "40," + String(h) + "," + String(t) + "," + String(soilMoistureTruePercent) + "," + String(lux) + "," + String(co_ppm) + "," + String(isRaining ? 1 : 0);
      mySerial.println(data);  // Gửi dữ liệu cho ESP32
      Serial.println(data);  // Gửi dữ liệu cho ESP32
      // delay(2000);
    }

}
