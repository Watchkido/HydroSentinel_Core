# 🚀 Installationsanleitung: AirScout Firmware auf Arduino Mega

Diese Anleitung beschreibt, wie du das AirScout-Programm aus VS Code (PlatformIO) auf deinen Arduino Mega 2560 überträgst.

---

## 1. Voraussetzungen

- Arduino Mega 2560 per USB am PC angeschlossen
- VS Code mit PlatformIO Extension installiert
- Projektordner geöffnet: `PlatformIO_Project/`

## 2. COM-Port prüfen

Finde den COM-Port deines Arduino (z.B. `COM8`).

- Im Windows-Geräte-Manager nachsehen
- Oder im VS Code Terminal:
  ```
  mode
  ```

### Das Ergebniss sollte so ähnlich aussehen:

```
Status von Gerät COM1:
----------------------
    Baudrate:        1200
    Parität:         None
    Datenbits:       7
    Stoppbits:       1
    Timeout:       OFF
    XON/XOFF:        OFF
    CTS-Handshaking: OFF
    DSR-Handshaking: OFF
    DSR-Prüfung:     OFF
    DTR-Signal:      ON
    RTS-Signal:      ON


Status von Gerät COM3:
----------------------
    Baudrate:        9600
    Parität:         None
    Datenbits:       8
    Stoppbits:       1
    Timeout:       OFF
    XON/XOFF:        OFF
    CTS-Handshaking: OFF
    DSR-Handshaking: OFF
    DSR-Prüfung:     OFF
    DTR-Signal:      OFF
    RTS-Signal:      OFF


Status von Gerät CON:
---------------------
    Zeilen:          25
    Spalten:         142
    Wiederholrate:   31
    Verzögerungszeit:1
    Codepage:        850
```

### Dieser Arduino ist mit COM3 Verbunden!

## 3. PlatformIO-Konfiguration prüfen

Öffne die im Verzeichnis `src` die Datei `platformio.ini` und prüfe/vergleiche die Ports:

```
upload_port = COM3
monitor_port = COM3
```

Passe ggf. den Port an deinen Arduino an.

## 4. Kompilieren

Im VS Code Terminal:

```
pio run
```

→ Kompiliert das Projekt und prüft auf Fehler.
→ Wenn Fehler gefunden werden, behebe diese.
→ Am Ende sollte `[SUCCESS]` stehen.

## 5. Upload auf Arduino

Im VS Code Terminal:

```
pio run --target upload
```

→ Überträgt das Programm auf den Arduino Mega.
→ Wenn Fehler gefunden werden, behebe diese.
→ avrdude: stk500v2_ReceiveMessage(): timeout
→
→
→
→ Am Ende sollte `[SUCCESS]` stehen.

## 6. Seriellen Monitor starten (optional)

Um die Ausgabe des Programms zu sehen:

```
pio device monitor
pio device monitor -b 115200
```

→ Zeigt die Serial-Ausgaben deines Programms.

---

## 7. Fehlerbehebung

- Bei Upload-Problemen: Arduino ab- und wieder anstecken, VS Code neu starten.
- COM-Port prüfen und ggf. in `platformio.ini` anpassen.

---

**Fertig! Dein Arduino läuft jetzt mit der HydroSentinal-Firmware.**
