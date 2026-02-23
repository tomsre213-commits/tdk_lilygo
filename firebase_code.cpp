#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ================= WIFI =================
#define WIFI_SSID     "NetPay"
#define WIFI_PASSWORD "netpay123"

// ================= FIREBASE =================
#define FIREBASE_DB_URL "https://tindak-8401f-default-rtdb.asia-southeast1.firebasedatabase.app"

#define LOCATION_PATH "/esp32-lilygo/location.json"

WiFiClientSecure client;
HTTPClient https;

unsigned long lastSend = 0;

void connectWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void writeLocationRaw(const String& latitude, const String& longitude) {
  String json = "{";
  json += "\"latitude\":\"" + latitude + "\",";
  json += "\"longitude\":\"" + longitude + "\"";
  json += "}";

  String url = String(FIREBASE_DB_URL) + LOCATION_PATH;

  https.begin(client, url);
  https.addHeader("Content-Type", "application/json");

  int httpCode = https.PUT(json);

  Serial.println("---- Firebase PUT ----");
  Serial.printf("URL: %s\n", url.c_str());
  Serial.printf("HTTP: %d\n", httpCode);

  if (httpCode > 0) {
    Serial.println("Response:");
    Serial.println(https.getString());
  } else {
    Serial.println("Failed to write to Firebase.");
  }

  https.end();
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  client.setInsecure();
}

void loop() {
  if (millis() - lastSend >= 3000) {
    lastSend = millis();

    String lat = "8.2280";
    String lon = "124.2452";

    writeLocationRaw(lat, lon);
  }
}