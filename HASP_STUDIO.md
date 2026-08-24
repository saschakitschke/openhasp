# HASP Studio

Dieser Fork erweitert openHASP um lokale Funktionen für HASP Studio.

## Ziele

- openHASP-Kompatibilität vollständig erhalten
- HASP Studio direkt auf dem ESP32-Panel nutzbar machen
- lokale API für HASP Studio erweitern
- später echten WLAN-Scan direkt auf dem ESP32 bereitstellen
- mehrere WLAN-Profile verwalten
- später Panel-Synchronisierung und Bluetooth-Unterstützung ergänzen

## Zielhardware

Guition ESP32-S3-4848S040 16MB

PlatformIO-Umgebung:

`esp32-s3-4848s040_16MB`

## Wichtige Regeln

Bestehendes openHASP-Verhalten soll erhalten bleiben.

Ohne ausdrückliche Freigabe dürfen insbesondere nicht verändert werden:

- WLAN-Zugangsdaten
- Hostname
- MQTT-Einstellungen
- Display-Konfiguration
- vorhandene Seiten
- Benutzerdateien
- Partitionslayout
- bestehende Geräteeinstellungen

Firmware wird erst auf ein physisches Panel übertragen, nachdem der Build erfolgreich geprüft wurde.
