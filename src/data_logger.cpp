/*
 * Implementierung des Data-Logger-Moduls
 */

#include "data_logger.h"
#include <Arduino.h>

// ==============================================
// GLOBALE VARIABLEN
// ==============================================

char globalLogFilename[FILENAME_LENGTH];
bool sdCardInitialized = false;

// ==============================================
// SD-KARTE TIMESTAMP CALLBACK
// ==============================================

// Callback-Funktion für korrekte Datei-Timestamps
void dateTime(uint16_t* date, uint16_t* time) {
  RTCData currentTime;
  readRTCData(&currentTime);
  
  // FAT Datei-Timestamp Format (MS-DOS kompatibel)
  // Datum: Bits 15-9 = Jahr-1980, Bits 8-5 = Monat, Bits 4-0 = Tag
  *date = ((currentTime.year - 1980) << 9) | (currentTime.month << 5) | currentTime.day;
  
  // Zeit: Bits 15-11 = Stunde, Bits 10-5 = Minute, Bits 4-0 = Sekunde/2
  *time = (currentTime.hour << 11) | (currentTime.minute << 5) | (currentTime.second >> 1);
}

// ==============================================
// SD-KARTE INITIALISIERUNG
// ==============================================

bool initSDCard() {
  DEBUG_PRINT(F("Initialisiere SD-Karte..."));
  
  if (!SD.begin(SD_CHIP_SELECT)) {
    DEBUG_PRINTLN(F("FEHLER: SD-Karte nicht gefunden oder defekt!"));
    sdCardInitialized = false;
    return false;
  }
  
  // Timestamp-Callback für korrekte Datei-Zeiten setzen
  SdFile::dateTimeCallback(dateTime);
  
  DEBUG_PRINTLN(F("OK"));
  sdCardInitialized = true;
  delay(SD_INIT_DELAY);
  
  printSDCardInfo();
  return true;
}

bool checkSDCard() {
  return sdCardInitialized && SD.begin(SD_CHIP_SELECT);
}

void printSDCardInfo() {
  // Grundlegende SD-Karten Informationen ausgeben
  DEBUG_PRINTLN(F("=== SD-Karte Info ==="));
  DEBUG_PRINT(F("Karte erkannt: "));
  DEBUG_PRINTLN(sdCardInitialized ? F("JA") : F("NEIN"));
}

// ==============================================
// DATEI-MANAGEMENT
// ==============================================

bool createLogFile(char* filename, uint8_t filenameSize) {
  if (!sdCardInitialized) {
    DEBUG_PRINTLN(F("FEHLER: SD-Karte nicht initialisiert!"));
    return false;
  }
  
  // Dateiname mit Zeitstempel generieren
  generateFilename(filename, filenameSize);
  
  // Vorhandene Datei löschen (für neuen Start)
  if (SD.exists(filename)) {
    SD.remove(filename);
    DEBUG_PRINT(F("Alte Datei gelöscht: "));
    DEBUG_PRINTLN(filename);
  }
  
  // Neue Datei erstellen und Header schreiben
  File logFile = SD.open(filename, FILE_WRITE);
  if (!logFile) {
    DEBUG_PRINT(F("FEHLER: Kann Datei nicht erstellen: "));
    DEBUG_PRINTLN(filename);
    return false;
  }
  
  // Header schreiben
  RTCData currentTime;
  readRTCData(&currentTime);
  
  logFile.print(F("# Umweltkontrollsystem Log\n"));
  logFile.print(F("# Start: "));
  logFile.print(currentTime.year);
  logFile.print('-');
  if (currentTime.month < 10) logFile.print('0');
  logFile.print(currentTime.month);
  logFile.print('-');
  if (currentTime.day < 10) logFile.print('0');
  logFile.print(currentTime.day);
  logFile.print(' ');
  if (currentTime.hour < 10) logFile.print('0');
  logFile.print(currentTime.hour);
  logFile.print(':');
  if (currentTime.minute < 10) logFile.print('0');
  logFile.print(currentTime.minute);
  logFile.print(':');
  if (currentTime.second < 10) logFile.print('0');
  logFile.println(currentTime.second);
  
  
  // CSV Header mit Komma-Trennung
  logFile.println(F("DateTime,SecSinceMidnight-MS,Temperature_DHT_C,Humidity_RH,Light_Level,Light_Percent,MQ2,MQ3,MQ4,MQ5,MQ6,MQ7,MQ8,MQ9,MQ135,Mic1,Mic2,TDS,Radiation_CPS"));
  
  logFile.close();
  
  // Globalen Dateinamen setzen
  strncpy(globalLogFilename, filename, FILENAME_LENGTH - 1);
  globalLogFilename[FILENAME_LENGTH - 1] = '\0';
  
  DEBUG_PRINT(F("Log-Datei erstellt: "));
  DEBUG_PRINTLN(filename);
  
  delay(SD_INIT_DELAY);
  return true;
}

void generateFilename(char* filename, uint8_t filenameSize) {
  RTCData currentTime;
  readRTCData(&currentTime);
  
  // 8.3 Format: MMDDhhmm.CSV (Monat, Tag, Stunde, Minute - 8 Zeichen, MESZ Zeit)
  snprintf(filename, filenameSize, "%02d%02d%02d%02d.CSV", 
           currentTime.month, currentTime.day, currentTime.hour, currentTime.minute);
}

// ==============================================
// DATENPROTOKOLLIERUNG
// ==============================================

bool logSensorData(float temperature_dht, float humidity, const RTCData* rtc) {
  if (!sdCardInitialized || strlen(globalLogFilename) == 0) {
    DEBUG_PRINTLN(F("FEHLER: Kein Log-File!"));
    return false;
  }
    // Mikrofone lesen (lokal begrenzt)  
    int microphones[MAX_MICROPHONES];
    readAllMicrophones(microphones);

    // TDS-Sensor auslesen
    float tdsValue = readTDSSensor();

    // Radioaktivität (Klicks pro Sekunde für CSV)
    int radiationCount = getRadiationClicksPerSecond();

    // Kompakte CSV-Zeile in einem Zug schreiben 
    String csvLine = "";

    // 1️⃣ DateTime an den Anfang
    if (rtc->year > 2000) {
      char dateTimeStr[40];
      formatLocalDateTime(rtc, dateTimeStr, sizeof(dateTimeStr));
      csvLine += dateTimeStr;
    } else {
      csvLine += "----/--/-- --:--:-- MEZ";
    }
    csvLine += ",";

    // 2️⃣ Restliche Felder
    csvLine += timestamp; csvLine += ",";
    csvLine += String(temperature_dht, 1); csvLine += ",";
    csvLine += String(humidity, 1); csvLine += ",";

    int lightLevel = readLightSensor();
    float lightPercent = getLightPercent();
    csvLine += String(lightLevel); csvLine += ",";
    csvLine += String(lightPercent, 1); csvLine += ",";

    // 3️⃣ Gassensoren
    for (uint8_t i = 0; i < MAX_GAS_SENSORS; i++) {
      csvLine += String(gasSensors[i]);
      csvLine += ",";
    }

    // 4️⃣ Mikrofone
    csvLine += String(microphones[0]); csvLine += ",";
    csvLine += String(microphones[1]); csvLine += ",";

    // 5️⃣ TDS-Sensor
    csvLine += String(tdsValue, 1); csvLine += ",";

    // 6️⃣ Radioaktivität
    csvLine += String(radiationCount);

    // 7️⃣ Ausgabe
    Serial.println(csvLine);
    logFile.println(csvLine);

    // WICHTIG: Sofort synchronisieren und schließen
    logFile.flush();
    logFile.close();

    // Serial-Ausgabe: Exakt das gleiche wie auf SD-Karte geschrieben
    //DEBUG_PRINT(F("JSON: "));
    //DEBUG_PRINTLN(csvLine);  // Zeigt die komplette CSV-Zeile an

    return true;

  // 3️⃣ Gassensoren
  for (uint8_t i = 0; i < MAX_GAS_SENSORS; i++) {
    csvLine += String(gasSensors[i]);
    csvLine += ",";
  }

  // 4️⃣ Mikrofone
  csvLine += String(microphones[0]); csvLine += ",";
  csvLine += String(microphones[1]); csvLine += ",";

  // 5️⃣ TDS-Sensor
  float tdsValue = readTDSSensor();
  csvLine += String(tdsValue, 1); csvLine += ",";

  // 6️⃣ Radioaktivität
  csvLine += String(radiationCount);

  // 7️⃣ Ausgabe
  Serial.println(csvLine);
  logFile.println(csvLine);
  
  // WICHTIG: Sofort synchronisieren und schließen
  logFile.flush();
  logFile.close();
  
  // Serial-Ausgabe: Exakt das gleiche wie auf SD-Karte geschrieben
  //DEBUG_PRINT(F("JSON: "));
 // DEBUG_PRINTLN(csvLine);  // Zeigt die komplette CSV-Zeile an

  return true;
}

// ==============================================
// HILFSFUNKTIONEN
// ==============================================

bool isSDCardAvailable() {
  return sdCardInitialized;
}
