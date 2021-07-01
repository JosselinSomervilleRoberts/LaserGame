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

const String softwareVersion = "2.7";


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

uint8_t redondanceEnvoi = 5;
uint8_t delaiEnvoi = 10;
int nbEnvoisRestants = 0;
bool entrainEnvoyer = false;


// Pour le jeu
int munitions = 40;
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
int vie = 0;
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
uint8_t r_nbMortsDefinitives = 255;
uint8_t r_viesMax = 1;
bool r_regen = false;
long r_timeRegen = 30000;
long r_timeGameDuration = 1200000;
uint8_t r_nbRespawns = 3;
long r_timeStartSemiRegen = 1500;
bool light_on = true;

uint8_t tueur = 0;
uint8_t cibleTueur = 0;
bool tue = false;
long timeStartedAffichageTue = 0;
long dureeAffichageTue = 0;
bool afficherTue = false;
bool tueurAllie = false;

uint8_t nb_fois_pistolet_eteint = 0;



// ADMIN Mode
bool admin = false;
bool enteringCodeMenu = false;
bool inAdminMenu = false;
bool inCodeMenu = false;
uint32_t timeStartedEnteringCodeMenu = 0;
uint32_t timeToEnterCodeMenu = 1500;
uint8_t code[] = {2,2,2,3,0,3,2,0,3,3,2};
uint8_t codeTape[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
uint8_t longueurCodeTape = 0;

bool invincible = false;
bool munitions_illim = false;
bool rapidFire = false;


typedef struct struct_message {
  uint8_t idMessage;
  uint32_t value;
} struct_message;

// Create a struct_message called myData
struct_message myData;


void deconnecter() {
  if(connectedCibleAvant) {
    myData.idMessage = 50;
    myData.value = 0;
    esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
  }

  if(connectedCibleArriere) {
    myData.idMessage = 50;
    myData.value = 0;
    esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
  }

  waitingConnectionAnswer = false;
  connectedCibleAvant = false;
  connectedCibleArriere = false;
  lastConnectionCheckAvant = 0;
  lastConnectionCheckArriere = 0;
  lastConnectionConfirmationAvant = 0;
  lastConnectionConfirmationArriere = 0;
  lastCheckValueAvant = 0;
  lastCheckValueArriere = 0;
  hasBeenConnectedCible = false;
  equipeChosen = false;
}




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
  



  if(myData.idMessage == 1) { // Demande de nouvelle connexiond e la part d'une cible avant
    for(int i=0; i<6; i++)
      Serial.println(addressReceived[i], HEX);

      if(not(connectedCibleAvant)) {
        if(not(connectedCibleArriere)) {
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

          if(not(hasBeenConnectedCible)) {
            EEPROM.write(100, 0);
            started = false;
        }
        }
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

  else if(myData.idMessage == 4) { // Demande de reconnexion de la part d'une cible
    if((cestCibleAvant and (started or hasBeenConnectedCible)) or (cestCibleArriere and (started or hasBeenConnectedCible))) {
      Serial.println("RECONNEXION");

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
        else {
          if(not(hasBeenConnectedCible))
            hue = myData.value;
        }
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
        else{
          if(not(hasBeenConnectedCible))
            hue = myData.value;
        }
      }

      myData.idMessage = 2; // ID pour dire qu'on confirme la connexion
      myData.value = 1; // Accepter
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
    }
  }

  else if(myData.idMessage == 6) { // Réponse d'une cible à la confirmation de connexion
    if(cestCibleAvant and (lastCheckValueAvant == myData.value))
      lastConnectionConfirmationAvant = millis();
    else if(cestCibleArriere and (lastCheckValueArriere == myData.value))
      lastConnectionConfirmationArriere = millis();
  }

  else if(myData.idMessage == 8) { // La cible accepte la demande de reconnexion effectuée par le pistolet
    if(cestCibleAvant or cestCibleArriere) {
      Serial.println("RECONNEXION");
      started = (EEPROM.read(100) == 1);

      if((millis() < 5000) and started and ((cestCibleAvant and connectedCibleArriere) or (connectedCibleAvant and not(cestCibleAvant)))){
        recupererParametres();
        munitions = 0;
        vie = EEPROM.read(198);
        respawnsRestants = EEPROM.read(199);
        nb_fois_pistolet_eteint = EEPROM.read(200);
        
        if(vie == 0) {
          if(respawnsRestants > 0) {
            respawnsRestants--;
            vie = r_viesMax;
            EEPROM.write(199,respawnsRestants);
            EEPROM.write(198,vie);
            EEPROM.commit();
            
            myData.idMessage = 12;
            myData.value = 0; // VALEUR ARBITRAIRE
            esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
            esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
          }
          else
          {   
            if(nb_fois_pistolet_eteint < r_nbMortsDefinitives) {
              nb_fois_pistolet_eteint++;
              respawnsRestants = r_nbRespawns;
              vie = r_viesMax;
              EEPROM.write(198,vie);
              EEPROM.write(199,respawnsRestants);
              EEPROM.write(200,nb_fois_pistolet_eteint);
              EEPROM.commit();
              
              myData.idMessage = 12;
              myData.value = 0; // VALEUR ARBITRAIRE
              esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
              esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
              myDFPlayer.play(musique[16-1]);
            }
            else {
              vivant = false;
            }
          }
        }
      }
      
      myData.idMessage = 2; // ID pour dire qu'on confirme la connexion
      myData.value = 1; // Accepter
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
      sendParametersCibles();

      if(cestCibleAvant) {
        connectedCibleAvant = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckAvant = millis();
        lastConnectionConfirmationAvant = millis();
        //hue = myData.value; // ERROR : Y A UN PROBLEME C EST DESACTIVE
      }

      if(cestCibleArriere) {
        connectedCibleArriere = true;
        hasBeenConnectedCible = true;
        needRefreshAll = true;
        lastConnectionCheckArriere = millis();
        lastConnectionConfirmationArriere = millis();
        //hue = myData.value; // ERROR : Y A UN PROBLEME C EST DESACTIVE
      }

      if(not(vivant)) {
        // On dit au cibles de mourir
        myData.idMessage = 11;
        myData.value = 0; // VALEUR ARBITRAIRE
        esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
        esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
      }
    }
  }

  else if(myData.idMessage == 10) { // On est touché
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
    if(!invincible)
      estTouche(playerShoot, weaponShoot, roleShoot, typeCibleShoot, equipeShoot);
  }

  else if (myData.idMessage == 30) { // Start the game
    if(not(started) and (myData.value == 1)) startGame();
    else if((started) and (myData.value == 2)) stopperPartie();
  }
  
  else if(myData.idMessage == 35) {
    uint8_t playerShoot = myData.value % 16;
    myData.value -= playerShoot;
    uint8_t tueurAllie = uint8_t((myData.value % 32) / 16.0);
    myData.value -= tueurAllie * 16;
    uint8_t cibleTueur = uint8_t((myData.value % 64) / 32.0);
    myData.value -= cibleTueur * 32;
    if(player != playerShoot)
      avoirTuerQqn(playerShoot, tueurAllie, cibleTueur);
  }

  else if (myData.idMessage == 40) { // changement du nombre de vie max
    r_nbRespawns = myData.value;
    vie = r_nbRespawns;
    EEPROM.write(193, r_nbRespawns);
    EEPROM.commit();
  }

  else if (myData.idMessage == 41) { // changement du nombre de morts
    r_nbMortsDefinitives = myData.value;
    EEPROM.write(194, r_nbMortsDefinitives);
    EEPROM.commit();
  }

  else if (myData.idMessage == 42) { // changement du temps de respawn
    r_timeRespawn = (long)(myData.value) * 1000;
    EEPROM.write(195, uint8_t((float)(r_timeRespawn/1000.0)));
    EEPROM.commit();
  }

  else if (myData.idMessage == 43) { // changement du temps de jeu
    r_timeGameDuration = 60000*myData.value;
    EEPROM.write(196, uint8_t((float)(r_timeGameDuration/60000.0)));
    EEPROM.commit();
  }

  else if (myData.idMessage == 44) { // changement de si le gilet s'allume ou non
    if(myData.value == 1) {
      light_on = true;
      EEPROM.write(197, 1);
    }
    else {
      light_on = false;
      EEPROM.write(197, 0);
  }
    sendParametersCibles();
    
    EEPROM.commit();
  }

  else if (myData.idMessage == 60) { // DOOM : tous les adversaires meurent
    if(myData.value != hue) { // Si on ets pas dans la bonne équipe on meurt
      vivant = false;
      vie = 0;
      respawnsRestants = 0;
      nb_fois_pistolet_eteint = r_nbMortsDefinitives;

      // On dit au cibles de mourir
      myData.idMessage = 11;
      myData.value = 0; // VALEUR ARBITRAIRE
      esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
      esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
    
      // On indique au tueur qu on a été tué
      myData.idMessage = 35;
      myData.value = player + 16*uint8_t(0) +32 *1;
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));

       EEPROM.write(198,vie);
       EEPROM.write(199,respawnsRestants);
       EEPROM.write(200,nb_fois_pistolet_eteint);
       EEPROM.commit();
       myDFPlayer.play(musique[17-1]);
       needRefreshAll = true;
    }
  }
}


void startGame() {
  inAdminMenu = false;
  inCodeMenu = false;
  enteringCodeMenu = false;
  
    nb_fois_pistolet_eteint = 0;
    EEPROM.write(200, nb_fois_pistolet_eteint);
    respawnsRestants = r_nbRespawns;
    EEPROM.write(199, respawnsRestants);
    vie = r_viesMax;
    EEPROM.write(198, vie);
    if(light_on)
      EEPROM.write(197, 1);
    else
      EEPROM.write(197, 0);
    EEPROM.write(196, uint8_t((float)(r_timeGameDuration/60000.0)));
    EEPROM.write(195, uint8_t((float)(r_timeRespawn/1000.0)));
    EEPROM.write(194, r_nbMortsDefinitives);
    EEPROM.write(193, r_nbRespawns);
    EEPROM.commit();
    
    started = true;
    EEPROM.write(100, 1);
    EEPROM.commit();
    myDFPlayer.play(musique[18-1]);
    needRefreshAll = true;

    // On dit aux cibles que la partie a démarée
    myData.idMessage = 31;
    myData.value = 1;
    envoyerDataCibles();
}

void recupererParametres() {
  
  // ADMIN rights
   admin = ((EEPROM.read(189) == 1) and started);

   // Cheats
   invincible = ((EEPROM.read(190) == 1) and admin);
   munitions_illim = ((EEPROM.read(191) == 1) and admin);
   if(munitions_illim) munitions = munitionsMax[weapon];
   rapidFire = ((EEPROM.read(192) == 1) and admin);
   
  // Regles
  light_on = (EEPROM.read(197) == 1);
  r_timeGameDuration = 60000*EEPROM.read(196);
  r_timeRespawn = 1000*EEPROM.read(195);
  r_nbMortsDefinitives = EEPROM.read(194);
  r_nbRespawns = EEPROM.read(193);

  if(connectedCibleAvant or connectedCibleArriere)
    sendParametersCibles();
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

void checkRestoreConnectionAvant() { 
  // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
  myData.idMessage = 7; // ID pour dire qu'on demande a se connecter
  myData.value = 0; // on envoie la couleur de la cible
  esp_now_add_peer(addressCibleAvant, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
}

void checkRestoreConnectionArriere() { 
  // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
  myData.idMessage = 7; // ID pour dire qu'on demande a se connecter
  myData.value = 0; // on envoie la couleur de la cible
  esp_now_add_peer(addressCibleArriere, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
}


void checkRestoreConnection() { 
  checkRestoreConnectionAvant();
  checkRestoreConnectionArriere();
}


void init_data() {
  player = EEPROM.read(30);
  equipeChosen = true;// (EEPROM.read(100) == 1);
  hue = EEPROM.read(99);

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
  for(int p=1; p<=9; p++) {
    if(p != player) {
      adressGuns[5] = p;
      esp_now_add_peer(adressGuns, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
    }
  }
}


void envoyerDataCibles() {
  for(int j=0; j<redondanceEnvoi; j++) {
      esp_now_send(addressCibleAvant, (uint8_t *) &myData, sizeof(myData));
      esp_now_send(addressCibleArriere, (uint8_t *) &myData, sizeof(myData));
      
      verifierConnexion();
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


void sendSingleTimeParametersCibles() {
  if(not(entrainEnvoyer) or not(nbEnvoisRestants > 0)) return;

  nbEnvoisRestants--;
  if(nbEnvoisRestants == 0) entrainEnvoyer = false;
  myData.idMessage = 45;
  if(light_on)
    myData.value = 1;
  else
    myData.value = 0;
  envoyerDataCibles();
}

void sendParametersCibles() {
  entrainEnvoyer = true;
  nbEnvoisRestants = redondanceEnvoi;
}

void sendParameters()
{
  sendParametersCibles();
          
  uint8_t adressGuns[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
  for(int j=0; j<redondanceEnvoi; j++) {
    for(int p=1; p<=9; p++) {
      if(p != player) {
        adressGuns[5] = p;
  
        // NB respawns
        myData.idMessage = 40;
        myData.value = r_nbRespawns;
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
  
        // NB morts definitives
        myData.idMessage = 41;
        myData.value = r_nbMortsDefinitives;
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
  
        // Temps de respawn
        myData.idMessage = 42;
        myData.value = (float)(r_timeRespawn / 1000.0);
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
  
        // Temps de jeu
        myData.idMessage = 43;
        myData.value = r_timeGameDuration / 60000;
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
  
        // gilets allumés
        myData.idMessage = 44;
        if(light_on)
          myData.value = 1;
        else
          myData.value = 0;
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
      }
      verifierConnexion();
    }
  }
}


void chooseRules() {
  //if(player != 1) return;

  r_timeRespawn      = 1000 *  chooseValue("Duree de la mort", r_timeRespawn/1000, 1, "s");
  r_nbRespawns       = 1    *  chooseValue("Nb. de respawns ", r_nbRespawns, 1, "");
  r_nbMortsDefinitives = 1   * chooseValue("Nb. morts defs  ", r_nbMortsDefinitives, 1, "");
  r_timeGameDuration = 60000 * chooseValue("Temps de jeu    ", r_timeGameDuration/60000, 1, "min");
  light_on           =        chooseBinary("Gilets allumes ?", light_on);
  
  sendParameters();
}




long chooseValue(String titre, int baseValue, long multiplier, String unite) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(titre);
  
  int k = baseValue;
  int inc = 1;
  bool changed = true;
  lcd.setCursor(0,1);
  lcd.print("<              >");

  while(not(checkButtonPress(1))) {
    if(checkButtonPress(0)) { // Bleu
      if(k < 10)
        inc = 1;
      else if (k < 25)
        inc = 5;
      else if (k < 55)
        inc = 10;
      else
        inc = 50;
      k += inc;
      if(k > 255) k=0;
      changed = true;
    }
    else if(checkButtonPress(3)) { // Noir
      if(k <= 10)
        inc = 1;
      else if (k <= 25)
        inc = 5;
      else if (k <= 55)
        inc = 10;
      else
        inc = 50;
      k -= inc;
      if(k < 0) k = 255;
      changed = true;
    }

    if(changed) {
      String s = String(multiplier*k);
      int espacesDebut = int((14-s.length()-1-unite.length())/2.0);
      String s2 = "";
      for(int i=0; i<espacesDebut; i++) s2 += " ";
      s2 += s + " " + unite;
      for(int i=0; i<espacesDebut; i++) s2 += " ";
      lcd.setCursor(1,1);
      lcd.print(s2);
    }
    changed = false;
    verifierConnexion();
    delay(20);
  }
  return k * multiplier;
}


bool chooseBinary(String titre, bool baseValue) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(titre);
  
  int k = baseValue;
  bool changed = true;

  while(not(checkButtonPress(1))) {
    if(checkButtonPress(0)) { // Bleu
      k = true;
      changed = true;
    }
    else if(checkButtonPress(3)) { // Noir
      k = false;
      changed = true;
    }

    if(changed) {
      lcd.setCursor(6,1);
      if(k)
        lcd.print("Oui");
      else
        lcd.print("Non");   
    }
    changed = false;
    verifierConnexion();
    delay(20);
  }
  
  return k;
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
  if(munitions_illim)
  munitions = munitionsMax[weapon];
  else
    startReload();
  needRefreshMunitions = true;
  needRefreshWeapon = true;
}


void shoot() {
  isReloading = false;
  timeLastShot = millis();

  if(munitions > 0) {
    if(weapon != 2) {
      if(!(munitions_illim)) munitions -= 1;
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

  if(not(started))
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
  EEPROM.write(199,respawnsRestants);
  EEPROM.write(198,vie);
  EEPROM.commit();

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
  needRefreshAll = true;
  
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
  vie = 0;
  vivant = false;
  timeDeath = millis();

  // La musique
  if (respawnsRestants > 0) {
    myDFPlayer.play(musique[13-1]);
    EEPROM.write(198,0);
    EEPROM.commit();
  }
  else {
    myDFPlayer.play(musique[17-1]);
    EEPROM.write(199,0);
    EEPROM.write(198,0);
    EEPROM.commit();
  }

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
    tue = false;
    
    EEPROM.write(199,respawnsRestants);
    EEPROM.write(198,vie);
    EEPROM.commit();
  }
}


void gererAttenteDeConnexion() {
  if(waitingConnectionAnswer) {
    int valueChoice = 0;
      long timeStartedChoice = millis();

      // CHANGEMENT
      valueChoice = 1;

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


  if(enteringCodeMenu) {
    if(needRefreshAll) {
      lcd.setCursor(0,0);
      lcd.print("Hold for menu");
      lcd.setCursor(0,1);
      lcd.print("|              |");
    }
    for(int i=0; i<14; i++) {
      if((millis() - timeStartedEnteringCodeMenu > i*timeToEnterCodeMenu*(1.0-0.7*admin)/14.0) and (millis() - timeStartedEnteringCodeMenu <= (i+1)*timeToEnterCodeMenu*(1.0-0.7*admin)/14.0)) {
        lcd.setCursor(i+1, 1);
        lcd.print("=");
      }
    }
    needRefreshAll = false;
    return;
  }
  if(inCodeMenu) {
    if(needRefreshAll) {
      lcd.setCursor(0,0);
      lcd.print("Entrer Code:");
    }
    if((needRefreshAll) or (needRefreshScore)) {
      for(int i=0; i<longueurCodeTape; i++) {
        lcd.setCursor(i,1);
        lcd.print("x");
      }
      needRefreshScore = false;
    }
    needRefreshAll = false;
    return;
  }

  if(not(connectedCibleAvant) or not(connectedCibleArriere)) {
    if(needRefreshAll) {
      lcd.setCursor(0,0);
      if(connectedCibleAvant)
        lcd.print("CONNECT AV");
      else
        lcd.print(" DECO AV  ");

      lcd.setCursor(0,1);
      if(connectedCibleArriere)
        lcd.print("CONNECT AR");
      else
        lcd.print(" DECO AR  ");

       lcd.setCursor(11,0);
       if(admin)
        lcd.print("|Main");
       else
       lcd.print("| P" + String(player) + " ");

       lcd.setCursor(11,1);
       lcd.print("|v" + softwareVersion);
    }
    needRefreshAll = false;
    return;
  }
  else if(needRefreshAll and not(started)) {
     if(admin) {
      lcd.setCursor(0,0);
      lcd.print("Noir: regles");
      lcd.setCursor(0,1);
      lcd.print("Bleu: demarrer");
      lcd.setCursor(14,0);
      lcd.print("P" + String(player));
    }
    else {
      lcd.setCursor(0,0);
      lcd.print("Noir: choix equ.");
      lcd.setCursor(0,1);
      lcd.print("En attente...");
      lcd.setCursor(14,1);
      lcd.print("P" + String(player));
    }
    needRefreshAll = false;
    return;
  }
  else if(started) {
    if((respawnsRestants <= 0) and not(vivant)) {
      if(needRefreshAll) {
        lcd.setCursor(0,0);
        lcd.print("MORT DEFINITIVE ");
        lcd.setCursor(0,1);
        if(nb_fois_pistolet_eteint < r_nbMortsDefinitives) {
          lcd.print("Retour base -   ");
        }
        else {
          lcd.print("Out respawn -   ");
        }
        lcd.setCursor(14,1);
        lcd.print(String(nb_fois_pistolet_eteint));
        needRefreshAll = false;
      }
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
      if(invincible) 
        lcd.print("INVINC");
      else
        lcd.print("Vie:" + String(respawnsRestants + vivant));
    }
  
    if(afficherTue) {
      if(tue) {
        // CHANGEMENT
        lcd.setCursor(0,1);
        Serial.print("cibleTueur = ");
        Serial.println(cibleTueur);
        if(cibleTueur == 1)
          lcd.print("Tue cible avant ");
        else
          lcd.print("Tue cible arr.  ");
        /*
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
         */
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
        if(rapidFire) {
          if (weapon == 0)
            lcd.print("Rapid P");
          else if (weapon == 1)
            lcd.print("Rapid S");
          else if (weapon == 2)
            lcd.print("Rapid L");
        }
        else {
          if (weapon == 0)
            lcd.print("Pistol ");
          else if (weapon == 1)
            lcd.print("Shotgun");
          else if (weapon == 2)
            lcd.print("Laser  ");
        }
      }
        
    
      // Munitions
      if(needRefreshAll or needRefreshMunitions) {
        needRefreshMunitions = false;
        lcd.setCursor(8,1);
        
        if(munitions_illim)
          lcd.print("Illim");
        else {
          String sMun = "";
          for(int i=0; i<2-String(munitions).length(); i++)
            sMun += "0";
          sMun += String(munitions) + "/";
          for(int i=0; i<2-String(munitionsMax[weapon]).length(); i++)
            sMun += "0";
          lcd.print(sMun + String(munitionsMax[weapon]));
        }
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
  }

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


  // Reconnexion si on a qu'une seule des 2 cibles connectées
  if(hasBeenConnectedCible) {
    if(connectedCibleArriere and not(connectedCibleAvant)) {
      if (millis() - lastConnectionCheckAvant >= TIME_WAIT_CHECK_CONNECTION) {
        checkRestoreConnectionAvant();
        lastConnectionCheckAvant = millis();
      }
    }

    if(connectedCibleAvant and not(connectedCibleArriere)) {
      if (millis() - lastConnectionCheckArriere >= TIME_WAIT_CHECK_CONNECTION) {
        checkRestoreConnectionArriere();
        lastConnectionCheckArriere = millis();
      }
    }
  }
}



void choixEquipe() {
  digitalWrite(laser, LOW);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Choix equipe  ");
  bool confirme = false;
  bool newColor = true;
  lcd.setCursor(0,1);
  String couleur = "noir";
  hue = 32 * uint8_t(hue/32);

  while(not(confirme) and not(equipeChosen) and connectedCibleAvant and connectedCibleArriere) {
    verifierConnexion();
    if(newColor) {
      Serial.println(hue);
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

      // CHANGEMENT
      choix = 1;
      
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

        EEPROM.write(99, hue);
        EEPROM.commit();
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

  //chooseRules();
  //started = true;
  needRefreshAll = true;
}



void loop() {
  //Serial.println("hue = " + String(hue));
  if(entrainEnvoyer) sendSingleTimeParametersCibles();
  
  // ADMIN Mode
  if(inAdminMenu)
    menuAdmin();
  else if(inCodeMenu) {
    if(admin) {
      inCodeMenu = false;
      inAdminMenu = true;
    }
    else
      menuCode();
  }
  else if((analogRead(0) >= 100) and not(digitalRead(2)) and not(digitalRead(3))) {
    digitalWrite(laser, LOW);
    if(enteringCodeMenu) {
      if(millis() - timeStartedEnteringCodeMenu > timeToEnterCodeMenu*(1.0-0.7*admin)) {
        inCodeMenu = true;
        enteringCodeMenu = false;
        needRefreshAll = true;
        buttonStates[0] = true;
        buttonStates[2] = true;
        buttonStates[3] = true;
      }
    }
    else {
      enteringCodeMenu = true;
      timeStartedEnteringCodeMenu = millis();
      needRefreshAll = true;
    }
  }
  else if(enteringCodeMenu) {
    enteringCodeMenu = false;
    needRefreshAll = true;
  }
  else {
    digitalWrite(laser, ((vivant) and (checkButtonPressed(2))));
  }

  // Si on est en attente de connexion affiche quiv eut se connecter et le choix d'accepter ou de refuser
  // Si on est pas en attente de connexion gère l'affichage de la LED
  verifierConnexion();
  gererAttenteDeConnexion();
  


  if(connectedCibleAvant and connectedCibleArriere and not(equipeChosen))
    choixEquipe();

  if(equipeChosen and not(started)) {
    affichage();

    if(!checkButtonPressed(2)) { // Cette condition sert à rentrer plus facilement dans le menu admin
      if(admin) {
        if(checkButtonPress(3)) { // Noir
          chooseRules();
          needRefreshAll = true;
        }
        if(checkButtonPress(0)) {// Bleu
          if(chooseBinary("Demarrer partie?", true)) {
            sendParameters();
            uint8_t adressGuns[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
            for(int p=1; p<=9; p++) {
              if(p != player) {
                adressGuns[5] = p;
                myData.idMessage = 30;
                myData.value = 1;
                esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
                delay(5);
              }
            }
            startGame();
          }
          needRefreshAll = true;
        }
      }
      else {
        if(checkButtonPress(3)) { // Noir
          equipeChosen = false;
          choixEquipe();
        }
      }
    }

    delay(20);
    return;
  }

  // ============= GERE les actions des 4 boutons ================ //
  if((vivant) and not(enteringCodeMenu)) {
    if((checkButtonPress(1)) and (millis() - timeLastShot >= (1.0 - 0.7*rapidFire)*timeBetweenShots[weapon]))
      shoot();    
    else if(checkButtonPress(3))
      startReload();//changeWeapon();
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



void menuCode() {
  
  // Rouge
  if((longueurCodeTape < 16) and checkButtonPress(2)) {
    codeTape[longueurCodeTape] = 2;
    longueurCodeTape++;
    needRefreshScore = true;
  }

  // Noir
  if((longueurCodeTape < 16) and checkButtonPress(3)) {
    codeTape[longueurCodeTape] = 3;
    longueurCodeTape++;
    needRefreshScore = true;
  }

  // Bleu
  if((longueurCodeTape < 16) and checkButtonPress(0)) {
    codeTape[longueurCodeTape] = 0;
    longueurCodeTape++;
    needRefreshScore = true;
  }

  // Validation
  if(checkButtonPress(1)) {
    bool same = (longueurCodeTape == 11);
    for(int i=0; i<longueurCodeTape; i++) {
      if(code[i] != codeTape[i]) same = false;
    }

    if(same) {
      inAdminMenu = true;
      inCodeMenu = false;
      longueurCodeTape = 0;
    }
    else {
      admin = false;
      inAdminMenu = false;
      inCodeMenu = false;
      longueurCodeTape = 0;
    }

    needRefreshAll = true;
  }
}


void stopperPartie() {
  started = false;

  // Reset cheats and rights
  admin = false;
  invincible = false;
  munitions_illim = false;
  rapidFire = false;

  // ARRETER LA PARTIE
  EEPROM.write(100, 0);

  // ADMIN rights
   EEPROM.write(189, 0); // false

   // Cheats
   EEPROM.write(190, invincible);
   EEPROM.write(191, munitions_illim);
   EEPROM.write(192, rapidFire);

   EEPROM.commit();
  needRefreshAll = true;
}

void stopAll() { 
  myData.idMessage = 30;
  myData.value = 2;
  uint8_t adressGuns[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
  
  for(int j=0; j<redondanceEnvoi; j++) {
    for(int p=1; p<=9; p++) {
      if(p != player) {
        adressGuns[5] = p; 
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
      }
      verifierConnexion();
    }
  }

  stopperPartie();
}

void menuAdmin() {
  do {
    if(connectedCibleAvant or connectedCibleArriere) { if(chooseBinary("Deconnecter ?", false)) deconnecter(); }
    if((started) and admin) { if(chooseBinary("Stop all ?", false)) stopAll();  }
    if(started) { if(chooseBinary("Stop individuel ?", false)) stopperPartie(); }
    if(chooseBinary("Devenir admin ?", admin)) {
        admin = true;
        EEPROM.write(189, 1);
        EEPROM.commit();
    }
    if (admin) { if(chooseBinary("Use cheats ?", false)) chooseCheats(); }
  } while (!(chooseBinary("Quitter menu ?", true)));

  needRefreshAll = true;
  inAdminMenu = false;
  inCodeMenu = false;
  enteringCodeMenu = false;
}


void chooseCheats() {
  r_timeRespawn      = 1000 *  chooseValue("Duree de la mort", r_timeRespawn/1000, 1, "s");
  r_nbRespawns       = 1    *  chooseValue("Nb. de respawns ", r_nbRespawns, 1, "");
  r_nbMortsDefinitives = 1   * chooseValue("Nb. morts defs  ", r_nbMortsDefinitives, 1, "");
  bool new_light_on           =        chooseBinary("Gilet allumé ?", light_on);
  if(new_light_on != light_on) { sendParametersCibles(); sendSingleTimeParametersCibles(); light_on = new_light_on; }
  invincible           =        chooseBinary("Invincible ?", invincible);
  munitions_illim           =        chooseBinary("Mun. illimités ?", munitions_illim);
  rapidFire           =        chooseBinary("Tir rapide ?", rapidFire);
  if(started) { if(chooseBinary("OS adversaires ?", false)) doom(); }

  
   vie = r_viesMax;
   respawnsRestants = min(r_nbRespawns, uint8_t(254)) + 1;
   if(munitions_illim) munitions = munitionsMax[weapon];

   // ADMIN rights
   EEPROM.write(189, 1); // true

   // Cheats
   EEPROM.write(190, invincible);
   EEPROM.write(191, munitions_illim);
   EEPROM.write(192, rapidFire);

   // Regles
   EEPROM.write(193, r_nbRespawns);
   EEPROM.write(194, r_nbMortsDefinitives);
   EEPROM.write(195, r_timeRespawn);
   EEPROM.write(197, light_on);

   // Etat partie
   EEPROM.write(198, vie);
   EEPROM.write(199, respawnsRestants);
   EEPROM.write(200, nb_fois_pistolet_eteint);

   EEPROM.commit();
   needRefreshAll = true;
}


void doom() {
  myData.idMessage = 60;
  myData.value = hue;
  uint8_t adressGuns[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};
  
  for(int j=0; j<redondanceEnvoi; j++) {
    for(int p=1; p<=9; p++) {
      if(p != player) {
        adressGuns[5] = p; 
        esp_now_send(adressGuns, (uint8_t *) &myData, sizeof(myData));
        delay(delaiEnvoi);
      }
      verifierConnexion();
    }
  }
}
