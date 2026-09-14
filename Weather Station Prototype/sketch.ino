#include <Wire.h>
#include <math.h>

// =====================================================
// PIN DEFINITIONS
// =====================================================

#define DHT_PIN 2
#define LDR_PIN A0
#define LED_PIN 7
#define BUZZER_PIN 8

#define LCD_ADDR 0x27
#define BMP180_ADDR 0x77

// =====================================================
// SENSOR VARIABLES
// =====================================================

float temperature = 0;
float humidity = 0;

// =====================================================
// BMP180 CALIBRATION DATA
// Renamed to avoid Arduino B1 macro conflict
// =====================================================

int16_t bmpAC1;
int16_t bmpAC2;
int16_t bmpAC3;

uint16_t bmpAC4;
uint16_t bmpAC5;
uint16_t bmpAC6;

int16_t bmpCalB1;
int16_t bmpCalB2;
int16_t bmpMB;
int16_t bmpMC;
int16_t bmpMD;

int32_t bmpB5;

// =====================================================
// LCD FUNCTIONS
// =====================================================

void lcdWrite4Bits(byte data) {

  Wire.beginTransmission(LCD_ADDR);
  Wire.write(data | 0x0C);
  Wire.endTransmission();

  delayMicroseconds(1);

  Wire.beginTransmission(LCD_ADDR);
  Wire.write(data | 0x08);
  Wire.endTransmission();

  delayMicroseconds(50);
}


void lcdSend(byte value, byte mode) {

  byte high = value & 0xF0;
  byte low = (value << 4) & 0xF0;

  lcdWrite4Bits(high | mode);
  lcdWrite4Bits(low | mode);
}


void lcdCommand(byte command) {

  lcdSend(command, 0);
}


void lcdChar(byte character) {

  lcdSend(character, 1);
}


void lcdPrint(String text) {

  for (int i = 0; i < text.length(); i++) {

    lcdChar(text[i]);
  }
}


void lcdSetCursor(byte col, byte row) {

  byte address;

  if (row == 0) {

    address = 0x80 + col;

  } else {

    address = 0xC0 + col;
  }

  lcdCommand(address);
}


void lcdClear() {

  lcdCommand(0x01);

  delay(2);
}


void lcdInit() {

  delay(50);

  lcdWrite4Bits(0x30);

  delay(5);

  lcdWrite4Bits(0x30);

  delayMicroseconds(150);

  lcdWrite4Bits(0x30);

  lcdWrite4Bits(0x20);

  lcdCommand(0x28);
  lcdCommand(0x08);
  lcdCommand(0x01);
  lcdCommand(0x06);
  lcdCommand(0x0C);
}


// =====================================================
// BMP180
// =====================================================

int16_t bmpRead16(byte address) {

  Wire.beginTransmission(BMP180_ADDR);

  Wire.write(address);

  Wire.endTransmission();

  Wire.requestFrom(BMP180_ADDR, 2);

  return (Wire.read() << 8) | Wire.read();
}


void bmpWrite8(byte address, byte value) {

  Wire.beginTransmission(BMP180_ADDR);

  Wire.write(address);
  Wire.write(value);

  Wire.endTransmission();
}


void bmpBegin() {

  bmpAC1 = bmpRead16(0xAA);
  bmpAC2 = bmpRead16(0xAC);
  bmpAC3 = bmpRead16(0xAE);

  bmpAC4 = bmpRead16(0xB0);
  bmpAC5 = bmpRead16(0xB2);
  bmpAC6 = bmpRead16(0xB4);

  bmpCalB1 = bmpRead16(0xB6);
  bmpCalB2 = bmpRead16(0xB8);

  bmpMB = bmpRead16(0xBA);
  bmpMC = bmpRead16(0xBC);
  bmpMD = bmpRead16(0xBE);
}


// =====================================================
// READ BMP180 TEMPERATURE
// =====================================================

int32_t bmpReadTemperature() {

  bmpWrite8(0xF4, 0x2E);

  delay(5);

  int32_t UT = bmpRead16(0xF6);

  int32_t X1 =
    ((UT - bmpAC6) * bmpAC5) >> 15;

  int32_t X2 =
    ((int32_t)bmpMC << 11) /
    (X1 + bmpMD);

  bmpB5 = X1 + X2;

  return (bmpB5 + 8) >> 4;
}


// =====================================================
// READ BMP180 PRESSURE
// =====================================================

long bmpReadPressure() {

  bmpWrite8(0xF4, 0x34);

  delay(5);

  Wire.beginTransmission(BMP180_ADDR);

  Wire.write(0xF6);

  Wire.endTransmission();

  Wire.requestFrom(BMP180_ADDR, 3);

  long MSB = Wire.read();
  long LSB = Wire.read();
  long XLSB = Wire.read();

  long UP =
    ((MSB << 16) +
     (LSB << 8) +
     XLSB) >> 8;


  long X1 =
    ((bmpB5 - 4000) * bmpCalB2) >> 11;

  long X2 =
    (bmpAC2 * (bmpB5 - 4000)) >> 11;

  long X3 =
    X1 + X2;

  long B3 =
    (((bmpAC1 * 4 + X3) + 2) / 4);


  X1 =
    (bmpAC3 * (bmpB5 - 4000)) >> 13;

  X2 =
    (bmpCalB1 *
     ((bmpB5 - 4000) *
      (bmpB5 - 4000) >> 12)) >> 16;

  X3 =
    ((X1 + X2) + 2) >> 2;


  unsigned long B4 =
    (bmpAC4 *
     (unsigned long)(X3 + 32768)) >> 15;


  unsigned long B7 =
    ((unsigned long)UP - B3) * 50000;


  long pressure;

  if (B7 < 0x80000000) {

    pressure =
      (B7 * 2) / B4;

  } else {

    pressure =
      (B7 / B4) * 2;
  }


  X1 =
    (pressure >> 8) *
    (pressure >> 8);

  X1 =
    (X1 * 3038) >> 16;

  X2 =
    (-7357 * pressure) >> 16;

  pressure =
    pressure +
    ((X1 + X2 + 3791) >> 4);


  return pressure;
}


// =====================================================
// DHT22
// =====================================================

bool readDHT22() {

  uint8_t data[5] =
    {0, 0, 0, 0, 0};


  pinMode(DHT_PIN, OUTPUT);

  digitalWrite(DHT_PIN, LOW);

  delay(2);

  digitalWrite(DHT_PIN, HIGH);

  delayMicroseconds(30);

  pinMode(DHT_PIN, INPUT_PULLUP);


  unsigned long timeout = micros();


  while (digitalRead(DHT_PIN) == HIGH) {

    if (micros() - timeout > 100)
      return false;
  }


  timeout = micros();

  while (digitalRead(DHT_PIN) == LOW) {

    if (micros() - timeout > 100)
      return false;
  }


  timeout = micros();

  while (digitalRead(DHT_PIN) == HIGH) {

    if (micros() - timeout > 100)
      return false;
  }


  for (int i = 0; i < 40; i++) {

    timeout = micros();

    while (digitalRead(DHT_PIN) == LOW) {

      if (micros() - timeout > 100)
        return false;
    }


    unsigned long start =
      micros();


    timeout = start;

    while (digitalRead(DHT_PIN) == HIGH) {

      if (micros() - timeout > 100)
        return false;
    }


    unsigned long duration =
      micros() - start;


    data[i / 8] <<= 1;


    if (duration > 40)
      data[i / 8] |= 1;
  }


  uint8_t checksum =
    data[0] +
    data[1] +
    data[2] +
    data[3];


  if (checksum != data[4])
    return false;


  humidity =
    ((data[0] << 8) | data[1]) / 10.0;


  temperature =
    ((data[2] & 0x7F) << 8 | data[3]) / 10.0;


  if (data[2] & 0x80)
    temperature =
      -temperature;


  return true;
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(9600);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin();

  lcdInit();


  lcdSetCursor(0, 0);
  lcdPrint("WEATHER STATION");

  lcdSetCursor(0, 1);
  lcdPrint("Starting...");

  delay(2000);


  bmpBegin();

  lcdClear();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // -------------------------------
  // DHT22
  // -------------------------------

  bool dhtOK =
    readDHT22();


  if (!dhtOK) {

    lcdClear();

    lcdSetCursor(0, 0);
    lcdPrint("DHT22 ERROR!");

    Serial.println("DHT22 ERROR");

    delay(2000);

    return;
  }


  // -------------------------------
  // BMP180
  // -------------------------------

  bmpReadTemperature();

  long pressure =
    bmpReadPressure();

  float pressureHpa =
    pressure / 100.0;


  // -------------------------------
  // LDR
  // -------------------------------

  int lightValue =
    analogRead(LDR_PIN);


  int lightPercent =
    map(lightValue,
        0,
        1023,
        0,
        100);


  lightPercent =
    constrain(lightPercent,
              0,
              100);


  // -------------------------------
  // WEATHER STATUS
  // -------------------------------

  String status;


  if (temperature >= 35) {

    status = "HOT";

  }
  else if (humidity >= 85) {

    status = "HUMID";

  }
  else if (lightPercent >= 70) {

    status = "SUNNY";

  }
  else if (lightPercent >= 35) {

    status = "CLOUDY";

  }
  else {

    status = "NIGHT";
  }


  // -------------------------------
  // ALERT
  // -------------------------------

  if (temperature >= 35 ||
      humidity >= 85) {

    digitalWrite(LED_PIN, HIGH);

    tone(BUZZER_PIN, 1000);

  }
  else {

    digitalWrite(LED_PIN, LOW);

    noTone(BUZZER_PIN);
  }


  // -------------------------------
  // SERIAL MONITOR
  // -------------------------------

  Serial.println();
  Serial.println("======================");

  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  Serial.print("Pressure: ");
  Serial.print(pressureHpa);
  Serial.println(" hPa");

  Serial.print("Light: ");
  Serial.print(lightPercent);
  Serial.println(" %");

  Serial.print("Weather: ");
  Serial.println(status);

  Serial.println("======================");


  // =================================================
  // LCD SCREEN 1
  // =================================================

  lcdClear();

  lcdSetCursor(0, 0);

  lcdPrint("T:");
  lcdPrint(String(temperature, 1));
  lcdPrint("C H:");
  lcdPrint(String(humidity, 0));
  lcdPrint("%");


  lcdSetCursor(0, 1);

  lcdPrint("Weather:");
  lcdPrint(status);


  delay(3000);


  // =================================================
  // LCD SCREEN 2
  // =================================================

  lcdClear();

  lcdSetCursor(0, 0);

  lcdPrint("PRESSURE");


  lcdSetCursor(0, 1);

  lcdPrint(String(pressureHpa, 0));
  lcdPrint(" hPa");


  delay(3000);


  // =================================================
  // LCD SCREEN 3
  // =================================================

  lcdClear();

  lcdSetCursor(0, 0);

  lcdPrint("LIGHT:");
  lcdPrint(String(lightPercent));
  lcdPrint("%");


  lcdSetCursor(0, 1);

  lcdPrint("STATUS:");
  lcdPrint(status);


  delay(3000);
}
