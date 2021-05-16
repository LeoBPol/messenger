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
#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ALLOC 2
#define NB_CLIENT_MAX


const char command_list[12][20] = {"/mp", "/whoishere", "/fin", "/file", "/getFile", "/newSalon", "/infoSalon", "/listSalon", "/modifSalon", "/salon", "/joinSalon","/delSalon"};
const char path_folder_serv[20] = "./file_on_serv/";

/* STRUCTURE UTILISATEUR */ 
typedef struct CLIENT CLIENT;
struct CLIENT{
    int dSC;
    char pseudo[100];
    pthread_t thread;
    char salon[100];
};

/* STRUCTURE SALON */ 
typedef struct SALON SALON;
struct SALON {
    char nom_salon[100];
    char description[200];
    int nb_connecte;
    int capacite;
    int admin; // 1 si on autorise la suppression/modification du salon , 0 sinon
};

/* CRÉATION DE L'UTILISATEUR */
struct CLIENT* users;

/* CRÉATION DU TABLEAU DE SALONS */
struct SALON salons[10];

/* COMPTEUR CLIENT */
int nb_client = 0;

/* COMPTEUR SALON */
int nb_salon = 0;

/* DECLARATION DU SEMAPHORE */
sem_t semaphore;

/* DECLARATION DU MUTEX */

/* DECLARATION DU SOCKET D'ECOUTE */
int dSE;

/* FONCTION DE RÉCUPÉRATION DES FICHIERS DU SERVEUR */
const char* get_file(){
    struct dirent *dir;
    char* file_list = "";
    DIR *d = opendir(path_folder_serv); 
    if (d){
        while ((dir = readdir(d)) != NULL)
        {  
        	char list[TMAX];
            if(strcmp(dir->d_name,".")!=0  && strcmp(dir->d_name,"..") != 0){
            	strcat(list, "[");
            	strcat(list, dir->d_name);
            	strcat(list, "] ");
            }
            file_list = list;
        }
        if (strlen(file_list) == 0)
        {
        	char error[200] = "Pas de fichier stocké par le serveur à l'adresse suivante ";
        	strcat(error, path_folder_serv);
        	file_list = error;
        }
        closedir(d);
    }
    return file_list;
}

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

void supprimer_client(int i){
	int found = 0;
	int client_id = 0;

	pthread_t thread_to_stop;

	close(users[i].dSC);
	printf("%s (client %d) est déconnecté\n", users[i].pseudo, i+1);

	memcpy(&thread_to_stop, &users[i].thread, sizeof(users[i].thread));

	int client_to_move_id = 0;
	for(;client_id < nb_client; client_id++){
		if(users[client_id].dSC == users[i].dSC){
			client_to_move_id = client_id;
       	}
	}
	for (; client_to_move_id <  nb_client; client_to_move_id++){
		memmove(&users[client_to_move_id], &users[client_to_move_id + 1], sizeof(users[client_to_move_id + 1]));
    }

	int sem = sem_post(&semaphore);
	if(sem == -1){
		perror("A problem occured (sem wait)");
	}

	nb_client--;

	printf("nb_client : %d\n", nb_client);

	users = realloc(users, sizeof(CLIENT)*(nb_client+1));
	
	pthread_cancel(thread_to_stop);
}

int recherche_tab_salon(char nom_salon[]){
	int tmp = 0;
	while (tmp < nb_salon){
		if(strcmp(nom_salon, salons[tmp].nom_salon) == 0){
			return tmp;
		}
		tmp++;
	}
	return -1;
}

void *add_to_salon(struct CLIENT *c, char nom_salon[]){
	int nb_of_salon = recherche_tab_salon(nom_salon);
	if(nb_of_salon < 0 || nb_of_salon > sizeof(salons)/8){
		perror("Impossible de trouver ce salon");
	}
	if(salons[nb_of_salon].nb_connecte + 1 <= salons[nb_of_salon].capacite){
		strcpy(c->salon,nom_salon);
		salons[nb_of_salon].nb_connecte = salons[nb_of_salon].nb_connecte +1; // Ajoute un connecté au salon
	}
	else{
		perror("Le Salon est plein impossible de se connecter dedans");
	}

}

void *remove_from_salon(struct CLIENT *c, char nom_salon[]){
	int nb_of_salon = recherche_tab_salon(nom_salon);
	if(nb_of_salon < 0 || nb_of_salon > sizeof(salons)/8){
		perror("Impossible de trouver ce salon");
	}

	strcpy(c->salon,"");
	salons[nb_of_salon].nb_connecte = salons[nb_of_salon].nb_connecte -1; // Enlève un connecté au salon
	
}

void *nouveau_salon(char nom_salon[], int capa, char description[],int admin){

	if (nb_salon+1 < sizeof(salons)/sizeof*(salons)){
		struct SALON newSalon;

	    strcpy(newSalon.nom_salon, nom_salon);
	    strcpy(newSalon.description, description);
	    newSalon.nb_connecte = 0;
	    newSalon.admin = admin;

	    /* REMPLISSAGE AUTOMATIQUE DE LA CAPACITE */
	    int tmp_capa = 10;
	    if (capa <= 0 || capa > 200){
	        tmp_capa = 10;
	    } 
	    else{
	    	tmp_capa = capa;
	    }
	    newSalon.capacite = tmp_capa;

	    *(salons + nb_salon) = newSalon;
	    nb_salon++;
	}
	else{
		printf("Il y a trop de salons");
	}
}

void *modif_salon(char salon_base[],char new_nom_salon[], int new_capa, char new_description[]){

	int nb_of_salon = recherche_tab_salon(salon_base);

	if (nb_of_salon != -1){
		if(salons[nb_of_salon].admin == 1){
			strcpy(salons[nb_of_salon].nom_salon, new_nom_salon);
		    strcpy(salons[nb_of_salon].description, new_description);

		    /* REMPLISSAGE AUTOMATIQUE DE LA CAPACITE */
		    int tmp_capa = 0;
		    if (new_capa <= 0 || new_capa > 200 ){
		        tmp_capa = 10;
		    } 
		    else{
		    	tmp_capa = new_capa;
		    }
		    salons[nb_of_salon].capacite = tmp_capa;
		}
		else{
			printf("Vous n'avez pas les droits pour modifier ce salon\n");
		}
	}
    else{
    	printf("Le salon n'existe pas\n");
    }

}

void *rejoindre_salon(struct CLIENT *c,char nom_salon[]){

	int nb_of_salon = recherche_tab_salon(nom_salon);

	if (nb_of_salon != -1){
		if(salons[nb_of_salon].nb_connecte + 1 <= salons[nb_of_salon].capacite){
			remove_from_salon(c,c->salon);
    		add_to_salon(c,nom_salon);
    		printf("%s à rejoint le salon : %s\n",c->pseudo,nom_salon);
		}
		else{
			printf("Plus de place dans le salon\n");
		}
	}
    else{
    	printf("Le salon n'existe pas\n");
    }

}

void *supprime_salon(char nom_salon[]){

	int nb_of_salon = recherche_tab_salon(nom_salon);
	if (nb_of_salon != -1){
		if(salons[nb_of_salon].admin ==1){ // SI ON A LES DROITS DE SUPPRIMER LE SALON
		/* ON PLACE TOUT LES CLIENTS DU SALON QUE L'ON VEUT SUPPRIMER VERS LE GENERAL(PAR DEFAUT)*/	
		for(int j=0; j<nb_client;j++){
			if(strcmp(users[j].salon,nom_salon)==0){
				rejoindre_salon(&users[j],"General");
			}
		}
		/* ON SUPPRIME LE SALON VOULU ET ON DECALE TOUT LES SALONS SUIVANT D'UN EN ARRIERE*/	
		for(int k = nb_of_salon + 1; k < nb_salon; k++) {
	        salons[k - 1] = salons[k];
	    }
	    /* ON DECREMANTE LE NOMBRE DE SALONS */
	    nb_salon--; 
		}
		else{
			printf("Vous n'avez pas les droits pour supprimer ce salon");
		}
	}
    else{
    	printf("Le salon n'existe pas\n");
    }
}

/* FONCTION DE TRANSMISSION D'UN MESSAGE D'UN CLIENT VERS L'AUTRE */
void *transmission(void *args){

	/* NUMÉRO DU CLIENT QUI ENVOIE LE MESSAGE */
	int i = (long) args;

	char message_recu[TMAX];

	char mot[TMAX];
	char pseudo[100];
	char command[100];
	char third_arg[100];
	char fourth_arg[100];
	int fin = 0;
	char d[] = " ";

	char file_name[100];


	int id_command;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(1){

		int clientID = -1;
		char mot[TMAX] = "";

		/* RECEPTION DU MESSAGE DU DESTINATAIRE */
		int mes = recv(users[i].dSC, &message_recu, sizeof(message_recu), 0);

		if (mes<0){
			perror("Erreur reception mot C1vC2\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			perror("Socket fermée reception mot C1vC2\n");
			supprimer_client(i);
			pthread_exit(NULL);
		}

		/* RECUPERATION DE LA COMMANDE SAISIE */
		char *p = strtok(message_recu, d);
		if (p == NULL){
			strcpy(command, message_recu);
		} else {
			strcpy(command, p);
		}
		
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
				
				int pseudo_id = 0;
				for (;pseudo_id < nb_client;++pseudo_id){
					if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
						clientID = pseudo_id;
					}
				}
				/* SI LE PSEUDO RECU EST "all" (ON ENVOI LE MESSAGE À TOUT LES CLIENTS) */
				if(strcmp(pseudo,"all")==0){

					pseudo_id = 0;
					for (;pseudo_id < nb_client;pseudo_id++){

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
				} else {
					/* ENVOIE PSEUDO À LA SOURCE */
					send(users[i].dSC, "serveur", sizeof("serveur"), 0);

					char error[200] = "Message non distribué. L'utilisateur '";
					strcat(error, pseudo);
					strcat(error, "' n'est pas connecté.");

					/* ENVOIE ERREUR À LA SOURCE */
					send(users[i].dSC, error, sizeof(error), 0);
				}
				break;

			case 1:
				send(users[i].dSC, "serveur", sizeof("serveur"), 0);

				char pseudos[65000] = "[";

				/* ENVOIE DES PSEUDOS AU CLIENT */
				pseudo_id = 0;
				for (;pseudo_id < nb_client;pseudo_id++){
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
				supprimer_client(i);
				pthread_exit(NULL);
				break;

			case 3 :

				/* RECUPERATION DU PSEUDO DU DESTINATAIRE */
				p = strtok(NULL, d);
				strcpy(pseudo, p);

				/* RECUPERATION DU NOM DU FICHIER */
				p = strtok(NULL, d);
				strcpy(file_name, p);

				/* RECUPERATION DU CONTENU DU FICHIER */
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(mot, p);
					strcat(mot, " ");
				    p = strtok(NULL, d);
				}

				pseudo_id = 0;
				for (;pseudo_id < nb_client;++pseudo_id){
					if(strcmp(users[pseudo_id].pseudo, pseudo)==0){
						clientID = pseudo_id;
					}
				}

				if(clientID != -1){

					int dSC = users[clientID].dSC;

					printf("Envoi du pseudo vers dSC : %d\n", users[clientID].dSC);

					/* ENVOIE DU PSEUDO AU DESTINATAIRE */
					send(dSC, &users[i].pseudo, sizeof(users[i].pseudo), 0);

					char message[TMAX] = "/file ";
					strcat(message, file_name);
					strcat(message, " ");
					strcat(message, mot);

					printf("message : %s\n", message);

					/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
					int mes = send(dSC, message, sizeof(message), 0);

					/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}

					printf("FICHIER TRANSMIS\n\n");
				} else {
					/* ENVOIE PSEUDO À LA SOURCE */
					send(users[i].dSC, "serveur", sizeof("serveur"), 0);

					char error[200] = "Fichier non distribué. L'utilisateur '";
					strcat(error, pseudo);
					strcat(error, "' n'est pas connecté.");

					/* ENVOIE ERREUR À LA SOURCE */
					send(users[i].dSC, error, sizeof(error), 0);

					printf("ERREUR TRANSMISE\n\n");
				}

				break;

			case 4:

				/* ENVOIE PSEUDO A LA SOURCE */
				send(users[i].dSC, "serveur", sizeof("serveur"), 0);

				/* RECUPERATION DU NOM DU FICHIER */
				p = strtok(NULL, d);

				if (p == NULL){

					//printf("wshhhh 1\n");

					/* ENVOIE LISTE DES FICHIERS A LA SOURCE */
					char file_list_to_send[TMAX];

					strcpy(file_list_to_send, get_file());

					//printf("wshhhh 2\n");
					//printf("file_list : %s\n", file_list_to_send);

					//printf("wshhhh 3\n");
					//printf("size : %ld\n", strlen(file_list_to_send));
					send(users[i].dSC, file_list_to_send, strlen(file_list_to_send), 0);

					printf("LISTE FICHIER TRANSMISE\n\n");

				} else {
					char file_name[100];
					strcpy(file_name, p);
					strtok(file_name, "\n");
					printf("file_name : %s\n", file_name);

					char path_file[100] = "";
					strcpy(path_file, path_folder_serv);
					strcat(path_file, p);
					strtok(path_file, "\n");

					FILE *fps = fopen(path_file, "r");
					if (fps == NULL){
						printf("Ne peux pas ouvrir le fichier suivant : %s", path_file);
					}
					else {
						printf("Fichier ouvert : %s\n", path_file);
						char file_content[TMAX] = "";
						char str[1000] = "";

						/*RECUPERER LE CONTENU DU FICHIER*/
						while (fgets(str, 1000, fps) != NULL) {
							strcat(file_content, str);
						}	
						file_content[strlen(file_content)-1] = '\0';

						char message[TMAX] = "/file ";
						strcat(message, pseudo);
						strcat(message, " ");
						strcat(message, file_name);
						strcat(message, " ");
						strcat(message, file_content);

						/* ENVOIE DU MESSAGE */
						int mes = send(users[i].dSC, message, strlen(message), 0);

						/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
						if (mes<0){
							perror("Erreur envoie fichier\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée envoie fichier\n");
							pthread_exit(NULL);
						}

						printf("FICHIER TRANSMIS\n\n");
					}
				}
				break;

			case 5 : // CREER UN NOUVEAU SALON

				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(pseudo, p);	

				/* RECUPERATION DE LA CAPACITE DU SALON */
				p = strtok(NULL, d);
				strcpy(mot, p);
				int capa = atoi(mot);

				/* RECUPERATION DE LA DESCRIPTION DU SALON */
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(third_arg, p);
					strcat(third_arg, " ");
				    p = strtok(NULL, d);
				}
				third_arg [strlen(third_arg)-1] = 0;

				nouveau_salon(pseudo,capa,third_arg,1);
				memset (third_arg, 0, sizeof (third_arg));// Remise à 0 de third_arg

    			rejoindre_salon(&users[i],pseudo);

				int dSC = users[i].dSC;

				send(dSC,"serveur", sizeof("serveur"), 0);

				/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
				char creation_message[200] = "Votre salon a ete cree !\n";
				int mes = send(dSC, creation_message, sizeof(creation_message), 0);

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur transmission mot C1vC2\n");
					pthread_exit(NULL);						}
				if (mes==0){
					perror("Socket fermée transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				break; 
			case 6 :
				printf("Numéro du salon: %d \nNom du salon : %s\n",recherche_tab_salon(users[i].salon),salons[recherche_tab_salon(users[i].salon)].nom_salon);
				printf("%d clients sur %d dans %s\n",salons[0].nb_connecte,salons[0].capacite,salons[0].nom_salon);
				break;
			case 7 :
				strcpy(pseudos,"Serveur");
				for(int j=0; j<nb_salon; j++){
					char buffer[100];
					sprintf(buffer, "%d", j);
			    	strcat( mot, buffer);
			    	strcat( mot, " - ");
			    	strcat( mot, salons[j].nom_salon);
			    	strcat( mot, " [");
			    	strcat( mot, salons[j].description);
			    	strcat( mot, "] ");
			    	memset (buffer, 0, sizeof (buffer));
			    	sprintf(buffer, "%d", salons[j].nb_connecte);
			    	strcat( mot, buffer);
			    	strcat( mot, "/");
			    	memset (buffer, 0, sizeof (buffer));
			    	sprintf(buffer, "%d", salons[j].capacite);
			    	strcat( mot, buffer);
			    	strcat( mot, "\n");
					
				}
				printf("%s",mot);

				/* ENVOIE DU PSEUDO */
				send(users[i].dSC, &pseudos, sizeof(pseudos), 0);
				/* ENVOIE DU MESSAGE */
				mes = send(users[i].dSC, &mot, sizeof(mot), 0); 

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur envoie fichier\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée envoie fichier\n");
					pthread_exit(NULL);
				}

				printf("INFOS TRANSMISES\n\n");  
				break;
			case 8 : /* MODIFIER LE SALON VOULU avec /modifSalon */
				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(pseudo, p);

				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(mot, p);		

				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(third_arg, p);
				capa = atoi(third_arg);

				/* RECUPERATION DE LA DESCRIPTION DU SALON */
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(fourth_arg, p);
					strcat(fourth_arg, " ");
				    p = strtok(NULL, d);
				}
				modif_salon(pseudo,mot,capa,fourth_arg);
				strcpy(fourth_arg,"\0");
				break;
			case 9 : /* ENVOYER UN MESSAGE A QUELQU'UN DE SON SALON AVEC /salon */
				/* RECUPERATION DU MESSAGE SAISIE */
				memset (mot, 0, sizeof (mot));
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(mot, p);
					strcat(mot, " ");
				    p = strtok(NULL, d);
				}
				
				pseudo_id = 0;
				for (;pseudo_id < nb_client;pseudo_id++){
					if(strcmp(users[pseudo_id].salon,users[i].salon)==0){
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
					
				}
				break;
			case 10: /* REJOINDRE UN DE SON SALON AVEC /joinSalon */
				dSC = users[i].dSC;

				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(pseudo, p);

				rejoindre_salon(&users[i],pseudo);

				/* CREATION DU MESSAGE A ENVOYER*/
				strcpy(mot, "Vous avez rejoint le salon ");
				strcat(mot, pseudo);

				send(dSC, "Serveur",sizeof("Serveur"), 0);
				mes = send(dSC,&mot,sizeof(mot), 0);
				if (mes<0){
					perror("Erreur transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				break;
			case 11:
				/* RECUPERATION DU NOM DU SALON */
				p = strtok(NULL, d);
				strcpy(pseudo, p);

				supprime_salon(pseudo);
				//printf("Suppresion effectuée\n");

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

	users = (CLIENT *) malloc(sizeof(CLIENT)*2);

	/* CREATION DU PREMIER SALON */
 	nouveau_salon("General",100,"Salon par défaut",0);

 	printf("En attente des clients\n");

	while(1){

		int dSC;

		dSC = accept(dSE, (struct sockaddr*) &aC, &lg);

		if (nb_client > 1){
			users = (CLIENT *) realloc(users, sizeof(CLIENT)*(nb_client+1));
		}

		/* CONNEXION AVEC UN CLIENT */
		users[nb_client].dSC = dSC;

		if (users[nb_client].dSC<0){
			perror("Erreur de connexion avec le client");
			return -1;
		}

		/* CONNEXION AVEC UN CLIENT */
		sem = sem_wait(&semaphore);
		if(sem == -1){
			perror("A problem occured (sem wait)");
		}

		/* DEMANDER LE PSEUDO AU CLIENT */
		send(users[nb_client].dSC, "Entrez votre pseudo : ", sizeof("Entrez votre pseudo : "), 0);

		char pseudo_buffer[100];
		int sizeof_pseudo;

		/* RECEVOIR LA TAILLE DU PSEUDO DU CLIENT */
		recv(users[nb_client].dSC, &sizeof_pseudo, sizeof(int), 0);
		
		/* RECEVOIR LE PSEUDO DU CLIENT */
		recv(users[nb_client].dSC, &pseudo_buffer, sizeof_pseudo, 0);
		strcpy(users[nb_client].pseudo, pseudo_buffer);
		printf("Client %d connecté avec le pseudo : %s\n", nb_client+1, users[nb_client].pseudo);
		
		/* AJOUTER LE CLIENT AU GENERAL */
		add_to_salon(&users[nb_client],"General");

		/* AFFICHAGE DES CLIENTS CONNECTÉS  */
		/*for (int i = 0; i < nb_client+1; ++i){
			printf("%d : ", i);
			printstruct(users[i]);
		}*/

		pthread_join(users[nb_client].thread,NULL);

		/* CREATION DES THREADS */
		if( pthread_create(&users[nb_client].thread, NULL, transmission, (void *) (long) nb_client)){
			perror("Erreur à la création du thread de transmission entre le client 1 et le client 2 ");
			return EXIT_FAILURE;
		}

		nb_client++;

	}
	close(dSE);
	printf("Fin du programme\n");
	free(users);
	return 0;
}
