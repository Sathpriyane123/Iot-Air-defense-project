//  ARDUINO ULTRASONIC RADAR WITH TARGET LOCK
#include <Servo.h>    // Library to control hobby servo motors
#include <EEPROM.h>   // Library to store data that survives power loss
Servo scanServo;      // Servo that rotates the ultrasonic sensor
Servo triggerServo;   // Servo that performs the "trigger" action
// ---------------- PINS ----------------
const int trigPin = 7;           // Ultrasonic TRIG pin (sends the pulse)
const int echoPin = 6;           // Ultrasonic ECHO pin (receives the echo)
const int greenLedPin = 3;       // Green LED = scanning / searching
const int redLedPin = 4;         // Red LED = target locked
const int buzzerPin = 5;         // Buzzer
const int scanServoPin = 9;      // Signal wire of the scanning servo
const int triggerServoPin = 10;  // Signal wire of the trigger servo
// ---------------- SETTINGS ----------------
const int minDistance = 5;         // Ignore anything closer than 5 cm
const int maxDistance = 100;       // Ignore anything farther than 100 cm
const int requiredReadings = 20;   // Continuous detections needed to lock
const int scanMin = 15;            // Sweep start angle (safe, not the 0 limit)
const int scanMax = 165;           // Sweep end angle (safe, not the 180 limit)
const int servoDelay = 25;         // Delay (ms) after each 1 degree step
const int beepInterval = 150;      // Beep ON time and OFF time in ms (beep-beep speed)
// ---------------- EEPROM SETTINGS ----------------
const int angleAddress = 0;        // EEPROM address where the angle is saved
const int saveStep = 10;           // Save when angle changed by this many degrees
// ---------------- VARIABLES ----------------
int currentAngle = scanMin;        // Current servo angle (overwritten in setup)
int lastSavedAngle = scanMin;      // Angle last written to EEPROM
int direction = 1;                 // +1 = sweeping up, -1 = sweeping down
int detectionCount = 0;            // Consecutive in-range detections so far
bool targetLocked = false;         // True when locked onto a target
bool triggerDone = false;          // Makes sure trigger fires once per lock
unsigned long lastBlinkTime = 0;   // Time (ms) of the last red LED toggle
bool redState = false;             // Current on/off state of the red LED
// ---------------- SAVE ANGLE ----------------
// Saves the angle to EEPROM only if it is different from the stored one
void saveAngle(int angle)
{
  EEPROM.update(angleAddress, angle);  // update() writes only if value changed
  lastSavedAngle = angle;              // Remember what we saved
}
// ---------------- ULTRASONIC ----------------
// Returns distance in cm, or 999 if no echo
long getDistance()
{
  digitalWrite(trigPin, LOW);          // Make sure trigger starts LOW
  delayMicroseconds(3);                // Short settle time
  digitalWrite(trigPin, HIGH);         // Start the 10 microsecond pulse
  delayMicroseconds(10);               // Pulse length required by the sensor
  digitalWrite(trigPin, LOW);          // End the pulse -> sensor sends sound
  //  Measure how long the echo pin stays HIGH (microseconds)| 30000 us timeout = about 5 m; returns 0 if no echo
  long duration = pulseIn(echoPin, HIGH, 30000);
  if (duration == 0)                   // No echo received
  {
    return 999;                        // 999 = "no object" flag
  }
  // Sound travels ~0.0343 cm per microsecond. Divide by 2 because the sound goes to the object and back.
  long distance = duration * 0.0343 / 2;
  return distance;                     // Distance in cm
}
// ---------------- TARGET LOCK ----------------
void lockTarget(long distance)
{
  targetLocked = true;                 // Switch program to locked mode
  scanServo.write(currentAngle);       // Hold scan servo at target angle
  digitalWrite(greenLedPin, LOW);      // Green LED off (not scanning)
  // Beep-beep + red LED blink together, without blocking the program
  if (millis() - lastBlinkTime >= beepInterval) // Has the interval passed?
  {
    lastBlinkTime = millis();          // Store time of this toggle
    redState = !redState;              // Flip ON <-> OFF
    digitalWrite(redLedPin, redState); // Apply to the LED
    if (redState)                      // LED ON phase -> buzzer ON
    {
      tone(buzzerPin, 3000);           // 3000 Hz beep
    }
    else                               // LED OFF phase -> buzzer silent
    {
      noTone(buzzerPin);               // Gap between beeps
    }
  }
  // Fire the trigger servo only once per lock
  if (!triggerDone)
  {
    triggerDone = true;                // Prevent repeating
    saveAngle(currentAngle);           // Save the lock angle to EEPROM
    Serial.println("TARGET LOCKED!");  // Print lock details
    Serial.print("Angle: ");
    Serial.print(currentAngle);
    Serial.print("  Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    triggerServo.write(90);            // Move trigger servo to 90 degrees
    delay(500);                        // Hold for half a second
    triggerServo.write(0);             // Return to rest position
  }
}
// ---------------- UNLOCK ----------------
void unlockTarget()
{
  if (targetLocked)                    // Print only if we were locked
  {
    Serial.println("TARGET LOST");
    Serial.println("RESUMING SCAN");
  }
  targetLocked = false;                // Back to scan mode
  triggerDone = false;                 // Allow trigger on next lock
  detectionCount = 0;                  // Reset detection counter
  digitalWrite(greenLedPin, HIGH);     // Green LED on (scanning)
  digitalWrite(redLedPin, LOW);        // Red LED off
  noTone(buzzerPin);                   // Buzzer off
  redState = false;                    // Reset blink state
}
//  set up Runs once at power-on or after every reset
void setup()
{
  Serial.begin(9600);                  // Start Serial Monitor at 9600 baud
  pinMode(trigPin, OUTPUT);            // TRIG sends pulses
  pinMode(echoPin, INPUT);             // ECHO receives pulses
  pinMode(greenLedPin, OUTPUT);        // Green LED output
  pinMode(redLedPin, OUTPUT);          // Red LED output
  pinMode(buzzerPin, OUTPUT);          // Buzzer output
  // ----- Restore last scan angle from EEPROM -----
  int savedAngle = EEPROM.read(angleAddress);   // Read stored byte
  // A blank EEPROM reads 255, so accept only values inside the scan range
  if (savedAngle >= scanMin && savedAngle <= scanMax)
  {
    currentAngle = savedAngle;         // Resume from the saved position
  }
  else
  {
    currentAngle = scanMin;            // Invalid/blank -> start at 15 degrees
  }
  lastSavedAngle = currentAngle;       // Sync the "last saved" tracker
  // IMPORTANT: write() BEFORE attach() so the servos do not jump to 90 degrees
  scanServo.write(currentAngle);       // Preset scan servo to saved angle
  triggerServo.write(0);               // Preset trigger servo to rest (0)
  scanServo.attach(scanServoPin);      // Now connect scan servo (no jump)
  triggerServo.attach(triggerServoPin);// Now connect trigger servo (no jump)
  digitalWrite(greenLedPin, HIGH);     // Green on = scanning
  digitalWrite(redLedPin, LOW);        // Red off
  noTone(buzzerPin);                   // Buzzer off
  Serial.println("RADAR READY");       // Startup message
  Serial.print("Resuming at angle: "); // Show restored angle
  Serial.println(currentAngle);
  delay(1000);                         // Wait 1 s for everything to settle
}
// ---------------- MAIN LOOP ----------------
void loop()
{
  // ===== PART A: TARGET ALREADY LOCKED =====
  if (targetLocked)
  {
    scanServo.write(currentAngle);     // Keep servo at target angle
    long distance = getDistance();     // Measure again
    Serial.print("LOCKED | Angle: ");  // Print status
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
    // Keep the lock only while the object stays within range
    if (distance >= minDistance && distance <= maxDistance)
    {
      lockTarget(distance);            // Still there: keep beeping/blinking
    }
    else
    {
      unlockTarget();                  // Gone: resume scanning
    }
    delay(50);                         // Small delay between locked readings
    return;                            // Skip scanning code below
  }
  // ===== PART B: NORMAL SCANNING =====
  scanServo.write(currentAngle);       // Move sensor to current angle
  delay(servoDelay);                   // Wait for servo to settle
  long distance = getDistance();       // Measure distance
  Serial.print("SCAN | Angle: ");      // Print scan status
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
  // Check for a possible large / stable object
  if (distance >= minDistance && distance <= maxDistance)
  {
    detectionCount++;                  // One more in-range reading
    Serial.print("Detection count: ");
    Serial.println(detectionCount);
    // Lock after 20 continuous readings
    if (detectionCount >= requiredReadings)
    {
      lockTarget(distance);
    }
  }
  else
  {
    detectionCount = 0;                // Noise / no object: reset counter
  }
  // ===== PART C: CONTINUE SWEEPING =====
  if (!targetLocked)                   // Skip if we just locked
  {
    currentAngle += direction;         // Move 1 degree in current direction
    // Save angle to EEPROM every 'saveStep' degrees (not every step)
    if (abs(currentAngle - lastSavedAngle) >= saveStep)
    {
      saveAngle(currentAngle);
    }
    if (currentAngle >= scanMax)       // Reached the upper limit
    {
      currentAngle = scanMax;
      direction = -1;                  // Sweep back down
    }
    if (currentAngle <= scanMin)       // Reached the lower limit
    {
      currentAngle = scanMin;
      direction = 1;                   // Sweep back up
    }
  }
}
