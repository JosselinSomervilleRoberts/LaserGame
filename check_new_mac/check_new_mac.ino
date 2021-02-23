#include <EEPROM.h>
#include <ESP8266WiFi.h>
#define EEPROM_SIZE 256

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(3000);
  Serial.println("hey");
  EEPROM.begin(EEPROM_SIZE);
  WiFi.mode(WIFI_STA);

  uint8_t mac[6];
  for(int i=0; i<6; i++)
    mac[i] = EEPROM.read(i);

  wifi_set_macaddr(STATION_IF, &mac[0]);
  Serial.print("[NEW] ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

void loop() {
  // put your main code here, to run repeatedly:

}
