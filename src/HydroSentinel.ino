/*
 * UMWELTKONTROLLSYSTEM v2.0
 * Arduino Mega 2560 Projekt
 * 
 * Modulare Architektur für Datensammlung und -protokollierung:
 * - Gas-Sensoren (MQ-Serie)
 * - Radioaktivitätsdetektor
 * - SD-Karten Datenlogger
 * - Echtzeituhr (RTC)
 * 
 * Hardware: Arduino Mega 2560
 * Compiler: Arduino IDE
 * 
 * Alle Module sind in separate Header-Dateien organisiert
 * für bessere Wartbarkeit und Wiederverwendbarkeit.
 */

// ==============================================
// INCLUDES
// ==============================================

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
// #include <OneWire.h>  // DEAKTIVIERT
#include "RTClib.h"


// Projekt-Module
#include "config.h"
#include "utilities.h"
#include "sensors.h"

#include "rtc_module.h"
#include "data_logger.h"
#include "display.h"  // OLED Display Modul

// ==============================================
// GLOBALE VARIABLEN
// ==============================================

unsigned long lastLoggingTime = 0;
unsigned long lastSensorTime = 0;
unsigned long systemStartTime = 0;

// State Machine für non-blocking Sensor-Initialisierung
enum SensorInitState {
  SENSOR_INIT_IDLE,
  SENSOR_INIT_DHT_WARMING,
  SENSOR_INIT_GAS_WARMING,
  SENSOR_INIT_COMPLETE
};
static SensorInitState sensorInitState = SENSOR_INIT_IDLE;
static unsigned long sensorInitTimer = 0;

// ==============================================
// SYSTEM-INITIALISIERUNG
// ==============================================

void setup() {
  // Serielle Kommunikation starten
  Serial.begin(SERIAL_BAUD);
  delay(2000);  // Zeit für Serial Monitor
  
  systemStartTime = millis();
  
  // System-Info ausgeben
  printSystemInfo();
  
  // Module initialisieren
  initializeSystem();
  
  DEBUG_PRINTLN(F("=== SYSTEM BEREIT ==="));
  DEBUG_PRINTLN(F("Starte Datensammlung..."));
}

void initializeSystem() {
  initTDSSensor();
  bool systemOK = true;
  
  DEBUG_PRINTLN(F("Initialisiere System..."));
  
  // 1. RTC initialisieren (wichtig für Zeitstempel)
  if (!initRTC()) {
    reportError(ERROR_RTC, "RTC Initialisierung fehlgeschlagen");
    systemOK = false;
  }
  
  // 2. SD-Karte initialisieren
  if (!initSDCard()) {
    reportError(ERROR_SD_CARD, "SD-Karte Initialisierung fehlgeschlagen");
    systemOK = false;
  }
  

  
  // 4. Sensoren initialisieren (non-blocking)
  // Starte DHT11 Aufwärmphase
  pinMode(DHT_SENSOR_PIN, INPUT);  // DHT11 Pin setzen
  sensorInitState = SENSOR_INIT_DHT_WARMING;
  sensorInitTimer = millis();
  DEBUG_PRINTLN(F("DHT11 Aufwärmphase gestartet (2s)..."));
  
  // Gas-Sensoren und Radioaktivitätssensor können sofort initialisiert werden
  initRadiationSensor();
  DEBUG_PRINTLN(F("Gas-Sensoren Aufwärmphase wird nach DHT11 gestartet..."));
  
  // 5. OLED Display initialisieren
  if (!initDisplay()) {
    reportError(ERROR_SYSTEM, "OLED Display Initialisierung fehlgeschlagen");
  }
  
  // 6. Log-Datei erstellen
  if (isSDCardAvailable()) {
    char filename[MAX_FILENAME_LEN];
    if (!createLogFile(filename, sizeof(filename))) {
      reportError(ERROR_SD_CARD, "Log-Datei konnte nicht erstellt werden");
      systemOK = false;
    }
  }
  // System-Status und Sensor-Test
  if (systemOK) {
    DEBUG_PRINTLN(F("System erfolgreich initialisiert."));
    // Kompletter Sensor-Test beim Start
    testAllSensors();
  } else {
    DEBUG_PRINTLN(F("WARNUNG: Einige Komponenten konnten nicht initialisiert werden!"));
    // Reduzierter Test bei Fehlern
    testTemperatureSensors();
  }
  
  printMemoryUsage();
}

// ==============================================
// HAUPTPROGRAMM-SCHLEIFE
// ==============================================

void loop() {

  if (Serial1.available() > 0) {
    DEBUG_PRINT(F("[DEBUG] Serial1.available(): "));
    DEBUG_PRINTLN(Serial1.available());
  }


  // HOCHFREQUENZ: Radioaktivitäts-Sensor läuft jetzt interruptbasiert

  // OLED Display aktualisieren (alle 2 Sekunden)
  updateDisplay();

  // Hauptdaten-Logging (alle 2 Sekunden gemäß LOGGING_INTERVAL)
  if (isTimeElapsed(&lastLoggingTime, LOGGING_INTERVAL)) {
    performDataLogging();
  }

  // Gas-Sensoren periodisch lesen (alle 2s)
  if (isTimeElapsed(&lastSensorTime, SENSOR_INTERVAL)) {
    performSensorReadings();
  }

  // Non-blocking Sensor-Initialisierung
  updateSensorInitialization();

  // System-Check alle 30 Sekunden
  static unsigned long lastSystemCheck = 0;
  if (isTimeElapsed(&lastSystemCheck, 30000)) {
    systemCheck();
  }
  
  // MINIMAL-DELAY für Radiation-Sensor Responsivität
  // Optimiert für ~1000 Hz Abtastrate (alle 1ms) für bessere Radioaktivitäts-Erfassung
  delay(1);
}

// ==============================================
// DATENSAMMLUNG UND PROTOKOLLIERUNG
// ==============================================

void performDataLogging() {
  // Aktuelle Zeit abrufen mit Stabiltiäts-Check
  RTCData currentTime;
  static RTCData lastTime = {0};
  
  if (!readRTCData(&currentTime)) {
    DEBUG_PRINTLN(F("WARNUNG: RTC-Zeit nicht verfügbar!"));
    return;
  }
  
  // RTC-Sprung-Detektion
  if (lastTime.year > 0) {
    long timeDiff = abs((long)currentTime.timestamp - (long)lastTime.timestamp);
    if (timeDiff > 10) {  // Mehr als 10 Sekunden Sprung?
      DEBUG_PRINT(F("WARNUNG: RTC-Zeitsprung erkannt! Diff: "));
      DEBUG_PRINTLN(timeDiff);
    }
  }
  lastTime = currentTime;
  
  // Debug: Logging-Zeitpunkt anzeigen
  //DEBUG_PRINT(F("Logging: "));
  //DEBUG_PRINT(currentTime.hour);
  //DEBUG_PRINT(F(":"));
  //DEBUG_PRINT(currentTime.minute);
  //DEBUG_PRINT(F(":"));
  //DEBUG_PRINTLN(currentTime.second);
  
 
  // float celsius = 0.0, fahrenheit = 0.0;
  // if (!readTemperature(&celsius, &fahrenheit)) {

  //   celsius = 0.0;
  //   fahrenheit = 0.0;
  // }
  
  // DHT11 lesen (Temperatur + Luftfeuchtigkeit)
  float dht_temperature, humidity;
  if (!readDHTSensor(&dht_temperature, &humidity)) {
    DEBUG_PRINTLN(F("WARNUNG: DHT11 Sensor nicht verfügbar!"));
    dht_temperature = 0.0;
    humidity = 0.0;
  }
  
  // Daten protokollieren mit Retry-Mechanismus (non-blocking)
  float tdsValue = readTDSSensor();
  if (isSDCardAvailable()) {
    bool logSuccess = false;
    for (int retry = 0; retry < 3 && !logSuccess; retry++) {
      if (retry > 0) {
        DEBUG_PRINT(F("Log-Retry: "));
        DEBUG_PRINTLN(retry);
        // Kein delay - sofortiger Retry
      }
      logSuccess = logSensorData(dht_temperature, humidity, &currentTime, tdsValue);
      if (!logSuccess) {
        DEBUG_PRINT(F("FEHLER: Datenprotokollierung fehlgeschlagen! Versuch: "));
        DEBUG_PRINTLN(retry + 1);
      }
    }
    if (!logSuccess) {
      DEBUG_PRINTLN(F("KRITISCH: Alle Log-Versuche fehlgeschlagen!"));
    }
  } else {
    // Fallback: Nur Serial-Ausgabe
    printDataToSerial(dht_temperature, humidity, &currentTime, tdsValue);
  }
}

void performSensorReadings() {

  // float celsius, fahrenheit;
  // if (readTemperature(&celsius, &fahrenheit)) {
  //   printTemperature(celsius, fahrenheit);
  // }
  
  // DHT11 lesen und ausgeben
  float dht_temp, humidity;
  if (readDHTSensor(&dht_temp, &humidity)) {
    printDHTValues(dht_temp, humidity);
  }

  // Lichtsensor lesen und ausgeben
  int lightLevel = readLightSensor();
  float lightPercent = getLightPercent();
  printLightLevel(lightLevel, lightPercent);

  // Gas-Sensoren lesen und ausgeben
  int gasSensors[9];
  readAllGasSensors(gasSensors);

#if DEBUG_ENABLED
  // Detaillierte Sensor-Ausgabe nur bei aktiviertem Debug
  printGasSensorValues(gasSensors);
#endif

  // Mikrofone lesen
  int microphones[2];
  readAllMicrophones(microphones);

  // TDS-Sensor lesen
  float tdsValue = readTDSSensor();

  // Radioaktivität: Wert aus neuer Logik holen
  int radiationCPS = getRadiationCountAndReset();

  // KOMPAKTE ÜBERSICHTS-AUSGABE für bessere Lesbarkeit (jetzt mit TDS)
  printCompactStatus(dht_temp, humidity, lightLevel, radiationCPS, gasSensors, microphones, tdsValue);

  // Detaillierte Radioaktivitäts-Statistik entfällt (alte Funktion entfernt)
}

void printDataToSerial(float dht_temp, float humidity, const RTCData* rtc, float tdsValue) {
  // Kompakte Serial-Ausgabe
  DEBUG_PRINT(F("Daten: "));
  
  // Timestamp
  char timestamp[32];
  unsigned long ms = millis() % 1000;
  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d-%02d-%02d-%02d-%03lu",
           rtc->year, rtc->month, rtc->day, 
           rtc->hour, rtc->minute, rtc->second, ms);
  DEBUG_PRINT(timestamp);
  
  // DHT11 Temperatur 
  DEBUG_PRINT(F(" ; DHT11: "));
  Serial.print(dht_temp, 1);
  DEBUG_PRINT(F("°C "));
  Serial.print(humidity, 1);
  DEBUG_PRINT(F("% ; "));
  DEBUG_PRINT(F("TDS: "));
  Serial.print(tdsValue, 0);
  DEBUG_PRINT(F("ppm ; "));
  
  
  // Zeit
  DEBUG_PRINT(F(" ; "));
  if (rtc->hour < 10) DEBUG_PRINT('0');
  DEBUG_PRINT(rtc->hour);
  DEBUG_PRINT(':');
  if (rtc->minute < 10) DEBUG_PRINT('0');
  DEBUG_PRINT(rtc->minute);
  DEBUG_PRINT(':');
  if (rtc->second < 10) DEBUG_PRINT('0');
  DEBUG_PRINTLN(rtc->second);
}

void printCompactStatus(float temp, float hum, int light, int rad, int* gas, int* mic, float tdsValue) {
  //DEBUG_PRINTLN(F("========== SENSOR STATUS =========="));
  
  // Zeile 1: Umweltdaten
  //DEBUG_PRINT(F("UMWELT: "));
  //Serial.print(temp, 1);
  //DEBUG_PRINT(F("°C "));
  //Serial.print(hum, 1);
  //DEBUG_PRINT(F("% | Licht: "));
  //DEBUG_PRINT(light);
  //DEBUG_PRINT(F(" | RAD: "));
  //DEBUG_PRINT(rad);
  //DEBUG_PRINT(F(" CPS | TDS: "));
  //Serial.print(tdsValue, 0);
  //DEBUG_PRINTLN(F(" ppm"));
  
  
  // Zeile 3: Gas-Sensoren kompakt
  //DEBUG_PRINT(F("GAS: MQ2:"));
  //DEBUG_PRINT(gas[0]);
  //DEBUG_PRINT(F(" MQ7:"));
  //DEBUG_PRINT(gas[5]);
  //DEBUG_PRINT(F(" MQ135:"));
  //DEBUG_PRINT(gas[8]);
  //DEBUG_PRINT(F(" | MIC: "));
  //DEBUG_PRINT(mic[0]);
  //DEBUG_PRINT(F("/"));
  //DEBUG_PRINTLN(mic[1]);
  
  //DEBUG_PRINTLN(F("=================================="));
}

// ==============================================
// NON-BLOCKING SENSOR-INITIALISIERUNG
// ==============================================

void updateSensorInitialization() {
  switch (sensorInitState) {
    case SENSOR_INIT_DHT_WARMING:
      if (millis() - sensorInitTimer >= 2000) {
        DEBUG_PRINTLN(F("DHT11 Aufwärmphase abgeschlossen"));
        // DHT11 korrekt initialisieren
        if (initDHTSensor()) {
          DEBUG_PRINTLN(F("DHT11 erfolgreich initialisiert"));
        } else {
          DEBUG_PRINTLN(F("WARNUNG: DHT11 Initialisierung fehlgeschlagen"));
        }
        
        // Starte Gas-Sensoren Aufwärmphase
        sensorInitState = SENSOR_INIT_GAS_WARMING;
        sensorInitTimer = millis();
        DEBUG_PRINTLN(F("Gas-Sensoren Aufwärmphase gestartet (10s)..."));
      }
      break;
      
    case SENSOR_INIT_GAS_WARMING:
      if (millis() - sensorInitTimer >= 10000) {
        DEBUG_PRINTLN(F("Gas-Sensoren Aufwärmphase abgeschlossen"));
        // Gas-Sensoren initialisieren
        initGasSensors();
        
        sensorInitState = SENSOR_INIT_COMPLETE;
        DEBUG_PRINTLN(F("*** ALLE SENSOREN BEREIT ***"));
      }
      break;
      
    case SENSOR_INIT_COMPLETE:
      // Alle Sensoren sind bereit - nichts zu tun
      break;
      
    default:
      break;
  }
}

bool areSensorsReady() {
  return (sensorInitState == SENSOR_INIT_COMPLETE);
}

// ==============================================
// SYSTEM-VERWALTUNG
// ==============================================

unsigned long getSystemUptime() {
  return millis() - systemStartTime;
}

void printSystemStatus() {
  DEBUG_PRINTLN(F("=== SYSTEM STATUS ==="));
  DEBUG_PRINT(F("Uptime: "));
  DEBUG_PRINT(getSystemUptime() / 1000);
  DEBUG_PRINTLN(F(" Sekunden"));
  
  
  DEBUG_PRINT(F("RTC: "));
  DEBUG_PRINTLN(isRTCRunning() ? F("OK") : F("FEHLER"));
  
  DEBUG_PRINT(F("SD: "));
  DEBUG_PRINTLN(isSDCardAvailable() ? F("OK") : F("FEHLER"));
  
  printMemoryUsage();
  DEBUG_PRINTLN(F("===================="));
}
