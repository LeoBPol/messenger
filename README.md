# Messenger

## Organisation des fichiers
Le projet s'articule autour de dossier principaux : SERVEUR et CLIENT.  
Autour de ces dossiers se trouvent un fichier pour compiler le serveur et le client nommé compil.sh et ce README.md  
Un fichier permettant de compiler le client et le serveur nommé "compil.sh"  

Dans le dossier SERVEUR : 
* les fichiers sources (serveur.h et serveur.c)
* un fichier compilé (serveur)
* un dossier "serveur_config" qui contient tous les fichiers stockant les messages, un readme.txt contenant le résultat de la commande "/man" et le dossier "file_on_serv". Ce dossier contient les fichiers que le client pour demander grâce à la commande "/getfile [nomfichier]"  
* un fichier permettant de compiler nommé "compilServeur.sh"

Dans le dossier CLIENT : 
* le fichier source (client.c)
* un fichier compilé (client)
* un dossier "file_received" contenant les fichiers reçu par le client
* un dossier "file_to_send" contenant les fichiers envoyé par le client
* un fichier permettant de compiler nommé "compilClient.sh"

## Utilisation
Pour exécuter le projet :

Ouvrir le terminal
* Placer vous dans le dossier "messenger" si vous voulez exécuter le serveur et le client sur la machine
* Placer vous dans le dossier "CLIENT" si vous voulez exécuter le client sur la machine
* Placer vous dans le dossier "SERVEUR" si vous voulez exécuter le serveur sur la machine

### Serveur 
* Donner les droits d'exécution sur le fichier si ce n'est pas le cas avec la commande "chmod 100 compilServeur.sh"
* Exécuter le fichier "compil.sh" avec la commande "./compilServeur.sh"
* Exécuter le fichier "serveur" avec la commande "./serveur PORT"
  
### Client 
* Donner les droits d'exécution sur le fichier si ce n'est pas le cas avec la commande "chmod 100 compilClient.sh"
* Exécuter le fichier "compil.sh" avec la commande "./compilClient.sh"
* Exécuter le fichier "client" avec la commande "./client ADDRESSE_IP_SERVEUR PORT" (vous pouvez la trouver en tapant la commande "ipconfig")
  
### Les deux
* Donner les droits d'exécution sur le fichier si ce n'est pas le cas avec la commande "chmod 100 compil.sh"
* Exécuter le fichier "compil.sh" avec la commande "./compil.sh"
* Exécuter le fichier "serveur" avec la commande "./serveur PORT"
* Exécuter le fichier "client" avec la commande "./client ADDRESSE_IP_SERVEUR PORT"

## Commandes
VOICI TOUTES LES COMMANDES DISPONIBLES SUR LE SERVEUR.

/whoishere  
LE SERVEUR VOUS ENVOIE TOUT LES CLIENTS CONNECTÉ SUR CELUI-CI  
  
/mp [destinataire] [message]  
ENVOI UN MESSAGE A UN DESTINATAIRE  
SI VOUS ÉCRIVEZ ALL POUR LE DESTINATAIRE CELA ENVERRA UN MESSAGE À TOUTES LES PERSONNES  
PRESENTENT SUR LE SERVEUR.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU DESTINATAIRE ET ÉVITEZ LES INSULTES.  
  
/file [pseudo] [nomFichier]  
VOUS ENVOYEZ LE FICHIER VOULU AU DESTINATAIRE VOULU.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU NOM DE FICHIER ET DU CLIENT.  
  
/getFile  
LE SERVEUR VOUS ENVOI LA LISTE DE TOUT LES FICHIERS PRÉSENTS SUR LE SERVEUR.  
  
/getFile [nomFichier]  
VOUS RECUPEREZ LE FICHIER VOULU.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU NOM DE FICHIER.  
  
/salonList  
LE SERVEUR VOUS ENVOIE LA LISTE DE TOUT LES SALONS PRÉSENTS SUR LE SERVEUR.  
  
/infoSalon  
LE SERVEUR VOUS ENVOIE TOUTES LES INFORMATIONS DU SALON DANS LEQUEL VOUS ETES.  
  
/salon [message]  
VOUS ENVOYEZ UN MESSAGE À TOUS LES CLIENTS CONNECTÉS SUR VOTRE SALON.  
  
/newSalon [nomSalon] [capacite] [description]  
VOUS AJOUTEZ UN SALON VOULU.  
ATTENTION À BIEN RESPECTER LE NOMBRE D'ARGUMENTS À RENTRER EN PARAMÈTRE.  
LE NOM DU SALON S'ÉCRIT SANS ESPACE.  
  
/modifChannel [nomSalon] [nouveauNomSalon] [nouvelleCapacite] [nouvelleDescription]  
VOUS MODIFIEZ LE SALON VOULU.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU SALON ET DE VÉRIFIER  
QUE VOUS AYEZ LES DROITS SINON LE SALON NE SE MODIFIERA PAS.  
  
/delSalon [nomSalon]  
VOUS SUPPRIMEZ LE SALON VOULU.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU SALON ET DE VÉRIFIER  
QUE VOUS AYEZ LES DROITS SINON LE SALON NE SE SUPPRIMERA PAS.  
  
/joinSalon [nomSalon]  
VOUS REJOIGNEZ LE SALON VOULU.  
ATTENTION À BIEN RESPECTER L'ORTHOGRAPHE DU SALON.  
  
/fin  
VOUS QUITTEZ LE SERVEUR.  
  
AVEC:  
destinataire : chaîne de caractère  
message : chaîne de caractère  
nomFichier : chaîne de caractère  
nomSalon : chaîne de caractère  
capacite : int  
description: chaîne de caractère  
nouveauNomSalon : chaîne de caractère  
nouvelleCapacite : int  
nouvelleDescription : chaîne de caractère  
