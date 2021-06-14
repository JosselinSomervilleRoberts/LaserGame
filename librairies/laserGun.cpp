#include <Arduino.h>
#include "LaserGun.h"


LaserGun::LaserGun() {
}



void LaserGun::initiate() {
	define_pins();
	init_lcd();
	init_wifi();
	init_df_player();
	init_data();
	init_player();
}



void LaserGun::init_lcd() {
  lcd.init();                    
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Laser Game");
}



void LaserGun:init_df_player() {
  mySoftwareSerial.begin(9600);  // Baud rate
  myDFPlayer.begin(mySoftwareSerial);
  myDFPlayer.volume(volume);  // ( valeur de 0 à 30 )
}



void LaserGun::init_wifi() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Charge l'adresse MAC
  for(int i=0; i<6; i++)
    addressSelf[i] = EEPROM.read(i);
  wifi_set_macaddr(STATION_IF, &addressSelf[0]);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    lcd.clear(); // on efface tout
    lcd.print(" PROBLEME WIFI ");
    //return;
  }
  else {
    // Once ESPNow is successfully Init, we will register for recv CB to
    // get recv packer info
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(OnDataRecv);
  }
}




void LaserGun::init_data() {
  idPlayer = EEPROM.read(30);
  if(not(started)) {
    player = EEPROM.read(30);
  }
  else {
    player = EEPROM.read(99);
    role = EEPROM.read(98);
  } 

  for(int i=0; i<6; i++) {
    addressCibleAvant[i] = EEPROM.read(6+i);
    addressCibleArriere[i] = EEPROM.read(12+i);
    Serial.print(addressCibleAvant[i]);
    Serial.print(":");
  }
  Serial.println("");

  // Musique
  for(int i=0; i<28; i++) {
    musique[i] = EEPROM.read(201+i);
    //Serial.print("musique " + String(i) + " ");
    Serial.println(musique[i]);
  }
}


void LaserGun::define_pins() {
  pinMode(laser, OUTPUT);
  pinMode(buttons[0], INPUT);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(led, OUTPUT);
}


void LaserGun::init_player() {
  respawnsRestants = r_nbRespawns;
  vie = r_viesMax;
}