
#include <Arduino.h>

// 73K212AL control interface
const uint8_t PIN_EXCLK = 2;   // Modem pin 19
const uint8_t PIN_AD7   = 3;   // Modem pin 11
const uint8_t PIN_RD    = 4;   // Modem pin 14
const uint8_t PIN_WR    = 5;   // Modem pin 13
const uint8_t PIN_ALE   = 6;   // Modem pin 12
const uint8_t PIN_CS    = 7;   // Modem pin 20
const uint8_t PIN_AD0   = 8;   // Modem pin 4
const uint8_t PIN_AD1   = 9;   // Modem pin 5
const uint8_t PIN_AD2   = 10;  // Modem pin 6

// Set output level before enabling GPIO output
void setOutput(uint8_t pin, uint8_t state) {
  digitalWrite(pin, state);
  pinMode(pin, OUTPUT);
}

// Write one 8-bit modem configuration register
void writeModemRegister(uint8_t address, uint8_t value) {
  digitalWrite(PIN_RD, HIGH);  // RD always HIGH
  digitalWrite(PIN_WR, HIGH);
  digitalWrite(PIN_EXCLK, LOW);

  // Select register address
  digitalWrite(PIN_AD0, (address & 0x01) ? HIGH : LOW);
  digitalWrite(PIN_AD1, (address & 0x02) ? HIGH : LOW);
  digitalWrite(PIN_AD2, (address & 0x04) ? HIGH : LOW);

  delayMicroseconds(10);

  // Send 8 bits, least significant bit first
  for (int bit = 0; bit < 8; bit++) {
    digitalWrite(PIN_EXCLK, HIGH);

    digitalWrite(PIN_AD7,
      (value & (1u << bit)) ? HIGH : LOW);

    delayMicroseconds(10);

    // Modem samples data on falling edge
    digitalWrite(PIN_EXCLK, LOW);
    delayMicroseconds(10);
  }

  // Write register on rising edge of WR
  delayMicroseconds(10);
  digitalWrite(PIN_WR, LOW);
  delayMicroseconds(10);
  digitalWrite(PIN_WR, HIGH);
  delayMicroseconds(10);
}

void printHexByte(uint8_t value) {
  if (value < 0x10) Serial.print('0');
  Serial.print(value, HEX);
}

// Transmit one byte and check the returned byte
void testOneByte(uint8_t sent) {
  // Clear old received data
  while (Serial1.available()) {
    Serial1.read();
  }

  Serial.print("Sent 0x");
  printHexByte(sent);

  // Transmit byte using Pico UART
  Serial1.write(sent);
  Serial1.flush();

  // Wait up to 300 ms for the returned byte
  uint32_t started = millis();

  while (millis() - started < 300) {
    if (Serial1.available()) {
      int received = Serial1.read();

      Serial.print(" | Received 0x");
      printHexByte((uint8_t)received);

      if (received == sent) {
        Serial.println(" | PASS");
      } else {
        Serial.println(" | FAIL");
      }
      return;
    }
  }

  Serial.println(" | TIMEOUT (no byte received)");
}

void setup() {
  // Initialize control GPIO
  // RD must remain HIGH to protect GP3
  setOutput(PIN_RD, HIGH);
  setOutput(PIN_WR, HIGH);
  setOutput(PIN_EXCLK, LOW);
  setOutput(PIN_AD7, LOW);

  setOutput(PIN_AD0, LOW);
  setOutput(PIN_AD1, LOW);
  setOutput(PIN_AD2, LOW);

  // Serial control mode
  setOutput(PIN_ALE, HIGH);
  setOutput(PIN_CS, LOW);

  // USB Serial Monitor
  Serial.begin(115200);

  // Hardware UART0 for modem communication
  Serial1.setTX(0);  // GP0 -> modem TXD
  Serial1.setRX(1);  // GP1 <- modem RXD
  Serial1.begin(1200, SERIAL_8N1);

  // Allow modem power-on reset to complete
  delay(2500);

  // CR1 = 0x03: local digital loopback
  writeModemRegister(0x01, 0x03);

  // CR0 = 0x1B:
  // 1200 baud DPSK, asynchronous 8N1
  // Originate mode, transmitter enabled
  writeModemRegister(0x00, 0x1B);

  Serial.println("73K212AL configured");
  Serial.println("Registers written (no readback)");
  Serial.println("CR1 = 0x03: digital loopback");
  Serial.println("CR0 = 0x1B: 1200 baud, 8N1");
  Serial.println("Starting UART tests...");
}

void loop() {
  // Different bit patterns for testing
  const uint8_t pattern[] = {
    0x01
  };

  for (uint8_t value : pattern) {
    testOneByte(value);
    delay(100);
  }

  Serial.println("--------------------");
  delay(2000);
}
