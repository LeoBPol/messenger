#ifndef SERVEUR_H   /* Include guard */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <dirent.h>
#include <ctype.h>
#include <fcntl.h>

#define SERVEUR_H

struct CLIENT{
    int dSC;
    int dSF;
    char pseudo[100];
    pthread_t thread;
    char salon[100];
};

struct SALON {
    char nom_salon[100];
    char description[200];
    int nb_connecte;
    int capacite;
    int admin; // 1 si on autorise la suppression/modification du salon , 0 sinon
};

/* GENERAL METHODS */

/**
* @brief fonction qui envoie un message
* @param socket, buffer
* @return -1 si erreur envoi 0 si pas d'envoi et 1 en cas de succes
*/
int envoi(int socket, char* buffer);

/**
* @brief fonction qui permet d'obtenir le numero associe a la commande tapee 
* @param command
* @return le numero associe a la commande tapee
*/
int command_id(char* command);

/**
* @brief fonction qui gere le lien entre le client et tous les autres
* @param *args correspond au client
* 
*/
void *transmission(void *args);

/**
* @brief fonction main qui initialise tout le serveur
* 
* 
*/
int main(int argc, char* argv[]);

/**
* @brief fonction qui ouvre le readme
* 
* 
*/
void open_readme();

/**
* @brief fonction qui retourne le nombre d'arguments minimum lies a une fonction
* @param msg[], mini
* @return nombre arguments minimum
*/
int arg_minimum(char msg[],int mini);

/* USERS METHODS */

/**
* @brief fonction qui permet de supprimer un client
* @param *c : client a supprimer
* 
*/
void supprimer_client(struct CLIENT *c);

/* FILES METHODS */

/**
* @brief fonction qui permet de telecharger un fichier
* 
* 
*/
void* get_file();

/**
* @brief fonction qui transfere un fichier envoye par le premier client a l'autre client
* @param CLIENT *src, CLIENT *dest, file_name[], message[]
* 
*/
void transmit_file(struct CLIENT *src, struct CLIENT *dest, char file_name[], char message[]);

/* ROOMS METHODS */

/**
* @brief fonction qui retourne l'id du salon
* @param nom_salon[] : nom du salon
* @return l'id du salon
*/
int recherche_tab_salon(char nom_salon[]);

/**
* @brief fonction qui initialise tous les salons enregistrés à partir d'un fichier
*/
void init_salon();

/**
* @brief fonction qui permet de retirer un salon de la liste des salons du fichier
* @param nom_salon[] : nom du salon
* 
*/
void save_salon(char nom_salon[]);

/**
* @brief fonction qui permet de retirer un salon de la liste des salons du fichier
* @param nom_salon[] : nom du salon
* 
*/
void unsave_salon(char nom_salon[]);

/**
* @brief fonction qui sauvegarde la liste des derniers messages envoyés
* @param last_message[], nom_salon[] 
* 
*/

void save_last_messages(char last_message[], char nom_salon[]);

/**
* @brief fonction qui recupere les 10 derniers messages d'un salon
* @param last_messages[] : 10 derniers messages, nom_salon[] : nom du salon
* @return le nombre de messages
*/
int get_last_messages(char* last_messages[], char nom_salon[]);

/**
* @brief fonction qui permet d'ajouter un utilisateur dans un salon
* @param CLIENT *c, nom_salon[]
* 
*/
void *add_to_salon(struct CLIENT *c, char nom_salon[]);

/**
* @brief fonction qui permet de retirer un utilisateur d'un salon
* @param CLIENT *c, nom_salon[]
* 
*/
void *remove_from_salon(struct CLIENT *c, char nom_salon[]);

/**
* @brief fonction qui retourne si le salon a bien été cree ou les erreurs si ce n'est pas le cas 
* @param nom_salon[], capa, description[], admin
* @return 0 or -1 or -2 (en fonction de l'erreur)
*/
int nouveau_salon(char nom_salon[], int capa, char description[],int admin);

/**
* @brief fonction qui retourne si le salon a bien ete modifie ou les erreurs si ce n'est pas le cas 
* @param nom_salon[] : nom du salon a modifie
* @return 0 or -1 or -2 (en fonction de l'erreur)
*/
int modif_salon(char salon_base[],char new_nom_salon[], int new_capa, char new_description[]);

/**
* @brief fonction qui retourne si le client a bien rejoint le salon ou les erreurs si ce n'est pas le cas 
* @param nom_salon[] : nom du salon a rejoindre
* @return 0 or -1 or -2 (en fonction de l'erreur)
*/
int rejoindre_salon(struct CLIENT *c,char nom_salon[]);

/**
* @brief fonction qui retourne si le salon a bien été supprime ou les erreurs si ce n'est pas le cas 
* @param nom_salon[] : nom du salon a supprimer
* @return 0 or -1 or -2 (en fonction de l'erreur)
*/
int supprime_salon(char nom_salon[]);

#endif // SERVEUR_H
