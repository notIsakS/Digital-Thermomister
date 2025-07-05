#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <string.h>
#include <math.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"


char ssid[] = WIFI_SSID;
char pass[] = WIFI_PASS;

const char discordHost[] = "discord.com";
const String discordPath = API_KEY;

int status = WL_IDLE_STATUS;

WiFiSSLClient wifi;
HttpClient client = HttpClient(wifi, discordHost, 443);

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int thermistorPin = A0;
const int numReadings = 10;
int readings[numReadings];

const float A = 0.001129148;
const float B = 0.000234125;
const float C = 0.0000000876741;

const int D10 = 10;
const int D9 = 9;

const int D7 = 7;
const int D6 = 6;

// Physical buttons
int value1 = 0;
int value2 = 0;

int upperLimit = 0;
int lowerLimit = 0;

int prefferedTemperature = 0;

float newRead = 0;
String intToString = "";

void setup() {

  Serial.begin(9600);
  while (!Serial)
    ;  // Wait for serial monitor to open

  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  while (status != WL_CONNECTED) {
    status = WiFi.begin(ssid, pass);
    delay(500);  // Wait for connection
  }
  Serial.println("Connected to Wi-Fi");
  Serial.println(WiFi.localIP());

  delay(2000);

  pinMode(D10, INPUT_PULLDOWN);
  pinMode(D9, INPUT_PULLDOWN);

  pinMode(D7, OUTPUT);  // GREEN
  pinMode(D6, OUTPUT);  // RED

  digitalWrite(D6, HIGH);
  digitalWrite(D7, LOW);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Set temp: ");

  lcd.setCursor(0, 1);
  lcd.print("Current temp: ");

  //sendDiscordMessage("Hello, from Arduino Nano 33 IoT!");
}

void loop() {
  lowerLimit = prefferedTemperature - 1;
  upperLimit = prefferedTemperature + 5;

  float Vout;
  for (int i = 0; i < numReadings; i++) {
    readings[i] = analogRead(thermistorPin);
    Vout += readings[i];
    delay(10);
  }

  Vout /= numReadings;
  delay(10);

  value1 = digitalRead(D9);
  if (value1 == HIGH) {
    prefferedTemperature -= 1;
    Serial.print(prefferedTemperature);
    Serial.println("Set temperature: \n");
    delay(50);
  }

  value2 = digitalRead(D10);
  if (value2 == HIGH) {
    prefferedTemperature += 1;
    Serial.print(prefferedTemperature);
    Serial.println("Set temperature: \n");
    delay(50);
  }

  if (lowerLimit <= newRead && upperLimit >= newRead) {
    digitalWrite(D7, HIGH);
    digitalWrite(D6, LOW);
    //sendDiscordMessage("Temperature is perfect, ready to drink!");
  } else {
    digitalWrite(D7, LOW);
    digitalWrite(D6, HIGH);
  }

  newRead = steinhartFormula(Vout);
  intToString = String(newRead);
  Serial.println(newRead);

  if (prefferedTemperature < 10) {
    lcd.setCursor(11, 0);
    lcd.print("C");
    lcd.setCursor(12,0);
    lcd.print(" ");
  } else {
    lcd.setCursor(11,0);
    lcd.print(" ");
    lcd.setCursor(12, 0);
    lcd.print("C");
  }

  lcd.setCursor(10, 0);
  lcd.print(prefferedTemperature);

  lcd.setCursor(13, 1);
  lcd.print(intToString);
  lcd.setCursor(15, 1);
  lcd.print("C");

  //sendDiscordMessage(newNewRead);
  delay(150);
}

// Steinhart formula and kelving to celcious convertion + sensor devation
float steinhartFormula(float R) {
  float T = 0;
  T = (A + B * (log(R)) + C * (log(R) * exp(3))) * exp(-1);
  Serial.println(T);
  return T - 375;
}

void sendDiscordMessage(String message) {
  String jsonPayload = "{\"content\": \"" + message + "\"}";

  Serial.println("Sending Discord webhook...");

  client.post(discordPath, "application/json", jsonPayload);

  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  Serial.print("Discord API Status Code: ");
  Serial.println(statusCode);
  Serial.print("Discord API Response: ");
  Serial.println(response);

  client.stop();
  Serial.print("Sent Webhook");
}