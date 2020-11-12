#include <Arduino.h>
#include "MHZ19.h"
#include "SSD1306Wire.h"
#include <Adafruit_NeoPixel.h>

// Maximum CO² levels for green and yellow, everything above is considered red.
#define GREEN_CO2 800
#define YELLOW_CO2 1000

// Measurement interval in miliseconds
#define INTERVAL 60000

// Pins for MH-Z19
#define RX_PIN 16
#define TX_PIN 17

// Pins for SD1306
#define SDA_PIN 21
#define SCL_PIN 22

// Pin for LED
#define LED_PIN 4

// number of LEDs connected
#define NUMPIXELS 12

MHZ19 myMHZ19;
HardwareSerial mySerial(1);
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_RGB + NEO_KHZ400);
 
unsigned long getDataTimer = 0;
int lastvals[120];
int dheight;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);
  pixels.clear();
  display.init();
  display.setContrast(255);
  delay(1000);
  display.clear();
  display.flipScreenVertically();
  dheight = display.getHeight();
  myMHZ19.autoCalibration();
  // Fill array of last measurements with -1
  for (int x = 0; x <= 119; x = x + 1) {
    lastvals[x] = -1;
  }
  
  pixels.begin();
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 0, 50));
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
        pixels.show();
      }
  } else if (co2 < YELLOW_CO2) {
    // Yellow
          for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, 40,40,0);
        pixels.show();
      }
  } else {
    // Red
              for(int i=0; i<NUMPIXELS; i++) {
        pixels.setPixelColor(i, 0,90,0);
        pixels.show();
      }
  }
  pixels.show();
}

void loop() {
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
    Serial.println(CO2);
    getDataTimer = millis();
  }
}
