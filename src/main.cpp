#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024
#include <TinyGsmClient.h>

// ================= WIFI =================
#define WIFI_SSID     "NetPay"
#define WIFI_PASSWORD "netpay123"

// ================= FIREBASE =================
#define FIREBASE_DB_URL "https://tindak-8401f-default-rtdb.asia-southeast1.firebasedatabase.app"
#define LOCATION_PATH "/esp32-lilygo/location.json"

// ================= SIM7000 PINS =================
#define UART_BAUD 115200
#define PIN_TX    27
#define PIN_RX    26
#define PWR_PIN   4

#define SerialMon Serial
#define SerialAT  Serial1

TinyGsm modem(SerialAT);

// HTTPS
WiFiClientSecure client;
HTTPClient https;

unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 5000; // send every 5 seconds

// ================= WIFI =================
void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());
}

// ================= FIREBASE SEND =================
void writeLocationRaw(String latitude, String longitude) {

  String json = "{";
  json += "\"latitude\":\"" + latitude + "\",";
  json += "\"longitude\":\"" + longitude + "\"";
  json += "}";

  String url = String(FIREBASE_DB_URL) + LOCATION_PATH;

  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");

  int httpCode = https.PUT(json);

  Serial.println("---- Firebase PUT ----");
  Serial.println(json);
  Serial.printf("HTTP: %d\n", httpCode);

  https.end();
}

// ================= GPS =================
void powerOnModem() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);
}

bool gpsPower(bool on) {
  modem.sendAT(String("+SGPIO=0,4,1,") + (on ? "1" : "0"));
  return modem.waitResponse(10000L) == 1;
}

// ================= SETUP =================
void setup() {
  SerialMon.begin(115200);

  // WiFi
  connectWiFi();
  client.setInsecure();

  // Start modem
  powerOnModem();
  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  Serial.println("Initializing modem...");
  modem.restart();

  Serial.println("Turning GPS power ON...");
  gpsPower(true);

  Serial.println("Enabling GPS...");
  modem.enableGPS();

  Serial.println("Waiting GPS warmup...");
  delay(15000);
}

// ================= LOOP =================
void loop() {

  modem.maintain();

  if (millis() - lastSend >= SEND_INTERVAL) {
    lastSend = millis();

    float lat = 0, lon = 0, speed, alt, accuracy;
    int vsat, usat;
    int year, month, day, hour, min, sec;

    Serial.println("Getting GPS...");

    if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy,
                     &year, &month, &day, &hour, &min, &sec)) {

      String latitude  = String(lat, 8);
      String longitude = String(lon, 8);

      Serial.println("GPS FIX!");
      Serial.println(latitude + ", " + longitude);

      // Send to Firebase
      writeLocationRaw(latitude, longitude);
    }
    else {
      Serial.println("No GPS fix yet...");
    }
  }
}