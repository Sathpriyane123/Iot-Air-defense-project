import processing.serial.*;

Serial myPort;

int angle = 15;
int distance = 100;

void setup()
{
  size(1200,700);
  smooth();

  println(Serial.list());

  // Change COM5 to your Arduino COM port
  myPort = new Serial(this,"COM5",9600);
  myPort.bufferUntil('\n');
}

void draw()
{
  background(0);

  translate(width/2,height-50);

  // Radar Circles
  stroke(0,255,0);
  strokeWeight(2);
  noFill();

  arc(0,0,1000,1000,PI,TWO_PI);
  arc(0,0,800,800,PI,TWO_PI);
  arc(0,0,600,600,PI,TWO_PI);
  arc(0,0,400,400,PI,TWO_PI);
  arc(0,0,200,200,PI,TWO_PI);

  // Angle Lines
  for(int a=0;a<=180;a+=30)
  {
    line(0,0,
         500*cos(radians(a)),
        -500*sin(radians(a)));
  }

  // Base Line
  line(-500,0,500,0);

  // Sweep Line
  stroke(0,255,0);
  strokeWeight(4);

  float sx = 500*cos(radians(angle));
  float sy = -500*sin(radians(angle));

  line(0,0,sx,sy);

  // Target
  if(distance<=20)
  {
    float r = map(distance,0,20,0,500);

    float tx = r*cos(radians(angle));
    float ty = -r*sin(radians(angle));

    stroke(255,0,0);
    strokeWeight(12);
    point(tx,ty);

    strokeWeight(1);
    stroke(255,0,0);
    line(0,0,tx,ty);
  }

  resetMatrix();

  fill(0,255,0);

  textSize(30);
  text(" Indian Army AIR DEFENSE RADAR",20,40);

  textSize(22);
  text("Angle : "+angle+"°",20,90);
  text("Distance : "+distance+" cm",20,120);

  if(distance<=20)
  {
    fill(255,0,0);
    text("TARGET LOCKED",20,160);
  }
  else
  {
    fill(0,255,0);
    text("NO TARGET",20,160);
  }
}

void serialEvent(Serial p)
{
  String data = trim(p.readStringUntil('\n'));

  if(data==null)
    return;

  println(data);

  try
  {
    if(data.startsWith("Angle:"))
    {
      String[] parts = split(data,' ');

      angle = int(parts[1]);

      if(parts[3].equals("Out"))
      {
        distance = 100;
      }
      else
      {
        distance = int(parts[3]);
      }
    }
  }
  catch(Exception e)
  {
    println("Bad Data : "+data);
  }
}
