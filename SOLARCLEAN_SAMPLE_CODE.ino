#include <Servo.h>
#include <HX711.h>
#include <Adafruit_TCS34725.h>
#include <NewPing.h>

#define TRIGGER_PIN 4
#define ECHO_PIN 5
#define MAX_DISTANCE 10
#define CAP_PIN 3
#define MOISTURE_PIN A2
#define SERVO_PAPER_PIN 9
#define SERVO_PLASTIC_PIN 10
#define DT A1
#define SCK A0

HX711 scale;
Servo servoPaper;
Servo servoPlastic;
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);

float weightThreshold = 20.0;
int colorThreshold = 150;
int moistureThreshold = 600;
const int SERVO_CLOSED = 0;
const int SERVO_OPEN = 90;

void setup() {
  pinMode(CAP_PIN, INPUT);

  servoPaper.attach(SERVO_PAPER_PIN);
  servoPlastic.attach(SERVO_PLASTIC_PIN);
  servoPaper.write(SERVO_CLOSED);
  servoPlastic.write(SERVO_CLOSED);

  scale.begin(DT, SCK);
  scale.set_scale();
  scale.tare();

  tcs.begin();
}

void loop() {
  if (sonar.ping_cm() <= 5 && sonar.ping_cm() != 0) {
    delay(200);

    int votesPlastic = 0;
    int votesPaper = 0;

    if (digitalRead(CAP_PIN) == HIGH) {
      votesPlastic++;
    } else {
      votesPaper++;
    }

    float w = scale.get_units(5);
    if (w > weightThreshold) {
      votesPaper++;
    } else {
      votesPlastic++;
    }

    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    if (r > colorThreshold && g > colorThreshold && b > colorThreshold) {
      votesPaper++;
    } else {
      votesPlastic++;
    }

    int moistureValue = analogRead(MOISTURE_PIN);
    if (moistureValue > moistureThreshold) {
      votesPaper++;
    } else {
      votesPlastic++;
    }

    if (votesPlastic > votesPaper) {
      activateServo(servoPlastic);
    } else {
      activateServo(servoPaper);
    }

    delay(2000);
  }
}

void activateServo(Servo &s) {
  s.write(SERVO_OPEN);
  delay(1200);
  s.write(SERVO_CLOSED);
}