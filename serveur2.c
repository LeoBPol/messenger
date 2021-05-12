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
#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ALLOC 2
#define NB_CLIENT_MAX

/* TABLEAUX CONTENANT LES SOCKETS, LES PSEUDOS DES CLIENTS ET LES THREADS */
/*int dSC[100];
char pseudos[100][100];
pthread_t threads[100];*/

/* STRUCTURE UTILISATEUR */ 
typedef struct CLIENT CLIENT;
struct CLIENT{
    int dSC;
    char pseudo[100];
    pthread_t thread;
    int salon;
};

/* STRUCTURE SALON */ 
typedef struct SALON SALON;
struct SALON {
    char nom_salon[100];
    char description[200];
    int nb_present;
    int capacite;
};

/* CRÉATION DE L'UTILISATEUR */
struct CLIENT users[100];

/* CRÉATION DU SALON */
struct SALON salons[10];

/* TESTS */
int nbClient = 0;
int nbClientDisconnected = 0;

/* DECLARATION DU SEMAPHORE */
sem_t semaphore;

/* DECLARATION DU MUTEX */

/* DECLARATION DU SOCKET D'ECOUTE */
int dSE;

void printstruct(struct CLIENT c){
	printf("%d - %s\n", c.dSC, c.pseudo);
}

/*
void *nouveau_salon(int i){

    struct SALON salon;
    
    reception(tabClient[i].socketMes, newSalon.name);
    reception(tabClient[i].socketMes, newSalon.desc);
    newSalon.nbrClientPres = 0;
    int capa = 0;
    int rec = recv(tabClient[i].socketMes, &capa, sizeof(int), 0);
    if (rec == -1){
        perror("Erreur 1ere reception\n");
        exit(0);
    }
    if (rec == 0){
        perror("Socket fermée\n");
        exit(0);
    }
    if (capa < 0 || capa > 200){
        capa = 10; //je choisi une capacité maximal par défault
    }
    newSalon.capacityMax = capa;

    tabSalon[nbrSalon] = newSalon;
    nbrSalon = nbrSalon + 1; 
} */

/* FONCTION DE TRANSMISSION D'UN MESSAGE D'UN CLIENT VERS L'AUTRE */
void *transmission(void *args){

	/* NUMÉRO DU CLIENT QUI ENVOIE LE MESSAGE */
	int i = (long) args;

	char message_recu[200];
	char pseudo[100];


	printf("Thread de transmission pour le client %d\n", i);

	char mot[TMAX];
	int fin = 0;
	int nb_octets;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */

	while(1){

		int clientID = -1;

		/* RECEPTION DU PSEUDO DU DESTINATAIRE */
		int mes = recv(users[i].dSC, &message_recu, sizeof(message_recu), 0);

		while(strcmp(mes," ") == 1){
		if (mes<0){
			perror("Erreur reception mot C1vC2\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			perror("Socket fermée reception mot C1vC2\n");
			pthread_exit(NULL);
		}
		printf("message : %s", message_recu);
		strtok(message_recu, "\n");
		char *p = strtok(message_recu, " ");
		
		while(p != NULL)
		{
		    printf("'%s'\n", p);
		    p = strtok(NULL, " ");
		}
		
		int pseudo_id = nbClientDisconnected;
		for (;pseudo_id < nbClient;++pseudo_id){
			if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
				clientID = pseudo_id;
			}
		}
		/* SI LE MOT RECU EST "fin" */
		if(strcmp(mot,"fin\n")==0){
			close(users[i].dSC);
			printf("%s (%d) est déconnecté\n", users[pseudo_id].pseudo, i+1);

			int sem = sem_post(&semaphore);
			if(sem == -1){
				perror("A problem occured (sem wait)");
			}
			
			pthread_cancel(users[pseudo_id].thread);
			nbClientDisconnected++;
		}

		/* SI LE MOT RECU EST "whoishere" */
		if(strcmp(mot,"whoishere\n")==0){

			send(users[i].dSC, "serveur", sizeof("serveur"), 0);

			char pseudos[65000] = "[";

			/* ENVOIE DES PSEUDOS AU CLIENT */
			int pseudo_id = nbClientDisconnected;
			for (;pseudo_id < nbClient;pseudo_id++){
				char temp_pseudo[100] = "";
				strcpy(temp_pseudo, users[pseudo_id].pseudo);
				strcat(pseudos, strcat(temp_pseudo, "] [")); 
			}
			pseudos[strlen(pseudos) - 1] = ' ';

			send(users[i].dSC, &pseudos, sizeof(pseudos), 0);

			clientID = -1;
			
		} /* SI LE PSEUDO RECU EST "all" (ON ENVOI LE MESSAGE À TOUT LES CLIENTS) */
		else if(strcmp(pseudo,"all")==0){
			char char_nb_octet[10];
			sprintf(char_nb_octet, "%d", nb_octets);
			char pseudoToSend[100];
			
			int pseudo_id = nbClientDisconnected;
			for (;pseudo_id < nbClient;pseudo_id++){
				strcpy(pseudoToSend, users[i].pseudo); 
				int dSC = users[pseudo_id].dSC;

				/* ENVOIE DU PSEUDO AU DESTINATAIRE */
				send(dSC, &users[i].pseudo, sizeof(users[i].pseudo), 0);

				/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
				int mes = send(dSC, mot, sizeof(mot), 0);

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
			}
		
		} else if(clientID != -1){


			char char_nb_octet[10];
			sprintf(char_nb_octet, "%d", nb_octets);

			char pseudoToSend[100];
			strcpy(pseudoToSend, users[i].pseudo); 
			int dSC = users[clientID].dSC;

			printf("Envoi du pseudo vers dSC : %d\n", users[clientID].dSC);

			/* ENVOIE DU PSEUDO AU DESTINATAIRE */
			send(dSC, &users[i].pseudo, sizeof(users[i].pseudo), 0);

			/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
			int mes = send(dSC, mot, sizeof(mot), 0);

			/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
			if (mes<0){
				perror("Erreur transmission mot C1vC2\n");
				pthread_exit(NULL);
			}
			if (mes==0){
				perror("Socket fermée transmission mot C1vC2\n");
				pthread_exit(NULL);
			}

			printf("MESSAGE TRANSMIS\n\n");
		}
		
	}
	}
	
	printf("La discussion est terminée\n");
	printf("En attente des clients\n");

	pthread_exit(NULL);
}

int main(int argc, char* argv[]){

	/* INITIALISATION DU SÉMAPHORE  */
	int sem = sem_init(&semaphore, 0, 10);
	if(sem == -1){
	    perror("Problème à l'intitialisation du sémaphore ");
	}

	/* CREATTION DU SOCKET ET VERIFICATION */
	dSE = socket(PF_INET, SOCK_STREAM, 0);
	if (dSE<0){
		perror("Erreur à la création du socket d'écoute ");
		return -1;
	}

	/* NOMMAGE DU SOCKET ET VERIFICATION */
	struct sockaddr_in adServ;
	adServ.sin_family = AF_INET;
	adServ.sin_addr.s_addr = INADDR_ANY;
	adServ.sin_port = htons((short)atoi(argv[1]));
	int verif = bind(dSE, (struct sockaddr*)&adServ, sizeof(adServ));
	if (verif<0){
		perror("Erreur au nommage du socket d'écoute ");
		return -1;
	}

	/* MISE EN ECOUTE DES CONNEXIONS ENTRANTES ET VERIFICATION */
	verif = listen(dSE,7);
	if (verif<0){
		perror("Erreur lors de la mise à l'écoute du socket ");
		return -1;
	}
	struct sockaddr_in aC;
	socklen_t lg = sizeof(struct sockaddr_in);

	printf("En attente des clients\n");

	while(1){
		struct CLIENT user;
	
		/* CONNEXION AVEC UN CLIENT */
		users[nbClient] = user;
		users[nbClient].dSC = accept(dSE, (struct sockaddr*) &aC, &lg);
		if (users[nbClient].dSC<0){
			perror("Erreur de connexion avec le client");
			return -1;
		}

		/* CONNEXION AVEC UN CLIENT */
		sem = sem_wait(&semaphore);
		if(sem == -1){
			perror("A problem occured (sem wait)");
		}

		/* DEMANDER LE PSEUDO AU CLIENT */
		send(users[nbClient].dSC, "Entrez votre pseudo : ", sizeof("Entrez votre pseudo : "), 0);

		char pseudo_buffer[100];
		int sizeof_pseudo;
		/* RECEVOIR LA TAILLE DU PSEUDO DU CLIENT */
		recv(users[nbClient].dSC, &sizeof_pseudo, sizeof(int), 0);
		
		/* RECEVOIR LE PSEUDO DU CLIENT */
		recv(users[nbClient].dSC, &pseudo_buffer, sizeof_pseudo, 0);
		//memcpy(users[nbClient].pseudo, pseudo_buffer, sizeof(users[nbClient].pseudo));
		strcpy(users[nbClient].pseudo, pseudo_buffer);
		printf("Client %d connecté avec le pseudo : %s\n", nbClient+1, users[nbClient].pseudo);
		
		/* AFFICHAGE DES CLIENTS CONNECTÉS  */
		/*for (int i = 0; i < nbClient+1; ++i){
			printf("%d : ", i);
			printstruct(users[i]);
		}*/

		/* CREATION DES THREADS */
		if( pthread_create(&users[nbClient].thread, NULL, transmission, (void *) (long) nbClient)){
			perror("Erreur à la création du thread de transmission entre le client 1 et le client 2 ");
			return EXIT_FAILURE;
		}

		nbClient++;
	}
	close(dSE);
	printf("Fin du programme\n");
	return 0;
}