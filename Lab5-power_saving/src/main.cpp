#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h" // For handling Firebase auth tokens
#include "addons/RTDBHelper.h"  // For Realtime Database operations

// WiFi and Firebase configuration details
const char *ssid = "UW MPSK";
const char *password = "K!(t7n4$#j";
#define DATABASE_URL "https://esp32-firebase-demo-4050d-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyBMNLogIIb-VXE5o80XCPwYXDesqyfpwCg"
#define STAGE_INTERVAL 12000 // Time in milliseconds for each operational stage
#define MAX_WIFI_RETRIES 5   // Maximum attempts to connect to WiFi

int uploadInterval = 1000; // Interval for uploading data to Firebase in milliseconds

// Firebase Data object for interacting with Firebase
FirebaseData fbdo;

// Firebase authentication and configuration
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0; // Tracks the last time data was sent
bool signupOK = false;                // Flag to check if Firebase signup was successful

// HC-SR04 Ultrasonic Sensor Pins
const int trigPin = 2;
const int echoPin = 3;

// Sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

void setup()
{
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initial delay allowing the system to settle
  Serial.println("Initial settling...");
  delay(STAGE_INTERVAL);

  // Connect to WiFi
  connectToWiFi();

  // Initialize Firebase
  initFirebase();

  // Main operational loop before deep sleep
  unsigned long startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    float distance = measureDistance();
    sendDataToFirebase(distance);
    delay(uploadInterval); // Delay between uploads
  }

  // Enter deep sleep mode to save power
  Serial.println("Entering deep sleep mode...");
  esp_sleep_enable_timer_wakeup(STAGE_INTERVAL * 1000); // Time in microseconds
  esp_deep_sleep_start();
}

void loop()
{
  // Not used due to deep sleep
}

float measureDistance()
{
  // Ultrasonic distance measurement
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < MAX_WIFI_RETRIES)
  {
    delay(1000);
    Serial.print(".");
    retryCount++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to WiFi. Please check your credentials");
    return;
  }
  Serial.println("Connected to WiFi");
}

void initFirebase()
{
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Attempt to sign up (this example assumes no specific user account)
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("Firebase sign-up successful.");
    signupOK = true;
  }
  else
  {
    Serial.print("Firebase sign-up failed: ");
    Serial.println(config.signer.signupError.message.c_str());
    return;
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void sendDataToFirebase(float distance)
{
  if (Firebase.ready() && signupOK && millis() - sendDataPrevMillis >= uploadInterval)
  {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.pushFloat(&fbdo, "/distanceMeasurements", distance))
    {
      Serial.print("Distance data sent: ");
      Serial.println(distance);
    }
    else
    {
      Serial.print("Failed to send data: ");
      Serial.println(fbdo.errorReason());
    }
  }
}
