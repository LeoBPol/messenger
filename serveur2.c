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

const char command_list[3][20] = {"/mp", "/whoishere", "/fin"};

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

int command_id(char* command){
	int id_command = 0;
	while (id_command < sizeof(command_list)/sizeof(command_list[0])){
		if(strcmp(command, command_list[id_command]) == 0){
			return id_command;
		}
		id_command ++;
	}
	return -1;
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

	char message_recu[TMAX];

	printf("Thread de transmission pour le client %d\n", i);

	//char message_recu_decompose[3][TMAX];

	char mot[TMAX];
	char pseudo[100];
	char command[100];
	int fin = 0;
	char d[] = " ";

	int id_command;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(1){

		int clientID = -1;

		/* RECEPTION DU PSEUDO DU DESTINATAIRE */
		int mes = recv(users[i].dSC, &message_recu, sizeof(message_recu), 0);

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

		/* RECUPERATION DE LA COMMANDE SAISIE */
		char *p = strtok(message_recu, d);
		if (p == NULL){
			strcpy(command, message_recu);
		} else {
			strcpy(command, p);
		}

		printf("command : %s\n", command);

		printf("id command = %d\n", command_id(command));

		id_command = command_id(command);

		switch (id_command){
			case 0:
				/* RECUPERATION DU PSEUDO DU DESTINATAIRE */
				p = strtok(NULL, d);
				strcpy(pseudo, p);

				/* RECUPERATION DU MESSAGE SAISIE */
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(mot, p);
					strcat(mot, " ");
				    p = strtok(NULL, d);
				}
				printf("pseudo : %s\n", pseudo);
				printf("mot : %s\n", mot);
				
				int pseudo_id = nbClientDisconnected;
				for (;pseudo_id < nbClient;++pseudo_id){
					if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
						clientID = pseudo_id;
					}
				}
				/* SI LE PSEUDO RECU EST "all" (ON ENVOI LE MESSAGE À TOUT LES CLIENTS) */
				if(strcmp(pseudo,"all")==0){

					pseudo_id = nbClientDisconnected;
					for (;pseudo_id < nbClient;pseudo_id++){

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
					printf("MESSAGES TRANSMIS\n\n");
				
				} else if(clientID != -1){

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
				break;

			case 1:
				send(users[i].dSC, "serveur", sizeof("serveur"), 0);

				char pseudos[65000] = "[";

				/* ENVOIE DES PSEUDOS AU CLIENT */
				pseudo_id = nbClientDisconnected;
				for (;pseudo_id < nbClient;pseudo_id++){
					char temp_pseudo[100] = "";
					strcpy(temp_pseudo, users[pseudo_id].pseudo);
					strcat(pseudos, strcat(temp_pseudo, "] [")); 
				}
				pseudos[strlen(pseudos) - 1] = ' ';

				send(users[i].dSC, &pseudos, sizeof(pseudos), 0);

				clientID = -1;

				printf("LISTE PSEUDO TRANSMISE\n\n");
				break;

			case 2:
				close(users[i].dSC);
				printf("%s (%d) est déconnecté\n", users[pseudo_id].pseudo, i+1);

				int sem = sem_post(&semaphore);
				if(sem == -1){
					perror("A problem occured (sem wait)");
				}
				
				pthread_cancel(users[pseudo_id].thread);
				nbClientDisconnected++;
				break;

			default :
				printf("default statement\n");
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