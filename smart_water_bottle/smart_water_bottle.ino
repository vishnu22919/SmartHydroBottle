#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <MPU6050.h>
#include <math.h>

// === Pin Definitions ===
#define ONE_WIRE_BUS 3          // DS18B20 on D3
#define WATER_LEVEL_PIN 5       // Water level sensor on D5
#define LED_PIN 2               // LED on D2

// === Objects and Sensors ===
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD at I2C address 0x27
MPU6050 mpu;

// === MPU6050 Data ===
int16_t ax, ay, az, gx, gy, gz;
float pitch = 0.0;
float roll = 0.0;
unsigned long lastTime = 0;

// === Sip Detection ===
float sipThreshold = 15000; // Change based on testing
bool sipDetected = false;
int sipCount = 0;

// === Setup ===
void setup() {
  Serial.begin(9600);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  tempSensor.begin();
  lcd.init();
  lcd.backlight();

  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection failed");
    lcd.setCursor(0, 0);
    lcd.print("MPU FAIL");
  }

  lcd.setCursor(0, 0);
  lcd.print("Smart Bottle");
  delay(1000);
  lcd.clear();

  lastTime = millis();
}

// === LED Blink Function ===
void blinkLED() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(1500);
    digitalWrite(LED_PIN, LOW);
    delay(1500);
  }
}

// === Main Loop ===
void loop() {
  // Read temperature
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);

  // Read water level
  int waterLevel = digitalRead(WATER_LEVEL_PIN);

  // Read MPU6050
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  long totalAccel = abs(ax) + abs(ay) + abs(az);

  // Sip detection
  if (totalAccel > sipThreshold && !sipDetected) {
    sipCount++;
    sipDetected = true;
    blinkLED(); // Trigger LED blink after sip
  }
  if (totalAccel < sipThreshold - 2000) {
    sipDetected = false;
  }

  // Pitch and roll estimation
  unsigned long currentTime = millis();
  float dt = (currentTime - lastTime) / 1000.0;
  lastTime = currentTime;

  float accPitch = atan2(ay, az) * 180 / PI;
  float accRoll = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;
  float gyroPitchRate = gx / 131.0;
  float gyroRollRate  = gy / 131.0;

  pitch = 0.98 * (pitch + gyroPitchRate * dt) + 0.02 * accPitch;
  roll  = 0.98 * (roll  + gyroRollRate  * dt) + 0.02 * accRoll;

  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print("T:");
  if (tempC == DEVICE_DISCONNECTED_C) {
    lcd.print("Err ");
  } else {
    lcd.print(tempC, 1);
    lcd.print((char)223);
    lcd.print("C ");
  }

  lcd.setCursor(9, 0);
  lcd.print("S:");
  lcd.print(sipCount);

  lcd.setCursor(0, 1);
  lcd.print("Level:");
  lcd.print(waterLevel == HIGH ? "OK " : "Low");

  // Print to Serial
  Serial.print("Temp: "); Serial.print(tempC); Serial.print(" C, ");
  Serial.print("Level: "); Serial.print(waterLevel == HIGH ? "OK" : "Low"); Serial.print(", ");
  Serial.print("Sips: "); Serial.print(sipCount); Serial.print(", ");
  Serial.print("Pitch: "); Serial.print(pitch); Serial.print(" deg, ");
  Serial.print("Roll: "); Serial.println(roll);

  delay(200);
}

