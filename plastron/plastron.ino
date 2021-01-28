#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>


#define AVANT 1
#define ARRIERE 2





 


#define NUM_LEDS 3//9
CRGB leds[NUM_LEDS];
const uint16_t ledPin = 4;
uint8_t hue = 0;
uint8_t hueBase = 0;

#define TIME_TO_CONFIRM_CONNECTION 5000 // Temps qu'a le pistolet pour confirmer la connexion
#define BLEEP_TIME_WAITING_TO_CONNECT 400 // Temps entre chaque changement d'état en attentant la connexion (la période vaut donc x2)
#define BLEEP_TIME_WAITING_TO_CONFIRM_CONNECTION 140 // Temps entre chaque changement d'état en attentant la confirmation de la connexion (la période vaut donc x2)
#define TIME_TURN_IN_GAME 500 // Temps pour faire un tour complet des leds lorsqu'une partie est en cours

long timeConnectionProcessStarted = 0;
uint8_t adressPistolet[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};
//uint8_t adressAutreCible[] = {0x8C, 0xAA, 0xB5, 0x78, 0x78, 0x8C};
uint8_t adressAutreCible[] = {0x84, 0xCC, 0xA8, 0x84, 0x3D, 0x18};
uint8_t addressReceived[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};
bool connected = false;
bool tryingToConnect = false;
uint8_t typeCible = ARRIERE; // AVANT
bool ledBlinkState = false;

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

long timeBeingShot = 1;
long timeTolerance = 250;
bool beingShot = false;
long timeShootingStarted = 0;
long delai = 20;
long lastShootingRecorded = 0;
long timeTurn = 500;
int currentLed = 0;
long lastUpdateLed = 0;
int nbLed = 2;

IRrecv irrecv(kRecvPin);

decode_results results;

#include <ESP8266WiFi.h>
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




void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  memcpy(&addressReceived, mac, sizeof(addressReceived));

  Serial.println("-> Wifi message received");
  Serial.print("- id = ");
  Serial.println(myData.idMessage);
  Serial.print("- value = ");
  Serial.println(myData.value, HEX);

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


  if(cestLePistolet)
    Serial.println("- expediteur: pistolet");
  else if(cestLautreCible)
    Serial.println("- expediteur: autre cible");
  else if(cestUnAutrePistolet)
    Serial.println("- expediteur: un autre pistolet");
    
  
  if (myData.idMessage == 2) { // Un pistolet envoie une confirmation de connexion
    Serial.print("CONFIRMATION NEW CONNECTION... ");

    // On vérifie que l'on attend toujours une connexion
    if((not(connected)) and tryingToConnect) {
      Serial.print("Timing correct... ");

      // On vérifie que c'est bien l'addresse MAC que l'on attendait
      // Si c'est la bonne adresse alors la connexion est bonne
      if(cestLePistolet) {
        Serial.println("Adresse correcte !");

        if(myData.value == 1) {
          connected = true;
          tryingToConnect = false;

          if(typeCible == AVANT) {
            // On informe l'autre cible de la connexion
            myData.idMessage = 3; // ID pour connecter l'autre cible
            // On transforme l'adresse MAC du pistolet en entier
            // Attention on envoie l'adresse dans le sens inverse
            uint64_t value = 0;
            for(int i=3; i<6; i++)
              value += pow(16,2*i-6) * adressPistolet[i];
            myData.value = uint32_t(value); // adresseMAC
            esp_now_send(adressAutreCible, (uint8_t *) &myData, sizeof(myData));
          }
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
      uint32_t message = myData.value;
      // On récupère l'adresse MAC du pistolet
      for(int i=0; i<3; i++) {
        uint32_t adressPart = uint32_t(uint32_t(message % uint32_t(pow(16,2+2*i)) / pow(16,2*i)));
        adressPistolet[3+i] = uint8_t(adressPart);
        message -= adressPart * pow(16,2*i);
      }

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
  

  else if (myData.idMessage == 20) { // Le pistolet indique de changer de couleur
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet)
      hue = uint8_t(myData.value);
  }


  else if (myData.idMessage == 30) { // Le pistolet indique que l'état de la la partie a changé
    // On vérifie que c'est bien l'addresse MAC que l'on attendait
    if(cestLePistolet) {
      if((myData.value == 1) and not(partieEnCours))
        partieEnCours = true;
      else if((myData.value == 2) and (partieEnCours))
        partieEnCours = false;
    }     
  }
}


// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {}


void setup() {
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
  for(int sat=255; sat>100; sat-=5) {
    for(int i=0; i<NUM_LEDS; i++)
      leds[i] = CHSV(0,sat,255);
  
    FastLED.show();
    delay(3);
  }
  turnOff();
  vivant = false;
  digitalWrite(led2, LOW);
}

/*
void mourir() {
  vivant = false;
  analogWrite(led, 0);
  lastDeath = millis();
}
*/

void revivre() {
  vivant = true;
  beingShot = false;
  turnOn();
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



void touche(uint32_t message) {
  // Les messages du laser game finissent par 0x66
  // Si ce n'est as le cas, on ignore le message
  uint32_t checkLaserGame = message % uint32_t(pow(16,2));
  Serial.println("TOUCHE");
  
  if (checkLaserGame == uint32_t(0x66)) {
    message -= checkLaserGame;
    Serial.println("Laser Game checked");

    uint8_t address[] = {0xF4, 0xCF, 0xA2, 0x00, 0x00, 0x00};

    for(int i=0; i<3; i++) {
      uint32_t adressPart = uint32_t(uint32_t(message % uint32_t(pow(16,4+2*i)) / pow(16,2+2*i)));
      address[5-i] = uint8_t(adressPart);
      message -= adressPart * pow(16,2+2*i);
    }

    if((not(connected)) and (typeCible == AVANT)) {
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
      // On vérifie que l'on ne se soit pas tiré dessus
      bool differentAdress = false;
      for(int i=3; i<6; i++) {
        if(address[i] != adressPistolet[i]) 
          differentAdress = true;
      }

      if(differentAdress) {
        // On envoie au pistolet qu'on a été touché
        myData.idMessage = 10; // ID pour dire qu'on a été touché
        myData.value = typeCible; // 1 = cible avant, 2 = cible arriere
        esp_now_send(adressPistolet, (uint8_t *) &myData, sizeof(myData));

        // On envoie à l'autre cible qu'on a été tué
        myData.idMessage = 11; // ID pour dire qu'on est mort
        myData.value = 0; // PAS DE VALEUR
        esp_now_send(adressAutreCible, (uint8_t *) &myData, sizeof(myData));

        // Enfin on fait mourir cette cible
        mourir();
      }
    }
  }
}

void loop() {
  if(not(connected)) {
    hue = hueBase;

    bool change = false;
    if(not(tryingToConnect))  {
      if(millis() - lastUpdateLed >= BLEEP_TIME_WAITING_TO_CONNECT) {
        ledBlinkState = not(ledBlinkState);
        change = true;
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
        if(typeCible == AVANT)
          turnOn();
        pinMode(led3, HIGH);
      }
      else {
        if(typeCible == AVANT)
          turnOff();
        pinMode(led3, LOW);
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
    pinMode(led3, HIGH);

    if(vivant) {
      // On fait "tourner" les LEDs
      if ((millis() - lastUpdateLed) > timeTurn/float(NUM_LEDS)) {
        leds[numeroLed(currentLed)] = CHSV(0,255,255);
        currentLed--;
        if (currentLed < 0)
          currentLed = NUM_LEDS - 1;
        for(int i=currentLed-nbLed+1; i<=currentLed; i++)
          leds[numeroLed(i)] = CHSV(0,0,0);
        FastLED.show();
        lastUpdateLed = millis();
      }
      
      if (irrecv.decode(&results)) {
        serialPrintUint64(results.value, HEX);
        Serial.println("");
        touche(results.value);
        irrecv.resume();  // Receive the next value
      }  
    }
    else {
      while (irrecv.decode(&results)) {
        irrecv.resume();
      }
    }

  
  }
  delay(delai);
}
