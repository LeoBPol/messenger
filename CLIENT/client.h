#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>

#define CLIENT_H

#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ASCII_ESC 27

#define MAX_BUF 1048576
#define S_PORT  6398

/* FONCTION DE SAISIE DE MESSAGES A ENVOYER */
/**
* @brief fonction qui demande au client de saisir son message et qui le récupère
* @param mot : le mot saisie par le client
*/
void saisie(char *mot);

/* FONCTION DE RÉCUPÉRATION DES FICHIERS A ENVOYER */
/**
* @brief fonction qui récupère la liste des fichiers disponible à l'envoie pour le client (stocké dans le dossier "file_to_send")
*/
void getFile();

/* FONCTION D'ENVOIE DE MESSAGES */
/**
* @brief fonction qui envoie un fichier au serveur
* @param file_name : nom du fichier à envoyer
*/
void *envoi_file(char file_name[]);

/* FONCTION RECEPTION DE FICHIER */
/**
* @brief fonction qui recoie un fichier du serveur
* @param file_name : nom du fichier reçu
*/
void *recevoir_file(char file_name[]);

/* FONCTION D'ENVOIE DE MESSAGES */
/**
* @brief fonction qui envoie les messages du serveur
*/
void *envoie();

/* FONCTION DE RECEPTION DE MESSAGES */
/**
* @brief fonction qui recoie les messages du serveur
*/
void *recoie(void* args);

/**
* @brief fonction main
*/
int main(int argc, char* argv[]);