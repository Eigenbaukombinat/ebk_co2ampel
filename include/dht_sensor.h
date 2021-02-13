#include "DHT.h"

#ifndef DHT_PIN
  #define DHT_PIN 18
#endif
#ifndef DHT_TYPE
  #define DHT_TYPE DHT22
#endif

String dht_s_temperature;
String dht_s_humidity;
String dht_s_hic;

DHT dht(DHT_PIN, DHT_TYPE);
String Float2String(float value)
{
  // Convert a float to String with two decimals.
  String s;
  s = String(int(value));
  s += '.';
  s += int((value - int(value)) * 100);

  return s;
}

void sensorDHT(){
  String data;
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // check if Not a Number
  if (isnan(t) || isnan(h)) {
    Serial.println("DHT Sensordaten konnten nicht gelesen werden");
  } else {
    //false = Celcius | true = Farenheit
    float hic = dht.computeHeatIndex(t, h, false);
    dht_s_temperature=Float2String(t);
    dht_s_humidity=Float2String(h);
    dht_s_hic=Float2String(hic);
    Serial.print("Luftfeuchte: ");
    Serial.print(dht_s_humidity);
    Serial.print(" %\n");
    Serial.print("Temperatur: ");
    Serial.print(dht_s_temperature);
    Serial.print(" °C\n");
    Serial.print("Gefühlte Temperatur: ");
    Serial.print(dht_s_hic);
    Serial.print(" °C\n");
  }
}