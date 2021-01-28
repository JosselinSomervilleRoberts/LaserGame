# LaserGame

**Auteur**
Josselin SOMERVILLE ROBERTS


**Envoi de données IR**
Les pistolets doivent être identifiables à la fois par leur adresse MAC mais également le code IR qu'ils envoient. Comme tous les pistolets partagent les mêmes 3 premiers octets, il ne suffit d'envoyer que les 3 derniers. On peut facilement envoyer 4 octets en IR, on envoie donc les 3 derniers octets de l'adresse MAC du pistolet suivit de *0x66* permettant d'identifier qu'il s'agit d'un pistolet du jeu.

**Fonctionnement de l'envoi WiFi**
Les messages sont seulement composés de :
- idMessage *(uint8_t)* : identifiant permettant de savoir quel type de message est envoyé
- value *(uint32_t)* : *optionnel*, valeur venant compléter le message
- (l'adresse MAC de l'expéditeur, *envoyée automatiquement*)

Voici les différents types de messages :
- *idMessage = 1* : demande de nouvelle connexion (value: couleur *hue* de la cible)
- *idMessage = 2* : réponse à la demande de nouvelle connexion (value: 1: acceptée, 2: refusée)
- *idMessage = 3* : se connecter au pistolet (value: 3 derniers octets de l'adresse MAC du pistolet)
- *idMessage = 10* : cible touchée (value: 3 derniers octets de l'adresse MAC du pistolet qui a touché puis un octer qui vaut 1 si c'est la cible avant, 2 sinon)
- *idMessage = 11* : mourir
- *idMessage = 12* : revivre
- *idMessage = 20* : changer couleur cible (value: couleur *huer* de la cible)
- *idMessage = 30* : changement de l'état de la partie (value: 1 pour commencer, 2 pour finir)
