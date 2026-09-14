#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// -------------------------
// Pin definitions
// -------------------------
#define DHT_PIN 2
#define DHT_TYPE DHT22

#define RELAY_PIN 5
#define BUTTON_PIN 7
#define BUZZER_PIN 8

// -------------------------
// Components
// -------------------------
DHT dht(DHT_PIN, DHT_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------------
// Temperature settings
// -------------------------
const float HEATER_ON_TEMP  = 29.0;
const float HEATER_OFF_TEMP = 31.0;

// -------------------------
// Variables
// -------------------------
bool heaterOn = false;
bool systemEnabled = true;

unsigned long lastReadTime = 0;
const unsigned long readInterval = 2000;

// Button state
bool lastButtonState = HIGH;


// -------------------------
// SETUP
// -------------------------
void setup() {

  Serial.begin(9600);

  // Pin modes
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Start with heater OFF
  digitalWrite(RELAY_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // Start sensors
  dht.begin();

  // Start LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Fermentation");
  lcd.setCursor(0, 1);
  lcd.print("Controller");
  
  delay(2000);

  lcd.clear();
}


// -------------------------
// MAIN LOOP
// -------------------------
void loop() {

  // -------------------------
  // Read pushbutton
  // -------------------------
  bool buttonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && buttonState == LOW) {

    systemEnabled = !systemEnabled;

    // If system is disabled, turn heater off
    if (!systemEnabled) {
      heaterOn = false;
      digitalWrite(RELAY_PIN, LOW);
      digitalWrite(BUZZER_PIN, LOW);
    }

    delay(200);
  }

  lastButtonState = buttonState;


  // -------------------------
  // Read temperature every 2 sec
  // -------------------------
  if (millis() - lastReadTime >= readInterval) {

    lastReadTime = millis();

    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();


    // Check sensor
    if (isnan(temperature) || isnan(humidity)) {

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sensor Error!");

      Serial.println("DHT22 error");

      return;
    }


    // -------------------------
    // Temperature control
    // -------------------------
    if (systemEnabled) {

      // Temperature too low
      if (temperature < HEATER_ON_TEMP) {

        heaterOn = true;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(BUZZER_PIN, LOW);
      }


      // Temperature too high
      else if (temperature > HEATER_OFF_TEMP) {

        heaterOn = false;
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(BUZZER_PIN, HIGH);
      }


      // Normal range
      else {

        digitalWrite(BUZZER_PIN, LOW);
      }
    }


    // -------------------------
    // LCD display
    // -------------------------
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature, 1);
    lcd.print((char)223);
    lcd.print("C ");

    lcd.print("H:");
    lcd.print(humidity, 0);
    lcd.print("%");


    lcd.setCursor(0, 1);

    if (!systemEnabled) {

      lcd.print("SYSTEM: OFF");

    } else if (heaterOn) {

      lcd.print("HEATER: ON ");

    } else if (temperature > HEATER_OFF_TEMP) {

      lcd.print("HIGH TEMP! ");

    } else {

      lcd.print("HEATER: OFF");
    }


    // -------------------------
    // Serial Monitor
    // -------------------------
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" C | Humidity: ");
    Serial.print(humidity);
    Serial.print("% | Heater: ");

    if (heaterOn) {
      Serial.println("ON");
    } else {
      Serial.println("OFF");
    }
  }
}
