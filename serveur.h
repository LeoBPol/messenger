#ifndef SERVEUR_H   /* Include guard */
#include <sys/socket.h>
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
#define SERVEUR_H

struct CLIENT{
    int dSC;
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
int command_id(char* command);
void *transmission(void *args);
int main(int argc, char* argv[]);

/* USERS METHODS */
void supprimer_client(struct CLIENT *c);

/* FILES METHODS */
const char* get_file();

/* ROOMS METHODS */
int recherche_tab_salon(char nom_salon[]);
void *add_to_salon(struct CLIENT *c, char nom_salon[]);
void *remove_from_salon(struct CLIENT *c, char nom_salon[]);
void *nouveau_salon(char nom_salon[], int capa, char description[],int admin);
void *modif_salon(char salon_base[],char new_nom_salon[], int new_capa, char new_description[]);
void *rejoindre_salon(struct CLIENT *c,char nom_salon[]);
void *supprime_salon(char nom_salon[]);

#endif // SERVEUR_H