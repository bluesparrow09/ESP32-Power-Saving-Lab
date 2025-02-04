#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "UW MPSK"
#define WIFI_PASSWORD ";3erXsU7+>"
#define DATABASE_SECRET "AIzaSyAp4dVlwavix9n1VuV9pQrx6q5xyZ_jqf4"
#define DATABASE_URL "https://esp32-firebase-demo-546be-default-rtdb.firebaseio.com/"

#define DEEP_SLEEP_DURATION 30000000 // 30 seconds in microseconds
#define MOTION_THRESHOLD 50          // cm
#define DATA_SEND_INTERVAL 120000    // 2 minutes in milliseconds

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));
FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

const int trigPin = D4;
const int echoPin = D5;
const float soundSpeed = 0.034; // cm/us
unsigned long lastDataSendMillis = 0;

float measureDistance();
void connectToWiFi();
void sendDataToFirebase(float distance);

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  connectToWiFi();
  ssl.setInsecure();
  initializeApp(client, app, getAuth(dbSecret));
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
}

void loop() {
  float distance = measureDistance();
  Serial.printf("Distance: %.2f cm\n", distance);

  if (distance < MOTION_THRESHOLD) {
    if (millis() - lastDataSendMillis >= DATA_SEND_INTERVAL) {
      sendDataToFirebase(distance);
      lastDataSendMillis = millis();
    }
  } else {
    Serial.println("No motion detected, entering deep sleep...");
    WiFi.disconnect();
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
    esp_deep_sleep_start();
  }
  delay(500); // Reduce power consumption
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  return duration * soundSpeed / 2;
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void sendDataToFirebase(float distance) {
  String path = "/distance";
  String result = Database.push<float>(client, path, distance);
  if (client.lastError().code() == 0) {
    Serial.printf("Data sent: %.2f cm\n", distance);
  } else {
    Serial.printf("Error sending data: %s\n", client.lastError().message().c_str());
  }
}
