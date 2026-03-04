#include <SoftwareSerial.h>  // Software serial for the UART to TMC2209 - https://www.arduino.cc/en/Reference/softwareSerial
#include <Streaming.h>       // For serial debugging output - https://www.arduino.cc/reference/en/libraries/streaming/
#include <TMC2209.h>         // https://github.com/janelia-arduino/TMC2209/tree/main


/*
 * Using internal 12 MHz clock (default): microsteps_per_second = microsteps_per_period * 0.715
 * 400 * 360 * 256 = 330 turns per period * 0,715 = ms / sec
 * 
 */

#define SPEED_SWITCH 8    // HIGH = 330 RPM

#define EN_PIN 2          // Enable - PURPLE
#define DIR_PIN 3         // Direction - WHITE
#define STEP_PIN 4        // Step - ORANGE
#define SW_TX 6           // SoftwareSerial receive pin - BROWN
#define SW_RX 7           // SoftwareSerial transmit pin - YELLOW

#define LED_PIN_GREEN 11  // indicator speed reached
#define LED_PIN_RED 9     // indicator speed reached

const uint8_t DIM = 70;
const uint8_t HOLD_CURRENT_STANDSTILL = 0;
const float mcstepsperstep = 64.0;
float mcSteps = mcstepsperstep / 128.0;
float baseSteps = (400.0 * mcSteps * 2.95);  //0.715 * (330.0 / 40.0) / 2.0);  // 0.9° = 400 Steps
int accel = 1000;                            // Speed increase/decrease amount

float speed330 = baseSteps * 142.0;
float speed450 = baseSteps * 192.3;
float stepperSpeed = speed330;

int speedChangeDelay = 100;  // Delay between speed changes
float currentSpeed = 0;
float correctedSpeed;

//HardwareSerial & serial_stream = Serial;
SoftwareSerial SoftSerial(SW_RX, SW_TX);  // Be sure to connect RX to TX and TX to RX between both devices

const long SERIAL_BAUD_RATE = 115200;
const int DELAY = 1000;

// current values may need to be reduced to prevent overheating depending on
// specific motor and power supply voltage
const uint8_t RUN_CURRENT_PERCENT = 15;


// Instantiate TMC2209
TMC2209 TMCdriver;


void setup() {
  Serial.begin(SERIAL_BAUD_RATE);
  SoftSerial.begin(SERIAL_BAUD_RATE);  // initialize software serial for UART motor control

  setLedState(false);

  TMCdriver.setup(SoftSerial, SERIAL_BAUD_RATE, TMC2209::SERIAL_ADDRESS_0);
  delay(DELAY);

  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_RED, OUTPUT);

  pinMode(EN_PIN, OUTPUT);    // Set pinmodes
  pinMode(STEP_PIN, OUTPUT);  // currently unused
  pinMode(DIR_PIN, OUTPUT);

  TMCdriver.enable();

  TMCdriver.clearReset();
  

  TMCdriver.enableAutomaticCurrentScaling();
  TMCdriver.setRunCurrent(RUN_CURRENT_PERCENT);

  TMCdriver.setMicrostepsPerStep(mcstepsperstep);
  //TMCdriver.setMicrostepsPerStepPowerOfTwo(8);
  TMCdriver.enableInverseMotorDirection();
  TMCdriver.setStandstillMode(TMCdriver.FREEWHEELING);
  TMCdriver.setHoldCurrent(HOLD_CURRENT_STANDSTILL);

  //TMCdriver.enableCoolStep();
  TMCdriver.enableStealthChop();
  //TMCdriver.setStealthChopDurationThreshold(0);

  delay(DELAY);

  
}

void loop() {

  static int loops = 0;

  selectRpm();
  correctedSpeed = stepperSpeed + getSpeedCorrection();

  if (currentSpeed < correctedSpeed) {
    setLedState(false);
    for (long i = currentSpeed; i <= correctedSpeed; i = i + accel) {  // Speed up to maxSpeed
      //Serial.println(i);
      TMCdriver.moveAtVelocity(i);  // Set motor speed
      delay(speedChangeDelay);

      // handle edge case
      if ( i + accel > correctedSpeed ) {
        i = correctedSpeed;
      }
    }
    currentSpeed = correctedSpeed;
    setLedState(true);
    TMCdriver.moveAtVelocity(currentSpeed);
  }

  if (currentSpeed > correctedSpeed) {
    setLedState(false);
    for (long i = currentSpeed; i >= correctedSpeed; i = i - accel) {  // Decrease speed
      //Serial.println(i);
      TMCdriver.moveAtVelocity(i);
      delay(speedChangeDelay);

      // handle edge case
      if ( i - accel < correctedSpeed ) {
        i = correctedSpeed;
      }
    }
    currentSpeed = correctedSpeed;
    setLedState(true);
    TMCdriver.moveAtVelocity(currentSpeed);
  }

  // if ( ++loops > 19 ) {
  //   loops = 0;
  //   Serial.println(currentSpeed);
  //   Serial.print("\t");
  //   Serial.println(currentSpeed);
  // }

  
  delay(50);  // not to high as otherwise hysteresis is to large on the pot changes
}

/**
 * toggles the red and green LEDs
 *
 */
void setLedState(bool speedReached) {
  if (speedReached) {
    analogWrite(LED_PIN_GREEN, DIM);
    analogWrite(LED_PIN_RED, 0);
  } else {
    analogWrite(LED_PIN_GREEN, 0);
    analogWrite(LED_PIN_RED, DIM);
  }
}

/**
 * checks the speed switch: high = 33 RPM, low = 45 RPM
 */
void selectRpm() {

  boolean should330 = digitalRead(SPEED_SWITCH) == 1;

  if (should330) {
    if (stepperSpeed == speed450) {
      stepperSpeed = speed330;
      Serial << "target speed (33.3) " << stepperSpeed << endl;
    }
  } else {
    if (stepperSpeed == speed330) {
      stepperSpeed = speed450;
      Serial << "target speed (45.0) " << stepperSpeed << endl;
    }
  }
}

/**
 * calculates the speed correction based on potentiometer value
 */
float getSpeedCorrection() {

  /**
   * interpolate 10 readings
   */
  static int loops = 0;
  const int mxlps = 9;  // count from zero
  static int accumulatedValues = 0;
  static int oldPotVal = 0;
  const float controlRange = 5.0;  //  +/- n% of given target speed
  int potVal;

  /**
   * value to be added to the target speed as a fine-tuning
   */
  static float speedCorrection = 0;

  potVal = analogRead(A5);  // Read potentiometer (0-1023)
  potVal = analogRead(A5);
  accumulatedValues += potVal;

  if (++loops > mxlps) {

    int avgVal = accumulatedValues / loops;
    loops = 0;
    accumulatedValues = 0;

    int diff = avgVal - oldPotVal;
    oldPotVal = avgVal;

    if (abs(diff) > 3) {  // poti has some bias
      Serial << "diff: " << diff << endl;
      if (avgVal <= 500)  // In lower half of range decrease speed
      {
        speedCorrection = map(avgVal, 0, 500, -stepperSpeed * (contr olRange / 100.0), 0);
      } else if (avgVal >= 520)  // In upper half of range increase speed
      {
        speedCorrection = map(avgVal, 520, 1023, 0, stepperSpeed * (controlRange / 100.0));
      } else  // Create a "dead zone" between increase and decrease of speed
      {       // if 500 < avgVal <520
        speedCorrection = 0.0;
      }
    }
  }

  return speedCorrection;
}