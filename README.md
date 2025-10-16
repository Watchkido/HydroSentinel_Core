# 🌱 Hydro-Balkonpflanzen Automatisierung & Luft-/Wasserqualitätsüberwachung

**Smartes Monitoring & Pflege für Hydrokultur-Balkonpflanzen**

---

## 🚀 Projektüberblick

Ein IoT-System zur vollautomatischen Überwachung und Pflege von Balkonpflanzen in Hydrokultur:

- **Luftqualität:** 9x MQ-Gassensoren (CO, CO₂, NOx, LPG, etc.)
- **Wasserstand:** Kapazitiver Sensor für Nährlösung
- **Nährstoffmessung:** TDS-Sensor (ppm, Leitfähigkeit)
- **Lichtmessung:** LDR-Sensor (Helligkeit, Tageslicht)
- **Temperatur & Luftfeuchte:** DHT11/DHT22
- **Krankheitsüberwachung:** IR-Kamera (Pflanzenstress, Pilzbefall)
- **Audio:** Dual-MEMS-Mikrofone (Schall, Störungserkennung)
- **Radioaktivität:** Geigerzähler (Umweltmonitoring)
- **Pflege & Steuerung:** Relais für Pumpen, LED-Growlights, Lüfter

**Features:**

- Automatisches Logging aller Sensordaten auf SD-Karte (CSV)
- Echtzeit-Visualisierung auf OLED-Display
- Web-Interface & REST-API für Fernüberwachung und Steuerung
- Benachrichtigungen bei kritischen Zuständen (Wasserstand, Krankheit, Nährstoffmangel)

---

## 🛠️ Hardware & Sensoren

```text
Arduino Mega 2560
├─ MQ2, MQ3, MQ4, MQ5, MQ6, MQ7, MQ8, MQ9, MQ135 (Gase)
├─ DHT11/DHT22 (Temp/Luftfeuchte)
├─ LDR (Licht)
├─ TDS-Sensor (Nährstoffgehalt)
├─ Kapazitiver Wasserstandssensor
├─ IR-Kamera (Krankheitserkennung)
├─ 2x MEMS-Mikrofone (Audio)
├─ Geigerzähler (Radioaktivität)
├─ SD-Karte (Logging)
├─ RTC (Zeitstempel)
├─ Relais (Pumpen, LEDs, Lüfter)
```

---

## 🧩 Software-Architektur

**Modularer Aufbau:**

- `sensors.{h,cpp}`: Initialisierung, Auslesen & Kalibrierung aller Sensoren
- `data_logger.{h,cpp}`: CSV-Logging auf SD-Karte
- `display.{h,cpp}`: OLED-Display, Statusseiten
- `rtc_module.{h,cpp}`: Echtzeituhr
- `utilities.{h,cpp}`: Hilfsfunktionen, Fehlerbehandlung

**Web & API:**

- Webserver für Live-Daten, Steuerung & Historie
- REST-API für Integration in Smart-Home/Apps

---

## � Beispiel-Datenstruktur (CSV)

| DateTime         | Temp | Humidity | Light | TDS | WaterLevel | MQ2-MQ135 | Audio1/2 | IR-Status | Radiation |
| ---------------- | ---- | -------- | ----- | --- | ---------- | --------- | -------- | --------- | --------- |
| 2025-10-16 12:00 | 24.5 | 60.2     | 320   | 850 | OK         | ...       | ...      | Healthy   | 0         |

---

## 🌐 Web & API

- **Webseite:** Übersicht, Live-Daten, Historie, Warnungen
- **API:** Endpunkte für Sensordaten, Steuerbefehle, Pflegeaktionen

---

## � Pflege & Automatisierung

- Automatische Nährstoffgabe & Bewässerung
- LED-Growlights nach Lichtmessung
- Lüftersteuerung nach Luftqualität
- Benachrichtigung bei Krankheitserkennung (IR-Kamera)

---

## � Sensorübersicht & Aufgaben

| Sensor         | Aufgabe                        |
| -------------- | ------------------------------ |
| MQ2-MQ135      | Gase/Luftqualität              |
| DHT11/DHT22    | Temperatur & Luftfeuchte       |
| LDR            | Lichtmessung                   |
| TDS            | Nährstoffgehalt (ppm)          |
| Wasserstand    | Füllstand Nährlösung           |
| IR-Kamera      | Krankheits-/Stressüberwachung  |
| MEMS-Mikrofone | Audio/Schall/Störungserkennung |
| Geigerzähler   | Radioaktivität                 |
| RTC            | Zeitstempel für Logging        |
| Relais         | Steuerung Pumpen, LEDs, Lüfter |

---

## 📜 Lizenz

MIT License – Freie Nutzung für Forschung & Citizen Science.

---

**Kontakt:**
Frank Albrecht | Pflanzenautomatisierung@watchkido.de
