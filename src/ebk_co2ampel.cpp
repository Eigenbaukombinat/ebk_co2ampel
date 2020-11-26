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
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_RGB + NEO_KHZ800);
 
unsigned long getDataTimer = 0;
unsigned long getDataTimer1 = 0;
int countdown = 0;
int lastvals[120];
int dheight;
String ampelversion = "0.11";
int safezone = 1;
int tocalibrateornot;
int initloop = 1;

void switchBootMode(int bm){
  switch (bm){
    case 23:
      preferences.putUInt("cal", 42);
      Serial.println("Startmodus nächster Reboot: Messmodus");
      break;
    case 42:
      preferences.putUInt("cal", 23);
      Serial.println("Startmodus nächster Reboot: Kalibrierungsmodus");
      break;
    case 69:
      Serial.println("EEPROM lesen war nicht möglich!");
      break;
    default:
      Serial.print("EEPROM lesen lieferte unerwarteten Wert: ");
      Serial.println(bm);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starte...");
  Serial.print("CO2-Ampel Firmware: ");Serial.println(ampelversion);

  // Ab hier Bootmodus initialisieren und festlegen
  preferences.begin("co2", false);
  tocalibrateornot = preferences.getUInt("cal",69); // wir lesen unser flag ein,
                                                    // 23 = reboot vor safezone, wir wollen kalibrieren,
                                                    // 42 = reboot nach safezone, wir tun nichts
  preferences.putUInt("cal", 23);  // wir sind gerade gestartet
  switch(tocalibrateornot){
    case 23:
      Serial.println("Startmodus Aktuell: Kalibrierungsmodus");
      break;
    case 42:
      Serial.println("Startmodus Aktuell: Messmodus");
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
  Serial.print("read EEPROM value: ");  Serial.println(tocalibrateornot);

  // Liste der Messwerte mit "-1" befüllen ("-1" wird beinm Graph nicht gezeichnet)
  for (int x = 0; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
  
  // Ab hier LED-Ring konfigurien
  pixels.begin();
  pixels.clear();
  pixels.fill(pixels.Color(0,0,0));
  pixels.show(); 
  
  switchBootMode(tocalibrateornot); // beim nächsten boot im anderen modus starten
}

int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}

void set_led_color(int co2) {
  if (co2 < GREEN_CO2) {
    pixels.fill(pixels.Color(10,0,0));      // Grün
  } else if (co2 < YELLOW_CO2) {
    pixels.fill(pixels.Color(40,40,0));     // Gelb
  } else {
    pixels.fill(pixels.Color(0,0,90));      // Rot
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
  preferences.putUInt("cal", 42);
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
    if (tocalibrateornot == 42) {set_led_color(CO2);}
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
    //Serial.print(myMHZ19.getBackgroundCO2());
    Serial.println("; uptime: " + uptime_formatter::getUptime());

    getDataTimer = millis();
  }
}

void loop() {
  if (initloop == 1) {
    if (millis() > 10000) {
      Serial.println("=== safe zone ===");
      switchBootMode(23);
      // preferences.putUInt("cal", 42);  // wir haben die safe zone erreicht, beim naechsten boot nicht kalibrieren!
      safezone = 0; initloop = 0;
      }
    }
  
  if (safezone == 0){    
      if (tocalibrateornot == 23){          
          if (millis() - getDataTimer1 <= CALINTERVAL) {
            rainbow(10);
            display.clear();
            display.setTextAlignment(TEXT_ALIGN_CENTER);
            //countdown = (CALINTERVAL - (getDataTimer1 + millis() * -1)) / 1000;
            //countdown = (millis() + getDataTimer1 - CALINTERVAL) * -1 / 1000;
            countdown = ((getDataTimer1 + CALINTERVAL) - millis()) / 1000;
            Serial.println("Countdown: " + String(countdown));
            display.drawString(64, 0, String(countdown));
            display.display();
            }
          else if (millis() - getDataTimer1 >= CALINTERVAL) {
            calibrateCO2();
            getDataTimer1 = millis();
             }
        }
    else if (tocalibrateornot ==42) {
      if(initloop==1){
        Serial.println("fake news, nobody has the intention to do calibration....");
        initloop=0;
      }

    }
  }
  
  readCO2();
}

