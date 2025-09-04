#include <BluetoothSerial.h>
#include "DHT.h"
#include "MAX30105.h"
#include "heartRate.h"

#define DHTPIN 14
#define DHTTYPE DHT11
#define SOUND_PIN 36
#define LED_PIN 12

DHT dht(DHTPIN, DHTTYPE);
MAX30105 particleSensor;
BluetoothSerial SerialBT;

float temperature;
int soundLevel;
int heartRateBPM = 0;
int spo2 = 0;
bool btConnected = false;
unsigned long lastSensorReadTime = 0;
const unsigned long SENSOR_READ_INTERVAL = 1000;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;
float beatsPerMinute;
int beatAvg;

void readDHT11();
void readSoundSensor();
void readMAX30102();
void sendDataViaBluetooth();
void controlLED();
void checkBluetoothConnection();
void handleSensorErrors();

void setup() {
  Serial.begin(115200);
  
  if (!SerialBT.begin("Collarm_ESP32")) {
    while (1);
  }

  dht.begin();
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    while (1);
  }
  
  byte ledBrightness = 0x1F;
  byte sampleAverage = 4;
  byte ledMode = 2;
  int sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;
  
  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
  particleSensor.enableDIETEMPRDY();
}

void loop() {
  unsigned long currentTime = millis();
  
  checkBluetoothConnection();
  
  if (currentTime - lastSensorReadTime >= SENSOR_READ_INTERVAL) {
    lastSensorReadTime = currentTime;
    
    readDHT11();
    readSoundSensor();
    readMAX30102();
    
    handleSensorErrors();
    
    if (btConnected) {
      sendDataViaBluetooth();
    }
    
    controlLED();
    
    Serial.print(temperature);
    Serial.print(",");
    Serial.print(heartRateBPM);
    Serial.print(",");
    Serial.print(spo2);
    Serial.print(",");
    Serial.println(soundLevel);
  }
  
  delay(10);
}

void readDHT11() {
  float newTemp = dht.readTemperature();
  if (!isnan(newTemp)) {
    temperature = newTemp;
  }
}

void readSoundSensor() {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(SOUND_PIN);
    delay(1);
  }
  soundLevel = sum / 10;
}

void readMAX30102() {
  long irValue = particleSensor.getIR();
  
  if (irValue > 50000) {
    if (checkForBeat(irValue)) {
      long delta = millis() - lastBeat;
      lastBeat = millis();
      
      beatsPerMinute = 60 / (delta / 1000.0);
      
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        rateSpot %= RATE_SIZE;
        
        beatAvg = 0;
        for (byte x = 0; x < RATE_SIZE; x++) {
          beatAvg += rates[x];
        }
        beatAvg /= RATE_SIZE;
        
        heartRateBPM = beatAvg;
      }
    }
    
    spo2 = map(constrain(irValue, 50000, 100000), 50000, 100000, 90, 100);
  } else {
    heartRateBPM = 0;
    spo2 = 0;
    for (byte x = 0; x < RATE_SIZE; x++) {
      rates[x] = 0;
    }
  }
}

void sendDataViaBluetooth() {
  String data = "{\"temp\":" + String(temperature, 1) +
                ",\"hr\":" + String(heartRateBPM) +
                ",\"spo2\":" + String(spo2) +
                ",\"sound\":" + String(soundLevel) +
                "}";
  
  SerialBT.println(data);
}

void controlLED() {
  if (heartRateBPM > 120 || temperature > 39.0 || soundLevel > 800) {
    digitalWrite(LED_PIN, (millis() % 500) < 250);
  } else if (!btConnected) {
    digitalWrite(LED_PIN, (millis() % 1000) < 100);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

void checkBluetoothConnection() {
  btConnected = (SerialBT.hasClient());
}

void handleSensorErrors() {
  if (isnan(temperature)) {
    Serial.println("DHT Error");
  }
  
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 Error");
  }
}