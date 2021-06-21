# LaserGame

<br>

## Auteur

Josselin SOMERVILLE ROBERTS

<br><br>

## Câblage

### Pistolet

| Pin | Valeur du pin dans Arduino | Nom de la variable | Composant                                                                                      |
| --- | -------------------------- | ------------------ | ---------------------------------------------------------------------------------------------- |
| D0  | 16                         | buttons[0]         | Bouton 1: **Gachette**                                                                         |
| D1  | 5                          | -                  | I2C Ecran                                                                                      |
| D2  | 4                          | -                  | I2C Ecran                                                                                      |
| D3  | 0                          | buttons[2]         | Bouton 3: **Noir**                                                                             |
| D4  | 2                          | buttons[1]         | Bouton 2: **Rouge**                                                                            |
| D5  | 14                         | laser              | Laser de visée                                                                                 |
| D6  | 12                         | kIrLed             | Led IR de tir                                                                                  |
| D7  | 13                         | df_player          | Pin d'envoi de données au DF Player (le pin de réception n'est pas utilisé par manque de pins) |
| D8  | 15                         | led                | Led de connexion                                                                               |
| A0  | A0                         | -                  | Bouton 0: **Bleu**                                                                             |

### Plastron

| Pin | Valeur du pin dans Arduino | Nom de la variable | Composant                         |
| --- | -------------------------- | ------------------ | --------------------------------- |
| D2  | 4                          | ledPin             | Bandeau Led                       |
| D4  | 2                          | krecvPin           | Récepteur IR                      |
| D5  | 14                         | led1               | Led **Rouge** *(plastron allumé)* |
| D6  | 12                         | led2               | Led **Verte** *(vivant)*          |
| D7  | 13                         | led3               | Led **Bleue** *(connexion)*       |

## <br><br>

## Communication

### Communication IR

Les pistolets doivent être identifiables à la fois par **leur adresse MAC** mais également par **le code IR qu'ils envoient**. Comme tous les pistolets partagent les mêmes 3 premiers octets pour leurs adresses MAC, il suffit d'envoyer que les 3 derniers. On peut facilement envoyer 4 octets en IR, on envoie donc les 3 derniers octets de l'adresse MAC du pistolet suivit de ***0x66*** permettant d'identifier qu'il s'agit d'un pistolet du jeu.**

<br>

### Communcation WiFi

#### Structure des messages

Les messages sont seulement composés de :

- l'adresse MAC de l'expéditeur

- un identifiant *(idMessage)*

- une valeur *(value - optionnelle)*
  
  

#### Liste des messages

| idMessage ***(uint8_t)*** | value ***(uint32_t)***                                                                                                | Expéditeur                  | Récepteur                   | Decription                                                                                                                                                                                                                                                                                       |
| ------------------------- | --------------------------------------------------------------------------------------------------------------------- | --------------------------- | --------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 1                         | Couleur **hue** de la cible                                                                                           | Cible avant                 | Pistolet                    | Demande de nouvelle connexion.                                                                                                                                                                                                                                                                   |
| 2                         | **Confirmation :** - 1=acceptée ; <br/>- 2=refusée                                                                    | Pistolet                    | Cible avant                 | **Réponse** à une demande de nouvelle connexion ***(id 1)***.                                                                                                                                                                                                                                    |
| 3                         | 3 derniers octets de l'adresse MAC du pistolet                                                                        | Cible avant                 | Cible arrière               | Indique à la cible secondaire de demander à se connecter à un pistolet (suite à une confirmation de connexion ***(id 2)***).                                                                                                                                                                     |
| 4                         | -                                                                                                                     | Cible avant / Cible arrière | Pistolet                    | Demande de reconnexion (notamment quand on rallume une cible).                                                                                                                                                                                                                                   |
| 5                         | Temps en secondes                                                                                                     | Pistolet                    | Cible avant / Cible arrière | Vérification que la connexion pistolet - cible est toujours active.                                                                                                                                                                                                                              |
| 6                         | Temps en seconde                                                                                                      | Cible avant / Cible arrière | Pistolet                    | **Réponse** à demande de vérification de la validité de la connexion ***(id 5)***. La valeur ens econdes sert à vérifier qu'on obtient bien la confirmation de la denière demande. (si après une demande à *t=17s* on obtient une confirmation pour *t=13s*, il y a un problème : *lag de 4s.*). |
| 7                         | -                                                                                                                     | Pistolet                    | Cible avant / Cible arrière | Demande de reconnexion (notamment quand onr allume le pistolet).                                                                                                                                                                                                                                 |
| 8                         | Couleur **hue** de la cible                                                                                           | Cible avant / Cible arrière | Pistolet                    | **Réponse** à la demande de reconnexion du pistolet ***(id 7)*** : la reconnexion ets acceptée.                                                                                                                                                                                                  |
| 9                         | Couleur **hue** de la cible                                                                                           | Pistolet                    | Cible avant / Cible arrière | Le pistolet a fait un choix d'équipe *(et donc les cibles doivent potentiellement changer de couleur)*.                                                                                                                                                                                          |
| 10                        | 3 derniers octets de l'adresse MAC du pistolet qui a touché puis un octer qui vaut 1 si c'est la cible avant, 2 sinon | Cible avant / Cible arrière | Pistolet                    | Cible touchée.                                                                                                                                                                                                                                                                                   |
| 11                        | -                                                                                                                     | Pistolet                    | Cible avant / Cible arrière | Mourir.                                                                                                                                                                                                                                                                                          |
| 12                        | -                                                                                                                     | Pistolet                    | Cible avant / Cible arrière | Revivre.                                                                                                                                                                                                                                                                                         |
| 20                        | Couleur **hue** de la cible                                                                                           | Pistolet                    | Cible avant / Cible arrière | Changement de couleur*(sans confirmation du choix d'équipe contrairement à **(id 9)**)*.                                                                                                                                                                                                         |
| 30                        | - 1: pour commencer<br/>- 2: pour finir                                                                               | Pistolet                    | Cible avant / Cible arrière | Changement d'état de la partie.                                                                                                                                                                                                                                                                  |
| 35                        | RANDOM JUNK ATM                                                                                                       | Pistolet touché             | Pistolet qui a touché       | Avoir tué quelqu'un.                                                                                                                                                                                                                                                                             |
| 40                        | Nombre de respawns max                                                                                                | Pistolet **principal**      | Autres pistolets            | **Changement de règle:** nombre de respawns max.                                                                                                                                                                                                                                                 |
| 41                        | Nombre de morts définitives max                                                                                       | Pistolet **principal**      | Autres pistolets            | **Changement de règle:** nombre de morts définitives max.                                                                                                                                                                                                                                        |
| 42                        | Temps de respawn ***(en secondes)***                                                                                  | Pistolet **principal**      | Autres pistolets            | **Changement de règle:** temps de respawn.                                                                                                                                                                                                                                                       |
| 43                        | Temps de jeu ***(en minutes)***                                                                                       | Pistolet **principal**      | Autres pistolets            | **Changement de règle:** temps d'une partie.                                                                                                                                                                                                                                                     |
| 44                        | - 0: Plastrons allumés<br/>- 1: Plastrons éteints                                                                     | Pistolet **principal**      | Autres pistolets            | **Changement de règle:** allumage des lumières des plastrons.                                                                                                                                                                                                                                    |
| 50                        | -                                                                                                                     | Pistolet                    | Cible avant / Cible arrière | Déconnexion.                                                                                                                                                                                                                                                                                     |

<br><br>

## Stockage des données

    Certaines données sont stockées dans la **mémoire EEPROM** afin d'être conservée même si la carte est éteinte. Cela sert notamment a restaurer la connexion si le pistolet ou une cible s'éteint et à s'assurer qu'un joueur ne triche pas en éteignant osn pistolet pour récupérer toute sa vie. Cela permet également de stocker les paramètres de la partie *(temps de respawn, nombre de respawns max, ...)*. Enfin certaines données ***statiques*** sont stockées dans cette mémoire *(id du pistolet, adresse MAC, numéro des musiques, ...)*. Cela permet de ne pas avoir à changer dans le programme certaines variables comme l'identifiant d'un pistolet et ainsi d'avoir un seul programme pour tous les pistolets et un autre pour toutes les cibles (avant et arrière).

    Le score était enregistré mais cette fonctionnalité n'est plus utilisée étant donnée que la comptabilisation du score est désactivée pour une meilleure réception des signaux IR.

    Il faut bien faire attention à ne pas enregistrer trop souvent des données dans la mémoire EEPROM car sa durée de vie est limitée (entre 100 000 et 1 000 000 d'enregistrements pour chaque octets).

<br>

### Pistolet

| Plage d'octets (inclus) | Variable stockée                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| ----------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 0 - 5                   | **Adresse MAC** du pistolet                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 6 - 11                  | **Adresse MAC** de la dernière cible avant connectée                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 12 - 17                 | **Adresse MAC** de la dernière cible arrière connectée                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            |
| 18 - 23                 | **Adresse MAC** du dernier pistolet secondaire connecté ***UNUSED***                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 24 - 29                 | **Adresse MAC** du pistolet de référence ***UNUSED***                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| 30                      | ID du pistolet                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    |
| 31                      | ID du pistolet principal *(référence)* ***UNUSED***                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| 98                      | **Etat partie :** Role *(principal / secondaire)*  ***UNUSED***                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| 99                      | **Etat partie :** ID de l'équipe                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| 100                     | **Etat partie :** Partie commencée ?                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| 101 - 172               | **Score :** Pattern x8 :<br/>- 101: Nombre de tirs sur le joueur 1 - cible avant<br/>- 102: Nombre de tirs sur le joueur 1 - cible avant - pistolet secondaire<br/>- 103: Nombre de tirs sur le joueur 1 - cible arrière<br/>- 104: Nombre de tirs sur le joueur 1 - cible arrière - pistolet secondaire<br/>- 105: Nombre de fois touché par joueur 1 cible avant<br/>- 106: Nombre de fois touché par joueur 1 cible avant - pistolet secondaire<br/>- 107: Nombre de fois touché par joueur 1 cible arrière<br/>- 108: Nombre de fois touché par joueur 1 cible arrière - pistolet secondaire<br/>***UNUSED*** |
| 193                     | **Règle :** Nombre de respawns max                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| 194                     | **Règle :** Nombre de morts définitives max                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| 195                     | **Règle :** Délai de respawn *(en secondes)*                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 196                     | **Règle :** Duréed'une partie *(en minutes)*                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 197                     | **Règle :** Lumières des plastons allumées ?                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| 198                     | **Etat partie :** Nombre de vies du joueur                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        |
| 199                     | **Etat partie :** Nombre de respawns restants du joueur                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| 200                     | **Etat partie :** Nombre de morts définitives restantes du joueur                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| 201 - 229               | Numéro des 28 musiques ***(la numérotation diffère pour certains DF players, d'ou l'intéret)***                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |

<br>

### Plastron

| Plage d'octets (inclus) | variable stockée                                                     |
| ----------------------- | -------------------------------------------------------------------- |
| 0 - 5                   | **Adresse MAC** de la cible                                          |
| 6 - 11                  | **Adresse MAC** de l'autre cible                                     |
| 12 - 17                 | **Adresse MAC** du dernier pistolet connecté                         |
| 18 - 23                 | **Adresse MAC** du dernier psitolet secondaire connecté ***UNUSED*** |
| 24                      | Type de la cible<br/>- 1 : AVANT<br/>- 2 : ARRIERE                   |
| 25                      | Couleur **hue**                                                      |

<br><br>

## Musiques et effets sonores

    Les effets sonores sont gérés par la carte DF Player qui lit les fichiers MP3 stockés sur la carte SD contenue dans le pistolet. 

    Pour une raison étonnante, certains DF Player ne lisent pas les fichiers dans le bon ordre (normalement *myDFPlayer.play(1)* est censé jouer le morceau *0001.wav* mais ce n'est pas le cas pour tous les modèles). C'est pour ça que l'ordre des musiques est stocké dans le mémoire EEPROM pour chaque pistolet.

    Voici la liste des effets sonores disponibles :

| Fichier *(.wav)* | Effet sonore                       |
| ---------------- | ---------------------------------- |
| 0001             | Tirer pistoler                     |
| 0002             | Tirer shotgun                      |
| 0003             | Tirer laser *(2 s)*                |
| 0004             | Tirer pistolet - plus de munitions |
| 0005             | Tirer shotgun - plus de munitions  |
| 0006             | Tirer laser - plus de munitions    |
| 0007             | Recharger pistolet                 |
| 0008             | Recharger shotgun                  |
| 0009             | Recharger laser                    |
| 0010             | Toucher ennemi                     |
| 0011             | Toucher allié                      |
| 0012             | Perdre une vie                     |
| 0013             | Mourir                             |
| 0014             | Revivre                            |
| 0015             | Gagner une vie                     |
| 0016             | Le médecin fait revivre quelqu'un  |
| 0017             | Mort définitive                    |
| 0018             | Début partie                       |
| 0019             | Fin partie                         |
| 0020             | Power up 1                         |
| 0021             | Power down 1                       |
| 0022             | Power up 2                         |
| 0023             | Power down 2                       |
| 0024             | Power up 3                         |
| 0025             | Power down 3                       |
| 0026             | Power up 4                         |
| 0027             | Power down 4                       |
| 0028             | Mort 2                             |
