  // #include <LiquidCrystal_I2C.h>
  #include <ESP32Servo.h>
  #include <WiFi.h>
  #include <HTTPClient.h>
  #include <Firebase_ESP_Client.h>

  #define API_KEY "";
  #define DATABASE_URL "";
  #define USER_EMAIL "";
  #define USER_PASSWORD "";
  
  //
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

 // Thông tin Wifi
  const char* ssid = "iotz";
  const char* password = "1234567891"; 

  String gardenId = "JG8OCODvigeOcDjdLNqA"; //garden1 trong database

  // biến flag kiểm tra điều khiển gửi email
  int isSendEmailDHT = 0;
  int isSendEmailLS = 0;
  int isSendEmailsoilMoisture = 0;
  int isSendEmailco_ppm = 0;
  int isSendEmailisRaining = 0;

  // biến điều khiển thiết bị thủ công (máy bơm/quạt)
  int pumpState = 0; // máy bơm
  int fanState = 0; // quạt
  int canopyState = 0; //(mái che)
  int autoState = 1;

  // biến flag kiểm tra điều khiện khởi động gửi nhận thông tin của hai vi điều khiển
  int globalCount = 0;
  int count = 0;

  // biến flag kiểm tra máy bơm
  int isPump = 0;

  // // set cột (colums) và hàng (rows) cho lcd
  // int lcdColumns = 16;
  // int lcdRows = 2;

  // // set địa chỉ của lcd và cột - hàng
  // LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

  // Khai báo servo và chân GPIO4 cho servo
  Servo myServo;
  int servoPin = 4; // Chân GPIO4 (servo)

  // Khai báo cổng UART2 (RX2, TX2)
  #define RX2_PIN 16  // Chân RX2 của ESP32
  #define TX2_PIN 17  // Chân TX2 của ESP32

  // Khai báo các chân GPIO điều khiển relay
  #define RELAY1 18  // Chân IN1
  #define RELAY2 19  // Chân IN2
  #define RELAY3 21  // Chân IN3
  #define RELAY4 5   // Chân IN4 (thay GPIO 22 bằng GPIO 5)

  HardwareSerial mySerial(1); // Sử dụng UART1 (RX2, TX2) - serial nhận dữ liệu của UNO R3

  unsigned long previousMillis = 0; // Biến lưu trữ thời gian
  const long interval = 120000; // Khoảng thời gian để làm mới thông tin LCD (120 giây) = 2p
  unsigned long sendDataPrevMillis = 0;

  void setup() {
    // Khởi tạo Serial và UART2
    Serial.begin(115200); // Serial Monitor (UART0)
    mySerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN); // Khởi tạo UART1 với RX2, TX2 (Đây là Serial tạo để nhận dữ liệu gửi từ UNO R3)

    // Khởi tạo LCD
    // lcd.init();
    // lcd.backlight();

    // set up cho relay 4 channels
    pinMode(RELAY1, OUTPUT);
    pinMode(RELAY2, OUTPUT);
    // pinMode(RELAY3, OUTPUT);
    // pinMode(RELAY4, OUTPUT);

    // Tắt tất cả các relay (relay thường active LOW)
    digitalWrite(RELAY1, HIGH);
    digitalWrite(RELAY2, HIGH);
    // digitalWrite(RELAY3, HIGH);
    // digitalWrite(RELAY4, HIGH);

    WiFi.begin(ssid, password); // Kết nối Wifi 

    // Kết nối WiFi
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("\nConnected to WiFi");

  /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
    Firebase.reconnectNetwork(true);

    // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
    // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */, 1024 /* Tx buffer size in bytes from 512 - 16384 */);

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.setDoubleDigits(5);

    config.timeout.serverResponse = 10 * 1000;

    unsigned long sendDataPrevMillis = 0;

    //Gửi tên các thiết bị hiện tại được sử dụng lên database
    // sendDevicesName();

    // Khởi tạo servo
    myServo.attach(servoPin);  // Đính kèm servo vào chân GPIO4 

  }

  void loop() 
  {
    // Kiểm tra xem có dữ liệu từ Arduino qua UART2
    if (mySerial.available()) {
      // Đọc dữ liệu từ Serial
      String data = mySerial.readStringUntil('\n');

      // ĐỌC DỮ LIỆU ĐIỀU KHIỂN THIẾT BỊ CỦA USER
      if (Firebase.ready() && (millis() - sendDataPrevMillis > 2000 || sendDataPrevMillis == 0))
      {
        sendDataPrevMillis = millis();

        int pumpFlag;
        int fanFlag;
        int autoFlag;
        int canopyFlag;

        if(Firebase.RTDB.getInt(&fbdo, "/6jk2PWTaobJgQCGPlVso/state", &autoFlag)){
          autoState = autoFlag;
        }else{
          Serial.println(fbdo.errorReason().c_str());
        }

        if(Firebase.RTDB.getInt(&fbdo, "/6jk2PWTaobJgQCGPlVso/pump", &pumpFlag)){
          pumpState = pumpFlag;
        }else{
          Serial.println(fbdo.errorReason().c_str());
        }

        if(Firebase.RTDB.getInt(&fbdo, "/6jk2PWTaobJgQCGPlVso/fan", &fanFlag)){
          fanState = fanFlag;
        }else{
          Serial.println(fbdo.errorReason().c_str());
        }

        if(Firebase.RTDB.getInt(&fbdo, "/6jk2PWTaobJgQCGPlVso/canopy", &canopyFlag)){
          canopyState = canopyFlag;
        }else{
          Serial.println(fbdo.errorReason().c_str());
        }
      }

      Serial.println("PUMPSTATE: " + String(pumpState));
      Serial.println("FANSTATE: " + String(fanState));

      // Phân tách các giá trị bằng dấu phẩy
      int prevIndex = 0;
      int commaIndex = data.indexOf(',');
      
      // Lấy giá trị count
      count = data.substring(prevIndex, commaIndex).toInt();
      prevIndex = commaIndex + 1;

      // Điều kiện để code hoạt động
      if(count == 0){
        return;
      }
      else if(count == globalCount && count != 40){
        return;
      }
      globalCount = count;
      
      // Lấy giá trị h
      commaIndex = data.indexOf(',', prevIndex);
      float h = data.substring(prevIndex, commaIndex).toFloat();
      prevIndex = commaIndex + 1;
      
      // Lấy giá trị t
      commaIndex = data.indexOf(',', prevIndex);
      float t = data.substring(prevIndex, commaIndex).toFloat();
      prevIndex = commaIndex + 1;
      
      // Thiết bị hỏng thì báo thông qua việc gửi mail
      if(h < 0 && t < 0){
        if(isSendEmailDHT == 0){
          String sendTo = "lexuanthanh159@gmail.com"; //địa chỉ nhận 
          String sub = "Device may have problems"; //Tên email
          String emailBody = "DHT22 may have some problems"; //Nội dung email
          sendEmail(sendTo,sub,emailBody);
          isSendEmailDHT = 1; //Gửi rồi thì không gửi lại
          // return;
        }
      }
      else if(h > 0 && t > 0){
        if(count != 40 && isSendEmailDHT == 1){
          isSendEmailDHT = 0;
        }
      }
      // Lấy giá trị soilMoisture
      commaIndex = data.indexOf(',', prevIndex);
      int soilMoisture = data.substring(prevIndex, commaIndex).toInt();
      prevIndex = commaIndex + 1;

      // Thiết bị hỏng thì báo thông qua việc gửi mail
      if(soilMoisture < 0){
        if(isSendEmailsoilMoisture == 0){
          String sendTo = "lexuanthanh159@gmail.com"; //địa chỉ nhận 
          String sub = "Device may have problems"; //Tên email
          String emailBody = "soilMoisture Sensor may have some problems"; //Nội dung email
          sendEmail(sendTo,sub,emailBody);
          isSendEmailsoilMoisture = 1; //Gửi rồi thì không gửi lại
          // return;
        }
      }
      else if(soilMoisture >= 0){
        if(count != 40 && isSendEmailsoilMoisture == 1){
          isSendEmailsoilMoisture = 0;
        }
      }   
      
      // Lấy giá trị lux
      commaIndex = data.indexOf(',', prevIndex);
      float lux = data.substring(prevIndex, commaIndex).toFloat();
      prevIndex = commaIndex + 1;
      
      if(lux < 0 && isSendEmailLS == 0){
        String sendTo = "lexuanthanh159@gmail.com"; //địa chỉ nhận 
        String sub = "Device may have problems"; //Tên email
        String emailBody = "Light Sensor may have some problems"; //Nội dung email
        sendEmail(sendTo,sub,emailBody);
        isSendEmailLS = 1; //Gửi rồi thì không gửi lại
        // return;
      }
      else if(lux >= 0 && count != 40){
        if(isSendEmailLS == 1){
          isSendEmailLS = 0;
        }
      }

      // Lấy giá trị co_ppm
      commaIndex = data.indexOf(',', prevIndex);
      float co_ppm = data.substring(prevIndex, commaIndex).toFloat();
      prevIndex = commaIndex + 1;

      if(co_ppm < 0){
        if(isSendEmailco_ppm == 0){
          String sendTo = "lexuanthanh159@gmail.com"; //địa chỉ nhận 
          String sub = "Device may have problems"; //Tên email
          String emailBody = "MQ-7 Sensor may have some problems"; //Nội dung email
          sendEmail(sendTo,sub,emailBody);
          isSendEmailco_ppm = 1; //Gửi rồi thì không gửi lại
          // return;
        }
      }
      else if(co_ppm >= 0){
        if(count != 40 && isSendEmailco_ppm == 1){
          isSendEmailco_ppm = 0;
        }
      } 

      // Lấy giá trị isRaining
      int isRaining = data.substring(prevIndex).toInt();

      if(isRaining < 0){
        if(isSendEmailisRaining == 0){
          String sendTo = "lexuanthanh159@gmail.com"; //địa chỉ nhận 
          String sub = "Device may have problems"; //Tên email
          String emailBody = "Rain Sensor may have some problems"; //Nội dung email
          sendEmail(sendTo,sub,emailBody);
          isSendEmailisRaining = 1; //Gửi rồi thì không gửi lại
          // return;
        }
      }
      else if(isRaining >= 0){
        if(count != 40 && isSendEmailisRaining == 1){
          isSendEmailisRaining = 0;
        }
      } 

      // Gửi dữ liệu từ cảm bién qua HTTP POST
      if(count != 40){
        sendSensorData(t,h,soilMoisture,lux,co_ppm,isRaining);
      }

      controlDevice(1, pumpState, RELAY1, soilMoisture); // đièu khiển máy bơm
      controlDevice(2, fanState, RELAY2, t); // điều khiển quạt

      // Điều khiển servo dựa trên cảm biến mưa
      if(canopyState == 0)
      {
        Serial.println("Turning OFF the canopy by User!");
        myServo.write(0); // Quay servo về 0 độ - tắt mái che
        if(autoState == 1)
        {
          if (isRaining == 1) {
            Serial.println("Auto turning ON the canopy!");
            myServo.write(90); // Quay servo đến 90 độ - bật mái che
          } else {
            if(lux <= 3000){
              Serial.println("Auto turning OFF the canopy!");
              myServo.write(0); // Quay servo về 0 độ - tắt mái che
            }
            else{
              Serial.println("Auto turning ON the canopy!");
              myServo.write(90); // Quay servo đến 90 độ - bật mái che
            }
          }
        }
      }
      else{
        Serial.println("Turning ON the canopy by User!");
        myServo.write(90); // Quay servo đến 90 độ - bật mái che
      }
      
    }
  }
  //--------------------------------------------------------------------------- GỬI EMAIL--------------------------------------------------------------------------------
  void sendEmail(String sendTo, String sub, String emailBody){
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String serverURL = "https://greeniotapi-812240783935.us-central1.run.app/api/Email";

      http.begin(serverURL); // Bắt đầu kết nối HTTP
      http.addHeader("Content-Type", "application/json");

      String jsonData = "{"
        "\"to\": \"" + sendTo + "\", "
        "\"subject\": \"" + sub + "\", "
        "\"body\": \"" + emailBody + "\""
        "}";

      int httpCode = http.POST(jsonData); // Gửi yêu cầu POST với dữ liệu JSON

      if (httpCode == 200) 
      {
        // Xử lý mã 200: Thành công
        Serial.println("Email sent successfully");
      } 
      else 
      {
        Serial.print("Error sending email, HTTP code: ");
        Serial.println(httpCode);
      }

      http.end(); // Kết thúc HTTP request
    }
  }

  //--------------------------------------------------------------------------- GỬI DATA CẢM BIẾN--------------------------------------------------------------------------------
  void sendSensorData(float t,float h,int soilMoisture,float lux,float co_ppm,int isRaining){
    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String serverURL = "https://greeniotapi-812240783935.us-central1.run.app/api/sensor-data?gardenId=" + gardenId;

      http.begin(serverURL); // Bắt đầu kết nối HTTP
      http.addHeader("Content-Type", "application/json");

      String jsonData = "{"
        "\"temperature\": " + String(t) + ", "
        "\"humidity\": " + String(h) + ", "
        "\"soilMoisture\": " + String(soilMoisture) + ", "
        "\"lightLevel\": " + String(lux) + ", "
        "\"coPpm\": " + String(co_ppm) + ", "
        "\"isRaining\": " + String(isRaining) +
        "}";


      int httpCode = http.POST(jsonData); // Gửi yêu cầu POST với dữ liệu JSON

      if (httpCode == 200) 
      {
        // Xử lý mã 200: Thành công
        Serial.println("Data sent successfully");
      } 
      else 
      {
        Serial.print("Error sending data, HTTP code: ");
        Serial.println(httpCode);
      }

      http.end(); // Kết thúc HTTP request
    }
  }

  //--------------------------------------------------------------------------- GỬI TÊN DEVICES ĐANG SỬ DỤNG--------------------------------------------------------------------------------
  void sendDevicesName(){
    if(WiFi.status() == WL_CONNECTED) {
      String deviceName = "MQ-7";
      HTTPClient http;

      String serverURL = "https://greeniotapi-812240783935.us-central1.run.app/api/devices?gardenId=" + gardenId;

      http.begin(serverURL); // Bắt đầu kết nối HTTP
      http.addHeader("Content-Type", "application/json");

      String jsonData = "{"
        "\"name\": " + deviceName + "}";
        // "\"name\": " + String(t) + ", "
        // "\"humidity\": " + String(h) + ", "
        // "\"soilMoisture\": " + String(soilMoisture) + ", "
        // "\"lightLevel\": " + String(lux) + ", "
        // "\"coPpm\": " + String(co_ppm) + ", "
        // "\"isRaining\": " + String(isRaining) +
        // "}";

      int httpCode = http.POST(jsonData); // Gửi yêu cầu POST với dữ liệu JSON

      if (httpCode == 200) 
      {
        // Xử lý mã 200: Thành công
        Serial.println("Device's name sent successfully");
      } 
      else 
      {
        Serial.print("Error sending device's name, HTTP code: ");
        Serial.println(httpCode);
      }

      http.end(); // Kết thúc HTTP request
    }

  }

  //--------------------------------------------------------------------------- ĐIỀU KHIỂN CÁC THIẾT BỊ (MÁY BƠM,QUẠT,...)--------------------------------------------------------------------------------
  void controlDevice(int deviceNum, int deviceState, int RELAY, int senData) 
  {
    String deviceName = "";

    if(deviceNum == 1){
      deviceName = "pump";
    }
    else{
      deviceName = "fan";
    }

    if (deviceState == 0 && autoState == 1) 
    {
      // Serial.println("Turning OFF the " + deviceName + " by User!");
      // digitalWrite(RELAY, HIGH); // Máy bơm tắt

      if(deviceNum == 1){ // là máy bơm
        if (senData < 55) 
        {
            Serial.println("Auto turning ON the " + deviceName + "!");
            digitalWrite(RELAY, LOW); // Máy bơm bật
        } 
        else 
        {
            Serial.println("Auto turning OFF the " + deviceName + "!");
            digitalWrite(RELAY, HIGH); // Máy bơm tắt
        }
      }
      else{
        if (senData > 32) 
        {
            Serial.println("Auto turning ON the " + deviceName + "!");
            digitalWrite(RELAY, LOW); // Máy quạt bật
        } 
        else 
        {
            Serial.println("Auto turning OFF the " + deviceName + "!");
            digitalWrite(RELAY, HIGH); // Máy quạt tắt
        }
      }
    }
    else if(deviceState == 0 && autoState == 0){
      Serial.println("Turning OFF the " + deviceName + " by User!");
      digitalWrite(RELAY, HIGH); // Máy bơm tắt
    } 
    else if(deviceState == 1) 
    {
      Serial.println("Turning ON the " + deviceName + " by User!");
      digitalWrite(RELAY, LOW); // Máy bơm bật
    }
  }

