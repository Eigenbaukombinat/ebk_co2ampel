#include <Arduino.h>
#include "MHZ19.h"                                        

 #include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#define RX_PIN 16                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN 17                                      // Tx pin which the MHZ19 Rx pin is attached to
#define BAUDRATE 9600                                      // Device to MH-Z19 Serial baudrate (should not be changed)

MHZ19 myMHZ19;                                             // Constructor for library

HardwareSerial mySerial(1);                              // (ESP32 Example) create device to MH-Z19 serial
 SSD1306Wire  display(0x3c, 21, 22);

unsigned long getDataTimer = 0;
int lastvals[120];

void setup()
{
    Serial.begin(9600);                                     // Device to serial monitor feedback

    mySerial.begin(BAUDRATE, SERIAL_8N1, RX_PIN, TX_PIN); // (ESP32 Example) device to MH-Z19 serial start   
    myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin(). 
  display.init();

  // display.flipScreenVertically();

  display.setContrast(255);
  delay(1000);
  display.clear();

    myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))


          for (int x; x<=119; x=x+1) {
            lastvals[x] = -1;
          }

}



void loop()
{
    if (millis() - getDataTimer >= 2000)
    {
        int CO2; 

        /* note: getCO2() default is command "CO2 Unlimited". This returns the correct CO2 reading even 
        if below background CO2 levels or above range (useful to validate sensor). You can use the 
        usual documented command with getCO2(false) */
        CO2 = myMHZ19.getCO2();                             // Request CO2 (as ppm)

          for (int x=1; x<=119; x=x+1) {
            lastvals[x-1] = lastvals[x];
          }
        
        lastvals[119] = CO2;


        display.clear();
        
        for (int h=1; h<120; h=h+1) {
            int curval = lastvals[h];
            if (curval > 0) {
            int vpos = display.getHeight() - int((float(display.getHeight()) / 3000) * lastvals[h]);
            int vpos_1 = display.getHeight() - int((float(display.getHeight()) / 3000) * lastvals[h-1]);

            display.drawLine(h-1, vpos_1, h, vpos);

        }
        }
        
        int8_t Temp;
        Temp = myMHZ19.getTemperature();                     // Request Temperature (as Celsius)
        
        display.setLogBuffer(5, 30);
        display.println(CO2);
        display.drawLogBuffer(0, 0);
        display.display();
        Serial.print("CO2 (ppm): ");                      
        Serial.println(CO2);                                
        Serial.println();
        Serial.print("Temperature (C): ");                  
        Serial.println(Temp);                               

        getDataTimer = millis();
    }
}
