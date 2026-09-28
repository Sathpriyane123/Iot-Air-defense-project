#include <Servo.h>
Servo scanServo;
Servo triggerServo;
// ---------------- PINS ----------------
const int trigPin = 7;          // Ultrasonic sensor TRIG pin (sends the pulse)
const int echoPin = 6;          // Ultrasonic sensor ECHO pin (receives the echo)
const int greenLedPin = 3;      // Green LED = scanning / searching
const int redLedPin = 4;        // Red LED = target locked
const int buzzerPin = 5;      // Buzzer
const int scanServoPin = 9;   // Signal wire of the scanning serv
const int triggerServoPin = 10;  // Signal wire of the trigger servo
// ---------------- SETTINGS ----------------
// Object must be between these distances 5cm to 100cm
const int minDistance = 5;
const int maxDistance = 100;
// Number of continuous detections required
const int requiredReadings = 20;
// Scan range 15dec to 165dec safe for 0 to 180
const int scanMin = 15;
const int scanMax = 165;
// Servo speed
const int servoDelay = 25;
// ---------------- VARIABLES ----------------
int currentAngle = scanMin;   // Current servo angle, starts at 15°
int direction = 1;            // +1 = sweeping up, -1 = sweeping down
int detectionCount = 0;       // How many consecutive detections so far
bool targetLocked = false;    // True when locked onto a target
bool triggerDone = false;     // Makes sure the trigger fires only once per lock
unsigned long lastBlinkTime = 0; // Time (ms) of the last red LED toggle
bool redState = false;          // Current on/off state of the red LED
// ---------------- ULTRASONIC ----------------
long getDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 30000);
  // No echo
  if (duration == 0)
  {
    return 999; //If nothing was detected, the function returns 999 as a "no object" flag.
  }
  long distance = duration * 0.0343 / 2;  //Sound travels about 0.0343 cm per µs. The result is divided by 2 because the sound goes to the object and back. The function then returns the distance in cm.
  return distance;
}
// ---------------- TARGET LOCK ----------------
void lockTarget(long distance)
{
  targetLocked = true;
  // Stop scan servo at target angle
  scanServo.write(currentAngle);
  digitalWrite(greenLedPin, LOW);
  // HIGH BEEP ONLY AFTER TARGET IS CONFIRMED
  tone(buzzerPin, 3000);
  // Red LED blinking
  if (millis() - lastBlinkTime >= 150)
  {
    lastBlinkTime = millis();
    redState = !redState;
    digitalWrite(redLedPin, redState);
  }
  // Trigger servo only once
  if (!triggerDone)
  {
    triggerDone = true;
    Serial.println("TARGET LOCKED!");
    Serial.print("Angle: ");
    Serial.print(currentAngle);
    Serial.print("  Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    // Trigger servo
    triggerServo.write(90);
    delay(500);
    triggerServo.write(0);
  }
}
// ---------------- UNLOCK ----------------
void unlockTarget()
{
  if (targetLocked)
  {
    Serial.println("TARGET LOST");
    Serial.println("RESUMING SCAN");
  }
  targetLocked = false; // Back to scan mode
  triggerDone = false;  // Allows the trigger to fire again on the next lock
  detectionCount = 0; // Reset the counter
  digitalWrite(greenLedPin, HIGH); // Green on
  digitalWrite(redLedPin, LOW);  // Red off
  noTone(buzzerPin);  // Buzzer off
  redState = false;  // Reset blink state
}
// ---------------- SETUP ----------------
void setup()
{
  Serial.begin(9600); //serial monitor  scan
  pinMode(trigPin, OUTPUT); //  uv pin7 is output
  pinMode(echoPin, INPUT); //uv eco pin 6 input
  pinMode(greenLedPin, OUTPUT); //green led pin3  is output
  pinMode(redLedPin, OUTPUT); //red led pin4 is output
  pinMode(buzzerPin, OUTPUT); // buzeer  pin 5 is output
  scanServo.attach(scanServoPin);
  triggerServo.attach(triggerServoPin);
  scanServo.write(currentAngle);  // Move to start angle (15°)
  triggerServo.write(0); // Trigger servo at rest position
  digitalWrite(greenLedPin, HIGH); // Green on = scanning
  digitalWrite(redLedPin, LOW);  //red led off
  noTone(buzzerPin); // buzzer  off
  Serial.println("RADAR READY"); // Wait 1 s for everything to settle
  delay(1000);
}
// ---------------- MAIN LOOP ----------------
void loop()
{
  // TARGET ALREADY LOCKED
  if (targetLocked)
  {
    // Keep scan servo at target angle
    scanServo.write(currentAngle);
    long distance = getDistance();
    Serial.print("LOCKED | Angle: ");
    Serial.print(currentAngle);
    Serial.print(" | Distance: ");
    if (distance == 999)
    {
      Serial.println("NO ECHO");
    }
    else
    {
      Serial.print(distance);
      Serial.println(" cm");
    }
    // Keep lock only while object remains
    if (distance >= minDistance &&
        distance <= maxDistance)
    {
      lockTarget(distance); // Still there: keep beeping and blinking
    }
    else
    {
      unlockTarget();  // Gone: resume scanning
    }
    delay(50);
    return;
  }
  
  // NORMAL SCANNING
  scanServo.write(currentAngle);
  delay(servoDelay);
  long distance = getDistance();
  Serial.print("SCAN | Angle: ");
  Serial.print(currentAngle);
  Serial.print(" | Distance: ");
  if (distance == 999)
  {
    Serial.println("NO OBJECT");
  }
  else
  {
    Serial.print(distance);
    Serial.println(" cm");
  }
  // CHECK FOR POSSIBLE LARGE/STABLE OBJECT
  if (distance >= minDistance &&
      distance <= maxDistance)
  {
    detectionCount++;
    Serial.print("Detection count: ");
    Serial.println(detectionCount);
    // Only lock after 10 continuous readings
    if (detectionCount >= requiredReadings)
    {
      lockTarget(distance);
    }
  }
  else
  {
    // Noise / no object
    detectionCount = 0;
  }
  // CONTINUE SCANNING
  if (!targetLocked)
  {
    currentAngle += direction;
    if (currentAngle >= scanMax)
    {
      currentAngle = scanMax;
      direction = -1;
    }

    if (currentAngle <= scanMin)
    {
      currentAngle = scanMin;
      direction = 1;
    }
  }
}