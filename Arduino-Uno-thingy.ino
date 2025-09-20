#include <Arduino.h>

#define NUM_CHIPS 8
#define NUM_LASERS 64
#define MAX_TARGETS 16   // max concurrent active targets
#define FIRE_TIMEOUT 3000 // ms

// Pin connections (Arduino Uno)
const int DATA_PIN  = 2;  // SERIN
const int CLOCK_PIN = 3;  // SRCK
const int LATCH_PIN = 4;  // RCK

// Laser states for each chip (1 byte per chip)
uint8_t laserState[NUM_CHIPS];

// Active targets
struct Target {
  int row;
  int col;
  unsigned long startTime;
};

Target activeTargets[MAX_TARGETS];
int targetCount = 0;

void setup() {
  Serial.begin(115200);

  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  clearLasers();
  updateShiftRegisters();
}

void loop() {
  // Handle serial commands
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.startsWith("FIRE") && cmd.length() >= 7) {
      int row = cmd.charAt(5) - '0';
      int col = cmd.charAt(6) - '0';
      addTarget(row, col);
    }
  }

  // Recompute lasers every loop
  updateLasers();
}

// Add a new target to the list
void addTarget(int row, int col) {
  if (targetCount < MAX_TARGETS) {
    activeTargets[targetCount].row = row;
    activeTargets[targetCount].col = col;
    activeTargets[targetCount].startTime = millis();
    targetCount++;
  }
}

// Recompute all lasers based on active targets
void updateLasers() {
  clearLasers();

  unsigned long now = millis();

  // Compact the target list as we go
  int newCount = 0;
  for (int i = 0; i < targetCount; i++) {
    if (now - activeTargets[i].startTime < FIRE_TIMEOUT) {
      // still active → fire cross
      fireCross(activeTargets[i].row, activeTargets[i].col);

      // keep it in the list
      activeTargets[newCount++] = activeTargets[i];
    }
  }
  targetCount = newCount;

  updateShiftRegisters();
}

// Fire main laser + cross pattern
void fireCross(int row, int col) {
  setLaser(row, col);
  if (row > 0) setLaser(row - 1, col);
  if (row < 7) setLaser(row + 1, col);
  if (col > 0) setLaser(row, col - 1);
  if (col < 7) setLaser(row, col + 1);
}

// Mark a laser ON in state array
void setLaser(int row, int col) {
  int idx  = row * 8 + col;
  int chip = idx / 8;
  int bit  = idx % 8;
  laserState[chip] |= (1 << bit);
}

// Clear all lasers
void clearLasers() {
  for (int i = 0; i < NUM_CHIPS; i++) laserState[i] = 0;
}

// Push laserState array to all TPIC6B595N chips
void updateShiftRegisters() {
  digitalWrite(LATCH_PIN, LOW); // prepare latch

  for (int i = NUM_CHIPS - 1; i >= 0; i--) {
    shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, laserState[i]);
  }

  digitalWrite(LATCH_PIN, HIGH); // latch outputs
}
