// include library to read and write from flash memory
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#define EEPROM_SIZE 256

// A CHANGER
uint8_t newMACAddress[] = {0x32, 0x32, 0x32, 0x32, 0x50, 0x14};
uint8_t autreCibleMACAddress[] = {0x32, 0x32, 0x32, 0x32, 0x50, 0x13};
uint8_t hue = 192;
uint8_t position = 2;



/* EEPROM STUCTURE
 *  - de 0 à 5:   MAC address
 *  - de 6 à 11:  MAC address de l'autre cible
 *  - de 12 à 17: MAC address du dernier pistolet connecté
 *  - de 18 à 23: MAC address du dernier pistolet secondaire connecté
 *  - 24: Avant ou arriere
 *  - 25: Hue valeur (en solo)
 */


void setup() {
  // Setup
  EEPROM.begin(EEPROM_SIZE);
  WiFi.mode(WIFI_STA);


  // Mac Adress
  Serial.print("[OLD] ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  wifi_set_macaddr(STATION_IF, &newMACAddress[0]);
  Serial.print("[NEW] ESP8266 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  for(int i=0; i<6; i++)
    EEPROM.write(i, newMACAddress[i]);


  // Mac adress de l'autre cible
  for(int i=0; i<6; i++)
    EEPROM.write(6+i, autreCibleMACAddress[i]);

   EEPROM.write(24, position);
    EEPROM.write(25, hue);
  EEPROM.commit();
}

void loop() {}
