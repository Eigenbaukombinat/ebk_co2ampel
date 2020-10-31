# Projekt: CO² Ampel

Implementierung einer CO² Ampel mit ESP32 + MH-Z19B + SSD1306.

Inspiriert vom Projekt ["CO² Ampeln für alle" von der Un-Hack-Bar](https://www.un-hack-bar.de/2020/10/25/co2-ampeln-fuer-alle/).

Eine weitere tolle Seite mit vielen Infos über das Lüften und CO², auch wie man den Bau einer solchen Ampel in den Unterricht integrieren kann, findet sich unter [co2ampel.org](https://co2ampel.org).

Unsere Ampel ist sehr simpel gehalten, und komplett offline. So verbraucht sie wenig Strom und kann mit einer Powerbank mehrere Stunden betrieben werden.

Auf dem Display erscheint nur der aktuelle Wert und der historische Verlauf der letzten 120 Messwerte als Grafik.

Eine RGB-LED zeigt rot, gelb oder grün, je nach Messwert.

## BOM


| Teil      | Preis China [1] | Preis DE    |
| --------- | ----------- | ----------- |
| Sensor    | 20 €        | 25 € [2]   |
| ESP32     | 0,50 €      | 8 €         |
| Buzzer    | 0,20 €      | 3 €         |
| Display   | 3 €         | 6 €         |
| LED       | 0,10 €      | 0,20 €      |
|           |             |             |
| **SUMME** | **23,80 €** | **42,20 €** |


* Sensor: Infrared CO2 Sensor	mh-z19
* ESP32: Mikrocontroller
* Buzzer: Active piezzo buzzer KY-012
* Display: OLED Display SSD1306	
* LED: ws2812 ("Single Chip")


[1]: Lieferzeiten China 1-3 Wochen bei hohen Versandkosten (30-40€)

[2]: Sensor in Deutschland derzeit nicht erhältlich, Preis oben ist aus Belgien, Lieferzeit ca. 1-2 Wochen


## Flashen

Wir benutzen die PlatformIO IDE. Dort kann das Projekt geöffnet und mit Klick auf "Upload" compiliert und auf den ESP32 geladen werden.


## Wiring

bild: todo

