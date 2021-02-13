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

