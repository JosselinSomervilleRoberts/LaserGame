#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include "SoftwareSerial.h"    // pour les communications series avec le DFplayer
#include <DFPlayerMini_Fast.h>  // bibliotheque pour le DFPlayer
//#include "DFRobotDFPlayerMini.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#define EEPROM_SIZE 256
#include <espnow.h>

#define ledBlinkWainting 150
#define ledBlinkOneConnected 500



// PINS DEFINITION
const uint8_t kIrLed = 12;    // D6
const uint8_t laser = 14;     // D5
const uint8_t led = 15;       // D8
const uint8_t buttons[] = {16, 2, 0}; // D0, D4, D3
const uint8_t df_player = 13; // D7
// D1 et D2 utilisés pour la liaison I2C avec l'écran
// Le dernier bouton est sur l'entrée analogique A0


// Parametres pour l'affichage et le son
const int lcdColumns = 16;
const int lcdRows = 2;
int volume = 22;
uint32_t timeLedLastUpdate = 0;
bool ledState = false;


// Définition des objets
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.
SoftwareSerial mySoftwareSerial(17,df_player); // RX, TX ( wemos D2,D8 ou 4,15 GPIO )  ou Tx,RX ( Dfplayer )
DFPlayerMini_Fast  myDFPlayer;  // init player
//DFRobotDFPlayerMini myDFPlayer;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 


// Variables
long last_update = 0;
bool buttonStates[4] = {false, false, false, false};
bool waitingConnectionAnswer = false;

uint8_t addressSelf[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
uint8_t addressReceived[] = {0x84, 0xCC, 0xA8, 0x00, 0x00, 0x00};
uint8_t addressCibleAvant[] = {0x84, 0xCC, 0xA8, 0x00, 0x00, 0x00};
uint8_t addressCibleArriere[] = {0x84, 0xCC, 0xA8, 0x00, 0x00, 0x00};
bool connectedCibleAvant = false;
bool connectedCibleArriere = false;


// Pour le jeu
int munitions = 0;
bool isReloading = false;
uint32_t timeStartedReloading = 0;
int timeReload[] = {2000, 1000, 3000};
int munitionsMax[] = {40, 3, 50};
int soundShoot[] = {1,2,3};
int soundReload[] = {7,8,9};
int soundEmpty[] = {4,5,6};
uint8_t musique[28];
long timeBetweenShots[] = {100, 1000, 0};
int weapon = 0;
bool isShootingLoop = false;

// Stats
uint16_t score = 0;
uint8_t vie = 0;
uint8_t kills = 0;
uint8_t deaths = 0;
bool vivant = true;
long timeDeath = 0;
uint8_t player = 1;
uint8_t role = 1; // 1 GUN Principal, 2 Secondaire

// Etat du jeu
bool started = false;
long timeGameStarted = 0;

// Regles du jeu
bool respawn = true;
long timeRespawn = 5000;
uint8_t viesMax = 1;
bool regen = false;
long timeRegen = 30000;
long timeGameDuration = 120000;
bool friendlyFire = true;

// Affichage
bool needRefreshAll = true;
bool needRefreshScore = false;
bool needRefreshWeapon = false;
bool needRefreshMunitions = false;
int lastPourcent = 100;



typedef struct struct_message {
  uint8_t idMessage;
  uint32_t value;
} struct_message;

// Create a struct_message called myData
struct_message myData;




void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  memcpy(&addressReceived, mac, sizeof(addressReceived));

  Serial.println("-> Wifi message received");
  Serial.print("- id = ");
  Serial.println(myData.idMessage);
  Serial.print("- value = ");
  Serial.println(myData.value, HEX);

  if(myData.idMessage == 1) {
    for(int i=0; i<6; i++)
      Serial.println(addressReceived[i], HEX);


      if(not(connectedCibleAvant)) {
        String couleur = "black";
        if(myData.value == 0)
          couleur = "rouge";
        else if(myData.value == 32)
          couleur = "orange";
        else if(myData.value == 64)
          couleur = "jaune";
        else if(myData.value == 96)
          couleur = "vert";
        else if(myData.value == 128)
          couleur = "cyan";
        else if(myData.value == 160)
          couleur = "bleu";
        else if(myData.value == 192)
          couleur = "violet";
        else if(myData.value == 224)
          couleur = "rose";
  
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Connect " + couleur + " ?");
        lcd.setCursor(0, 1);
        lcd.print("Noir:No Bleu:Oui");
  
        waitingConnectionAnswer = true;
      }
      else {
        // On regarde si c'est la cible qui veut se reconnecter
        bool reconnexionAvant = true;
        for(int i=0; i<6; i++) {
          if(addressCibleAvant[i] != addressReceived[i])
            reconnexionAvant = false;
        }

        bool reconnexionArriere = true;
        for(int i=0; i<6; i++) {
          if(addressCibleArriere[i] != addressReceived[i])
            reconnexionArriere = false;
        }
        
        if(reconnexionAvant or reconnexionArriere) {
          myData.idMessage = 2; // ID pour dire qu'on demande a se connecter
          myData.value = 1; // Accepter
          esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
        }
        else { // Sinon c'est la cible arriere qui se connecte pour la premiere fois
          for(int i=0; i<6; i++)
            addressCibleArriere[i] = addressReceived[i];
            
          esp_now_add_peer(addressReceived, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
          myData.idMessage = 2; // ID pour dire qu'on demande a se connecter
          myData.value = 1; // PAS DE VALEUR
          esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
          connectedCibleArriere = true;
        }
      }
  }

  if(myData.idMessage == 2) {
    
  }
}


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  if(sendStatus == 1)
    Serial.println("Envoi réussi !");
  else
    Serial.println("PROBLEME D'ENVOI");
}





void init_lcd() {
  lcd.init();                    
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Laser Game");
}


void init_df_player() {
  mySoftwareSerial.begin(9600);
  myDFPlayer.begin(mySoftwareSerial);
  //myDFPlayer.setTimeout(500); // Définit un temps de time out sur la communication série à 500 ms
  myDFPlayer.volume(volume);  // ( valeur de 0 à 30 )
  //myDFPlayer.outputDevice(DFPLAYER_DEVICE_SD); // indique d'utiliser le player de carte SD interne
}


void init_wifi() {
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


void init_data() {
  started = (EEPROM.read(100) == 1);

  // Musique
  for(int i=0; i<28; i++) {
    musique[i] = EEPROM.read(201+i);
    Serial.print("musique " + String(i) + " ");
    Serial.println(musique[i]);
  }

  if(not(started)) {
    player = EEPROM.read(30);
  }
  else {
    player = EEPROM.read(99);
    role = EEPROM.read(98);
  } 
}


void define_pins() {
  pinMode(laser, OUTPUT);
  pinMode(buttons[0], INPUT);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(led, OUTPUT);
}


void setup() {
  EEPROM.begin(EEPROM_SIZE);
  Serial.begin(115200);
  define_pins();
  irsend.begin();
  init_lcd();
  init_wifi();
  init_df_player();
  init_data();
}


bool checkButtonPressed(int i) {
  /* Renvoie si le bouton i est enfoncé ou non */
  
  bool state = false;

  if(i == 0)
    state = analogRead(0) >= 100;
  else if (i == 1)
    state = digitalRead(buttons[i-1]);
  else if (i < 4)
    state = not(digitalRead(buttons[i-1]));

  buttonStates[i] = state;
  return state;
}


bool checkButtonPress(int i) {
  /* Renvoie si le bouton i vient d'être enfoncé ou non */

  bool prevState = buttonStates[i];
  bool state = checkButtonPressed(i);
  return (not(prevState) && state);
}


void startReload() {
  if(isReloading)
    return;

  isReloading = true;
  timeStartedReloading = millis();
  myDFPlayer.play(musique[soundReload[weapon]-1]);
}


void finishReload() {
  if(not(isReloading))
    return;

  isReloading = false;
  munitions = munitionsMax[weapon];
  needRefreshMunitions = true;
}


void changeWeapon() {
  isReloading = false;
  weapon = (weapon+1)%3;
  munitions = 0;
  startReload();
  needRefreshMunitions = true;
  needRefreshWeapon = true;
}


void shoot() {
  isReloading = false;

  if(munitions > 0) {
    if(weapon != 2) {
      munitions -= 1;
      myDFPlayer.play(musique[soundShoot[weapon]-1]);
      irsend.sendNEC(0x0008666 + pow(16,6) + (1+weapon)*pow(16,5) + role*pow(16,4));
      needRefreshMunitions = true;
    }
    else if(not(isShootingLoop)) {
      isShootingLoop = true;
      myDFPlayer.loop(musique[soundShoot[weapon]-1]);
    }
  }
  else {
    myDFPlayer.play(musique[soundEmpty[weapon]-1]);
  }
}

void estTouche() {
  
}



void gererAttenteDeConnexion() {
  if(waitingConnectionAnswer) {
    int valueChoice = 0;
      long timeStartedChoice = millis();

      while((millis() - timeStartedChoice <= 5000) and (valueChoice == 0)) {     
        if (millis() - timeLedLastUpdate >= ledBlinkWainting) {
          ledState = not(ledState);
          timeLedLastUpdate = millis();
          digitalWrite(led, ledState);
        }
        
        if(checkButtonPress(0)) {
          Serial.println("ACCEPTER");
          valueChoice = 1; // Accepter
        }
        else if(checkButtonPress(3)) {
          Serial.println("REFUS");
          valueChoice = 2; // Refuser
        }

        delay(10);
      }


    if(valueChoice == 0) {
      valueChoice = 2;
      Serial.println("REFUS PAR DEFAUT");
    }

    if (valueChoice == 1) {
      connectedCibleAvant = true;

      for(int i=0; i<6; i++)
          addressCibleAvant[i] = addressReceived[i];
    }

    waitingConnectionAnswer = false;
    esp_now_add_peer(addressReceived, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    myData.idMessage = 2; // ID pour dire qu'on demande a se connecter
    myData.value = valueChoice; // Accepter ou refuser
    esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));

    needRefreshAll = true;
  }
  else {
    if((connectedCibleAvant) and (connectedCibleArriere))
      ledState = true;
    else if(connectedCibleAvant) {
      if (millis() - timeLedLastUpdate >= ledBlinkOneConnected) {
        ledState = not(ledState);
        timeLedLastUpdate = millis();
        digitalWrite(led, ledState);
      }
    }
    else
      ledState = false;

    digitalWrite(led, ledState);
  }
}



void affichage() {
  if(waitingConnectionAnswer)
    return;
  
  if(needRefreshAll)
    lcd.clear();

  // Score
  if(needRefreshAll or needRefreshScore) {
    needRefreshScore = false;
    lcd.setCursor(0, 0);
    String sScore = "S:";
    for(int i=0; i<5-String(score).length(); i++)
      sScore += "0";
    lcd.print(sScore + String(score));
  }

  // Arme
  if(needRefreshAll or needRefreshWeapon) {
    needRefreshWeapon = false;
    lcd.setCursor(0, 1);
    if (weapon == 0)
      lcd.print("Pistol ");
    else if (weapon == 1)
      lcd.print("Shotgun");
    else if (weapon == 2)
      lcd.print("Laser  ");
  }
    

  // Munitions
  if(needRefreshAll or needRefreshMunitions) {
    needRefreshMunitions = false;
    lcd.setCursor(8,1);
    String sMun = "";
    for(int i=0; i<2-String(munitions).length(); i++)
      sMun += "0";
    sMun += String(munitions) + "/";
    for(int i=0; i<2-String(munitionsMax[weapon]).length(); i++)
      sMun += "0";
    lcd.print(sMun + String(munitionsMax[weapon]));
  }

  // Numero du joeur
  if(needRefreshAll) {
    lcd.setCursor(14,1);
    lcd.print("P" + String(player));
  }

  // Avancement de la partie
 if(started) {
    int pourcent = int(100.0 * (timeGameStarted+timeGameDuration - millis())  / float(timeGameDuration));
    if((pourcent != lastPourcent) or needRefreshAll) {
      lcd.setCursor(13,2);
      String sPourcent = "";
      for(int i=0; i<2-String(pourcent).length(); i++)
        sPourcent += "0";
      lcd.print(sPourcent + String(pourcent) + "%");
      lastPourcent = pourcent;
    }
  }
  else if(needRefreshAll) {
    lcd.setCursor(13,2);
    lcd.print(" NS");
  }

  needRefreshAll = false;
}




void loop() {

  // Si on est en attente de connexion affiche quiv eut se connecter et le choix d'accepter ou de refuser
  // Si on est pas en attente de connexion gère l'affichage de la LED
  gererAttenteDeConnexion();
  

  // Gère la fin du chargement
  if (isReloading) {
    if(millis() - timeStartedReloading >= timeReload[weapon])
      finishReload();
  }

  // ============= GERE les actions des 4 boutons ================ //
  if(checkButtonPress(1))
    shoot();    
  else if(checkButtonPress(3))
    changeWeapon();
  else if(checkButtonPress(0))
    startReload();   
  digitalWrite(laser, (checkButtonPressed(2)));
  if((weapon==2) and isShootingLoop and not(buttonStates[1])) {
    isShootingLoop = false;
    myDFPlayer.pause();
  }
  // ============================================================ //
    

  // Rafraichit l'ecran LCD si besoin, sinon ne fait rien
  affichage();

  if(isShootingLoop) {
    irsend.sendNEC(0xEFE86866);
    delay(10);
  }
  /*
   if(checkButtonPress(1)) {
    volume = min(volume+1, 30);
    myDFPlayer.volume(volume);
   }*/

/*
  for(int i=0; i<4; i++) {
    Serial.print(checkButtonPressed(i));
    Serial.print(" , ");
  }
  Serial.println("");*/
   delay(4);
}
