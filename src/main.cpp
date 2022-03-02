#include <Arduino.h>

//libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//libraries for DHT
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

//other libraries
#include <iostream>
#include <string>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

//packet counter
int counter = 0;

#define DHTPIN 25 // Digital pin connected to the DHT sensor

#define DHTTYPE DHT22 // DHT 22 (AM2302)

// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview

DHT_Unified dht(DHTPIN, DHTTYPE, 1, 1, 1);

uint32_t delayMS;

//moisture Sensor
#define PIN_SOIL_MOISTURE_SENSOR_1 35 // Analog pin connected to the moisture sensor
#define PIN_SOIL_MOISTURE_SENSOR_2 34 // Analog pin connected to the moisture sensor
#define PIN_SOIL_MOISTURE_SENSOR_3 39 // Analog pin connected to the moisture sensor
#define PIN_SOIL_MOISTURE_SENSOR_4 38 // Analog pin connected to the moisture sensor

int soilMoistureValue = 0;
int soilmoisturepercent = 0;
const int AirValue = 2700;
const int WaterValue = 1200;

//range Sensor
const int trigPin = 17;
const int echoPin = 23;

//define sound speed in cm/uS
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701

long duration;
float distanceCm;
float distanceInch;

void setupLoRa()
{
  Serial.println("LoRa Sender Initialization...");

  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND))
  {
    Serial.println("Starting LoRa failed!");
    while (1)
      ;
  }
  Serial.println("LoRa Initializing OK!");
  delay(2000);
}

void sendLoRaPayload(String entity_id, String payload)
{

  Serial.print("Sending packet: ");
  Serial.println(counter);

  //Send LoRa packet to receiver
  LoRa.beginPacket();
  LoRa.print(entity_id);
  LoRa.print("|");
  LoRa.print(payload);
  LoRa.endPacket();

  counter++;

  delay(2000);
}

void setupDHT()
{

  dht.begin();
  Serial.println(F("DHTxx Unified Sensor Example"));
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("°C"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("°C"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print(F("Sensor Type: "));
  Serial.println(sensor.name);
  Serial.print(F("Driver Ver:  "));
  Serial.println(sensor.version);
  Serial.print(F("Unique ID:   "));
  Serial.println(sensor.sensor_id);
  Serial.print(F("Max Value:   "));
  Serial.print(sensor.max_value);
  Serial.println(F("%"));
  Serial.print(F("Min Value:   "));
  Serial.print(sensor.min_value);
  Serial.println(F("%"));
  Serial.print(F("Resolution:  "));
  Serial.print(sensor.resolution);
  Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;
}

void getDHTMetrics()
{
  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    sendLoRaPayload("sensor.greenhouse_temperature", "{\"state\": \"" + (String)event.temperature + "\",\"attributes\": {\"unit_of_measurement\": \"°C\"}}");
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println(F("Error reading humidity!"));
  }
  else
  {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    sendLoRaPayload("sensor.greenhouse_humidity", "{\"state\": \"" + (String)event.relative_humidity + "\",\"attributes\": {\"unit_of_measurement\": \"%\"}}");
  }
}

void setupRangeSensor()
{
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);  // Sets the echoPin as an Input
}

void getRangeSensorMetrics()
{
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculate the distance
  distanceCm = duration * SOUND_SPEED / 2;

  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;

  // Prints the distance in the Serial Monitor
  Serial.print("Duration : ");
  Serial.println(duration);
  Serial.print("Distance (cm): ");
  Serial.println(distanceCm);
  Serial.print("Distance (inch): ");
  Serial.println(distanceInch);

  sendLoRaPayload("sensor.greenhouse_range", "{\"state\": \"" + (String)distanceCm + "\",\"attributes\": {\"unit_of_measurement\": \"cm\"}}");
}

void getMoistureMetrics(int pin)
{
  soilMoistureValue = analogRead(pin);
  if (isnan(soilMoistureValue))
  {
    Serial.println(F("Error reading Moisture!"));
  }
  else
  {
    Serial.print("(PIN");
    Serial.print(pin);
    Serial.print(")");
    Serial.print(F("Soil Moisture value: "));
    Serial.println(soilMoistureValue);
    switch (soilMoistureValue)
    {
    case 0 ... WaterValue - 1:
      soilmoisturepercent = 100;
      break;
    case WaterValue ... AirValue:
      soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
      break;
    case AirValue + 1 ... 3000:
      soilmoisturepercent = 0;
      break;
    }

    Serial.print(F("Soil Moisture percent: "));
    Serial.println(soilmoisturepercent);

    sendLoRaPayload("sensor.greenhouse_soil_moisture_" + (String)pin, "{\"state\": \"" + (String)soilmoisturepercent + "\",\"attributes\": {\"unit_of_measurement\": \"%\"}}");
  }
}

void setup()
{
  //initialize Serial Monitor
  Serial.begin(115200);

  setupLoRa();

  setupDHT();

  setupRangeSensor();
}

void loop()
{
  delay(30000);

  getDHTMetrics();

  getMoistureMetrics(PIN_SOIL_MOISTURE_SENSOR_1);
  getMoistureMetrics(PIN_SOIL_MOISTURE_SENSOR_2);
  getMoistureMetrics(PIN_SOIL_MOISTURE_SENSOR_3);
  getMoistureMetrics(PIN_SOIL_MOISTURE_SENSOR_4);

  getRangeSensorMetrics();
}