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

#define TIME_WAIT_MAX_CHECK_CONNECTION 750
#define TIME_WAIT_CHECK_CONNECTION 350
#define TIME_WAIT_BEFORE_SENDING_AGAIN 100

#define TIME_IMMUNE 200



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
uint32_t lastConnectionCheckAvant = 0;
uint32_t lastConnectionCheckArriere = 0;
uint32_t lastConnectionConfirmationAvant = 0;
uint32_t lastConnectionConfirmationArriere = 0;
uint32_t lastCheckValueAvant = 0;
uint32_t lastCheckValueArriere = 0;
bool hasBeenConnectedCible = false;


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
long timeLastShot = 0;
long timeLastBeingSemiShot = 0;
int weapon = 0;
bool isShootingLoop = false;
bool equipeChosen = false;
uint8_t hue = 0;
bool isImmune = false;
long timeStartedBeingImmune = 0;
  

// Stats
int score = 0;
float vie = 0;
uint8_t kills = 0;
uint8_t deaths = 0;
bool vivant = true;
long timeDeath = 0;
uint8_t player = 1;
uint8_t role = 1; // 1 GUN Principal, 2 Secondaire
int respawnsRestants = 0;

// Etat du jeu
bool started = false;
long timeGameStarted = 0;

// Affichage
bool needRefreshAll = true;
bool needRefreshScore = false;
bool needRefreshWeapon = false;
bool needRefreshMunitions = false;
int lastPourcent = 100;


// Score
uint8_t s_nbTirsAvant[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTirsAvantSecondaire[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTirsArriere[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTirsArriereSecondaire[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTouchesAvant[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTouchesAvantSecondaire[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTouchesArriere[] = {0,0,0,0,0,0,0,0};
uint8_t s_nbTouchesArriereSecondaire[] = {0,0,0,0,0,0,0,0};
uint8_t s_equipe[] = {0,0,0,0,0,0,0,0};


// Rules
bool r_friendlyFire = true;
int r_friendlyShootScore = - 50;
int r_ennemyShootScore = +100;
int r_friendlyBeingShotScore = 0;
int r_ennemyBeingShotScore = -50;
long r_timeRespawn = 8000;
bool r_respawn = true;
uint8_t r_viesMax = 1;
bool r_regen = false;
long r_timeRegen = 30000;
long r_timeGameDuration = 1200000;
int r_nbRespawns = 3;
long r_timeStartSemiRegen = 1500;

uint8_t tueur = 0;
uint8_t cibleTueur = 0;
bool tue = false;
long timeStartedAffichageTue = 0;
long dureeAffichageTue = 0;
bool afficherTue = false;
bool tueurAllie = false;

uint8_t nb_fois_pistolet_eteint = 0;



typedef struct struct_message {
  uint8_t idMessage;
  uint32_t value;
} struct_message;

// Create a struct_message called myData
struct_message myData;




void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  memcpy(&addressReceived, mac, sizeof(addressReceived));

  if(myData.idMessage != 6) {
    Serial.println("-> Wifi message received");
    Serial.print("- id = ");
    Serial.println(myData.idMessage);
    Serial.print("- value = ");
    Serial.println(myData.value, HEX);
  }

  bool cestCibleAvant = true;
  for(int i=0; i<6; i++) {
    if(addressCibleAvant[i] != addressReceived[i])
      cestCibleAvant = false;
  }

  bool cestCibleArriere = true;
  for(int i=0; i<6; i++) {
    if(addressCibleArriere[i] != addressReceived[i])
      cestCibleArriere = false;
  }
  

  if(myData.idMessage == 1) {
    for(int i=0; i<6; i++)
      Serial.println(addressReceived[i], HEX);


      if(not(connectedCibleAvant)) {
        String couleur = "No color";
        if(myData.value == 0)
          couleur = "rouge";
        else if(myData.value == 32)
          couleur = "jaune";
        else if(myData.value == 64)
          couleur = "vert c";
        else if(myData.value == 96)
          couleur = "vert";
        else if(myData.value == 128)
          couleur = "ciel";
        else if(myData.value == 160)
          couleur = "bleu";
        else if(myData.value == 192)
          couleur = "violet";
        else if(myData.value == 224)
          couleur = "rose";
        hue = myData.value;

        
        EEPROM.write(200,nb_fois_pistolet_eteint);
        EEPROM.commit();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Connect " + couleur + " ?");
        lcd.setCursor(0, 1);
        lcd.print("Noir:No Bleu:Oui");
  
        waitingConnectionAnswer = true;
      }
      else {
        // Sinon c'est la cible arriere qui se connecte pour la premiere fois
        for(int i=0; i<6; i++) {
          addressCibleArriere[i] = addressReceived[i];
          EEPROM.write(12+i, addressCibleArriere[i]);
        }
        EEPROM.commit();
            
        esp_now_add_peer(addressReceived, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
        myData.idMessage = 2; // ID pour dire qu'on demande a se connecter
        myData.value = 1; // PAS DE VALEUR
        esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
        connectedCibleArriere = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
      }
  }

  if(myData.idMessage == 4) { // Reconnexion
    if((cestCibleAvant and (started or hasBeenConnectedCible)) or (cestCibleArriere and (started or hasBeenConnectedCible))) {
      Serial.println("RECONNECTION");

      if(cestCibleAvant) {
        connectedCibleAvant = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckAvant = millis();
        lastConnectionConfirmationAvant = millis();
        if(equipeChosen) {
          myData.idMessage = 9;
          myData.value = hue;
          esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
        }
        else
          hue = myData.value;
      }

      if(cestCibleArriere) {
        connectedCibleArriere = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckArriere = millis();
        lastConnectionConfirmationArriere = millis();
        if(equipeChosen) {
          myData.idMessage = 9;
          myData.value = hue;
          esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
        }
        else
          hue = myData.value;
      }

      myData.idMessage = 2; // ID pour dire qu'on confirme la connexion
      myData.value = 1; // Accepter
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
    }
  }

  if(myData.idMessage == 8) { // Reconnexion acceptée
    if(cestCibleAvant or cestCibleArriere) {
      Serial.println("RECONNECTION");

      if((millis() < 5000) and ((cestCibleAvant and connectedCibleArriere) or (connectedCibleAvant and not(cestCibleAvant)))){
        nb_fois_pistolet_eteint = EEPROM.read(200);
        nb_fois_pistolet_eteint++;
        EEPROM.write(200,nb_fois_pistolet_eteint);
        EEPROM.commit();
        myData.idMessage = 12;
        myData.value = 0; // VALEUR ARBITRAIRE
        esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
        esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));

        myDFPlayer.play(musique[16-1]);
      }
      
      myData.idMessage = 2; // ID pour dire qu'on confirme la connexion
      myData.value = 1; // Accepter
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));

      if(cestCibleAvant) {
        connectedCibleAvant = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckAvant = millis();
        lastConnectionConfirmationAvant = millis();
        hue = myData.value;
      }

      if(cestCibleArriere) {
        connectedCibleArriere = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckArriere = millis();
        lastConnectionConfirmationArriere = millis();
        hue = myData.value;
      }
    }
  }


  if (myData.idMessage == 9) { // Le pistolet a validé l equipe
    equipeChosen = true;
    hue = myData.value;
  }
  

  if(myData.idMessage == 6) {
    if(cestCibleAvant and (lastCheckValueAvant == myData.value))
      lastConnectionConfirmationAvant = millis();
    else if(cestCibleArriere and (lastCheckValueArriere == myData.value))
      lastConnectionConfirmationArriere = millis();
  }



  if(myData.idMessage == 10) { // On est touché
    Serial.println("");
    Serial.println("");
    Serial.println("TOOOOOOOOOOOOUUUUUUUUUUUUUUUUCHEEEE");
    uint8_t playerShoot = uint8_t(uint32_t(myData.value / pow(16,6)) % 16);
    uint8_t weaponShoot = uint8_t(uint32_t(myData.value / pow(16,5)) % 16);
    uint8_t roleShoot   = uint8_t(uint32_t(myData.value / pow(16,4)) % 16);
    uint8_t equipeShoot  = uint8_t(uint32_t(myData.value / pow(16,3)) % 16);
    uint8_t typeCibleShoot   = uint8_t(myData.value % 16);
    Serial.println(playerShoot);
    Serial.println(weaponShoot);
    Serial.println(roleShoot);
    Serial.println(equipeShoot);
    Serial.println(typeCibleShoot);
    Serial.println("");
    Serial.println("");
    Serial.println("");
    estTouche(playerShoot, weaponShoot, roleShoot, typeCibleShoot, equipeShoot);
  }

  if(myData.idMessage == 35) {
    uint8_t playerShoot = myData.value % 16;
    myData.value -= playerShoot;
    uint8_t tueurAllie = uint8_t((myData.value % 32) / 16.0);
    myData.value -= tueurAllie * 16;
    uint8_t cibleTueur = uint8_t((myData.value % 64) / 32.0);
    myData.value -= cibleTueur * 32;
    if(player != playerShoot)
      avoirTuerQqn(playerShoot, tueurAllie, cibleTueur);
  }
}


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.println("-> Wifi message sent");
  Serial.print("- id = ");
  Serial.println(myData.idMessage);
  Serial.print("- value = ");
  Serial.println(myData.value, HEX);
}


void calculScore() {
  score = 0;
  kills = 0;
  deaths = 0;

  for(int i=0; i<8; i++) {
    if(i != player-1) {
      if(s_equipe[i] == 0) {}
      else if(s_equipe[i] == s_equipe[player-1]) {
        if(r_friendlyFire) {
          score += r_friendlyShootScore * (s_nbTirsAvant[i] + s_nbTirsAvantSecondaire[i] + s_nbTirsArriere[i] + s_nbTirsArriereSecondaire[i]);
          score += r_friendlyBeingShotScore * (s_nbTouchesAvant[i] + s_nbTouchesAvantSecondaire[i] + s_nbTouchesArriere[i] + s_nbTouchesArriereSecondaire[i]);
          deaths += (s_nbTouchesAvant[i] + s_nbTouchesAvantSecondaire[i] + s_nbTouchesArriere[i] + s_nbTouchesArriereSecondaire[i]);
        }
      }
      else {
        score += r_ennemyShootScore * (s_nbTirsAvant[i] + s_nbTirsAvantSecondaire[i] + s_nbTirsArriere[i] + s_nbTirsArriereSecondaire[i]);
        kills += (s_nbTirsAvant[i] + s_nbTirsAvantSecondaire[i] + s_nbTirsArriere[i] + s_nbTirsArriereSecondaire[i]);
        score += r_ennemyBeingShotScore * (s_nbTouchesAvant[i] + s_nbTouchesAvantSecondaire[i] + s_nbTouchesArriere[i] + s_nbTouchesArriereSecondaire[i]);
        deaths += (s_nbTouchesAvant[i] + s_nbTouchesAvantSecondaire[i] + s_nbTouchesArriere[i] + s_nbTouchesArriereSecondaire[i]);
      }
    }
  }
}


void avoirTuerQqn(uint8_t playerShoot, uint8_t tueurAllie, uint8_t cibleTueur) {
  uint8_t roleShoot = 1;
  
  if(tueurAllie == 1) {
    if(r_friendlyFire) {
      //score += r_friendlyShootScore; // CHANGEMENT
      myDFPlayer.play(musique[11-1]);
    }
    else
      return;
  }
  else {
    //score += r_ennemyShootScore; // CHANGEMENT
    myDFPlayer.play(musique[10-1]);
  }
  needRefreshScore = true;

  // CHANGEMENT
  /*
  if(cibleTueur == 1) { // Avant
    if(roleShoot == 1) // Principal
       s_nbTirsAvant[playerShoot-1] = s_nbTirsAvant[playerShoot-1] + 1;
    else
       s_nbTirsAvantSecondaire[playerShoot-1] = s_nbTirsAvantSecondaire[playerShoot-1] + 1;
  }
  else{
    if(roleShoot == 1) // Principal
      s_nbTirsArriere[playerShoot-1] = s_nbTirsArriere[playerShoot-1] + 1;
    else
      s_nbTirsArriereSecondaire[playerShoot-1] = s_nbTirsArriereSecondaire[playerShoot-1] + 1;
  }
  */
}


void recupererScore() {
  for(int i=0; i<8; i++) {
    s_nbTirsAvant[i] = EEPROM.read(101 + 9*i);
    s_nbTirsAvantSecondaire[i] = EEPROM.read(102 + 9*i);
    s_nbTirsArriere[i] = EEPROM.read(103 + 9*i);
    s_nbTirsArriereSecondaire[i] = EEPROM.read(104 + 9*i);
    s_nbTouchesAvant[i] = EEPROM.read(105 + 9*i);
    s_nbTouchesAvantSecondaire[i] = EEPROM.read(106 + 9*i);
    s_nbTouchesArriere[i] = EEPROM.read(107 + 9*i);
    s_nbTouchesArriereSecondaire[i] = EEPROM.read(108 + 9*i);
    s_equipe[i] = EEPROM.read(109 + 9*i);
  }
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


void checkRestoreConnection() { 
  // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
  esp_now_add_peer(addressCibleAvant, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  myData.idMessage = 7; // ID pour dire qu'on demande a se connecter
  myData.value = 0; // on envoie la couleur de la cible
  esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));

  esp_now_add_peer(addressCibleArriere, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
}


void init_data() {
  started = (EEPROM.read(100) == 1);

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
    Serial.print("musique " + String(i) + " ");
    Serial.println(musique[i]);
  }
}


void define_pins() {
  pinMode(laser, OUTPUT);
  pinMode(buttons[0], INPUT);
  pinMode(buttons[1], INPUT_PULLUP);
  pinMode(buttons[2], INPUT_PULLUP);
  pinMode(led, OUTPUT);
}


void init_player() {
  respawnsRestants = r_nbRespawns;
  vie = r_viesMax;
}


void connect_to_other_guns() {
  uint8_t adressGuns[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
  for(int p=1; p<=8; p++) {
    if(p != player) {
      adressGuns[5] = p;
      esp_now_add_peer(adressGuns, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    }
  }
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
  init_player();
  checkRestoreConnection();
  connect_to_other_guns();
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
  weapon = (weapon+1)%1; // weapon = (weapon+1)%3; // CHANGEMENT
  munitions = 0;
  startReload();
  needRefreshMunitions = true;
  needRefreshWeapon = true;
}


void shoot() {
  isReloading = false;
  timeLastShot = millis();

  if(munitions > 0) {
    if(weapon != 2) {
      munitions -= 1;
      myDFPlayer.play(musique[soundShoot[weapon]-1]);
      irsend.sendNEC(0x0000866 + pow(16,6)*player + (1+weapon)*pow(16,5)*(connectedCibleAvant and connectedCibleArriere) + role*pow(16,4) + uint8_t(hue/32.0)*pow(16,3));
      needRefreshMunitions = true;

      //if(weapon == 1) { // CHANGEMENT
      if(weapon == 0) {
        long timeStartShotgun = millis();
        while(millis() - timeStartShotgun <= 100) {
          irsend.sendNEC(0x0000866 + pow(16,6)*player + (1+weapon)*pow(16,5)*(connectedCibleAvant and connectedCibleArriere) + role*pow(16,4) + uint8_t(hue/32.0)*pow(16,3));
          verifierConnexion();
          delay(20);
        }
      }
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

void estTouche(uint8_t playerShoot, uint8_t weaponShoot, uint8_t roleShoot, uint8_t typeCibleShoot, uint8_t equipeShoot) {
  if(not(vivant))
    return;
  
  if(player != playerShoot) { // Si on ne s'est pas tiré dessus
    // On enregistre l'equipe du joueur
    //s_equipe[playerShoot] = equipeShoot; // CHANGEMENT

    // Si on a été touché par un allié mais que on a pas friendlyFire d'activé, le tir n'est pas comptabilisé
    // CHANGEMENT
    /*
    if(not(r_friendlyFire) and (equipeShoot==uint8_t(hue/32.0)))
      return;
      */

    // On gere l'immunité (pour ne pas perdre toutes ses vies d un coup)
    if(isImmune) {
      if(millis() - timeStartedBeingImmune > TIME_IMMUNE)
        isImmune = false;
    } 
    if(isImmune)
      return;

    // pour regen plus tard
    timeLastBeingSemiShot = millis();
    
    int viePrevInt = int(vie);

    // CHANGEMENT 
    vie -= 1;
    /*
    if(weaponShoot == 1)
      vie -= 1;
    else if(weaponShoot == 2)
      vie -= 3;
    else if(weaponShoot == 3) {
      Serial.println("LASER");
      vie -= 0.05;
    }*/

    if((vie > 0) and (vie < viePrevInt)) // On perd au moins une vie mais on est pas mort
      perdreUneVie(playerShoot, typeCibleShoot, (equipeShoot == uint8_t(hue/32.0)));
    else if(vie <= 0) // On est mort
      mourir(playerShoot, typeCibleShoot, (equipeShoot == uint8_t(hue/32.0)), roleShoot);
  }
  else
    Serial.println("Tir sur soi meme");
}


void perdreUneVie(uint8_t playerShoot, uint8_t typeCibleShoot, bool shooterAllie) {
  // Pour l'affichage
  tue = false;
  tueur = playerShoot;
  cibleTueur = typeCibleShoot;
  tueurAllie = shooterAllie;
  timeStartedAffichageTue = millis();
  dureeAffichageTue = 2000;
  afficherTue = true;
      
  // On rend le joueur immunisé pour ne pas qu'il reperde une autre vie instantanément
  isImmune = true;
  timeStartedBeingImmune = millis();
  vie = ceil(vie);

  // Musique
  myDFPlayer.play(musique[12-1]);
}


void mourir(uint8_t playerShoot, uint8_t typeCibleShoot, bool shooterAllie, uint8_t roleShoot) {
  // Pour l'affichage
  tue = true;
  tueur = playerShoot;
  tueurAllie = shooterAllie;
  cibleTueur = typeCibleShoot;
  timeStartedAffichageTue = millis();
  dureeAffichageTue = r_timeRespawn -20;
  afficherTue = true;

  // Pour le score
  if(tueurAllie)
    score += r_friendlyBeingShotScore;
  else
    score += r_ennemyBeingShotScore;
  needRefreshScore = true;
  
  if(typeCibleShoot == 1) { // Avant
    if(roleShoot == 1) // Principal
       s_nbTouchesAvant[playerShoot-1] = s_nbTouchesAvant[playerShoot-1] + 1;
    else
       s_nbTouchesAvantSecondaire[playerShoot-1] = s_nbTouchesAvantSecondaire[playerShoot-1] + 1;
  }
  else{
    if(roleShoot == 1) // Principal
      s_nbTouchesArriere[playerShoot-1] = s_nbTouchesArriere[playerShoot-1] + 1;
    else
      s_nbTouchesArriereSecondaire[playerShoot-1] = s_nbTouchesArriereSecondaire[playerShoot-1] + 1;
  }

  // pour l'état du joueur
  isReloading = false;
  deaths += 1;
  vivant = false;
  timeDeath = millis();

  // La musique
  if (respawnsRestants > 0)
    myDFPlayer.play(musique[13-1]);
  else
    myDFPlayer.play(musique[17-1]);

  // On dit au cibles de mourir
  myData.idMessage = 11;
  myData.value = 0; // VALEUR ARBITRAIRE
  esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
  esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));

  // On indique au tueur qu on a été tué
  uint8_t addressTueur[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
  addressTueur[5] = tueur;
  myData.idMessage = 35;
  myData.value = player + 16*uint8_t(shooterAllie) +32 *cibleTueur;
  esp_now_send(addressTueur, (uint8_t *) &myData, sizeof(myData));
}


void revivre() {
  if(respawnsRestants > 0) {
    myDFPlayer.play(musique[14-1]);
    long timeStartSong = millis();
    while(millis() - timeStartSong <= 500) {
      verifierConnexion();
      delay(20);
    }
  
    myData.idMessage = 12;
    myData.value = 0; // VALEUR ARBITRAIRE
    esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
    esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
    vivant = true;
    respawnsRestants -= 1;
    vie = r_viesMax;
    munitions = munitionsMax[weapon];
    needRefreshAll = true;
  }
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
      hasBeenConnectedCible = true;
      lastConnectionConfirmationAvant = millis();
      lastConnectionCheckAvant = millis();
      lastConnectionConfirmationArriere = millis();
      lastConnectionCheckArriere = millis();

      for(int i=0; i<6; i++) {
          addressCibleAvant[i] = addressReceived[i];
          EEPROM.write(6+i, addressCibleAvant[i]);
      }
      EEPROM.commit();
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

  if((respawnsRestants <= 0) and not(vivant)) {
    lcd.setCursor(0,0);
    lcd.print(" MORT DEFINITIVE ");
    lcd.setCursor(0,1);
    lcd.print(" Retourner base  ");
    needRefreshAll = false;
    needRefreshScore = false;
    needRefreshWeapon = false;
    needRefreshMunitions = false;
    return;
  }

  if(not(connectedCibleAvant) or not(connectedCibleArriere)) {
    if(needRefreshAll) {
      lcd.setCursor(0,0);
      if(connectedCibleAvant)
        lcd.print("  CONNECTE AV.  ");
      else
        lcd.print("NON CONNECTE AV.");

      lcd.setCursor(0,1);
      if(connectedCibleArriere)
        lcd.print("  CONNECTE AR.  ");
      else
        lcd.print("NON CONNECTE AR.");
    }
    needRefreshAll = false;
    return;
  }

  // Score
  if(needRefreshAll or needRefreshScore) {
    /*
    needRefreshScore = false;
    lcd.setCursor(0, 0);
    String sScore = "S:";
    for(int i=0; i<5-String(score).length(); i++)
      sScore += " ";
    lcd.print(sScore + String(score));*/
    
    needRefreshScore = false;
    lcd.setCursor(0, 0);
    lcd.print("Vie:" + String(respawnsRestants));
  }

  if(afficherTue) {
    if(tue) {
      lcd.setCursor(0,1);
      if(not(tueurAllie)) {
        lcd.print(" Tue par P" + String(tueur) + "   ");
        lcd.setCursor(14,1);
        if(cibleTueur == 1)
          lcd.print("Av");
        else
          lcd.print("Ar");
      }
      else
        lcd.print("Tue par allie P" + String(tueur));
        
    }
    else {
      lcd.setCursor(0,1);
      if(not(tueurAllie)) {
        lcd.print("Touche par P" + String(tueur));
        lcd.setCursor(14,1);
        if(cibleTueur == 1)
          lcd.print("Av");
        else
          lcd.print("Ar");
      }
      else
        lcd.print("Tir par allie P" + String(tueur));
    }
  }
  else {

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
  }

  // Avancement de la partie
 /*
 if(started) {
    int pourcent = int(100.0 * (timeGameStarted+r_timeGameDuration - millis())  / float(r_timeGameDuration));
    if((pourcent != lastPourcent) or needRefreshAll) {
      lcd.setCursor(13,0);
      String sPourcent = "";
      for(int i=0; i<2-String(pourcent).length(); i++)
        sPourcent += "0";
      lcd.print(sPourcent + String(pourcent) + "%");
      lastPourcent = pourcent;
    }
    
  }
  else if(needRefreshAll) {
    lcd.setCursor(13,0);
    lcd.print(" NS");
  }*/
  lcd.setCursor(7,0);
  lcd.print("Respawn:" + String(nb_fois_pistolet_eteint));

  needRefreshAll = false;
}



void verifierConnexion() {
  if(connectedCibleAvant) {
    if(millis() - lastConnectionConfirmationAvant >= TIME_WAIT_MAX_CHECK_CONNECTION) {
      connectedCibleAvant = false;
      needRefreshAll = true;
    }
    else if ((millis() - lastConnectionConfirmationAvant >= TIME_WAIT_CHECK_CONNECTION) and (millis() - lastConnectionCheckAvant >= TIME_WAIT_BEFORE_SENDING_AGAIN)) {
      myData.idMessage = 5;
      myData.value = uint32_t(float(millis() / 1000.0));
      lastCheckValueAvant = myData.value;
      lastConnectionCheckAvant = millis();
      esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
    }
  }

  if(connectedCibleArriere) {
    if(millis() - lastConnectionConfirmationArriere >= TIME_WAIT_MAX_CHECK_CONNECTION) {
      connectedCibleArriere = false;
      needRefreshAll = true;
    }
    else if ((millis() - lastConnectionConfirmationArriere >= TIME_WAIT_CHECK_CONNECTION) and (millis() - lastConnectionCheckArriere >= TIME_WAIT_BEFORE_SENDING_AGAIN)) {
      myData.idMessage = 5;
      myData.value = uint32_t(float(millis() / 1000.0));
      lastCheckValueArriere = myData.value;
      lastConnectionCheckArriere = millis();
      esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
    }
  }
}



void choixEquipe() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Choix equipe  ");
  bool confirme = false;
  bool newColor = true;
  lcd.setCursor(0,1);
  String couleur = "noir";

  while(not(confirme) and not(equipeChosen) and connectedCibleAvant and connectedCibleArriere) {
    verifierConnexion();
    if(newColor) {
        lcd.setCursor(0,1);
        if(hue == 0)
          couleur = "Rouge";
        else if(hue == 32)
          couleur = "Jaune";
        else if(hue == 64)
          couleur = "Vert clair";
        else if(hue == 96)
          couleur = "Vert";
        else if(hue == 128)
          couleur = "Bleu ciel";
        else if(hue == 160)
          couleur = "Bleu";
        else if(hue == 192)
          couleur = "Violet";
        else if(hue == 224)
          couleur = "Rose";

      String s = "";
      for(int i=0; i<int(float(16-couleur.length())/2.0); i++)
        s += " ";

      lcd.print(s+couleur+ "    ");
      newColor = false; 

      myData.idMessage = 20;
      myData.value = hue;
      esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
      esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
    }

    if(checkButtonPress(0)) { // Bleu
      hue = (hue+32)%256;
      newColor = true;
    }
    else if(checkButtonPress(3)) { // Noir
      hue = (hue-32)%256;
      newColor = true;
    }
    else if(checkButtonPress(1)) { // Gachete
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Confirmer " + couleur + " ?");
      lcd.setCursor(0, 1);
      lcd.print("Noir:No Bleu:Oui");

      uint32_t timeStart = millis();
      uint8_t choix = 0;
      while((choix == 0) and (millis() - timeStart <= 5000)) {
        verifierConnexion();
        if(checkButtonPress(0)) // Bleu
          choix = 1;
        else if(checkButtonPress(3)) // Noir
          choix = 2;
        delay(5);
      }

      if(choix == 1) {
        confirme = true;
        equipeChosen = true;

        myData.idMessage = 9; // ID pour dire qu'on demande a se connecter
        myData.value = hue; // on envoie la couleur de la cible
        esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
        esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
      }
      else {
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("  Choix equipe  ");
        lcd.setCursor(0,1);
        newColor = true;
      }
    }
    delay(5);
  }

  started = true;
}



void loop() {

  // Si on est en attente de connexion affiche quiv eut se connecter et le choix d'accepter ou de refuser
  // Si on est pas en attente de connexion gère l'affichage de la LED
  verifierConnexion();
  gererAttenteDeConnexion();


  if(connectedCibleAvant and connectedCibleArriere and not(equipeChosen))
    choixEquipe();

  // ============= GERE les actions des 4 boutons ================ //
  digitalWrite(laser, ((vivant) and (checkButtonPressed(2))));   
  if(vivant) {
    if((checkButtonPress(1)) and (millis() - timeLastShot >= timeBetweenShots[weapon]))
      shoot();    
    else if(checkButtonPress(3))
      changeWeapon();
    else if(checkButtonPress(0))
      startReload();   

    // Gère la fin du chargement
    if (isReloading) {
      if(millis() - timeStartedReloading >= timeReload[weapon])
        finishReload();
    }

    // Pour le laser
    if((weapon==2) and isShootingLoop and not(buttonStates[1])) {
      isShootingLoop = false;
      myDFPlayer.pause();
    }
    if(isShootingLoop) {
      irsend.sendNEC(0x0000866 + pow(16,6)*player + (1+weapon)*pow(16,5)*(connectedCibleAvant and connectedCibleArriere) + role*pow(16,4) + uint8_t(hue/32.0)*pow(16,3));
      delay(15);
    }
  }
  // ============================================================ //
    

  // Rafraichit l'ecran LCD si besoin, sinon ne fait rien
  if(afficherTue) {
    if(millis() - timeStartedAffichageTue >= dureeAffichageTue) {
      afficherTue = false;
      needRefreshAll = true;
    }
  }
  affichage();

  if(true) {
    // Faire revivre si on est mort depuis suffisament logntemps
    // Pas besoin de check s'il nous reste des respawns, revivre le fait déja
    if(not(vivant) and (millis() - timeDeath >= r_timeRespawn))
      revivre();

    // Si cela fait plusieurs secondes que l'on a pas été touché, on reprend la vie EN COURS (par exemple si on a 2.4 / 5, on va regen jusqu'a 3/5)
    /*
    if((int(vie) != vie) and (millis() - timeLastBeingSemiShot >= r_timeStartSemiRegen)) {
      int viePrev = int(vie);
      vie += 0.01;
      if(vie >= viePrev)
        vie = int(vie);
    }*/
  }

  delay(4);
}
