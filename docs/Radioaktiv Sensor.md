---
# Geigerzähler / Radioaktivitätssensor
---

## Problemstellung

Bei der Nutzung eines Radioaktivitätssensors (z. B. Geiger-Müller-Röhre) am Arduino Mega 2560 trat folgendes Problem auf:

- Der Sensor liefert kurze Spannungspulse, wenn ein radioaktives Teilchen detektiert wird.
- Erste Implementierung mit `digitalRead()` und `INPUT_PULLUP` oder Polling: Nicht alle Impulse wurden zuverlässig gezählt, manche doppelt.
- Die Zählung sollte alle 5 Sekunden gesammelt und ausgegeben werden (für spätere Speicherung).

### Hauptursachen

- **Falsche Pin-Konfiguration:** `INPUT_PULLUP` hält den Pin dauerhaft auf HIGH, kurze LOW-Impulse werden teilweise nicht erkannt.
- **Polling-Loop zu langsam:** Wenn der Impuls kürzer ist als die Loop-Zeit, wird er übersehen.
- **Flanken-Doppeltzählung:** Jeder Impuls erzeugt zwei Flanken (HIGH→LOW und LOW→HIGH), manche Implementierungen zählen doppelt.

---

## Lösung

---

Die Probleme lassen sich wie folgt lösen:

1. **Interrupt-fähigen Pin verwenden** (z. B. Digitalpin 2, 3, 18, 19, 20, 21 am Arduino Mega 2560).
2. **Hardware-Interrupt mit `attachInterrupt()`** auf jede Flanke (`CHANGE`) setzen, um jeden Impuls sofort zu erfassen.
3. **Entprellung im Interrupt** (z. B. 2 ms), um Doppelzählungen durch Flatterimpulse zu vermeiden.
4. **5-Sekunden-Zähler** implementieren, der die Impulse sammelt und anschließend ausgibt (oder abspeichert).

---

## Falscher Ansatz (Polling, Pin 29)

---

```cpp
// Falscher Radioaktiv Sensor Tick-Zähler
// Problem: beide Flanken werden gemessen, funktioniert aber nicht zuverlässig!
#define RADIATION_INPUT_PIN 29
volatile int tickCount = 0;
bool lastState = HIGH;
unsigned long lastEventTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(RADIATION_INPUT_PIN, INPUT);
  lastState = digitalRead(RADIATION_INPUT_PIN);
  Serial.println("Starte Tick-Prüfung an Pin 29...");
}

void loop() {
  bool currentState = digitalRead(RADIATION_INPUT_PIN);
  unsigned long now = millis();

  // Flanke erkennen und entprellen (2ms)
  if (currentState != lastState && (now - lastEventTime > 2)) {
    tickCount++;
    lastEventTime = now;
    Serial.print("Tick #");
    Serial.print(tickCount);
    Serial.print(" Flanke: ");
    Serial.println(currentState ? "HIGH" : "LOW");
  }
  lastState = currentState;

  // Optional: Tick-Zähler jede Sekunde ausgeben
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 1000) {
    Serial.print("Ticks/Sekunde: ");
    Serial.println(tickCount);
    tickCount = 0;
    lastPrint = now;
  }
}
```

---

## Richtiger Ansatz (Interrupt, Pin 19)

---

```cpp
#define RADIATION_INPUT_PIN 19
// Möglich sind auch: 2, 3, 18, 19, 20, 21
volatile unsigned long tickCount = 0;  // Zähler, wird im Interrupt erhöht
unsigned long lastEventTime = 0;       // Für Entprellung (in µs)
unsigned long lastPrint = 0;           // Zeitstempel der letzten Ausgabe

void setup() {
  Serial.begin(9600);
  pinMode(RADIATION_INPUT_PIN, INPUT);
  // Interrupt auf jede Flanke (steigend und fallend)
  attachInterrupt(digitalPinToInterrupt(RADIATION_INPUT_PIN), countTick, CHANGE);
  Serial.println("Starte 5-Sekunden-Tickzählung an Pin 19...");
}

void loop() {
  unsigned long now = millis();
  // Alle 5 Sekunden den Zähler ausgeben
  if (now - lastPrint >= 5000) {
    noInterrupts();                // kurzzeitig stoppen, um sicheren Zugriff zu haben
    unsigned long countCopy = tickCount;
    tickCount = 0;
    interrupts();
    Serial.print("Impulse in den letzten 5 Sekunden: ");
    Serial.println(countCopy);
    lastPrint = now;
  }
}

// Wird bei jeder Signalflanke aufgerufen
void countTick() {
  unsigned long now = micros();
  if (now - lastEventTime > 2000) {  // Entprellung: 2 ms
    tickCount++;
    lastEventTime = now;
  }
}
```

---

## Zusammenfassung

---

Mit dieser Lösung werden alle Impulse zuverlässig erkannt, Doppeltzählungen vermieden und die Zählergebnisse in festen Zeitintervallen ausgegeben. So ist eine einfache Speicherung oder Weiterverarbeitung möglich.
