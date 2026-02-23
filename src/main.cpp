#include <Arduino.h>

#define TINY_GSM_MODEM_SIM7000
#define TINY_GSM_RX_BUFFER 1024

#include <TinyGsmClient.h>

// LilyGO T-SIM7000G Pinout
#define UART_BAUD   115200
#define PIN_DTR     25
#define PIN_TX      27
#define PIN_RX      26
#define PWR_PIN     4

#define LED_PIN     12

#define SerialMon Serial
#define SerialAT  Serial1

TinyGsm modem(SerialAT);

// ---- SETTINGS ----
const unsigned long GPS_INTERVAL_MS = 1000;   // print every 1s (change as you want)
const unsigned long GPS_WARMUP_MS   = 15000;  // wait for first fix attempt

unsigned long lastGpsMs = 0;
bool gpsReady = false;

void powerOnModem() {
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, HIGH);
  delay(300);
  digitalWrite(PWR_PIN, LOW);
}

bool gpsPower(bool on) {
  // SIM7000G GPS power via GPIO4:
  // AT+SGPIO=0,4,1,1  -> ON
  // AT+SGPIO=0,4,1,0  -> OFF
  modem.sendAT(String("+SGPIO=0,4,1,") + (on ? "1" : "0"));
  if (modem.waitResponse(10000L) != 1) {
    SerialMon.println(on ? " SGPIO GPS ON failed " : " SGPIO GPS OFF failed ");
    return false;
  }
  return true;
}

void printGpsOnce() {
  float lat = 0, lon = 0, speed = 0, alt = 0, accuracy = 0;
  int vsat = 0, usat = 0;
  int year = 0, month = 0, day = 0, hour = 0, min = 0, sec = 0;

  if (modem.getGPS(&lat, &lon, &speed, &alt, &vsat, &usat, &accuracy,
                   &year, &month, &day, &hour, &min, &sec)) {

    SerialMon.println("✅ GPS Fix!");
    SerialMon.println("Latitude: " + String(lat, 8) + "\tLongitude: " + String(lon, 8));
    SerialMon.println("Speed: " + String(speed) + "\tAltitude: " + String(alt));
    SerialMon.println("Visible Satellites: " + String(vsat) + "\tUsed Satellites: " + String(usat));
    SerialMon.println("Accuracy: " + String(accuracy));
    SerialMon.println("Date: " + String(year) + "-" + String(month) + "-" + String(day));
    SerialMon.println("Time: " + String(hour) + ":" + String(min) + ":" + String(sec));

    // optional raw string
    String gps_raw = modem.getGPSraw();
    SerialMon.println("RAW: " + gps_raw);
  } else {
    SerialMon.println("⏳ No GPS fix yet...");
  }

  SerialMon.println("----------------------------------");
}

void setup() {
  SerialMon.begin(115200);
  delay(200);
  SerialMon.println("Place your board outside to catch satellite signal");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH); // LED off (depending on board wiring)

  powerOnModem();

  SerialAT.begin(UART_BAUD, SERIAL_8N1, PIN_RX, PIN_TX);

  SerialMon.println("Initializing modem...");
  if (!modem.restart()) {
    SerialMon.println("Failed to restart modem, continuing...");
  }

  String modemName = modem.getModemName();
  delay(200);
  SerialMon.println("Modem Name: " + modemName);

  String modemInfo = modem.getModemInfo();
  delay(200);
  SerialMon.println("Modem Info: " + modemInfo);

  // Turn ON GPS power + GPS only once
  SerialMon.println("Turning on GPS power...");
  gpsPower(true);

  SerialMon.println("Enabling GPS...");
  modem.enableGPS();

  SerialMon.println("Warming up GPS...");
  delay(GPS_WARMUP_MS);

  gpsReady = true;
  lastGpsMs = millis();
}

void loop() {
  modem.maintain(); // keep modem background tasks running

  if (!gpsReady) return;

  unsigned long now = millis();
  if (now - lastGpsMs >= GPS_INTERVAL_MS) {
    lastGpsMs = now;
    SerialMon.println("Requesting current GPS...");
    printGpsOnce();
  }
}