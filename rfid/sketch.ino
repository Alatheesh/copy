#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 rfid(SS_PIN, RST_PIN);

void setup() {

  Serial.begin(9600);

  SPI.begin();

  rfid.PCD_Init();

  Serial.println("RFID Attendance System");
  Serial.println("Scan your RFID card...");

}

void loop() {

  if (!rfid.PICC_IsNewCardPresent()) {
    return;
  }

  if (!rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.print("Card UID: ");

  for (byte i = 0; i < rfid.uid.size; i++) {

    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");

  }

  Serial.println();

  // Card 1
  if (rfid.uid.uidByte[0] == 0xC0 &&
      rfid.uid.uidByte[1] == 0xFF &&
      rfid.uid.uidByte[2] == 0xEE &&
      rfid.uid.uidByte[3] == 0x99) {

    Serial.println("TOPPER 1");
    Serial.println("Attendance: PRESENT");

  }

  // Card 2
  else if (rfid.uid.uidByte[0] == 0x11 &&
           rfid.uid.uidByte[1] == 0x22 &&
           rfid.uid.uidByte[2] == 0x33 &&
           rfid.uid.uidByte[3] == 0x44) {

    Serial.println("TOPPER 2");
    Serial.println("Attendance: PRESENT");

  }

  // Unknown card
  else {

    Serial.println("Unknown Card");
    Serial.println("Attendance: NO ATTENDANCE");

  }

  Serial.println("--------------------");

  rfid.PICC_HaltA();

}
