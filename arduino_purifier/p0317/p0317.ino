#include "DHT.h"
#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define DHTPIN 4     
#define DHTTYPE DHT11 
const int buttonPin = 6;    
const int mosfetPin = 5;    

// 狀態變數
int state = 0;              
bool lastButtonState = HIGH; 
unsigned long lastDebounceTime = 0; 
unsigned long debounceDelay = 50;   

uint8_t buffer[32];
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial pmsSerial(2, 3);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);
  pmsSerial.begin(9600);
  dht.begin();

  lcd.init();          
  lcd.backlight();
  
  pinMode(buttonPin, INPUT_PULLUP); 
  pinMode(mosfetPin, OUTPUT);
  
  // 初始顯示
  updateFanSpeed(0); 
  Serial.println(F("System On"));
}

void loop() {
  // --- 1. 讀取與顯示 DHT11 ---
  static unsigned long lastSensorTime = 0;
  if (millis() - lastSensorTime > 2000) { // 每 2 秒更新一次感測器，避免卡死
    lastSensorTime = millis();
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    lcd.setCursor(0, 0); 
    lcd.print(F("T:"));
    lcd.print(t, 1);
    lcd.setCursor(8, 0);  
    lcd.print(F("H:"));
    lcd.print(h, 1);
    lcd.print(F("% "));
  }

  // --- 2. 讀取 PMS5003 ---
  if (pmsSerial.available() >= 32) {
    if (pmsSerial.read() == 0x42 && pmsSerial.read() == 0x4D) {
      pmsSerial.readBytes(&buffer[2], 30);
      int pm25 = (buffer[12] << 8) | buffer[13];
      
      lcd.setCursor(0, 1); 
      lcd.print(F("PM:"));
      if(pm25 > 999) lcd.print(F("err "));
      else {
        lcd.print(pm25);
        lcd.print(F("   ")); // 清除殘影
      }
    }
  }

  // --- 3. 按鈕邏輯 (核心修正區) ---
  int reading = digitalRead(buttonPin);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // 只有當狀態真的改變時才執行
    static bool buttonPressed = false;
    if (reading == LOW && !buttonPressed) {
      state++;
      if (state > 4) state = 0;
      updateFanSpeed(state);
      buttonPressed = true; // 標記已按下，防止連發
    } else if (reading == HIGH) {
      buttonPressed = false;
    }
  }
  lastButtonState = reading;
}

// 根據狀態設定 PWM 與 LCD 顯示
void updateFanSpeed(int mode) {
  lcd.setCursor(8, 1); 
  lcd.print(F("FAN:"));
  lcd.setCursor(12, 1);
  lcd.print(F("    ")); // 先清除該區域
  lcd.setCursor(12, 1);

  switch (mode) {
    case 0:
      analogWrite(mosfetPin, 255);
      Serial.println(F("Speed: 100%"));
      lcd.print(F("100"));
      break;
    case 1:
      analogWrite(mosfetPin, 166);
      Serial.println(F("Speed: 65%"));
      lcd.print(F("65"));
      break;
    case 2:
      analogWrite(mosfetPin, 127);
      Serial.println(F("Speed: 50%"));
      lcd.print(F("50"));
      break;
    case 3:
      analogWrite(mosfetPin, 64);
      Serial.println(F("Speed: 25%"));
      lcd.print(F("25"));
      break;
    case 4:
      analogWrite(mosfetPin, 0);
      Serial.println(F("Speed: 0%"));
      lcd.print(F("0"));
      break;
  }
}