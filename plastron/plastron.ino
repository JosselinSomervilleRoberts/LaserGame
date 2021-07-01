#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>


#define AVANT 1
#define ARRIERE 2





 


#define NUM_LEDS 9
CRGB leds[NUM_LEDS];
const uint16_t ledPin = 4;
uint8_t hue = 0;
uint8_t hueBase = 0;

#define TIME_TO_CONFIRM_CONNECTION 5000 // Temps qu'a le pistolet pour confirmer la connexion
#define BLEEP_TIME_WAITING_TO_CONNECT 400 // Temps entre chaque changement d'état en attentant la connexion (la période vaut donc x2)
#define BLEEP_TIME_WAITING_TO_CONFIRM_CONNECTION 140 // Temps entre chaque changement d'état en attentant la confirmation de la connexion (la période vaut donc x2)
#define TIME_TURN_IN_GAME 500 // Temps pour faire un tour complet des leds lorsqu'une partie est en cours

#define TIME_WAIT_MAX_CHECK_CONNECTION 750

long timeConnectionProcessStarted = 0;
uint8_t adressPistolet[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};
uint8_t addressPistoletSecondaire[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};
uint8_t addressSelf[] = {0x8C, 0xAA, 0xB5, 0x78, 0x78, 0x8C};
uint8_t adressAutreCible[] = {0x84, 0xCC, 0xA8, 0x84, 0x3D, 0x18};
uint8_t addressReceived[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};
bool connected = false;
bool hasBeenConnected = false;
bool tryingToConnect = false;
bool tryingToReconnect = false;
uint8_t typeCible = AVANT; // AVANT
bool ledBlinkState = false;
uint32_t lastConnectionCheck = 0;
bool equipeChosen = false;
bool besoinDeFaireAnimationMort = false;
bool light_on = true;

// For led chips like WS2812, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
// Clock pin only needed for SPI based chipsets when not using hardware SPI





#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

// An IR detector/demodulator is connected to GPIO pin 14(D5 on a NodeMCU
// board).
// Note: GPIO 16 won't work on the ESP8266 as it does not have interrupts.
const uint16_t kRecvPin = 2;
//const uint16_t led = 4;

bool vivant = true;
long lastDeath = 0;
long timeRespawn = 1000;
bool partieEnCours = false;
long delai = 20;

long timeTurn = 500;
int currentLed = 0;
long lastUpdateLed = 0;
int nbLed = 2;

IRrecv irrecv(kRecvPin);

decode_results results;

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#define EEPROM_SIZE 25
#include <espnow.h>


const uint8_t led1 = 14;
const uint8_t led2 = 12;
const uint8_t led3 = 13;

// REPLACE WITH RECEIVER MAC Address

uint8_t id = 0;

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  uint8_t idMessage;
  uint32_t value;
} struct_message;

// Create a struct_message called myData
struct_message myData;





void deconnecter() {
  // On déconnecte en mettant toutes les variabels a False
  connected = false;
  hasBeenConnected = false;
  tryingToConnect = false;
  tryingToReconnect = false;
  lastConnectionCheck = 0;
  timeConnectionProcessStarted = 0;

  // On efface l'adresse du pistolet
  for(int i=0; i<6; i++)
    EEPROM.write(12+i, 0);
  EEPROM.commit();
  adressPistolet[5] = 0;
}


void turnOn() {
  for(int i=0; i<NUM_LEDS; i++)
    leds[i] = CHSV(hue,255,255);

  FastLED.show();
}

void turnOff() {
  for(int i=0; i<NUM_LEDS; i++)
    leds[i] = CHSV(0,0,0);

  FastLED.show();
}


void mourir() {
  Serial.println("MOURIR");
  besoinDeFaireAnimationMort = true;
  vivant = false;
  digitalWrite(led2, LOW);
  Serial.println("MORT TERMINEE");
}


void animationMort() {
  for(int sat=255; sat>180; sat-=3) {
    for(int i=0; i<NUM_LEDS; i++)
      leds[i] = CHSV(hue,sat,255);
  
    FastLED.show();
    delay(6);
  }
  turnOff();
  besoinDeFaireAnimationMort = false;
}


void revivre() {
  vivant = true;
  if(light_on)
    turnOn();
  else
    turnOff();
  lastUpdateLed = millis();
  currentLed = NUM_LEDS-1;
  digitalWrite(led2, HIGH);

  // On vide le buffer de messages IR pour ne pas mourir instantanément
  while (irrecv.decode(&results)) {
    irrecv.resume();
  }
}

int numeroLed(int i){
  if (i<0)
    return i+NUM_LEDS;
  else if (i>=NUM_LEDS)
    return i-NUM_LEDS;
  return i;
}


void checkRestoreConnection() { 
  // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
  esp_now_add_peer(adressPistolet, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  myData.idMessage = 4; // ID pour dire qu'on demande a se connecter
  myData.value = hueBase; // on envoie la couleur de la cible
  esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));

  tryingToReconnect = true;

  myData.idMessage = 13; // ID pour dire qu'on demande si l'autre cible est connectée
  myData.value = 0; //(pas de valeur);
  esp_now_send(adressAutreCible, (uint8_t *) &myData, sizeof(myData));
}



void touche(uint32_t message) {
  // Les messages du laser game finissent par 0x66
  // Si ce n'est as le cas, on ignore le message
  uint32_t checkLaserGame = message % uint32_t(pow(16,3));
  Serial.println("TOUCHE");

  if(connected)
    checkLaserGame = uint32_t(0x866);
    
  if (checkLaserGame == uint32_t(0x866)) {
    message -= message % uint32_t(pow(16,3));
    uint32_t uintAddress = message;
    Serial.println("Laser Game checked");

    uint8_t address[] = {0x32, 0x32, 0x32, 0x32, 0x32, 0x00};


    uint8_t nbBytesMAC = 1;
    for(int i=0; i<nbBytesMAC; i++) {
      uint32_t adressPart = uint32_t(uint32_t(message % uint32_t(pow(16,10-2*nbBytesMAC+2*i)) / pow(16,8-2*nbBytesMAC+2*i)));
      address[5-i] = uint8_t(adressPart);
      message -= adressPart * pow(16,8-2*nbBytesMAC+2*i);
    }

    if((not(connected)) and (typeCible == AVANT) and not(hasBeenConnected)) {
      // On copie l'adresse du pistolet
      for(int i=0; i<6; i++)
        adressPistolet[i] = address[i];
        
      // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
      esp_now_add_peer(adressPistolet, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
      myData.idMessage = 1; // ID pour dire qu'on demande a se connecter
      myData.value = hueBase; // on envoie la couleur de la cible
      esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));

      // On commence à compter, on a 10s pour confirmer la connexion
      tryingToConnect = true;
      timeConnectionProcessStarted = millis();
    }
    else if(connected) {
      // On envoie au pistolet qu'on a été touché
      myData.idMessage = 10; // ID pour dire qu'on a été touché
      myData.value = uintAddress + typeCible; // 1 = cible avant, 2 = cible arriere
      esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));
    }
  }
}


void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  memcpy(&addressReceived, mac, sizeof(addressReceived));

  // On regarde si c'est le pistolet qui nous envoie un message
  bool cestLePistolet = true;
  for(int i=0; i<6; i++) {
    if(adressPistolet[i] != addressReceived[i])
      cestLePistolet = false;
  }

  // On regarde si c'est l'autre cible qui nous envoie un message
  bool cestLautreCible = true;
  for(int i=0; i<6; i++) {
    if(adressAutreCible[i] != addressReceived[i])
      cestLautreCible = false;
  }

  // On regarde si c'est un autre pistolet (normalement toutes les addresses MAC des pistolets commencent par les 6 même bytes
  bool cestUnAutrePistolet = not(cestLePistolet);
  for(int i=0; i<3; i++) {
    if(adressPistolet[i] != addressReceived[i])
      cestUnAutrePistolet = false;
  }

  if((myData.idMessage != 4) and (myData.idMessage != 5)) {
    Serial.println("-> Wifi message received");
    Serial.print("- id = ");
    Serial.println(myData.idMessage);
    Serial.print("- value = ");
    Serial.println(myData.value, HEX);

  if(cestLePistolet)
    Serial.println("- expediteur: pistolet");
  else if(cestLautreCible)
    Serial.println("- expediteur: autre cible");
  else if(cestUnAutrePistolet)
    Serial.println("- expediteur: un autre pistolet");
  }
    



  
  if (myData.idMessage == 2) { // Un pistolet envoie une confirmation de connexion
    Serial.print("CONFIRMATION NEW CONNEXION... ");

    // On vérifie que l'on attend toujours une connexion
    if((not(connected)) and (tryingToConnect or tryingToReconnect)) {
      Serial.print("Timing correct... ");

      // On vérifie que c'est bien l'addresse MAC que l'on attendait
      // Si c'est la bonne adresse alors la connexion est bonne
      if(cestLePistolet) {
        Serial.println("Adresse correcte !");

        if(myData.value == 1) {
          connected = true;
          hasBeenConnected = true;
          tryingToConnect = false;
          lastConnectionCheck = millis();

          // on enregistre l'adresse du pistolet
          for(int i=0; i<6; i++)
            EEPROM.write(12+i, adressPistolet[i]);
          EEPROM.commit();

          if(typeCible == AVANT) {
            // On informe l'autre cible de la connexion
            myData.idMessage = 3; // ID pour connecter l'autre cible
            // On transforme l'adresse MAC du pistolet en entier
            // Attention on envoie l'adresse dans le sens inverse
            uint64_t value = adressPistolet[5];
            myData.value = uint32_t(value); // adresseMAC
            esp_now_send(adressAutreCible, (uint8_t *) &myData, sizeof(myData));
          }

          if(not(light_on))
            turnOff();
        }
        else if(myData.value == 2) {
          tryingToConnect = false;
          connected = false;
        }
      }
      else
        Serial.println("L'adresse ne correspond pas.");
    }
    else
      Serial.println("Aucune tentative de connexion n'est en cours.");
  }


  else if (myData.idMessage == 3) { // L'autre cible indique de se connecter
    // On vérifie que c'est bien l'addresse MAC de l'autre cible
    if(cestLautreCible) {
      // On récupère l'adresse MAC du pistolet
      for(int i=0; i<6; i++)
        adressPistolet[i] = 0x32;

      adressPistolet[5] = uint8_t(myData.value);

      // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
      esp_now_add_peer(adressPistolet, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
      myData.idMessage = 1; // ID pour dire qu'on demande a se connecter
      myData.value = hueBase; // on envoie la couleur de la cible
      esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));

      // On commence à compter, on a 10s pour confirmer la connexion
      tryingToConnect = true;
      timeConnectionProcessStarted = millis();
      
    }
  }

  else if (myData.idMessage == 5) { // Le pistolet vérifie la connection
    // On renvoie une confirmation
    myData.idMessage = 6;
    esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));
    lastConnectionCheck = millis();
  }

  else if (myData.idMessage == 7) { // Demande de reconnexion de la part du pistolet
    if(hasBeenConnected) {
      // On ajoute le pistolet comme destinataire et on lui envoie une confirmation de connexion
      esp_now_add_peer(addressReceived, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
      myData.idMessage = 8; // ID pour dire qu'on demande a se connecter
      myData.value = hue;//hueBase; // on envoie la couleur de la cible
      esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
    
      tryingToReconnect = true;

      // BIZZARRE CA (ne correspond pas a la doc)
      /*
      if(equipeChosen) {
         myData.idMessage = 9; // ID pour dire qu'on demande a se connecter
        myData.value = hue; // on envoie la couleur de la cible
        esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
      }*/
    }
  }

  else if (myData.idMessage == 9) { // Le pistolet a validé l equipe
    equipeChosen = true;
    hue = myData.value;
  }

  else if (myData.idMessage == 11) { // Le pistolet indique de mourir
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet) {
      if(vivant)
        mourir();
    }
  }

  else if (myData.idMessage == 12) { // Le pistolet indique de revivre
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet) {
      if(not(vivant))
        revivre();
    }
  }

  else if (myData.idMessage == 13) { // L'autre cible demande si on a été connectée et la couleur
    myData.value = hue + pow(16,2)*hasBeenConnected + equipeChosen * pow(16,3),
    myData.idMessage = 14;
    esp_now_send(addressReceived, (uint8_t *) &myData, sizeof(myData));
  }

  else if (myData.idMessage == 14) { // L'autre cible réponds à la demande de si elle est connectée ou non
    equipeChosen = (myData.value >= uint32_t(pow(16,3)));
    hasBeenConnected = (myData.value % uint32_t(pow(16,3)) >= uint32_t(pow(16,2)));
    hue = uint8_t(myData.value);
  }
  

  else if (myData.idMessage == 20) { // Le pistolet indique de changer de couleur
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet)
      hue = uint8_t(myData.value);
  }


  else if (myData.idMessage == 31) { // Le pistolet indique que l'état de la la partie a changé
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet) {
      if((myData.value == 1) and not(partieEnCours))
        partieEnCours = true;
      else if((myData.value == 2) and (partieEnCours))
        partieEnCours = false;
    }     
  }

  else if (myData.idMessage == 45) { // Light on
    light_on = (myData.value == 1);
    if(equipeChosen) {
      if(light_on)
        turnOn();
      else
        turnOff();
    }
  }

  else if (myData.idMessage == 50) { // Le pistolet indique que l'état de la la partie a changé
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet)
      deconnecter();
  }
} 


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {}


void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  hueBase = EEPROM.read(25);
  hue = hueBase;
  typeCible = EEPROM.read(24);

  // Charge l'adresse MAC
  for(int i=0; i<6; i++) {
    addressSelf[i] = EEPROM.read(i);
    adressAutreCible[i] = EEPROM.read(6+i);
    adressPistolet[i] = EEPROM.read(12+i);
  }
  wifi_set_macaddr(STATION_IF, &addressSelf[0]);
  
  pinMode(led1, OUTPUT);
  digitalWrite(led1, HIGH);
  
  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);

  FastLED.addLeds<WS2812, ledPin, GRB>(leds, NUM_LEDS);
  pinMode(led2, OUTPUT);
  digitalWrite(led2, HIGH);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    for(int i=0; i<NUM_LEDS; i++)
      leds[i] = CHSV(100,255,255);

    FastLED.show();
    delay(2000);
  }
  else {
    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
    
    // Register peer
    esp_now_add_peer(adressAutreCible, ESP_NOW_ROLE_COMBO, 1, NULL, 0);
  }
  digitalWrite(led2, LOW);
  pinMode(led3, OUTPUT);
  checkRestoreConnection();
}

void loop() {
  // On vérifie que l'on recoit régulièrement des uptates du flingue
  if(connected and (millis() - lastConnectionCheck >= TIME_WAIT_MAX_CHECK_CONNECTION)) {
    connected = false;
    Serial.println("WIFI Connection timeout");

    if(not(equipeChosen))
      hue = hueBase;
  }

  
  if(not(connected)) {
    //hue = hueBase;

    bool change = false;
    if(not(tryingToConnect))  {
      if(millis() - lastUpdateLed >= BLEEP_TIME_WAITING_TO_CONNECT) {
        ledBlinkState = not(ledBlinkState);
        change = true;
        
        //checkRestoreConnection();
      }
    }
    else if(tryingToConnect) {
      if(millis() - lastUpdateLed >= BLEEP_TIME_WAITING_TO_CONFIRM_CONNECTION) {
        ledBlinkState = not(ledBlinkState);
        change = true;
      } 

      if(millis() - timeConnectionProcessStarted >= TIME_TO_CONFIRM_CONNECTION)
        tryingToConnect = false;
    }

    if(change) {
      if(ledBlinkState) {
        turnOn();
        digitalWrite(led3, HIGH);
      }
      else {
        if(typeCible == AVANT)
          turnOff();
        digitalWrite(led3, LOW);
      }
          
      lastUpdateLed = millis();
      }

    if (irrecv.decode(&results)) {
        serialPrintUint64(results.value, HEX);
        Serial.println("");
        if(typeCible == AVANT)
          touche(results.value);
        irrecv.resume();  // Receive the next value
      }
  }
  else { // On est connecté
    digitalWrite(led3, HIGH);

    if(vivant) {
      // On fait "tourner" les LEDs
      if(light_on) {
        if ((millis() - lastUpdateLed) > timeTurn/float(NUM_LEDS)) {
          leds[numeroLed(currentLed)] = CHSV(hue,255,255);
          currentLed--;
          if (currentLed < 0)
            currentLed = NUM_LEDS - 1;
          for(int i=currentLed-nbLed+1; i<=currentLed; i++)
            leds[numeroLed(i)] = CHSV(hue,0,0);
          FastLED.show();
          lastUpdateLed = millis();
        }
      }
      
      if (irrecv.decode(&results)) {
        serialPrintUint64(results.value, HEX);
        Serial.println("");
        touche(results.value);
        irrecv.resume();  // Receive the next value
      }  
    }
    else {
      if(besoinDeFaireAnimationMort)
        animationMort();
        
      while (irrecv.decode(&results)) {
        irrecv.resume();
      }
    }
  }
  
  delay(delai);
}
