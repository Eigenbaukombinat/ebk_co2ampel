#include <Arduino.h>
#include "MHZ19.h"
#include "SSD1306Wire.h"

#include <Adafruit_NeoPixel.h>

#define RX_PIN 16
#define TX_PIN 17
#define BAUDRATE 9600

MHZ19 myMHZ19;

HardwareSerial mySerial(1);
SSD1306Wire  display(0x3c, 21, 22);

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, 4, NEO_GRB + NEO_KHZ800);
 

unsigned long getDataTimer = 0;
int lastvals[120];
int dheight;

void setup()
{
  Serial.begin(9600);
  mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);
  display.init();
  display.setContrast(255);
  delay(1000);
  display.clear();
  dheight = display.getHeight();

  myMHZ19.autoCalibration();
  for (int x; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
pixels.begin(); // This initializes the NeoPixel library.
pixels.setPixelColor(0, pixels.Color(255,0,0)); // Moderately bright green color.
pixels.show(); 
}


int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}



void loop()
{
  if (millis() - getDataTimer >= 5000)
  {
    int CO2;
    CO2 = myMHZ19.getCO2();
    for (int x = 1; x <= 119; x = x + 1) {
      lastvals[x - 1] = lastvals[x];
    }
    lastvals[119] = CO2;
    display.clear();
    for (int h = 1; h < 120; h = h + 1) {
      int curval = lastvals[h];
      if (curval > 0) {
        int vpos = calc_vpos_for_co2(lastvals[h], dheight);
        int vpos_last = calc_vpos_for_co2(lastvals[h - 1], dheight);
        display.drawLine(h - 1, vpos_last, h, vpos);

      }
    }


    display.setLogBuffer(5, 30);
    display.println(CO2);
    display.drawLogBuffer(0, 0);
    display.display();
    Serial.print("CO2 (ppm): ");
    Serial.println(CO2);
    getDataTimer = millis();
  }
}
