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
int envoi(int socket, char* buffer);
int command_id(char* command);
void *transmission(void *args);
int main(int argc, char* argv[]);
void open_readme();
int arg_minimum(char msg[],int mini);

/* USERS METHODS */
void supprimer_client(struct CLIENT *c);

/* FILES METHODS */
void* get_file();
void transmit_file(struct CLIENT *src, struct CLIENT *dest, char file_name[], char message[]);

/* ROOMS METHODS */
int recherche_tab_salon(char nom_salon[]);
void init_salon();
void save_salon(char nom_salon[]);
void unsave_salon(char nom_salon[]);
void save_last_messages(char last_message[], char nom_salon[]);
void get_last_messages(char* last_messages[], char nom_salon[]);
void *add_to_salon(struct CLIENT *c, char nom_salon[]);
void *remove_from_salon(struct CLIENT *c, char nom_salon[]);
int nouveau_salon(char nom_salon[], int capa, char description[],int admin);
int modif_salon(char salon_base[],char new_nom_salon[], int new_capa, char new_description[]);
int rejoindre_salon(struct CLIENT *c,char nom_salon[]);
int supprime_salon(char nom_salon[]);

#endif // SERVEUR_H