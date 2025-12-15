// ============================================================================
// LC Meter V7 - Manual Calibration Edition
// Designedby kpa radio of nobcha
// For Hackster.io Project, PCB was promoted with PCBWAY
// ============================================================================
// 2025.12.12 debug  lc_meter_v7_10.ino 93%/73%
//  Integrated with lc_meter_v_64.ino logic
//  D6 is SW2 sensor (not relay control)
//  
// Thanks for Teensy-based project to refer FreqCount Library.
// https://www.pjrc.com/teensy/td_libs_FreqCount.html
//
// Design Philosophy:
// 1. Manual DPDT calibration for educational transparency
// 2. 3-steps-layer PCB structure for functional beauty
// 3. Open-source for community learning
// ============================================================================
// ============================================================================
// LC Meter V7 - Manual Calibration Edition V10 (FIXED)
// ============================================================================

#include <Wire.h>
#include <U8g2lib.h>
#include <FreqCount.h>

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define PIN_FREQ_IN     5
#define PIN_CAL_SW      6
#define PIN_LC_SW       7

const float REF_CAPACITANCE = 1000.0e-12;

// ============================================================================
// OLED DISPLAY
// ============================================================================
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0);

// ============================================================================
// SYSTEM STATES
// ============================================================================
enum SystemState {
  STATE_INIT,
  STATE_DIAGNOSTIC,
  STATE_WAIT_CALIB,
  STATE_CALIBRATING,
  STATE_MEASURING_L,
  STATE_MEASURING_C,
  STATE_ERROR
};

SystemState currentState = STATE_INIT;

// ============================================================================
// CALIBRATION DATA
// ============================================================================
struct CalibrationData {
  float baseInductance;
  float baseCapacitance;
  unsigned long freqOpen;
  unsigned long freqRef;
  bool isValid;
};

CalibrationData calData = {0, 0, 0, 0, false};

// ============================================================================
// FUNCTION PROTOTYPES
// ============================================================================
void initializeSystem();
void runStateMachine();
void updateDisplayHeader();

void handleDiagnostic();
void handleWaitCalibration();
void handleCalibration();
void handleMeasurement();
void handleError();

unsigned long measureStableFrequency();
bool performCalibrationSequence();
bool calculateCalibrationValuesDouble(unsigned long F1, unsigned long F2);  // 
float calculateInductance(unsigned long frequency);
float calculateCapacitance(unsigned long frequency);

void displaySplashScreen();
void displayMessage(const char* line1, const char* line2 = "");
void displayCalibrationScreen();
void displayMeasurementScreen(float value, const char* unit, bool isInductor);
void displayErrorScreen(const char* errorMsg);

void debugCalibrationResults();
void quickFCNTCheck();
void manualFrequencyMeasurement();

void formatValueUnit(float &value, char* unit, bool isCapacitance);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println("LC Meter V7.10 by kpa radio");
  Serial.println("Commands: c=cal, f=FCNT, s=state");
  
  initializeSystem();
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
  if(Serial.available()) {
    char cmd = Serial.read();
    if(cmd == 'c') debugCalibrationResults();
    else if(cmd == 'f') quickFCNTCheck();
    else if(cmd == 's') {
      Serial.print("State:"); Serial.print(currentState);
      Serial.print(" SW1:"); Serial.print(digitalRead(PIN_LC_SW) == LOW ? "L" : "C");
      Serial.print(" SW2:"); Serial.println(digitalRead(PIN_CAL_SW) == LOW ? "CAL" : "MEAS");
    }
  }
  
  runStateMachine();
  delay(50);
}

// ============================================================================
// CORE FUNCTIONS
// ============================================================================
void initializeSystem() {
  pinMode(PIN_FREQ_IN, INPUT);
  pinMode(PIN_CAL_SW, INPUT_PULLUP);
  pinMode(PIN_LC_SW, INPUT_PULLUP);
  
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  
  displaySplashScreen();
  delay(2000);
  
  if (digitalRead(PIN_CAL_SW) == LOW) {
    currentState = STATE_DIAGNOSTIC;
    Serial.println(F("DIAG mode"));
  } else {
    currentState = STATE_WAIT_CALIB;
    Serial.println(F("Wait CAL"));
  }
  
  // Prior to SW2 get F1
  // get F1 prior to set SW2
  // unsigned long F1 = measureStableFrequency();
  calData.freqOpen = measureStableFrequency();
  
}

void runStateMachine() {
  static SystemState lastState = STATE_INIT;
  if(currentState != lastState) {
    updateDisplayHeader();
    lastState = currentState;
  }
  
  switch (currentState) {
    case STATE_DIAGNOSTIC: handleDiagnostic(); break;
    case STATE_WAIT_CALIB: handleWaitCalibration(); break;
    case STATE_CALIBRATING: handleCalibration(); break;
    case STATE_MEASURING_L:
    case STATE_MEASURING_C: handleMeasurement(); break;
    case STATE_ERROR: handleError(); break;
    default: currentState = STATE_WAIT_CALIB;
  }
}

void updateDisplayHeader() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawUTF8(0, 0, "LCM V7");
    
    const char* stateText = "";
    switch(currentState) {
      case STATE_DIAGNOSTIC: stateText = "DIAG"; break;
      case STATE_WAIT_CALIB: stateText = "WAIT"; break;
      case STATE_CALIBRATING: stateText = "CAL"; break;
      case STATE_MEASURING_L: stateText = "MEAS L"; break;
      case STATE_MEASURING_C: stateText = "MEAS C"; break;
      case STATE_ERROR: stateText = "ERR"; break;
    }
    u8g2.setCursor(90, 0);
    u8g2.print(stateText);
    
    u8g2.setCursor(0, 10);
    u8g2.print("S1:");
    u8g2.print(digitalRead(PIN_LC_SW) == LOW ? "L" : "C");
    u8g2.print(" S2:");
    u8g2.print(digitalRead(PIN_CAL_SW) == LOW ? "CAL" : "MEA");
    
    u8g2.setCursor(0, 20);
    u8g2.print("CAL:");
    u8g2.print(calData.isValid ? "OK" : "--");

    if (currentState == STATE_WAIT_CALIB) {
      u8g2.setFont(u8g2_font_9x15_tf);
      u8g2.setCursor(0, 35);
      u8g2.print(!calData.isValid ? "Set S2=CAL" : "Ready");
    }
    
  } while (u8g2.nextPage());
}

// ============================================================================
// STATE HANDLERS
// ============================================================================
void handleDiagnostic() {
  displayMessage("DIAG MODE", "Testing");
  delay(2000);
  
  unsigned long freq = measureStableFrequency();
  char buffer[20];  
  if (freq > 0) {
    sprintf(buffer, "%lu Hz", freq);
    displayMessage("FCNT OK", buffer);
  } else {
    displayMessage("FCNT ", "FAILED");
  }
  delay(3000);
  
  displayMessage("DIAG End,Reset", buffer);
  while(1) delay(1000);
}

void handleWaitCalibration() {


  if (digitalRead(PIN_CAL_SW) == LOW) {
    currentState = STATE_CALIBRATING;
    Serial.println("CAL starting");
    return;
  }
  
  if (calData.isValid) {
    currentState = (digitalRead(PIN_LC_SW) == LOW) ? STATE_MEASURING_L : STATE_MEASURING_C;
  }
}

void handleCalibration() {
  
  // step1 SW1 check
  if (digitalRead(PIN_LC_SW) != HIGH) {
    displayMessage("Set SW1=C", "");
    delay(1000);
    return;
  }

  //
  // step2 Remove DUT
  displayMessage("Remove DUT", "");
  delay(1000);
  displayMessage("Calibrating", "Remove DUT");
  Serial.println("Calibrating");  
  // step3 achieve CAL
  if (performCalibrationSequence()) {
    calData.isValid = true;
    
    displayCalibrationScreen();
    delay(3000);
    displayMessage("CAL SUCCESS", "Set SW2=MEAS");
    Serial.println("CAL SUCCESS");
        
    currentState = STATE_WAIT_CALIB;
  } else {
    displayMessage("CAL Failed", "Check & retry");
    Serial.println("CAL FAILED");
    currentState = STATE_ERROR;
  }
}

void handleMeasurement() {
  if (digitalRead(PIN_CAL_SW) == LOW) {
    currentState = STATE_WAIT_CALIB;
    return;
  }
  
  if (!calData.isValid) {
    currentState = STATE_WAIT_CALIB;
    return;
  }
  
  unsigned long freq = measureStableFrequency();
  if (freq < 100) {
    displayErrorScreen("No Signal");
    return;
  }
  
  float value = 0;
  char unit[4] = "";
  bool isInductor = (currentState == STATE_MEASURING_L);
  
  if (isInductor) {
    value = calculateInductance(freq);
  } else {
    value = calculateCapacitance(freq);
  }
  
  formatValueUnit(value, unit, !isInductor);
  displayMeasurementScreen(value, unit, isInductor);
  
  bool currentIsLMode = (digitalRead(PIN_LC_SW) == LOW);
  if (currentIsLMode && currentState != STATE_MEASURING_L) {
    currentState = STATE_MEASURING_L;
  } else if (!currentIsLMode && currentState != STATE_MEASURING_C) {
    currentState = STATE_MEASURING_C;
  }
}

void handleError() {
  displayErrorScreen("Error");
  delay(2000);
  currentState = STATE_WAIT_CALIB;
}

// ============================================================================
// HARDWARE OPERATIONS
// ============================================================================
unsigned long measureStableFrequency() {
  const int SAMPLES = 2;
  
  unsigned long readings[SAMPLES];
  
  for (int i = 0; i < SAMPLES; i++) {
    FreqCount.begin(1000);
    delay(1100);
    
    if (!FreqCount.available()) {
      Serial.print("[FCNT] Fail");
      return 0;
    }
    
    readings[i] = FreqCount.read();
    delay(100);
  }
  
  // 0.5%チェック
  long diff = abs((long)readings[0] - (long)readings[1]);
  unsigned long tolerance = readings[0] / 200;  // 0.5% = 1/200
  
  if (diff > max(tolerance, 1000UL)) {
    Serial.print("[FCNT] Unstable diff=");
    Serial.print(diff);
    Serial.print(" > ");
    Serial.print(max(tolerance, 1000UL));
    Serial.print(" (");
    Serial.print((float)diff * 100.0 / readings[0], 2);
    Serial.print("%)");
    return 0;
  }
  
  unsigned long avg = (readings[0] + readings[1]) / 2;
  Serial.print("[FCNT] ");
  Serial.print(readings[0]);
  Serial.print(",");
  Serial.print(readings[1]);
  Serial.print(" → ");
  Serial.print(avg);
  Serial.print(" Hz (");
  Serial.print((float)diff * 100.0 / readings[0], 2);
  Serial.println("%)");
  
  return avg;
}

// ============================================================================
// CALIBRATION CALCULATION (DOUBLE PRECISION)
// ============================================================================
bool calculateCalibrationValuesDouble(unsigned long F1, unsigned long F2) {
  // Validate input
  if (F1 <= F2) {
    Serial.println("ERR: F1 be smaller than F2");
    return false;
  }
  
  // Convert to double for precise calculation
  double F1_d = (double)F1;
  double F2_d = (double)F2;
  
  // Calculate (F1/F2)^2
  double ratio = F1_d / F2_d;
  double ratio_sq = ratio * ratio;
  
  // Calculate denominator = (F1/F2)^2 - 1
  double denominator = ratio_sq - 1.0;
  if (denominator <= 0.0) {
    Serial.print("ERR: Invalid: ");
    Serial.println(denominator, 6);
    return false;
  }
  
  // Calculate C0 = Cref / denominator
  calData.baseCapacitance = REF_CAPACITANCE / denominator;
  
  // Calculate L0 = 1 / ((2πF1)^2 * C0)
  double omega1 = 2.0 * M_PI * F1_d;
  double omega1_sq = omega1 * omega1;
  calData.baseInductance = 1.0 / (omega1_sq * calData.baseCapacitance);
  
  // Debug output
  Serial.print("[CAL] F1:"); Serial.print(F1);
  Serial.print(" F2:"); Serial.print(F2);
  Serial.print(" L0:"); Serial.print(calData.baseInductance * 1e6, 1);
  Serial.print(" C0:"); Serial.println(calData.baseCapacitance * 1e12, 1);
  
  return true;
}

bool performCalibrationSequence() {
  Serial.println("\n[CAL] Step1: F2");
  delay(500);
  unsigned long F2 = measureStableFrequency();
  if (F2 == 0) return false;
  displayMessage("Got F2", "Set SW2=MEAS");
  while (digitalRead(PIN_CAL_SW) == LOW) {
    delay(500);
  }
  delay(500);
  
  Serial.println("[CAL] Step2: F1");
  delay(1000);  
//  unsigned long F1 = measureStableFrequency();
  unsigned long F1 = calData.freqOpen;
  if (F1 == 0) return false;
  
  // Double precision
  if (!calculateCalibrationValuesDouble(F1, F2)) {
    Serial.println("[CAL] CAL failed");
    return false;
  }
  
//  calData.freqOpen = F1;
  calData.freqRef = F2;
  calData.isValid = true;
  
  Serial.println("[CAL] SUCCESS");
  return true;
}

float calculateInductance(unsigned long frequency) {
  float omega = 2 * PI * frequency;
  float Ltotal = 1.0 / (omega * omega * calData.baseCapacitance);
  return Ltotal - calData.baseInductance;
}

float calculateCapacitance(unsigned long frequency) {
  float omega = 2 * PI * frequency;
  float Ctotal = 1.0 / (omega * omega * calData.baseInductance);
  return Ctotal - calData.baseCapacitance;
}

// ============================================================================
// DEBUG FUNCTIONS
// ============================================================================
void debugCalibrationResults() {
  Serial.print("\n[CAL] F1:"); Serial.print(calData.freqOpen);
  Serial.print(" F2:"); Serial.print(calData.freqRef);
  Serial.print(" L0:"); Serial.print(calData.baseInductance * 1e6, 2);
  Serial.print(" C0:"); Serial.print(calData.baseCapacitance * 1e12, 2);
  Serial.print(" Valid:"); Serial.println(calData.isValid ? "Y" : "N");
}

void quickFCNTCheck() {
  Serial.println("\n[FCNT Test]");
  for(int i = 0; i < 3; i++) {
    FreqCount.begin(100);
    delay(120);
    if(FreqCount.available()) {
      Serial.print("  "); Serial.print(FreqCount.read() * 10); Serial.println("Hz");
    } else {
      Serial.println("  No signal");
    }
    delay(300);
  }
}

// ============================================================================
// DISPLAY FUNCTIONS
// ============================================================================
void displaySplashScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_10x20_tf);
    u8g2.drawUTF8(10, 20, "LC Meter V7");
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawUTF8(10, 45, "Manual CAL");
    
  } while (u8g2.nextPage());
}

void displayMessage(const char* line1, const char* line2) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.setCursor(0, 30);
    u8g2.print(line1);

    if (strlen(line2) > 0) {
      u8g2.setCursor(0, 50);
      u8g2.print(line2);
    }
  } while (u8g2.nextPage());
  
  Serial.print("MSG: "); Serial.println(line1);
  delay(300);
}

void displayCalibrationScreen() {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.setCursor(0, 0);
    u8g2.print("CAL Results:");
    
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 20);
    u8g2.print("F1: "); u8g2.print(calData.freqOpen);
    
    u8g2.setCursor(0, 32);
    u8g2.print("F2: "); u8g2.print(calData.freqRef);
    
    u8g2.setCursor(0, 44);
    u8g2.print("L0: "); u8g2.print(calData.baseInductance * 1e6, 1);
    
    u8g2.setCursor(0, 56);
    u8g2.print("C0: "); u8g2.print(calData.baseCapacitance * 1e12, 1);
  } while (u8g2.nextPage());
}

void displayMeasurementScreen(float value, const char* unit, bool isInductor) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_fub20_tr);
    u8g2.setCursor(0, 26);
    
    if (value >= 1000) u8g2.print(value / 1000, 1);
    else if (value >= 100) u8g2.print(value, 0);
    else if (value >= 10) u8g2.print(value, 1);
    else u8g2.print(value, 2);
    
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.setCursor(80, 26);
    u8g2.print(unit);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.setCursor(0, 56);
    u8g2.print(isInductor ? "INDUCTANCE" : "CAPACITANCE");

    drawIcon(isInductor);
  } while (u8g2.nextPage());
}

void displayErrorScreen(const char* errorMsg) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.setCursor(0, 30);
    u8g2.print("ERR:");
    u8g2.setCursor(0, 50);
    u8g2.print(errorMsg);
  } while (u8g2.nextPage());
  
  Serial.print("ERR: "); Serial.println(errorMsg);
}

void drawIcon(bool isLmode) {
  if (isLmode) {
    u8g2.drawLine(5,10,10,5);
    u8g2.drawLine(10,5,15,10);
    u8g2.drawLine(15,10,20,5);
    u8g2.drawLine(20,5,25,10);
    u8g2.drawLine(25,10,30,5);
    u8g2.drawLine(30,5,35,10);
  } else {
    u8g2.drawVLine(15, 5, 15);
    u8g2.drawVLine(25, 5, 15);
    u8g2.drawLine(0, 12, 15, 12);
    u8g2.drawLine(25, 12, 50, 12);
  }
}
// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================
void formatValueUnit(float &value, char* unit, bool isCapacitance) {
  if (isCapacitance) {
    if (value >= 1e-6) { value *= 1e6; strcpy(unit, "uF"); }
    else if (value >= 1e-9) { value *= 1e9; strcpy(unit, "nF"); }
    else { value *= 1e12; strcpy(unit, "pF"); }
  } else {
    if (value >= 1e-3) { value *= 1e3; strcpy(unit, "mH"); }
    else { value *= 1e6; strcpy(unit, "uH"); }
  }
}
