#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <avr/sleep.h>
#include "DHT.h"


#define DHTPIN              2
#define DHTTYPE             DHT11
#define BUTTON_PIN          3
#define PROXIMITY_LED_PIN   4     // Obstacle Alert Red LED (<20cm)
#define MOTOR_PIN           5     // Driver pin for 3V DC Motor
#define BUZZER_PIN          6
#define ECHO_PIN            7
#define TRIG_PIN            8
#define SERVO_PIN           9
#define TEMP_GREEN_LED_PIN  10    // Green LED: Temp < 26C
#define TEMP_RED_LED_PIN    11    // Red LED: Temp >= 26C
#define LDR_PIN             A0

// Thresholds
const float TEMP_ALERT_THRESHOLD = 26.0;
const int LDR_OFF_THRESHOLD = 30;
const int LDR_ON_THRESHOLD  = 20;

// LCD configuration (Default I2C address is 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
Servo radarServo;

//State and Timing Variables
int servoAngle = 0;
int servoStep = 5;
bool isBacklightOn = true;
unsigned long lastSensorRead = 0;
unsigned long lastServoMove = 0;

void setup() {
  Serial.begin(9600);
  
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(PROXIMITY_LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TEMP_GREEN_LED_PIN, OUTPUT);
  pinMode(TEMP_RED_LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(MOTOR_PIN, LOW);
  digitalWrite(TEMP_GREEN_LED_PIN, LOW);
  digitalWrite(TEMP_RED_LED_PIN, LOW);

  // Initializing I2C communication and LCD
  Wire.begin();
  lcd.init();
  lcd.backlight();
  
  // Initializing DHT11 and Servo
  dht.begin();
  radarServo.attach(SERVO_PIN);
  radarServo.write(0);
}

void loop() {
  updateBacklightDirect();

  if (digitalRead(BUTTON_PIN) == LOW) {
    handleEmergency();
    return;
  }

  // Servo Sweep every 30ms
  if (millis() - lastServoMove >= 30) {
    lastServoMove = millis();
    servoAngle += servoStep;
    if (servoAngle >= 180 || servoAngle <= 0) {
      servoStep = -servoStep;
    }
    radarServo.write(servoAngle);
  }

  // Sensor Update, Display Refresh (every 250ms)
  if (millis() - lastSensorRead >= 250) {
    lastSensorRead = millis();

    // Ultrasonic Distance Measurement
    long distance = getDistanceCM();
    bool proximityAlert = (distance > 0 && distance < 20);

    if (proximityAlert) {
      digitalWrite(PROXIMITY_LED_PIN, HIGH);
    } else {
      digitalWrite(PROXIMITY_LED_PIN, LOW);
    }

    // Light Sensor
    int ldrValue = analogRead(LDR_PIN);
    int lightPercent = map(ldrValue, 0, 300, 0, 100);
    lightPercent = constrain(lightPercent, 0, 100);

    // DHT11 Environmental Readings
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    // Temperature LED Logic (< 26C Green ON, >= 26C Red ON)
    if (!isnan(temp)) {
      if (temp < TEMP_ALERT_THRESHOLD) {
        digitalWrite(TEMP_GREEN_LED_PIN, HIGH);
        digitalWrite(TEMP_RED_LED_PIN, LOW);
      } else {
        digitalWrite(TEMP_GREEN_LED_PIN, LOW);
        digitalWrite(TEMP_RED_LED_PIN, HIGH);
      }
    }

    // LCD Line 1: Temp, Humidity, Light
    lcd.setCursor(0, 0);
    lcd.print("T:");
    if (isnan(temp)) lcd.print("--");
    else lcd.print((int)temp);
    lcd.print((char)223);

    lcd.print(" H:");
    if (isnan(hum)) lcd.print("--");
    else lcd.print((int)hum);
    lcd.print("%");

    lcd.print(" L:");
    if (lightPercent < 10) lcd.print(" ");
    lcd.print(lightPercent);
    lcd.print("%");

    // --- LCD Line 2: Distance & Servo Angle ---
    lcd.setCursor(0, 1);
    lcd.print("D:");
    if (distance >= 100) lcd.print(distance);
    else if (distance >= 10) { lcd.print(" "); lcd.print(distance); }
    else { lcd.print("  "); lcd.print(distance); }
    lcd.print("cm A:");
    if (servoAngle < 10) lcd.print("  ");
    else if (servoAngle < 100) lcd.print(" ");
    lcd.print(servoAngle);
    lcd.print((char)223);
    lcd.print(" ");

    // --- Serial JSON Output ---
    bool isAlarm = proximityAlert;
    sendJsonData(temp, hum, servoAngle, distance, ldrValue, lightPercent, isAlarm);
  }
}

void updateBacklightDirect() {
  int ldr = analogRead(LDR_PIN);
  
  // DARK, analog value > 30
  if (isBacklightOn && ldr > LDR_OFF_THRESHOLD) {
    lcd.noBacklight();
    isBacklightOn = false;
  } 
  // BRIGHT, analog value < 20
  else if (!isBacklightOn && ldr < LDR_ON_THRESHOLD) {
    lcd.backlight();
    isBacklightOn = true;
  }
}

long getDistanceCM() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 25000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

void sendJsonData(float temp, float hum, int angle, long distance, int ldr, int lightPercent, bool alarm) {
  Serial.print("{\"temperature\":");
  if (isnan(temp)) Serial.print("null");
  else Serial.print(temp, 1);

  Serial.print(",\"humidity\":");
  if (isnan(hum)) Serial.print("null");
  else Serial.print(hum, 1);

  Serial.print(",\"servo_angle\":");
  Serial.print(angle);

  Serial.print(",\"distance\":");
  Serial.print(distance);

  Serial.print(",\"ldr_raw\":");
  Serial.print(ldr);

  Serial.print(",\"light_percent\":");
  Serial.print(lightPercent);

  Serial.print(",\"alarm\":");
  Serial.print(alarm ? "true" : "false");
  Serial.println("}");
}

void handleEmergency() {
  lcd.backlight();
  isBacklightOn = true;
  digitalWrite(PROXIMITY_LED_PIN, HIGH);
  digitalWrite(TEMP_RED_LED_PIN, HIGH);
  digitalWrite(TEMP_GREEN_LED_PIN, LOW);
  digitalWrite(MOTOR_PIN, HIGH);
  tone(BUZZER_PIN, 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("** EMERGENCY ** ");
  lcd.setCursor(0, 1);
  lcd.print(" EMERGENCY FIRE ");

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  long distance = getDistanceCM();
  int ldrValue = analogRead(LDR_PIN);
  int lightPercent = constrain(map(ldrValue, 0, 300, 0, 100), 0, 100);

  sendJsonData(temp, hum, servoAngle, distance, ldrValue, lightPercent, true);

  unsigned long buttonPressStart = millis();

  // SHUTDOWN
  while (digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - buttonPressStart >= 5000) {
      shutDownSystem();
    }
    delay(10);
  }

  noTone(BUZZER_PIN);
  digitalWrite(PROXIMITY_LED_PIN, LOW);
  digitalWrite(TEMP_RED_LED_PIN, LOW);
  digitalWrite(MOTOR_PIN, LOW);
  lcd.clear();
}

void shutDownSystem() {

  noTone(BUZZER_PIN);
  digitalWrite(PROXIMITY_LED_PIN, LOW);
  digitalWrite(TEMP_GREEN_LED_PIN, LOW);
  digitalWrite(TEMP_RED_LED_PIN, LOW);
  digitalWrite(MOTOR_PIN, LOW);
  radarServo.detach();

  lcd.clear();
  lcd.noBacklight();
  lcd.noDisplay();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}