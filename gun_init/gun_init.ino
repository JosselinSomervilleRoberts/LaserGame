// include library to read and write from flash memory
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#define EEPROM_SIZE 256

// A CHANGER
uint8_t newMACAddress[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x02};
uint8_t blankMACAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t refMACAddress[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x01};
uint8_t id = 2;
uint8_t mainGun = 9;

uint8_t musiques[] = {4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,1,2,3};
//uint8_t musiques[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28};



/* EEPROM STUCTURE
 *  - de 0 à 5:   MAC address
 *  - de 6 à 11:  MAC address de la dernière cible avant connectée
 *  - de 12 à 17: MAC address de la dernière cible arriere connectée
 *  - de 18 à 23: MAC address du dernier pistolet secondaire connecté
 *  - de 24 à 29: MAC adress du pistolet de référence
 *  - 30: ID du pistolet
 *  - 31: ID du pistolet principal (la ref)
 *  
 *  
 *  - 98: Role actuel (principal ou secondaire)
 *  - 99: ID du joueur
 *  - 100: partie en cours ?
 *  PATTERN x8
 *  - 101: Nombre de tirs sur joueur 1 cible avant
 *  - 102: Nombre de tirs sur joueur 1 cible avant avec pistolet secondaire
 *  - 103: Nombre de tirs sur joueur 1 cible arrière
 *  - 104: Nombre de tirs sur joueur 1 cible arrière avec pistolet secondaire
 *  - 105: Nombre de fois touché par joueur 1 cible avant
 *  - 106: Nombre de fois touché par joueur 1 cible avant avec pistolet secondaire
 *  - 107: Nombre de fois touché par joueur1 cible arrière
 *  - 108: Nombre de fois touché par joueur 1 cible arrière avec pistolet secondaire
 *  - 109: equipe
 *  JUSQU'A 172
 *  
 *  
 *  - de 201 à 228: numéro des musiques de 0001 à 0028
 */


void setup() {
  // Setup
  Serial.begin(115200);
  delay(3000);
  Serial.println("hey");
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


  // Mac adress des cibles et pistolet secondaire
  for(int i=0; i<6; i++)
    EEPROM.write(6+i, blankMACAddress[i]);
  for(int i=0; i<6; i++)
    EEPROM.write(12+i, blankMACAddress[i]);
  for(int i=0; i<6; i++)
    EEPROM.write(18+i, blankMACAddress[i]);


  // MAC address de ref
  for(int i=0; i<6; i++)
    EEPROM.write(24+i, refMACAddress[i]);


  // INFO flingue
  EEPROM.write(30, id);
  EEPROM.write(31, mainGun);

  
  // Info d'une partie
  EEPROM.write(98, 1); // 1= principal, 2=secondaire
  EEPROM.write(99, id);
  EEPROM.write(100,0);
  for(int i=0; i<72; i++) {
    EEPROM.write(101+i, 0);
  }


  // Musiques
  for(int i=1; i<=28; i++) {
    EEPROM.write(200+i, musiques[i-1]);
  }
  
  EEPROM.commit();
}

void loop() {Serial.println("a");delay(100);}
