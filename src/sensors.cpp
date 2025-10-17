// Dummy-Initialisierung für TDS-Sensor (wird im System erwartet)
void initTDSSensor() {
  // Für diesen Sensor ist keine Initialisierung nötig
}


#include "sensors.h"
#include <Arduino.h>

// DHT Library macht eigene DEBUG Macros - wir deaktivieren sie temporär
#ifdef DEBUG_PRINT
#undef DEBUG_PRINT
#undef DEBUG_PRINTLN
#endif

#include <DHT.h>

// Unsere DEBUG Macros wieder aktivieren
#include "config.h"
#define TdsSensorPin A12            // Pin, an dem der TDS-Sensor angeschlossen ist
#define VREF 5.0                    // Referenzspannung des ADC (in Volt)
#define SCOUNT  30                  // Anzahl der Messwerte für Mittelwertbildung

int analogBuffer[SCOUNT];           // Puffer für die ADC-Messwerte
int analogBufferIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 18; // Temperatur für Kompensation


// ==============================================
// GLOBALE VARIABLEN
// ==============================================

// OneWire temperatureSensor(TEMP_SENSOR_PIN);  // DEAKTIVIERT
DHT dhtSensor(DHT_SENSOR_PIN, DHT11);

// ==============================================
// DHT11 TEMPERATUR & LUFTFEUCHTIGKEIT
// ==============================================

bool initDHTSensor() {
  DEBUG_PRINTLN(F("Initialisiere DHT11 Sensor..."));
  dhtSensor.begin();
  // KEIN delay(2000) mehr - non-blocking Aufwärmung in State Machine
  
  // Ersten Test-Wert lesen
  float testTemp = dhtSensor.readTemperature();
  float testHum = dhtSensor.readHumidity();
  
  if (isnan(testTemp) || isnan(testHum)) {
    DEBUG_PRINTLN(F("FEHLER: DHT11 antwortet nicht!"));
    DEBUG_PRINTLN(F("Prüfe: VCC->5V, GND->GND, DATA->Pin 22"));
    return false;
  }
  
  DEBUG_PRINT(F("DHT11 Test OK: "));
  DEBUG_PRINT(testTemp);
  DEBUG_PRINT(F("°C, "));
  DEBUG_PRINT(testHum);
  DEBUG_PRINTLN(F("%"));
  return true;
}

bool readDHTSensor(float* temperature, float* humidity) {
  // DHT11 lesen mit mehreren Versuchen
  for (int attempt = 0; attempt < 3; attempt++) {
    *humidity = dhtSensor.readHumidity();
    *temperature = dhtSensor.readTemperature();
    
    // Prüfen ob Messung gültig ist
    if (!isnan(*humidity) && !isnan(*temperature)) {
      // Zusätzliche Plausibilitätsprüfung
      if (*temperature > -40 && *temperature < 80 && *humidity >= 0 && *humidity <= 100) {
        return true;
      }
    }
    
    DEBUG_PRINT(F("DHT11 Versuch "));
    DEBUG_PRINT(attempt + 1);
    DEBUG_PRINTLN(F(" fehlgeschlagen, wiederhole..."));
    // KEIN delay(500) mehr - non-blocking!
  }
  
  DEBUG_PRINTLN(F("DHT11 komplett fehlgeschlagen!"));
  *temperature = 0.0;
  *humidity = 0.0;
  return false;
}

void printDHTValues(float temperature, float humidity) {
  DEBUG_PRINT(F("DHT11 - Temperatur: "));
  DEBUG_PRINT(temperature);
  DEBUG_PRINT(F("°C, Luftfeuchtigkeit: "));
  DEBUG_PRINT(humidity);
  DEBUG_PRINTLN(F("%"));
}

// ==============================================
// GAS-SENSOREN (MQ-SERIE)
// ==============================================

void initGasSensors() {
  DEBUG_PRINTLN(F("Initialisiere Gas-Sensoren..."));
  // Analoge Pins benötigen keine explizite Initialisierung
  warmupGasSensors();
}

void warmupGasSensors() {
  DEBUG_PRINTLN(F("Gas-Sensoren Aufwärmphase..."));
  // KEIN delay(GAS_SENSOR_WARMUP) mehr - non-blocking in State Machine
  DEBUG_PRINTLN(F("Gas-Sensoren bereit."));
}

int readGasSensor(uint8_t pin) {
  return analogRead(pin);
}

void readAllGasSensors(int* values) {
  values[0] = readGasSensor(MQ2_PIN);    // Methan, Butan, LPG
  values[1] = readGasSensor(MQ3_PIN);    // Alkohol
  values[2] = readGasSensor(MQ4_PIN);    // Methan, CNG
  values[3] = readGasSensor(MQ5_PIN);    // Natürliche Gase
  values[4] = readGasSensor(MQ6_PIN);    // LPG, Butan
  values[5] = readGasSensor(MQ7_PIN);    // Kohlenmonoxid
  values[6] = readGasSensor(MQ8_PIN);    // Wasserstoff
  values[7] = readGasSensor(MQ9_PIN);    // CO, entflammbare Gase
  values[8] = readGasSensor(MQ135_PIN);  // Luftqualität
}

void printGasSensorValues(int* values) {
#if DEBUG_ENABLED
  const char* sensorNames[] = {
    "MQ2(Methan)", "MQ3(Alkohol)", "MQ4(CNG)", "MQ5(LPG)", 
    "MQ6(Butan)", "MQ7(CO)", "MQ8(H2)", "MQ9(CO/Gas)", "MQ135(Luft)"
  };
  (void)sensorNames; // Verhindert unused variable warning bei Release builds
  
  DEBUG_PRINTLN(F("=== Gas-Sensoren ==="));
  for (uint8_t i = 0; i < 9; i++) {
    DEBUG_PRINT(sensorNames[i]);
    DEBUG_PRINT(F(": "));
    DEBUG_PRINTLN(values[i]);
  }
#else
  // Debug ist deaktiviert - keine Ausgabe
  (void)values; // Verhindert unused parameter warning
#endif
}

// ==============================================
// RADIOAKTIVITÄTS-SENSOR (INTERRUPT-BASIERT)
// ==============================================

#define RADIATION_INPUT_PIN 19  // Möglich: 2, 3, 18, 19, 20, 21

volatile unsigned long tickCount = 0;    // Zähler, wird im Interrupt erhöht
unsigned long lastEventTime = 0;         // Für Entprellung (in µs)
unsigned long lastRadiationPrint = 0;    // Zeitstempel der letzten Ausgabe

void countTick() {
  unsigned long now = micros();
  if (now - lastEventTime > 2000) {  // Entprellung: 2 ms
    tickCount++;
    lastEventTime = now;
  }
}

void initRadiationSensor() {
  pinMode(RADIATION_INPUT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RADIATION_INPUT_PIN), countTick, CHANGE);
}

unsigned long getRadiationCountAndReset() {
  noInterrupts();
  unsigned long countCopy = tickCount;
  tickCount = 0;
  interrupts();
  return countCopy;
}


// ==============================================
// MIKROFON-SENSOREN (OPTIMIERT FÜR SOUND-ERKENNUNG)
// ==============================================

int readMicrophone(uint8_t pin) {
  // MEHRFACH-SAMPLING für bessere Schall-Erkennung
  int maxValue = 0;
  int minValue = 1023;
  
  // 20 schnelle Messungen in 10ms
  for (int i = 0; i < 20; i++) {
    int value = analogRead(pin);
    if (value > maxValue) maxValue = value;
    if (value < minValue) minValue = value;
    delayMicroseconds(500); // 0.5ms zwischen Messungen
  }
  
  // Peak-to-Peak Wert zurückgeben (Amplitude)
  return maxValue - minValue;
}

void readAllMicrophones(int* micValues) {
  DEBUG_PRINTLN(F("=== MIKROFON-TESTS (OPTIMIERT) ==="));
  
  // Mehrere Messungen für Stabilität - OHNE delay()
  int mic1_readings[5], mic2_readings[5];
  
  for (int i = 0; i < 5; i++) {
    mic1_readings[i] = readMicrophone(MIC_KLEIN_PIN);
    mic2_readings[i] = readMicrophone(MIC_GROSS_PIN);
    // KEIN delay(20) mehr - non-blocking!
  }
  
  // Maximum der letzten 5 Messungen (für Schall-Peaks)
  micValues[0] = 0;
  micValues[1] = 0;
  
  for (int i = 0; i < 5; i++) {
    if (mic1_readings[i] > micValues[0]) micValues[0] = mic1_readings[i];
    if (mic2_readings[i] > micValues[1]) micValues[1] = mic2_readings[i];
  }
  
  DEBUG_PRINT(F("Mikrofon Klein (A"));
  DEBUG_PRINT(MIC_KLEIN_PIN - A0);
  DEBUG_PRINT(F("): Peak-Peak = "));
  DEBUG_PRINT(micValues[0]);
  DEBUG_PRINT(F(" (Messungen: "));
  for (int i = 0; i < 5; i++) {
    DEBUG_PRINT(mic1_readings[i]);
    if (i < 4) DEBUG_PRINT(F(","));
  }
  DEBUG_PRINTLN(F(")"));
  
  DEBUG_PRINT(F("Mikrofon Gross (A"));
  DEBUG_PRINT(MIC_GROSS_PIN - A0);
  DEBUG_PRINT(F("): Peak-Peak = "));
  DEBUG_PRINT(micValues[1]);
  DEBUG_PRINT(F(" (Messungen: "));
  for (int i = 0; i < 5; i++) {
    DEBUG_PRINT(mic2_readings[i]);
    if (i < 4) DEBUG_PRINT(F(","));
  }
  DEBUG_PRINTLN(F(")"));
  
  // Verbesserte Bewertung der Mikrofon-Pegel (Peak-to-Peak)
  for (int i = 0; i < 2; i++) {
    const char* micName = (i == 0) ? "Klein" : "Gross";
    DEBUG_PRINT(F("Mikrofon "));
    DEBUG_PRINT(micName);
    DEBUG_PRINT(F(" (P2P): "));
    
    // Vermeide Compiler-Warnung bei ausgeschaltetem Debug
    (void)micName;
    
    if (micValues[i] < 2) {
      DEBUG_PRINTLN(F("SEHR LEISE/RAUSCHEN"));
    } else if (micValues[i] < 5) {
      DEBUG_PRINTLN(F("Leise"));
    } else if (micValues[i] < 15) {
      DEBUG_PRINTLN(F("Normal"));
    } else if (micValues[i] < 30) {
      DEBUG_PRINTLN(F("LAUT"));
    } else if (micValues[i] < 50) {
      DEBUG_PRINTLN(F("SEHR LAUT"));
    } else {
      DEBUG_PRINTLN(F("EXTREM LAUT"));
    }
  }
}

// ==============================================
// LICHTSENSOR (LDR)
// ==============================================

int readLightSensor() {
  return analogRead(LDR_PIN);
}

float getLightPercent() {
  int lightValue = readLightSensor();
  // INVERTIERTE Umrechnung für typische LDR-Schaltung:
  // Hell: niedriger analogRead() = hohe Lichtintensität
  // Dunkel: hoher analogRead() = niedrige Lichtintensität
  return ((1023 - lightValue) / 1023.0) * 100.0;
}

void printLightLevel(int lightValue, float lightPercent) {
  DEBUG_PRINT(F("Licht RAW: "));
  DEBUG_PRINT(lightValue);
  DEBUG_PRINT(F(" -> "));
  DEBUG_PRINT(lightPercent, 1);
  DEBUG_PRINT(F("% | "));
  
  // Zusätzliche Klassifizierung
  if (lightPercent > 80) {
    DEBUG_PRINTLN(F("SEHR HELL"));
  } else if (lightPercent > 60) {
    DEBUG_PRINTLN(F("Hell"));
  } else if (lightPercent > 40) {
    DEBUG_PRINTLN(F("Normal"));
  } else if (lightPercent > 20) {
    DEBUG_PRINTLN(F("Dämmrig"));
  } else {
    DEBUG_PRINTLN(F("DUNKEL"));
  }
}


// ==============================================
// TDS SENSOR (WASSERQUALITÄT)
// ==============================================

// Beispielhafte Implementierung für TDS-Sensor (z.B. analog an Pin A12)



// ==============================================
// TDS SENSOR (WASSERQUALITÄT) - Integration für Hauptsystem
// ==============================================

// Median-Berechnung für stabile Messwerte
int getMedianNum(int bArray[], int iFilterLen)
{
  int bTab[30];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++)
  {
    for (i = 0; i < iFilterLen - j - 1; i++)
    {
      if (bTab[i] > bTab[i + 1])
      {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  // Gibt den Medianwert zurück
  if ((iFilterLen & 1) > 0)
    bTemp = bTab[(iFilterLen - 1) / 2];
  else
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  return bTemp;
}

float readTDSSensor(float temperature) {
  // TDS-Buffer und Index als static, damit sie zwischen Aufrufen erhalten bleiben
  static int analogBuffer[30];
  static int analogBufferIndex = 0;
  static unsigned long lastSampleTime = 0;
  static float lastTDSValue = 0;
  static unsigned long lastCalcTime = 0;

  // Sample alle 33ms
  if (millis() - lastSampleTime >= 33) {
    lastSampleTime = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex >= 30) analogBufferIndex = 0;
  }

  // Nur alle 1s Median berechnen und Wert merken
  if (millis() - lastCalcTime >= 1000) {
    lastCalcTime = millis();
    int analogBufferTemp[30];
    for (int i = 0; i < 30; i++) analogBufferTemp[i] = analogBuffer[i];
    float averageVoltage = getMedianNum(analogBufferTemp, 30) * (float)VREF / 1024.0;
    float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
    float compensationVolatge = averageVoltage / compensationCoefficient;
    lastTDSValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5;
  }
  return lastTDSValue;
}

// ==============================================
// SENSOR-DIAGNOSE
// ==============================================

void testTemperatureSensors() {
  DEBUG_PRINTLN(F("=== TEMPERATURSENSOR-TEST ==="));
  

  
  // DHT11 Test
  float dht_temp, humidity;
  DEBUG_PRINT(F("DHT11: "));
  if (readDHTSensor(&dht_temp, &humidity)) {
    DEBUG_PRINT(dht_temp);
    DEBUG_PRINT(F("°C, "));
    DEBUG_PRINT(humidity);
    DEBUG_PRINTLN(F("% - OK"));
  } else {
    DEBUG_PRINTLN(F("FEHLER: Verkabelung prüfen!"));
  }
}

void testAllSensors() {
  DEBUG_PRINTLN(F("========= SENSOR-DIAGNOSE ========="));
  
  testTemperatureSensors();
  
  // Lichtsensor Test
  DEBUG_PRINT(F("LDR: "));
  DEBUG_PRINT(readLightSensor());
  DEBUG_PRINT(F(" ("));
  DEBUG_PRINT(getLightPercent(), 1);
  DEBUG_PRINTLN(F("%) - OK"));
  
  // Gas-Sensoren Test
  int gasSensors[9];
  readAllGasSensors(gasSensors);
  DEBUG_PRINT(F("Gas-Sensoren: "));
  for (int i = 0; i < 9; i++) {
    DEBUG_PRINT(gasSensors[i]);
    if (i < 8) DEBUG_PRINT(F(","));
  }
  DEBUG_PRINTLN(F(" - OK"));
  
  // Radioaktivität Test
  DEBUG_PRINT(F("Radioaktivität: "));
  DEBUG_PRINT(getRadiationCountAndReset());
  DEBUG_PRINTLN(F(" Impulse (seit letztem Test) - OK"));
  
  DEBUG_PRINTLN(F("================================="));
}
