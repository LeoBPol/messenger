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
/mp [destinataire] [message]  
/file [nomFichier]  
/getFile  
/getFile [nomfichier]  
/salonList  
/salon [message]  
/newSalon [nomSalon] [capacite] [description]  
/modifChannel [nomSalon][nouveauNomSalon][nouvelleCapacite] [nouvelleDescription]  
/delChannel [nomSalon]  
/joinSalon [nomSalon]  
/fin  
