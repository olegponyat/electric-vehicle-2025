
//NOTE: this should all be in an ARDUINO file, NOT run as its own cpp file.
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RobojaxBTS7960.h>
#include <math.h>

// ---------------- LCD ----------------
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- MOTOR ----------------
#define RPWM 10
#define R_EN 11
#define R_IS 13
#define LPWM 9
#define L_EN 8
#define L_IS 12
#define CW 1
#define CCW 0
#define debug 1
RobojaxBTS7960 motor(R_EN, RPWM, R_IS, L_EN, LPWM, L_IS, debug);

// ---------------- ENCODER ----------------
#define ENCA 2
#define ENCB 3
volatile long counter = 0;

// ---------------- DIAL (ROTARY ENCODER) ----------------
#define DialCLK 5
#define DialDT 4
#define DialButtonPin A0
int lastCLK = HIGH;
int incIndex = 0;
#define StartButtonPin 6

// ---------------- TARGETS & WHEEL ----------------
double TargetDistance = 10.0;   // meters
double TargetTime = 20.0;       // seconds
double wheelDiameter = 7.3025;  // cm
double pulsesPerRev = 1200.0;

// ---------------- MOTION ----------------
bool moving = false;
unsigned long startTime;
double runTime;
double finalDist;

// ---------------- SPEED ----------------
int minPWM = 20;
int maxPWM = 70;
int scaledMaxPWM = 70;

double accelMeters = 0.5;
double accelEncoder = 0.0;

double countsPerMeter = 0.0;
double referenceSpeed = 1.226; // measured value

// ---------------- MODE ----------------
enum Mode { DIST_ADJ, TIME_ADJ, RUN };
Mode mode = DIST_ADJ;

// ---------------- INCREMENTS ----------------
double distInc[] = {1.0, 0.5, 0.1, 0.01};
double timeInc[] = {5.0, 1.0, 0.1};

void setup() {
  Serial.begin(115200);
  motor.begin();

  lcd.init();
  lcd.backlight();

  pinMode(StartButtonPin, INPUT_PULLUP);
  pinMode(DialCLK, INPUT_PULLUP);
  pinMode(DialDT, INPUT_PULLUP);
  pinMode(DialButtonPin, INPUT_PULLUP);
  pinMode(ENCA, INPUT_PULLUP);
  pinMode(ENCB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCA), ai0, RISING);
  attachInterrupt(digitalPinToInterrupt(ENCB), ai1, RISING);

  countsPerMeter = getEncoderValue(1.0);
  accelEncoder = getEncoderValue(accelMeters);

  drawDistanceScreen();
  Serial.println("Setup complete.");
  Serial.print("countsPerMeter: "); Serial.println(countsPerMeter);
}

void loop() {
  handleDial();
  handleDialButton();
  handleStart();

  // no way in FUCKING hell is this the most efficient method
  if (mode == RUN && moving) {
    double elapsedTime = (millis() - startTime) / 1000.0;
    double remainingTime = TargetTime - elapsedTime;
    if (remainingTime < 0.05) remainingTime = 0.05;

    double traveledMeters = 0.0;
    if (countsPerMeter > 0.0) traveledMeters = ((double)counter) / countsPerMeter;
    double remainingMeters = TargetDistance - traveledMeters;
    if (remainingMeters < 0.0) remainingMeters = 0.0;

    double requiredSpeed = remainingMeters / remainingTime;

    double speedScale = 0.0;
    if (referenceSpeed > 0.0001) speedScale = requiredSpeed / referenceSpeed;
    else speedScale = 1.0;

    speedScale = constrain(speedScale, 0.3, 1.3);

    int targetPWM = (int)round(maxPWM * speedScale);

    double rampProgress = 0.0;
    if (accelEncoder > 0.0) rampProgress = ((double)counter) / accelEncoder;
    if (rampProgress > 1.0) rampProgress = 1.0;

    double pwmDouble = (double)minPWM + ( (double)targetPWM - (double)minPWM ) * rampProgress;

    int pwm = (int)round(pwmDouble);
    pwm = constrain(pwm, 0, 100);

    if (pwm > maxPWM) pwm = maxPWM;

    motor.rotate(pwm, CW);

    if (remainingMeters < 0.05) {
      pwm = (int)round((double)pwm * 0.5);
      pwm = constrain(pwm, minPWM, 100);
      motor.rotate(pwm, CW);
    }

    // stop condition
    if (traveledMeters >= TargetDistance || counter >= getEncoderValue(TargetDistance)) {
      motor.stop();
      runTime = elapsedTime;
      finalDist = TargetDistance * ( (double)counter / getEncoderValue(TargetDistance) );

      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Fin Dist:");
      lcd.setCursor(10,0);
      lcd.print(finalDist,3);
      lcd.setCursor(0,1);
      lcd.print("Fin Time:");
      lcd.setCursor(10,1);
      lcd.print(runTime,3);

      Serial.print("Run complete. finalDist: "); Serial.print(finalDist, 6);
      Serial.print("  runTime: "); Serial.println(runTime, 6);

      moving = false;
      mode = DIST_ADJ;
      counter = 0;
    }
  }
}

// ---------------- INPUT HANDLING ----------------
void handleStart() {
  static bool last = HIGH;
  bool now = digitalRead(StartButtonPin);

  if (now == LOW && last == HIGH) {
    if (mode == DIST_ADJ) {
      mode = TIME_ADJ;
      incIndex = 0;
      drawTimeScreen();
    }
    else if (mode == TIME_ADJ) {
      counter = 0;
      startTime = millis();
      moving = true;
      mode = RUN;
      countsPerMeter = getEncoderValue(1.0);
      accelEncoder = getEncoderValue(accelMeters);
    }
  }
  last = now;
}

void handleDial() {
  int clk = digitalRead(DialCLK);
  if (clk != lastCLK) {
    bool dir = digitalRead(DialDT) != clk;

    if (mode == DIST_ADJ) {
      TargetDistance += dir ? distInc[incIndex] : -distInc[incIndex];
      if (TargetDistance < 0.0) TargetDistance = 0.0;
      drawDistanceScreen();
    }
    else if (mode == TIME_ADJ) {
      TargetTime += dir ? timeInc[incIndex] : -timeInc[incIndex];
      if (TargetTime < 0.1) TargetTime = 0.1;
      drawTimeScreen();
    }
    delay(120);
  }
  lastCLK = clk;
}

void handleDialButton() {
  if (digitalRead(DialButtonPin) == LOW) {
    incIndex++;
    if (mode == DIST_ADJ && incIndex >= 4) incIndex = 0;
    if (mode == TIME_ADJ && incIndex >= 3) incIndex = 0;
    delay(300);
  }
}

// ---------------- DISPLAY ----------------
void drawDistanceScreen() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tgt Dist:");
  lcd.setCursor(10,0);
  lcd.print(TargetDistance,3);

  lcd.setCursor(0,1);
  lcd.print("Inc:");
  lcd.setCursor(6,1);
  lcd.print(distInc[incIndex]);

  Serial.print("TgtDist: "); Serial.println(TargetDistance, 6);
}

void drawTimeScreen() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Tgt Time:");
  lcd.setCursor(10,0);
  lcd.print(TargetTime,2);

  lcd.setCursor(0,1);
  lcd.print("Inc:");
  lcd.setCursor(6,1);
  lcd.print(timeInc[incIndex]);

  Serial.print("TgtTime: "); Serial.println(TargetTime, 6);
}

// ---------------- ENCODER ISRs ----------------
void ai0() {
  if (digitalRead(ENCB) == LOW) counter++;
  else counter--;
}
void ai1() {
  if (digitalRead(ENCA) == LOW) counter--;
  else counter++;
}

// ---------------- ENCODER MATH ----------------
double getEncoderValue(double meters) {
  double circ = 3.14159 * wheelDiameter; // cm
  // meters * 100 = cm
  return (meters * 100.0 / circ) * pulsesPerRev;
}