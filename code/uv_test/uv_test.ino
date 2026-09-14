/*
  HC-SR04 Ultrasonic Sensor + Buzzer (2 Meter Detection)

  Wiring:
  HC-SR04 VCC  -> Arduino 5V
  HC-SR04 GND  -> Arduino GND
  HC-SR04 TRIG -> Pin 7
  HC-SR04 ECHO -> Pin 6

  Buzzer +     -> Pin 5
  Buzzer -     -> Arduino GND

  The buzzer beeps faster as an object gets closer.
  Maximum detection range: 2 meters (200 cm)
*/

const int trigPin = 7;
const int echoPin = 6;
const int buzzerPin = 5;

const int maxDistance = 200;   // 2 meters

void setup() {
  Serial.begin(9600);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(buzzerPin, OUTPUT);

  noTone(buzzerPin);

  Serial.println("HC-SR04 + Buzzer Test Started");
}

void loop() {

  // Clear trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Send 10 µs pulse
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read echo (40 ms timeout)
  long duration = pulseIn(echoPin, HIGH, 40000);

  if (duration == 0) {
    Serial.println("No Echo Received");
    noTone(buzzerPin);
    delay(300);
    return;
  }

  // Calculate distance
  float distance = duration * 0.0343 / 2.0;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  if (distance <= maxDistance) {

    // Faster beep when object is closer
    int beepDelay = map((int)distance, 0, maxDistance, 50, 500);

    tone(buzzerPin, 1000);
    delay(beepDelay);

    noTone(buzzerPin);
    delay(beepDelay);

  } else {

    // Object beyond 2 meters
    noTone(buzzerPin);
    delay(100);

  }
}