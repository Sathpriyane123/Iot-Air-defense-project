// ============================================================
//  PROCESSING RADAR DISPLAY
//  Reads the Serial output of the Arduino radar sketch and draws
//  a live radar screen: sweep line, detected objects, lock status.
//
//  Works with the existing Arduino messages such as:
//    SCAN | Angle: 45 | Distance: 60 cm
//    SCAN | Angle: 46 | Distance: NO OBJECT
//    LOCKED | Angle: 90 | Distance: 40 cm
//    TARGET LOCKED!  /  TARGET LOST
// ============================================================
import processing.serial.*;          // Serial communication library
// ---------------- SETTINGS ----------------
boolean useSerial = true;            // true = read Arduino, false = demo mode
int portIndex = 0;                   // Which port in the printed list is your Arduino
int baudRate = 9600;                 // Must match Serial.begin(9600) in Arduino
float maxRange = 100;                // Same as maxDistance in Arduino (cm)
float minRange = 5;                  // Same as minDistance in Arduino (cm)
int blipLifetime = 4000;             // How long a detected dot stays (ms)
// ---------------- VARIABLES ----------------
Serial port;                         // Serial port object
float cx, cy;                        // Radar center point (bottom middle)
float radiusPx;                      // Radar radius in pixels
float pxPerCm;                       // Pixels per centimeter
int angle = 90;                      // Current servo angle
int sweepDir = 1;                    // Sweep direction (+1 / -1), used for the trail
float dist = 999;                    // Latest distance (999 = nothing)
boolean locked = false;              // True when Arduino reports a target lock
ArrayList<float[]> blips = new ArrayList<float[]>();  // Detected dots {angle, dist, time}
// Demo mode variables
int demoAngle = 15;
int demoDir = 1;
// ---------------- SETUP ----------------
void setup()
{
  size(1000, 620);                   // Window size
  cx = width / 2;                    // Center X
  cy = height - 50;                  // Center Y near the bottom
  radiusPx = min(width / 2 - 50, height - 110);  // Radar radius
  pxPerCm = radiusPx / maxRange;     // Scale: pixels for each cm
  textFont(createFont("Consolas", 16));
  if (useSerial)
  {
    println("Available ports:");
    println(Serial.list());          // Shows all serial ports in the console
    if (Serial.list().length == 0)   // No Arduino found
    {
      println("No serial port found. Switching to DEMO mode.");
      useSerial = false;
    } else
    {
      port = new Serial(this, Serial.list()[portIndex], baudRate);
      port.bufferUntil('\n');        // Call serialEvent() at every new line
    }
  }
}
// ---------------- MAIN DRAW LOOP ----------------
void draw()
{
  background(0, 15, 8);              // Dark green-black background
  if (!useSerial) runDemo();         // Fake data when no Arduino connected
  drawGrid();                        // Rings, angle lines, labels
  drawBlips();                       // Detected objects
  drawSweep();                       // Sweep line with fading trail
  drawInfo();                        // Text information panel
}
// ---------------- SERIAL EVENT ----------------
// Runs automatically when a full line arrives from the Arduino
void serialEvent(Serial p)
{
  String line = p.readStringUntil('\n');   // Read one line
  if (line == null) return;                // Nothing read
  line = trim(line);                       // Remove spaces / newline
  parseLine(line);                         // Extract the data
}
// ---------------- PARSE ARDUINO TEXT ----------------
void parseLine(String line)
{
  if (line.startsWith("TARGET LOCKED"))    // Lock message
  {
    locked = true;
    return;
  }
  if (line.startsWith("TARGET LOST"))      // Lost message
  {
    locked = false;
    return;
  }
  // Only SCAN and LOCKED lines contain angle and distance
  if (line.startsWith("SCAN") || line.startsWith("LOCKED"))
  {
    locked = line.startsWith("LOCKED");    // LOCKED lines mean lock is active

    String[] a = match(line, "Angle: (\\d+)");          // Find the angle number
    String[] d = match(line, "Distance: (\\d+) cm");    // Find the distance number

    if (a != null)
    {
      float distance = (d != null) ? float(d[1]) : 999; // No number = no object
      addReading(int(a[1]), distance);
    }
  }
}
// ---------------- STORE A READING ----------------
void addReading(int a, float d)
{
  if (a != angle) sweepDir = (a > angle) ? 1 : -1;  // Track sweep direction
  angle = a;                                        // Update current angle
  dist = d;                                         // Update current distance

  if (d >= minRange && d <= maxRange)               // Valid object -> add dot
  {
    blips.add(new float[] { a, d, millis() });
  }
}
// ---------------- DEMO MODE ----------------
// Simulates a sweep and an object so you can test without Arduino
void runDemo()
{
  if (frameCount % 2 == 0)                          // Slow the simulation a bit
  {
    demoAngle += demoDir;
    if (demoAngle >= 165) demoDir = -1;
    if (demoAngle <= 15)  demoDir = 1;

    float d = 999;
    if (demoAngle > 95 && demoAngle < 125) d = 55;  // Fake object at 95-125 degrees
    locked = (demoAngle > 105 && demoAngle < 115);  // Fake lock in the middle
    addReading(demoAngle, d);
  }
}
// ---------------- DRAW GRID ----------------
void drawGrid()
{
  noFill();
  stroke(0, 160, 60);                               // Green lines
  strokeWeight(1.5);

  // Distance rings every 25 cm
  for (int i = 1; i <= 4; i++)
  {
    float r = radiusPx * i / 4.0;
    arc(cx, cy, r * 2, r * 2, PI, TWO_PI);          // Half circle
    fill(0, 200, 80);
    textAlign(CENTER);
    text(int(maxRange * i / 4) + "cm", cx + r - 25, cy + 20);  // Label
    noFill();
  }

  // Angle lines every 30 degrees
  for (int a = 0; a <= 180; a += 30)
  {
    float x = cx + radiusPx * cos(radians(a));
    float y = cy - radiusPx * sin(radians(a));
    line(cx, cy, x, y);
    fill(0, 200, 80);
    textAlign(CENTER);
    text(a + "\u00B0", cx + (radiusPx + 25) * cos(radians(a)),
      cy - (radiusPx + 12) * sin(radians(a)));  // Angle label
    noFill();
  }

  line(cx - radiusPx, cy, cx + radiusPx, cy);       // Baseline
}
// ---------------- DRAW SWEEP LINE ----------------
void drawSweep()
{
  strokeWeight(3);
  for (int i = 0; i < 25; i++)                      // 25 lines make the fading trail
  {
    float a = angle - sweepDir * i;                 // Trail sits behind the sweep
    float alpha = map(i, 0, 25, 220, 0);            // Fade out
    if (locked) stroke(255, 40, 40, alpha);         // Red when locked
    else        stroke(0, 255, 90, alpha);          // Green when scanning
    line(cx, cy, cx + radiusPx * cos(radians(a)),
      cy - radiusPx * sin(radians(a)));
  }
}
// ---------------- DRAW DETECTED OBJECTS ----------------
void drawBlips()
{
  noStroke();
  for (int i = blips.size() - 1; i >= 0; i--)       // Backwards so we can remove
  {
    float[] b = blips.get(i);
    float age = millis() - b[2];                    // How old is this dot

    if (age > blipLifetime)                         // Too old -> delete
    {
      blips.remove(i);
      continue;
    }
    float alpha = map(age, 0, blipLifetime, 255, 0); // Fade with age
    float px = cx + b[1] * pxPerCm * cos(radians(b[0]));
    float py = cy - b[1] * pxPerCm * sin(radians(b[0]));

    fill(255, 60, 60, alpha);                       // Red dot
    ellipse(px, py, 14, 14);
  }
}
// ---------------- INFO PANEL ----------------
void drawInfo()
{
  fill(0, 255, 90);
  textAlign(LEFT);
  text("Indian Army Deffence RADAR System", 390, 30);
  text("Angle   : " + angle + "\u00B0", 20, 55);

  if (dist >= minRange && dist <= maxRange)
    text("Distance: " + int(dist) + " cm", 20, 80);
  else
    text("Distance: --", 20, 80);

  if (locked)
  {
    fill(255, 40, 40);
    text("STATUS  : TARGET LOCKED", 20, 105);
  } else
  {
    fill(0, 255, 90);
    text("STATUS  : SCANNING", 20, 105);
  }
  fill(120);
}
