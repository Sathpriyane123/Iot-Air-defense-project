/*
  Air Defense / Sentry Radar System
  ---------------------------------
  - Servo 1 (Scan Servo) sweeps 180 degrees carrying an ultrasonic sensor.
  - Servo 2 (Trigger Servo) fires a short action when an object is detected
    within range.
  - Green LED = area clear, Red LED + Buzzer = target detected.
  - Maximum sensing range: 1 meter (100 cm).
  - Filters out sensor noise: requires 6 consistent readings before
    confirming a real object (reduces false triggers).

  Hardware:
    Scan Servo    -> Pin 9
    Trigger Servo -> Pin 10
    Ultrasonic TRIG -> Pin 7
    Ultrasonic ECHO -> Pin 6
    Green LED     -> Pin 3
    Red LED       -> Pin 4
    Buzzer        -> Pin 5

  NOTE: Power both servos from an external 5V supply, with GND common
  to the Arduino. Do not run two servos off the Uno's onboard 5V pin.
*/

#include <Servo.h>

Servo scanServo;
Servo triggerServo;

const int trigPin = 7;
const int echoPin = 6;
const int greenLedPin = 3;
const int redLedPin = 4;
const int buzzerPin = 5;

const int scanServoPin = 9;
const int triggerServoPin = 10;

const int detectionRangeCm = 20;   // distance threshold for "target detected"
const int minValidCm = 8;          // ignore readings closer than this (sensor noise, not a real object)
const int maxRangeCm = 100;        // maximum sensor range: 1 meter
const int confirmSamples = 6;      // consistent readings required before confirming a real detection
const int sweepMin = 15;           // avoid extreme mechanical ends 0/180
const int sweepMax = 165;
const int sweepStepDelay = 25;     // ms between each 1-degree step (sweep speed)

int currentAngle = sweepMin;
int sweepDirection = 1;            // 1 = increasing, -1 = decreasing

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  scanServo.attach(scanServoPin);
  triggerServo.attach(triggerServoPin);

  triggerServo.write(0);       // resting position
  scanServo.write(currentAngle);

  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  noTone(buzzerPin);

  delay(500);
}

long getDistanceCm() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Timeout tuned for ~1m max range (round trip time for 100cm + margin)
  long duration = pulseIn(echoPin, HIGH, 7000); // 7ms timeout (~1.2m max)
  if (duration == 0) {
    return 999; // no echo received within range, treat as "nothing detected"
  }
  long distanceCm = duration * 0.0343 / 2;

  if (distanceCm > maxRangeCm || distanceCm < minValidCm) {
    return 999; // out of valid range, or too close to be a real object (noise)
  }
  return distanceCm;
}

// Takes several readings in a row and only confirms a detection if they
// are all consistently within detectionRangeCm. This filters out random
// noise spikes / stray echoes so only a real, solid object triggers the alert.
bool isRealObjectDetected(long &confirmedDistance) {
  int consistentCount = 0;
  long total = 0;

  for (int i = 0; i < confirmSamples; i++) {
    long d = getDistanceCm();
    if (d < detectionRangeCm) {
      consistentCount++;
      total += d;
    }
    delay(15); // brief pause between samples
  }

  if (consistentCount == confirmSamples) {
    confirmedDistance = total / confirmSamples;
    return true;
  }
  return false;
}

void soundAlert(long distanceCm) {
  digitalWrite(greenLedPin, LOW);
  digitalWrite(redLedPin, HIGH);
  tone(buzzerPin, 1000);

  Serial.print("OBJECT DETECTED! Distance: ");
  Serial.print(distanceCm);
  Serial.println(" cm");
}

void clearAlert() {
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
  noTone(buzzerPin);
}

void fireTriggerServo() {
  Serial.println("Target locked -> firing trigger servo");
  triggerServo.write(90);
  delay(500);
  triggerServo.write(0);
  delay(300);
}

void loop() {
  scanServo.write(currentAngle);
  delay(sweepStepDelay);

  long distance = getDistanceCm();
  Serial.print("Angle: ");
  Serial.print(currentAngle);
  Serial.print(" | Distance: ");
  if (distance >= 999) {
    Serial.println("out of range");
  } else {
    Serial.print(distance);
    Serial.println(" cm");
  }

  if (distance < detectionRangeCm) {
    // Possible object - confirm with multiple samples to filter out noise
    long confirmedDistance;
    if (isRealObjectDetected(confirmedDistance)) {
      soundAlert(confirmedDistance);
      fireTriggerServo();
    }
    clearAlert();
  } else {
    clearAlert();
  }

  // Update sweep angle for next loop iteration
  currentAngle += sweepDirection;
  if (currentAngle >= sweepMax) {
    currentAngle = sweepMax;
    sweepDirection = -1;
  } else if (currentAngle <= sweepMin) {
    currentAngle = sweepMin;
    sweepDirection = 1;
  }
}