# http://www.cqrobot.wiki/index.php/TDS_(Total_Dissolved_Solids)_Meter_Sensor_SKU:_CQRSENTDS01

möchte einen TDS sensor auf pin A12 integrieren hier der Beispielcode.
wo machen wir das hin?
wo bauen wir das ein und wie?
Der TDS-Wert wird mit in csvLine in jeder CSV-Zeile auf der SD-Karte gespeichert.
Der CSV-Header wurde um das TDS-Feld erweitert.

Um den TDS-Sensor sauber und modular in dein Projekt zu integrieren, folge diesen Schritten:
beachte auch: Loggen (Speichern in der CSV-Datei auf SD-Karte)

1. Pin-Definition in config.h
   Füge die Pin-Definition hinzu ( auf A12, damit keine Kollision mit MQ3 auf A1 entsteht):

const uint8_t TDS_SENSOR_PIN = A12; // TDS Sensor (Wasserqualität)

2. Funktionen in sensors.h deklarieren

void initTDSSensor();
float readTDSSensor(float temperature = 25.0);

3. Implementierung in sensors.cpp
   Füge die TDS-Logik am Ende von sensors.cpp hinzu:

#define TDS_VREF 5.0
#define TDS_SCOUNT 30

static int tdsAnalogBuffer[TDS_SCOUNT];
static int tdsAnalogBufferTemp[TDS_SCOUNT];
static int tdsAnalogBufferIndex = 0;

void initTDSSensor() {
pinMode(TDS_SENSOR_PIN, INPUT);
tdsAnalogBufferIndex = 0;
}

float getMedianNum(int bArray[], int iFilterLen) {
int bTab[iFilterLen];
for (byte i = 0; i < iFilterLen; i++)
bTab[i] = bArray[i];
int i, j, bTemp;
for (j = 0; j < iFilterLen - 1; j++) {
for (i = 0; i < iFilterLen - j - 1; i++) {
if (bTab[i] > bTab[i + 1]) {
bTemp = bTab[i];
bTab[i] = bTab[i + 1];
bTab[i + 1] = bTemp;
}
}
}
if ((iFilterLen & 1) > 0)
bTemp = bTab[(iFilterLen - 1) / 2];
else
bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
return bTemp;
}

float readTDSSensor(float temperature) {
// Non-blocking: alle 40ms einen Wert einlesen
static unsigned long lastSampleTime = 0;
if (millis() - lastSampleTime > 40U) {
lastSampleTime = millis();
tdsAnalogBuffer[tdsAnalogBufferIndex] = analogRead(TDS_SENSOR_PIN);
tdsAnalogBufferIndex++;
if (tdsAnalogBufferIndex == TDS_SCOUNT)
tdsAnalogBufferIndex = 0;
}

// Median alle 800ms berechnen
static unsigned long lastCalcTime = 0;
static float tdsValue = 0;
if (millis() - lastCalcTime > 800U) {
lastCalcTime = millis();
for (int i = 0; i < TDS_SCOUNT; i++)
tdsAnalogBufferTemp[i] = tdsAnalogBuffer[i];
float averageVoltage = getMedianNum(tdsAnalogBufferTemp, TDS_SCOUNT) _ (float)TDS_VREF / 1024.0;
float compensationCoefficient = 1.0 + 0.02 _ (temperature - 25.0);
float compensationVoltage = averageVoltage / compensationCoefficient;
tdsValue = (133.42 _ compensationVoltage _ compensationVoltage _ compensationVoltage - 255.86 _ compensationVoltage _ compensationVoltage + 857.39 _ compensationVoltage) \* 0.5;
}
return tdsValue;
}

4. Initialisierung im Hauptprogramm
   Füge in initializeSystem() in EnvironmentalMonitor.ino die Initialisierung hinzu:

initTDSSensor();

5. Auslesen und Logging
   Füge das Auslesen in performSensorReadings() und/oder performDataLogging() ein:

float tdsValue = readTDSSensor(); // Temperatur optional übergeben
;

---

Und im Logging (data_logger.cpp) kannst du den Wert wie die anderen Sensoren in die CSV-Zeile aufnehmen.

Zusammengefasst:

Pin in config.h definieren
Funktionen in sensors.h/.cpp implementieren
Initialisierung im Hauptprogramm
Wert regelmäßig auslesen und loggen
So ist der TDS-Sensor sauber und modular integriert!

So loggst du den TDS-Wert korrekt:
Im Logging-Modul (data_logger.cpp):

csvLine += String(tdsValue, 1); csvLine += ",";

Rufe float tdsValue = readTDSSensor(); im Logging-Bereich auf.
Ergänze die CSV-Zeile um den TDS-Wert:
Ergänze den CSV-Header:

DateTime,SecSinceMidnight-MS,Temperature_DHT_C,Humidity_RH,Light_Level,Light_Percent,MQ2,MQ3,MQ4,MQ5,MQ6,MQ7,MQ8,MQ9,MQ135,Mic1,Mic2,TDS,Radiation_CPS

Reihenfolge:

Der TDS-Wert sollte nach den Mikrofonwerten und vor der Radioaktivität stehen.
Fazit:

Das Auslesen ist korrekt und non-blocking integriert.
Das Logging fehlt noch – der Wert wird nicht in der SD-Logdatei gespeichert.
Ich kann das Logging für dich direkt ergänzen, damit der TDS-Wert in jeder CSV-Zeile erscheint. Sag Bescheid, wenn du das möchtest!2. Reihenfolge:
Der TDS-Wert sollte nach den Mikrofonwerten und vor der Radioaktivität stehen.
