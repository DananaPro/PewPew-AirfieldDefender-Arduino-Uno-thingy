#include <Arduino.h>

#define NUM_CHIPS 8
#define NUM_LASERS 64

// Pin connections (Arduino Uno)
const int DATA_PIN  = 2;  // SERIN
const int CLOCK_PIN = 3;  // SRCK
const int LATCH_PIN = 4;  // RCK

// Laser states for each chip (1 byte per chip)
uint8_t laserState[NUM_CHIPS];

void setup() {
  Serial.begin(115200);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  // Initialize all lasers off
  for (int i = 0; i < NUM_CHIPS; i++) laserState[i] = 0;
  updateShiftRegisters();
}

void loop() {
  // Read commands from PC (C#)
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("FIRE")) {
      if (cmd.length() < 7) return;

      int row = cmd.charAt(5) - '0';
      int col = cmd.charAt(6) - '0';

      int idx  = row * 8 + col;
      int chip = idx / 8;
      int bit  = idx % 8;

      // Turn off all lasers first
      for (int i = 0; i < NUM_CHIPS; i++) laserState[i] = 0;

      // Turn on target laser
      laserState[chip] |= (1 << bit);

      // Send updated state to all chips
      updateShiftRegisters();
    }
  }
}

// Push laserState array to all TPIC6B595N chips
void updateShiftRegisters() {
  digitalWrite(LATCH_PIN, LOW); // prepare latch

  // Shift out data to last chip first
  for (int i = NUM_CHIPS - 1; i >= 0; i--) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, laserState[i]);
  }

  digitalWrite(LATCH_PIN, HIGH); // latch outputs
}
