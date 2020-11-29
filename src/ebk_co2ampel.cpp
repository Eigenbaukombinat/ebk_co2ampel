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

// CO2 Messintervall in Milisekunden
#define INTERVAL 15*1000
// Dauer der Kalibrierungsphase in Milisekunden
#define CALINTERVAL 180*1000

// Boot-Mode Konstanten
#define BOOT_NORMAL    42
#define BOOT_CALIBRATE 23
#define BOOT_UNKNOWN   63

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
 
String ampelversion = "0.12";
unsigned long getDataTimer = 0;
unsigned long calibrationStart = 0;
int countdown = 0;
int lastvals[120];
int dheight;
int safezone = false;
int currentBootMode;

void setBootMode(int bootMode) {
  if(bootMode == BOOT_NORMAL) { 
    Serial.println("Startmodus nächster Reboot: Messmodus");
    preferences.putUInt("cal", bootMode);
  }
  else if(bootMode == BOOT_CALIBRATE) {
    Serial.println("Startmodus nächster Reboot: Kalibrierungsmodus");
    preferences.putUInt("cal", bootMode);
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
  display.setFont(Cousine_Regular_54);
  display.setContrast(255);
  delay(500);
  display.clear();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64 ,0 , String(ampelversion));
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

  // Liste der Messwerte mit "-1" befüllen ("-1" wird beinm Graph nicht gezeichnet)
  for (int x = 0; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
  
  // Ab hier LED-Ring konfigurien
  pixels.begin();
  pixels.clear();
  pixels.fill(pixels.Color(0,0,0));
  pixels.show(); 
  
}

int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}

void set_led_color(int co2) {
  if (co2 < GREEN_CO2) {
    pixels.fill(pixels.Color(0,0,0));      // Grün
    pixels.setPixelColor(4,pixels.Color(0,2,0));
  } else if (co2 < YELLOW_CO2) {
    pixels.fill(pixels.Color(40,30,0));     // Gelb
  } else {
    pixels.fill(pixels.Color(90,0,0));      // Rot
  }
  pixels.show();
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
  display.setFont(ArialMT_Plain_24);
  display.clear();  display.drawString(64, 0, "Kalibriere!"); display.display();
  Serial.println("Kalibrierung startet nun");
  
  myMHZ19.setRange(5000);
  delay(500);
  myMHZ19.calibrateZero();
  delay(500);
  myMHZ19.autoCalibration(false);
  delay(500);
  
  display.clear(); display.drawString(64, 0, "Fertig!"); display.display();
  preferences.putUInt("cal", BOOT_NORMAL);
  delay(2000);

  display.clear(); display.setFont(Cousine_Regular_54);
}

void readCO2(){
  if (millis() - getDataTimer >= INTERVAL) {
    // Neuen CO2 Wert lesen
    int CO2 = myMHZ19.getCO2();
    // Alle Werte in der Messwertliste um eins verschieben
    for (int x = 1; x <= 119; x = x + 1) {
      lastvals[x - 1] = lastvals[x];
    }
    // Aktuellen Messer am Ende einfügen
    lastvals[119] = CO2;
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
    // Farbe des LED-Rings setzen
    if (currentBootMode == BOOT_NORMAL) { set_led_color(CO2); }
    
    // Aktuellen CO2 Wert ausgeben
    //display.setLogBuffer(1, 30);
    display.setFont(Cousine_Regular_54);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64 ,0 , String(CO2));
    //display.drawLogBuffer(0, 0);
    display.display();
    
    // Ein wenig Debug-Ausgabe
    Serial.print("Neue Messung - Aktueller CO2-Wert: ");
    Serial.print(CO2);
    Serial.print("; Background CO2: " + String(myMHZ19.getBackgroundCO2()));
    Serial.print("; Temperatur: " + String(myMHZ19.getTemperature()) + " Temperature Adjustment: " + String(myMHZ19.getTempAdjustment()));
    Serial.println("; uptime: " + uptime_formatter::getUptime());

    getDataTimer = millis();
  }
}

void loop() {
  // Nur für die ersten 10 Sekunden wichtig,
  if ( (!safezone) & (millis() > 10000) ) {
    Serial.println("=== 10 Sekunden im Betrieb, nächster Boot im Messmodus ===");
    setBootMode(BOOT_NORMAL);   //
    safezone = true;
  }
  
  if (safezone) {    
    if (currentBootMode == BOOT_CALIBRATE){          
      if (millis() - calibrationStart <= CALINTERVAL) {
        rainbow(10);
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        countdown = ((calibrationStart + CALINTERVAL) - millis()) / 1000;
        Serial.println("Countdown: " + String(countdown));
        display.drawString(64, 0, String(countdown));
        display.display();
        }
      else if (millis() - calibrationStart >= CALINTERVAL) {
        calibrateCO2();
        calibrationStart = millis();
      }
    }
  }
  
  readCO2();
}

