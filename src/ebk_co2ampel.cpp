#include <Arduino.h>
#include "MHZ19.h"
#include "SSD1306Wire.h"
#include <Adafruit_NeoPixel.h>
#include "fonts-custom.h"
#include <Preferences.h>

// Maximum CO² levels for green and yellow, everything above is considered red.
#define GREEN_CO2 800
#define YELLOW_CO2 1000

// Measurement interval in miliseconds
#define INTERVAL 15000

// Pins for MH-Z19
#define RX_PIN 16
#define TX_PIN 17

// Pins for SD1306
#define SDA_PIN 21
#define SCL_PIN 22

// Pin for LED
#define LED_PIN 4

// number of LEDs connected
#define NUMPIXELS 8

Preferences preferences;

MHZ19 myMHZ19;
HardwareSerial mySerial(1);
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_RGB + NEO_KHZ800);
 
unsigned long getDataTimer = 0;
int lastvals[120];
int dheight;
String ampelversion = "0.1";
int safezone = 1;
int tocalibrateornot;


void setup() {
  Serial.begin(9600);
  Serial.println("boot...");
  preferences.begin("co2", false);
  tocalibrateornot = preferences.getUInt("cal",69); // wir lesen unser flag ein, 23 = reboot vor safezone, wir wollen kalibrieren, 42 = reboot nach safezone, wir tun nichts
  preferences.putUInt("cal", 23);  // wir sind gerade gestartet
  Serial.println(ampelversion);
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);
  pixels.clear();
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
  // myMHZ19.autoCalibration();
  Serial.print("read EEPROM value: ");
  Serial.println(tocalibrateornot);
  if (tocalibrateornot == 23){
    Serial.println("brace yourself, calibration starting! things either be better or all fucked up beyond this point...");
    display.drawString(64, 20, "CAL!");
    display.display();
    for(int i=60; i > 0; i--){
      display.clear();
      display.drawString(64, 0, String(i));
      display.display();
      delay(1000);
    }
    myMHZ19.setRange(5000);
    delay(500);
    myMHZ19.calibrateZero();
    delay(500);
    myMHZ19.autoCalibration(false);
    delay(500);
    display.clear();
    display.drawString(64, 0, "DONE");
    display.display();
    preferences.putUInt("cal", 42);
    char myVersion[4];          
    myMHZ19.getVersion(myVersion);
    Serial.print("\nFirmware Version: ");
    for(byte i = 0; i < 4; i++)
    {
     Serial.print(myVersion[i]);
     if(i == 1)
       Serial.print(".");    
    }
    Serial.println("");

    Serial.print("Range: ");
    Serial.println(myMHZ19.getRange());   
    Serial.print("Background CO2: ");
    Serial.println(myMHZ19.getBackgroundCO2());
    Serial.print("Temperature Cal: ");
    Serial.println(myMHZ19.getTempAdjustment());
    Serial.println("calibration done");
    Serial.print("ABC Status: "); myMHZ19.getABC() ? Serial.println("ON") :  Serial.println("OFF");
  }
  else if (tocalibrateornot ==42){
    Serial.println("fake news, nobody has the intention to do calibration....");
  }
  // Fill array of last measurements with -1
  for (int x = 0; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
  
  pixels.begin();
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, 0,0,50);
    pixels.show(); 
  }
}

int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}

void set_led_color(int co2) {
  if (co2 < GREEN_CO2) {
    // Green
      for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, 30,0,0);
      }
  } else if (co2 < YELLOW_CO2) {
    // Yellow
          for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, 40,40,0);
      }
  } else {
    // Red
              for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, 0,90,0);
      }
  }
  pixels.show();
}

void loop() {
  if (safezone == 1){
  if (millis() == 10000) {
    Serial.println(" safe zone, sensor stayed up longer than 10s");
    preferences.putUInt("cal", 42);  // wir haben die safe zone erreicht, beim naechsten boot nicht kalibrieren!
    safezone = 0;
  }
  }

  if (millis() - getDataTimer >= INTERVAL) {
    // Get new CO² value.
    int CO2 = myMHZ19.getCO2();
    // Shift entries in array back one position.
    for (int x = 1; x <= 119; x = x + 1) {
      lastvals[x - 1] = lastvals[x];
    }
    // Add new measurement at the end.
    lastvals[119] = CO2;
    // Clear display and redraw whole graph.
    display.clear();
    for (int h = 1; h < 120; h = h + 1) {
      int curval = lastvals[h];
      if (curval > 0) {
        int vpos = calc_vpos_for_co2(lastvals[h], dheight);
        int vpos_last = calc_vpos_for_co2(lastvals[h - 1], dheight);
        display.drawLine(h - 1, vpos_last, h, vpos);
      }
    }
    // Set LED color and print value on display
    set_led_color(CO2);
    display.setLogBuffer(1, 30);
    display.setFont(Cousine_Regular_54);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64 ,0 , String(CO2));
    display.drawLogBuffer(0, 0);
    display.display();
    // Debug output
    Serial.print("CO2 (ppm): ");
    Serial.print(CO2);
    Serial.print(" Background CO2: ");
    Serial.print(myMHZ19.getBackgroundCO2());
    Serial.print(" uptime (seconds): ");
    Serial.println(millis()/1000);
    getDataTimer = millis();
  }
}
