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
};

/* CRÉATION DE L'UTILISATEUR */
struct CLIENT users[100];

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

/* FONCTION DE TRANSMISSION D'UN MESSAGE D'UN CLIENT VERS L'AUTRE */
void *transmission(void *args){

	/* NUMÉRO DU CLIENT QUI ENVOIE LE MESSAGE */
	int i = (long) args;
	char pseudo[100];

	printf("Thread de transmission pour le client %d\n", i);

	char mot[TMAX];
	int fin = 0;
	int nb_octets;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(1){
		
		int clientID = -1;

		/* RECEPTION DU PSEUDO DU DESTINATAIRE */
		recv(users[i].dSC, &pseudo, sizeof(pseudo), 0);
		strtok(pseudo, "\n");
		int pseudo_id = nbClientDisconnected;
		for (;pseudo_id < nbClient;++pseudo_id){
			if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
				clientID = pseudo_id;
			}
		}

		/* RECEPTION TAILLE DU MESSAGE DU CLIENT 1 */
		int mes = recv(users[i].dSC, &nb_octets, sizeof(int), 0); 

		/* GESTION DES ERREURS DE LA RECEPTION DE LA TAILLE DU MESSAGE */
		if (mes<0){
			perror("Erreur reception taille C1vC2\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			pthread_exit(NULL);
		}

		/* BOUCLE POUR RECEVOIR L'INTEGRALITE DU MESSAGE */
		int nb_recu = 0;
		while(nb_recu<nb_octets){
			mes = recv(users[i].dSC, mot, nb_octets*sizeof(char), 0);
			if (mes<0){
				perror("Erreur reception mot C1vC2\n");
				pthread_exit(NULL);
			}
			if (mes==0){
				perror("Socket fermée reception mot C1vC2\n");
				pthread_exit(NULL);
			}
			nb_recu+=mes;
		}

		printf("Mot reçu : %s\n", mot);

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

			printf("Début whoishere\n");

			char pseudos[100][100];

			/* ENVOIE DES PSEUDOS AU CLIENT */
			int pseudo_id = nbClientDisconnected;
			for (;pseudo_id < nbClient;++pseudo_id){
				if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
					printf("%s\n", users[pseudo_id].pseudo);
					
				}
			}

			send(users[i].dSC, users[pseudo_id].pseudo, sizeof(100*100), 0);

			/* ENVOIE DES PSEUDOS AU CLIENT */
			//mes = send(users[i].dSC, users[i].pseudo, sizeof(100), 0);

			printf("Fin whoishere\n");

			//send(users[i].dSC, users[i].pseudo, sizeof(100), 0);
			
		} else if(clientID != -1){

			char pseudoToSend[100];
			strcpy(pseudoToSend, users[i].pseudo); 

			/* ENVOIE DU PSEUDO AU DESTINATAIRE */
			send(users[clientID].dSC, &users[i].pseudo, sizeof(100), 0);


			/* ENVOIE DE LA TAILLE DU MESSAGE A ENVOYER */
			mes = send(users[clientID].dSC, &nb_octets, sizeof(int), 0);

			/* GESTION DES ERREURS DE L'ENVOIE DE LA TAILLE DU MESSAGE */
			if (mes<0){
				perror("Erreur transmission taille C1vC2\n");
				pthread_exit(NULL);
			}
			if (mes==0){
				perror("Socket fermée recption taille C1vC2\n");
				pthread_exit(NULL);
			}

			/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
			mes = send(users[clientID].dSC, mot, nb_octets, 0);


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