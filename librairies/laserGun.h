#ifndef LASERGUN
#define LASERGUN

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <HardwareSerial.h>

class LaserGun
{
	public:
	
		LaserGun();
		void define_pins();
		
		void initiate();  // initialize everything
		void init_lcd();  // Set up the display
		void init_df_player();  // Set up the mp3 player
		void init_wifi();  // Set up the wifi connections and connect to other guns
		void init_data();  // Read the data to get the player id and things like that
		void init_player(); // Gives the player the appropriate number of lives and respawns
	
		void receiveData(uint8_t * mac, uint8_t idMessage, uint8_t value);
	
	private:
		uint8_t idPlayer = 0;
		bool mainGun = false;
		
		// WiFi Related variables
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
		
		// Music
		int soundShoot[] = {1,2,3};
		int soundReload[] = {7,8,9};
		int soundEmpty[] = {4,5,6};
		
		// Display
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
		
};

#endif // LASERGUN