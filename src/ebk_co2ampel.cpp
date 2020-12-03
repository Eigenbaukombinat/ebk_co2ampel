#include <Arduino.h>
#include "MHZ19.h"
#include "SSD1306Wire.h"
#include <Adafruit_NeoPixel.h>
#include "fonts-custom.h"
#include <Preferences.h>
#include "uptime_formatter.h"

// Grenzwerte für die CO2 Werte für grün und gelb, alles überhalb davon bedeutet rot
#define GREEN_CO2 800
#define YELLOW_CO2 1000

// CO2 Mess-Intervall in Milisekunden
#define CO2_INTERVAL 15*1000
// Display Update-Intervall in Milisekunden
#define DISPLAY_INTERVAL 2500
// Dauer der Kalibrierungsphase in Milisekunden
#define CAL_INTERVAL 180*1000

// Boot-Mode Konstanten
#define BOOT_NORMAL    42
#define BOOT_CALIBRATE 23
#define BOOT_UNKNOWN   69

// Pins für den MH-Z19b
#define RX_PIN 16
#define TX_PIN 17

// Pins für das SD1306 OLED-Display
#define SDA_PIN 21
#define SCL_PIN 22

// Pin für den LED-Ring
#define LED_PIN 4

// Anzahl der angeschlossenen LEDs am Ring
#define NUMPIXELS 8

Preferences preferences;
MHZ19 myMHZ19;
HardwareSerial mySerial(1);
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
 
String ampelversion = "0.50";

int lastvals[120];
int dheight;
int currentBootMode;


void setBootMode(int bootMode) {
  if(bootMode == BOOT_NORMAL) { 
    Serial.println("Startmodus nächster Reboot: Messmodus");
    preferences.putUInt("cal", bootMode);
    if (bootMode != preferences.getUInt("cal", BOOT_UNKNOWN)) Serial.println("Konnte neuen Bootmodus nicht schreiben :-(");
  }
  else if(bootMode == BOOT_CALIBRATE) {
    Serial.println("Startmodus nächster Reboot: Kalibrierungsmodus");
    preferences.putUInt("cal", bootMode);
    if (bootMode != preferences.getUInt("cal", BOOT_UNKNOWN)) Serial.println("Konnte neuen Bootmodus nicht schreiben :-(");
  } else {
    Serial.println("Unerwarteter Boot-Mode soll gespeichert werden. Abgebrochen.");
  } 
}

void toggleBootMode(int bootMode) {
  switch (bootMode){
    case BOOT_CALIBRATE:
      setBootMode(BOOT_NORMAL);   break;
    case BOOT_NORMAL:
      setBootMode(BOOT_CALIBRATE); break;
    case BOOT_UNKNOWN:
      Serial.println("Bootmode Unbekannt! Neue Ampel? Nächster Start wird Messmodus.");
      setBootMode(BOOT_NORMAL);   break;
    default:
      Serial.print("Unerwarteter Bootmode-Wert: "); Serial.println(bootMode);
      Serial.println("Nächster Start wird Messmodus.");
      setBootMode(BOOT_NORMAL);   break;
  }
}

int readCO2(){
  static int co2=400;
  static unsigned long getDataTimer = 0;
  
  if (millis() - getDataTimer >= CO2_INTERVAL) {
    // Neuen CO2 Wert lesen
    co2 = myMHZ19.getCO2();
    // Alle Werte in der Messwertliste um eins verschieben
    for (int x = 1; x <= 119; x = x + 1) {
      lastvals[x - 1] = lastvals[x];
    }
    // Aktuellen Messer am Ende einfügen
    lastvals[119] = co2;
   
    // Ein wenig Debug-Ausgabe
    Serial.print("Neue Messung - Aktueller CO2-Wert: ");
    Serial.print(co2);
    Serial.print("; Background CO2: " + String(myMHZ19.getBackgroundCO2()));
    Serial.print("; Temperatur: " + String(myMHZ19.getTemperature()) + " Temperature Adjustment: " + String(myMHZ19.getTempAdjustment()));
    Serial.println("; uptime: " + uptime_formatter::getUptime());

    getDataTimer = millis();
  }
  return co2;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starte...");
  Serial.print("CO2-Ampel Firmware: ");Serial.println(ampelversion);

  // Ab hier Bootmodus initialisieren und festlegen
  preferences.begin("co2", false);
  currentBootMode = preferences.getUInt("cal", BOOT_UNKNOWN);  // Aktuellen Boot-Mode lesen und speichern

  switch(currentBootMode){
    case BOOT_CALIBRATE:
      Serial.println("Startmodus Aktuell: Kalibrierungsmodus");
      toggleBootMode(currentBootMode); // beim nächsten boot ggfs. im anderen modus starten, wird später nach 10 Sekunden zurückgesetzt
      break;
    case BOOT_NORMAL:
      Serial.println("Startmodus Aktuell: Messmodus");
      toggleBootMode(currentBootMode); // beim nächsten boot ggfs. im anderen modus starten, wird später nach 10 Sekunden zurückgesetzt
      break;
    default:
      Serial.println("Startmodus Aktuell: Unbekannt oder Ungültig");
      Serial.println("Nächster Start im Messmodus");
      setBootMode(BOOT_NORMAL);
      break;
  }

  // Ab hier Display einrichten
  display.init();
  // display.setLogBuffer(5,30);
  display.setContrast(255);
  delay(500);
  display.clear();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64,  0, "Version: " + String(ampelversion));
  if(currentBootMode == BOOT_NORMAL) {
    display.drawString(64, 17, "Zum Kalibrieren");
    display.drawString(64, 34, "jetzt Neustarten" );
  } else {
    display.drawString(64, 17, "Zum Messen");
    display.drawString(64, 34, "jetzt Neustarten" );
  }
  display.display();
  dheight = display.getHeight();
  
  // Ab hier Sensor einrichten
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);
  myMHZ19.autoCalibration(false); // "Automatic Baseline Calibration" (ABC) erstmal aus
  char myVersion[4];          
  myMHZ19.getVersion(myVersion);
  Serial.print("\nMH-Z19b Firmware Version: ");
  Serial.print(myVersion[0]);Serial.print(myVersion[1]);;Serial.print(".");Serial.print(myVersion[2]);Serial.println(myVersion[3]);
  Serial.print("Range: ");  Serial.println(myMHZ19.getRange());   
  Serial.print("Background CO2: ");  Serial.println(myMHZ19.getBackgroundCO2());
  Serial.print("Temperature Cal: ");  Serial.println(myMHZ19.getTempAdjustment());
  Serial.print("ABC Status: "); myMHZ19.getABC() ? Serial.println("ON") :  Serial.println("OFF");
  Serial.print("read EEPROM value: ");  Serial.println(currentBootMode);
  Serial.print("First CO2 value (should be 400): ");  Serial.println(readCO2());
 
  // Liste der Messwerte mit "-1" befüllen ("-1" wird beinm Graph nicht gezeichnet)
  for (int x = 0; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
 
  // Ab hier LED-Ring konfigurien
  pixels.begin();
  pixels.clear();
  pixels.fill(pixels.Color(0,0,0));
  pixels.show(); 

  // Wir lesen schonmal einen CO2 Sensorwert, da die erste Werte meist Müll sind
  delay(5000);
  Serial.print("Second CO2 value: ");  Serial.println(readCO2());
  Serial.flush();
}

int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}

void set_led_color(int co2) {
  static char blinkState=0;
  static signed char blinkDirection=1;
  static int blinkOn=0;
  static int blinkOff=0;

  if (co2 < GREEN_CO2) {
    pixels.fill(pixels.Color(0,0,0));      // Grün
    pixels.setPixelColor(4,pixels.Color(0,2,0));
  } else if (co2 < YELLOW_CO2) {
    pixels.fill(pixels.Color(50,30,0));     // Gelb
  } else {
    blinkState+=blinkDirection;
    if( (blinkState<90) & (blinkState>0) ) {
      pixels.fill(pixels.Color(blinkState,00,0));  
    }
    else if (blinkState==90) {
      blinkDirection=0; blinkOn++;
      if(blinkOn==400) {
        blinkOn=0; blinkDirection=-1; blinkState=89;
      }
    }
    else if (blinkState==0) {
      blinkDirection=0; blinkOff++;
      if(blinkOff==50) {
        blinkOff=0; blinkDirection=1; blinkState=1;
      }
    }
  }
  pixels.show();
  delay(10);
}


void rainbow(int wait) {
  for(long firstPixelHue = 0; firstPixelHue < 65536; firstPixelHue += 256) {
    for(int i=0; i<NUMPIXELS; i++) {
      int pixelHue = firstPixelHue + (i * 65536L / NUMPIXELS);
      pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(pixelHue)));
    }
    pixels.show();
    delay(wait);
  }
}

void calibrateCO2() {
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.clear();
  display.drawString(64, 0, "! Kalibriere !");
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 18, "NICHT");
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 44, "Neustarten");
  display.display();
  Serial.println("Kalibrierung startet nun");
  
  myMHZ19.setRange(5000);
  delay(500);
  myMHZ19.calibrateZero();
  delay(500);
  myMHZ19.autoCalibration(false);
  delay(500);
  
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.drawString(64, 0, "Fertig!");
  display.display();
  setBootMode(BOOT_NORMAL);
  delay(10000);
  display.clear();
}

void updateDisplayCO2(int co2) {
  static unsigned long getUpdateTimer = 0;
  
  if (millis() - getUpdateTimer >= DISPLAY_INTERVAL) {
    // Display löschen und alles neu schreiben/zeichnen
    display.clear();
    for (int h = 1; h < 120; h = h + 1) {
      int curval = lastvals[h];
      if (curval > 0) {
        int vpos = calc_vpos_for_co2(lastvals[h], dheight);
        int vpos_last = calc_vpos_for_co2(lastvals[h - 1], dheight);
        display.drawLine(h - 1, vpos_last, h, vpos);
      }
    }
    // Aktuellen CO2 Wert ausgeben
    display.setFont(Cousine_Regular_54);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64 ,0 , String(co2));
    display.display();
    
    // Fertig mit update; Zeitpunkt für das nächste Update speichern
    getUpdateTimer = millis();
  }
}

void loop() {
  static unsigned long calibrationStart = 0;
  static int countdown = 0;
  static int safezone = false;

  int co2;
  
  // Nur für die ersten 10 Sekunden wichtig,
  if ( (!safezone) & (millis() > 10000) ) {
    Serial.println("=== 10 Sekunden im Betrieb, nächster Boot im Messmodus ===");
    setBootMode(BOOT_NORMAL); 
    safezone = true;
  }
  
  if (safezone) {    
    if (currentBootMode == BOOT_CALIBRATE){          
      if (millis() - calibrationStart <= CAL_INTERVAL) {
        rainbow(10);
        countdown = ((calibrationStart + CAL_INTERVAL) - millis()) / 1000;
        Serial.println("Countdown: " + String(countdown));

        display.clear();
        display.setFont(ArialMT_Plain_16);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 0, "Kalibrierung");
        
        display.setFont(ArialMT_Plain_10);;
        display.drawString(0, 17, "Abbrechen durch Neustart");
        
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.setFont(ArialMT_Plain_16);;
        display.drawString(64, 35, "Noch: " + String(countdown) + " Sek.");
        
        display.display();
        }
      else if (millis() - calibrationStart >= CAL_INTERVAL) {
        calibrateCO2();
        currentBootMode = BOOT_NORMAL;  //Fertig, ab jetzt kann es normal weitergehen
      }
    } else {
      // Achtung: readCO2() liefer nur alle "INTERVAL" ms ein neuen Wert, der alte wird aber zwischengespeichert
      co2 = readCO2();
  
      // Update Display
      updateDisplayCO2(co2);

      // Farbe des LED-Rings setzen
      if(currentBootMode == BOOT_NORMAL) { set_led_color(co2); }
    }
  }
}

