#include <Arduino.h>
#include "MHZ19.h"
#include "SSD1306Wire.h"
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include "config.h"
#include "html.h"
#include "javascript.h"

// Maximum CO² levels for green and yellow, everything above is considered red.
#define GREEN_CO2 800
#define YELLOW_CO2 1500

// Measurement interval in miliseconds
#define INTERVAL 10000

// Pins for MH-Z19
#define RX_PIN 16
#define TX_PIN 17

// Pins for SD1306
#define SDA_PIN 21
#define SCL_PIN 22

// Pin for LED
#define LED_PIN 4

const char* ssid = SSID
const char* password = PASSWORD

const char* html = HTML;
const char* chartjs = CHARTJS;

WiFiServer server(80);
MHZ19 myMHZ19;
HardwareSerial mySerial(1);
SSD1306Wire  display(0x3c, SDA_PIN, SCL_PIN);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, LED_PIN, NEO_RGB + NEO_KHZ400);
String header;
String response_body;
WiFiClient client;

unsigned long getDataTimer = 0;
int lastvals[12000];
int dheight;
int CO2;
IPAddress IP;

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);
  myMHZ19.begin(mySerial);
  display.init();
  display.setContrast(255);
  delay(1000);
  display.clear();
  dheight = display.getHeight();
  myMHZ19.autoCalibration(false);
  // Fill array of last measurements with -1
  for (int x = 0; x <= 11999; x = x + 1) {
    lastvals[x] = -1;
  }
  pixels.begin();
  pixels.setPixelColor(0, 30,0,0);
  pixels.show(); 
  WiFi.softAP(ssid, password);
  IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();
}

int calc_vpos_for_co2(int co2val, int display_height) {
  return display_height - int((float(display_height) / 3000) * co2val);
}

void set_led_color(int co2) {
  if (co2 < GREEN_CO2) {
    // Green
    pixels.setPixelColor(0, 30,0,0);
  } else if (co2 < YELLOW_CO2) {
    // Yellow
    pixels.setPixelColor(0, 40,40, 0);
  } else {
    // Red
    pixels.setPixelColor(0, 0,90,0);
  }
  pixels.show();
}

void json_header() {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:application/json");
            client.println("Connection: close");
            client.println();
}


void html_header() {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
}


void js_header() {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/javascript");
            client.println("Connection: close");
            client.println();
}


void not_found() {
            client.println("HTTP/1.1 404 NOT FOUND");
            client.println("Content-type:text/plain");
            client.println("Connection: close");
            client.println();
}

void loop() {
  if (millis() - getDataTimer >= INTERVAL) {
    // Get new CO² value.
    CO2 = myMHZ19.getCO2();
    // Shift entries in array back one position.
    for (int x = 1; x <= 11999; x = x + 1) {
      lastvals[x - 1] = lastvals[x];
    }
    // Add new measurement at the end.
    lastvals[11999] = CO2;
    // Clear display and redraw graph of last 120 values.
    display.clear();
    for (int hs = 1; hs < 120; hs = hs + 1) {
      int h = 1200-120+hs;
      int curval = lastvals[h];
      if (curval > 0) {
        int vpos = calc_vpos_for_co2(lastvals[h], dheight);
        int vpos_last = calc_vpos_for_co2(lastvals[h - 1], dheight);
        display.drawLine(h, vpos - 1, h, vpos + 1);
      }
    }
    // Set LED color and print value on display
    set_led_color(CO2);
    display.setLogBuffer(1, 30);
    display.println(CO2);
    display.drawLogBuffer(0, 0);
    display.display();
    // Debug output
    Serial.print("CO2 (ppm): ");
    Serial.println(CO2);
    Serial.println(IP);
    getDataTimer = millis();
  }

    WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request.
            // handle requests here
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:


            if (header.indexOf("GET /data.json") >= 0) {
                json_header();
                client.print("{ data: [");
                  for (int x = 0; x <= 11999; x = x + 1) {
                    if (x % 60 == 0) {
                      client.print("{ minutes:");
                      client.print(x/60);
                      client.print(", ppm:");
                      client.print(lastvals[x]);
                      client.println("}, ");
                    }
                 }

                client.print("] }");
            } else if (header.indexOf("GET /chart.js") >= 0) {
                js_header();
                client.print(chartjs);
            } else if (header.indexOf("GET /favicon.ico") >= 0) {
                not_found();
                client.println("nope");
            } else if (header.indexOf("GET /") >= 0) {
                html_header();
                client.println(html);
                // client.println(CO2);
                // client.println(co2_html_2);
            }        
          
                        
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    header = "";
    response_body = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }



}