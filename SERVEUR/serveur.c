#include "serveur.h"
#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ALLOC 2
#define NB_CLIENT_MAX 10

#define MAX_BUF 1048576
#define S_PORT  6398

const char command_list[13][20] = {"/mp", "/whoishere", "/fin", "/file", "/getfile", "/newsalon", "/infosalon", "/listsalon", "/modifsalon", "/salon", "/joinsalon", "/delsalon", "/man"};
const char path_folder_serv[30] = "serveur_config/file_on_serv/";
const char path_folder_channel_list[40] = "serveur_config/channelList.txt";
const char path_folder_channel_messages[20] = "serveur_config/";
const char path_folder_readme[40] = "serveur_config/readme.txt";
const char pseudo_serveur[30] = "\033[1;31m[SERVEUR]\033[1;37m";

char port[5];
char addr[15];

/* STRUCTURE UTILISATEUR */ 
typedef struct CLIENT CLIENT;

/* STRUCTURE SALON */ 
typedef struct SALON SALON;

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

char d[] = " ";
/* DECLARATION DU MUTEX */

/* DECLARATION DU SOCKET D'ECOUTE */
int dSE;
int dSE_f;


void * strtolower(char * src) {
	const char * dest = src; 
    char * result = src;
    while( *src++ = tolower( *dest++ ) );
    return result;
}

/* FONCTION PERMETTANT DE COMPTER LE NOMBRE D'ARGUMENTS */
int arg_minimum(char msg[],int mini){
	int tmp = 0;
	for(int i=0;i<strlen(msg);i++){
		if(msg[i] == ' '){			
			tmp++;
		}
	}
	return tmp+1>= mini;
}

void open_readme(char file_content[]){
	char file_name[300];
	strcpy(file_name, path_folder_readme);
	strcpy(file_content, "");

	printf("file_name : %s\n", file_name);

	FILE* fps = fopen(file_name, "r");

	if (fps == NULL){
		printf("Ne peux pas ourvrir le fichier à l'emplacement suivante : %s", file_name);
	} else {
		char str[1000] = "";

		/*RECUPERER LE CONTENU DU FICHIER*/
		while (fgets(str, 1000, fps) != NULL) {
			strcat(file_content, str);
		}

		printf("Fichier %s ouvert\n", file_name);
	}
	fclose(fps);
}

/* FONCTION DE RÉCUPÉRATION DES FICHIERS DU SERVEUR */
void* get_file(char *file_list){
	strcpy(file_list, "");
    struct dirent *dir;
    DIR *d = opendir(path_folder_serv); 
    if (d){
        while ((dir = readdir(d)) != NULL)
        {  
            if(strcmp(dir->d_name,".")!=0  && strcmp(dir->d_name,"..") != 0){
            	strcat(file_list, "[");
            	strcat(file_list, dir->d_name);
            	strcat(file_list, "] ");
            }
        }
        if (strlen(file_list) == 0)
        {
        	char error[200] = "Pas de fichier stocké par le serveur à l'adresse suivante ";
        	strcat(error, path_folder_serv);
        	file_list = error;
        }
        closedir(d);
    }
}

void transmit_file(struct CLIENT *src, struct CLIENT *dest, char file_name[], char message[]){
	int count;
	char buf[MAX_BUF];


	struct sockaddr_in aCF;
	socklen_t lg = sizeof(struct sockaddr_in);

	if(src->dSF == -1){
		src->dSF = accept(dSE_f, (struct sockaddr*)&aCF, &lg);
	}

	/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
	int mes = envoi(dest->dSC, message);

	/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
	if (mes<0){
		perror("Erreur transmission mot C1vC2\n");
		pthread_exit(NULL);
	}
	if (mes==0){
		perror("Socket fermée transmission mot C1vC2\n");
		pthread_exit(NULL);
	}

	if(dest->dSF == -1){
		dest->dSF = accept(dSE_f, (struct sockaddr*)&aCF, &lg);
	}

	
	int fd;

	printf("Fichier du client reçu nommé : %s\n", file_name);

	//fd = open(file, O_WRONLY | O_CREAT, 0666);
	while ((count = read(src->dSF, buf, MAX_BUF))>0)
	{
		write(dest->dSF, buf, count);
	}
	if (count == -1)
	{
		perror("Read error");
		exit(1);
	} else {
		printf("Fichier envoyé\n");
	}
	close(dest->dSF);
	dest->dSF = -1;
	src->dSF = -1;
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

void supprimer_client(struct CLIENT *c){
	int client_id = 0;

	pthread_t thread_to_stop;
	remove_from_salon(c,c->salon);
	close(c->dSC);
	printf("%s s'est déconnecté\n", c->pseudo);

	memcpy(&thread_to_stop, &c->thread, sizeof(c->thread));

	int client_to_move_id = 0;
	for(;client_id < nb_client; client_id++){
		if(users[client_id].dSC == c->dSC){
			client_to_move_id = client_id;
       	}
	}

	for (; client_to_move_id <  nb_client; client_to_move_id++){
		printstruct(users[client_to_move_id]);
		memmove(&users[client_to_move_id], &users[client_to_move_id + 1], sizeof(users[client_to_move_id + 1]));
    }

	int sem = sem_post(&semaphore);
	if(sem == -1){
		perror("A problem occured (sem wait)");
	}

	nb_client--;

	printf("nb_client : %d\n", nb_client);

	if (nb_client > 0){
		users = (CLIENT *) realloc(users, sizeof(CLIENT)*(nb_client));
	} else {
		free(users);
	}
	
	pthread_cancel(thread_to_stop);
	pthread_exit(NULL);
}


/* FONCTIONS SALON */
void save_salon(char nom_salon_a_sauv[]){

	FILE* fps = fopen(path_folder_channel_list, "a");

	if (fps == NULL){
		printf("Ne peux pas ouvrir le fichier à l'emplacement suivante : %s", path_folder_channel_list);
	} else {
		printf("Fichier %s modifié en ajoutant le salon : %s\n", path_folder_channel_list, nom_salon_a_sauv);
		strcat(nom_salon_a_sauv, "\n");
		fputs(nom_salon_a_sauv, fps);
	}
	fclose(fps);
}

void unsave_salon(char nom_salon[]){

	char d[] = "\n";
	char *channel_list[10];
	int nb_channel = 0;

	FILE* fps = fopen(path_folder_channel_list, "r");

	if (fps == NULL){
		printf("Ne peux pas ourvrir le fichier à l'emplacement suivante : %s", path_folder_channel_list);
	} else {
		char file_content[TMAX] = "";
		char str[1000] = "";

		/*RECUPERER LE CONTENU DU FICHIER*/
		while (fgets(str, 1000, fps) != NULL) {
			strcat(file_content, str);
		}

		char *p = strtok(file_content, d);
		while(p != NULL){
			if (strcmp(p, nom_salon) != 0){
				channel_list[nb_channel] = p;
				nb_channel++;
			}
			p = strtok(NULL, d);
		}

		fps = freopen(path_folder_channel_list, "w", fps);

		/* REENREGISTREMENT DES SALONS */
		int id_channel = 0;
		for(;id_channel < nb_channel;id_channel++){
			printf("channel_list[%d] : %s\n", id_channel, channel_list[id_channel]);
 			fputs(channel_list[id_channel], fps);
 			fputs("\n", fps);
		}
		printf("Fichier %s modifié en supprimant le salon : %s\n", path_folder_channel_list, nom_salon);
	}

	char file_messages[TMAX];
	strcpy(file_messages, path_folder_channel_messages);
	strcat(file_messages, nom_salon);
	strcat(file_messages, ".txt");
	remove(file_messages);
	fclose(fps);
}

void save_last_messages(char last_message[], char nom_salon[]){
	char d[] = "\n";
	char *message_list[10];
	int nb_message = 0;
	char file_name[300];
	strcpy(file_name, path_folder_channel_messages);
	strcat(file_name, nom_salon);
	strcat(file_name, ".txt");

	printf("file_name : %s\n", file_name);

	FILE* fps = fopen(file_name, "r");

	if (fps == NULL){
		//printf("Ne peux pas ourvrir le fichier à l'emplacement suivante : %s", file_name);
		fps = fopen(file_name, "w");
		fputs(last_message, fps);
 		fputs("\n", fps);
		printf("Fichier %s modifié en ajoutant le message : %s\n", file_name, last_message);
	} else {
		char file_content[TMAX] = "";
		char str[1000] = "";

		/*RECUPERER LE CONTENU DU FICHIER*/
		while (fgets(str, 1000, fps) != NULL) {
			strcat(file_content, str);
		}

		char *p = strtok(file_content, d);
		while(p != NULL){
			message_list[nb_message] = p;
			nb_message++;
			p = strtok(NULL, d);
		}

		fps = freopen(file_name, "w", fps);

		/* REENREGISTREMENT DES MESSAGES */
		int id_message = 0;
		if(nb_message == 10){	
			id_message = 1;
		} 
		
		for(;id_message < nb_message;id_message++){
			printf("message_list[%d] : %s\n", id_message, message_list[id_message]);
 			fputs(message_list[id_message], fps);
 			fputs("\n", fps);
		}
		fputs(last_message, fps);
 		fputs("\n", fps);
		printf("Fichier %s modifié en ajoutant le message : %s\n", file_name, last_message);
	}
	fclose(fps);
}

int get_last_messages(char* last_messages[], char nom_salon[]){
	char d[] = "\n";
	int nb_message = 0;
	char file_name[300];
	strcpy(file_name, path_folder_channel_messages);
	strcat(file_name, nom_salon);
	strcat(file_name, ".txt");

	printf("file_name : %s\n", file_name);

	FILE* fps = fopen(file_name, "r");

	if (fps == NULL){
		printf("Ne peux pas ourvrir le fichier à l'emplacement suivante : %s", file_name);
	} else {
		char file_content[TMAX] = "";
		char str[1000] = "";

		/*RECUPERER LE CONTENU DU FICHIER*/
		while (fgets(str, 1000, fps) != NULL) {
			strcat(file_content, str);
		}

		char *p = strtok(file_content, d);
		while(p != NULL){
			last_messages[nb_message] = p;
			nb_message++;
			p = strtok(NULL, d);
		}

		for(int i = 0; i < nb_message; i++){
			printf("last_messages[%d] : %s\n", i, last_messages[i]);
		}

		printf("Fichier %s ouvert\n", file_name);
		fclose(fps);
	}
	return nb_message;
}

int recherche_tab_salon(char nom_salon[]){
	int tmp = 0;
	/* ON CHHERCHE L'INDICE DU SALON VOULU EN PARCOURANT TOUT LE TABLEAU DE SALON */
	while (tmp < nb_salon){
		if(strcmp(nom_salon, salons[tmp].nom_salon) == 0){
			/* RETOURNE L'INDICE DU SALON SI CELUI SI EXISTE */
			return tmp;
		}
		tmp++;
	}
	/* RETOURNE -1 SI LE SALON N'EXISTE PAS */
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

int nouveau_salon(char nom_salon[], int capa, char description[],int admin){
	/* SI EN AJOUTANT UN SALON IL Y A TOUJOURS UNE PLACE DANS LE TABLEAU DE SALON ALORS: */
	if (nb_salon+1 < sizeof(salons)/sizeof*(salons)){

		int nb_of_salon = recherche_tab_salon(nom_salon);
		/* SI LE SALON N'EXISTE DEJA PAS */
		if (nb_of_salon == -1){
			/* ON CREE UN NOUVEAU SALON */
			struct SALON newSalon;

			/* ON AFFECTE LES DIFFERENTS ATTRIBUTS AU SALON QUE L'ON VIENT DE CREER */
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

		    /* ON PLACE LE SALON QUE L'ON VIENT DE CREER DANS LE TABLEAU DE SALON */
		    *(salons + nb_salon) = newSalon;

		    /* ON INCREMANTE LE NOMBRE DE SALONS*/
		    nb_salon++;
		    return 0;		
		}
		/* SINON ON ENVOI UNE ERREUR SI LE SALON EXISTE DEJA  */
		else{
			return -1;
		}
	}
	/* SINON ON ENVOI UNE ERREUR SI IL Y A TROP DE SALON  */
	else{
		return -2;
	}
}

int modif_salon(char salon_base[],char new_nom_salon[], int new_capa, char new_description[]){

	int nb_of_salon = recherche_tab_salon(salon_base);
	int nb_of_salon2 = recherche_tab_salon(new_nom_salon);
	/* SI LE SALON EXISTE */
	if (nb_of_salon != -1){
		if(nb_of_salon2 ==-1){
			/* SI ON A LES DROITS DE MODIFICATION */
			if(salons[nb_of_salon].admin == 1){
				/* ON REMPLACE LES ATTRIBUTS*/
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

			    unsave_salon(salon_base);
			    save_salon(new_nom_salon);
			    return 0;
			}
			else{ /* SINON ON ENVOI UNE ERREUR SI ON A PAS LES DROITS POUR MODIFIER LE SALON */
				return -1;
			}
		}
		/* SINON ON ENVOI UNE ERREUR */
		else{
			return -3;
		}
	}
	/* SINON ON ENVOI UNE ERREUR */
    else{
    	return -2;
    }
}

int rejoindre_salon(struct CLIENT *c,char nom_salon[]){

	int nb_of_salon = recherche_tab_salon(nom_salon);
	/* SI LE SALON EXISTE */
	if (nb_of_salon != -1){  
		/* ON REGARDE SI LE SALON EXISTE */
		if(salons[nb_of_salon].nb_connecte + 1 <= salons[nb_of_salon].capacite){
			remove_from_salon(c,c->salon);
    		add_to_salon(c,nom_salon);
    		printf("%s à rejoint le salon : %s\n",c->pseudo,nom_salon);
    		return 0;
		}
		/* SINON ON ENVOI UN MESSAGE */
		else{
			return -2;
		}
	}
	/* SINON ON ENVOI UNE ERREUR */
    else{
    	return -1;
    }

}

int supprime_salon(char nom_salon[]){

	int nb_of_salon = recherche_tab_salon(nom_salon);
	/* SI LE SALON EXISTE */
	if (nb_of_salon != -1){
		/*SI ON A LES DROITS DE SUPPRIMER LE SALON */
		if(salons[nb_of_salon].admin ==1){ 
		/* ON PLACE TOUT LES CLIENTS DU SALON QUE L'ON VEUT SUPPRIMER VERS LE GENERAL(PAR DEFAUT)*/	
		for(int j=0; j<nb_client;j++){
			if(strcmp(users[j].salon,nom_salon)==0){
				int res = rejoindre_salon(&users[j],"General");
			}
		}
		/* ON SUPPRIME LE SALON VOULU ET ON DECALE TOUT LES SALONS SUIVANT D'UN EN ARRIERE*/	
		for(int k = nb_of_salon + 1; k < nb_salon; k++) {
	        salons[k - 1] = salons[k];
	    }
	    /* ON DECREMANTE LE NOMBRE DE SALONS */
	    nb_salon--; 

	    unsave_salon(nom_salon);
	    return 0;
		}
		/* SINON ON ENVOI UNE ERREUR SI L'UTILISATEUR N'A PAS LES DROITS POUR SUPPRIMER LE SALON  */
		else{
			return -1;
		}
	}
	/* SINON ON ENVOI UNE ERREUR SI LE SALON N'EXISTE PAS  */
    else{
    	return -1;
    }
}

int envoi(int socket, char* buffer) {

	int mes;
	//Envoi du message
	mes = send(socket, buffer, (strlen(buffer)+1)*sizeof(char), 0);
	if (mes == -1){
		perror("Erreur envoie\n");
		return -1;
	}
	if (mes == 0){
		perror("Aucun envoie\n");
		return 0;
	}

	return 1;
}
// (strlen(buffer)+1)*sizeof(char)
/* FONCTION DE TRANSMISSION D'UN MESSAGE D'UN CLIENT VERS L'AUTRE */
void *transmission(void* args){

	/* NUMÉRO DU CLIENT QUI ENVOIE LE MESSAGE */
	int i = 0;

	struct CLIENT *c = (CLIENT *) args;

	char message_recu[TMAX];
	char message_copy[TMAX];
	char command[100];
	char first_arg[TMAX];
	char second_arg[TMAX];
	char third_arg[TMAX];
	char fourth_arg[TMAX];
	char file_name[100];

	char buffer[TMAX];

	int id_command;
	int id_salon;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(1){

		/*	REMISE A À DE TOUT LES ARGUMENTS */
		strcpy (first_arg, "");// Remise à 0 de first_arg
		strcpy (message_recu, "");// Remise à 0 de first_arg

		int clientID = -1;

		/* RECEPTION DU PSEUDO DU DESTINATAIRE */
		int mes = recv(c->dSC, &message_recu, sizeof(message_recu), 0);

		if (mes<0){

			perror("Erreur reception mot C1vC2\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			perror("Socket fermée reception mot C1vC2\n");
			supprimer_client(c);
			pthread_exit(NULL);
		}

		if(strcmp(message_recu, "")!=0){
			printf("message : %s", message_recu);
			strcpy(message_copy,"");
			strcat(message_copy,message_recu);
			strtok(message_recu, "\n");
		}

		/* RECUPERATION DE LA COMMANDE SAISIE */
		char *p = strtok(message_recu, d);
		if (p == NULL){
			strcpy(command, message_recu);
		} else {
			strtolower(p);
			strcpy(command, p);
		}
		
		id_command = command_id(command);

		switch (id_command){
			case 0: /* ENVOI D'UN MESSAGE A UN OU PLUSIEURS DESTINATAIRES AVEC /mp pseudo message */
				if(arg_minimum(message_copy,3)==1){
					/* RECUPERATION DU PSEUDO DU DESTINATAIRE */
					p = strtok(NULL, d);
					strcpy(second_arg, p);

					/* RECUPERATION DU MESSAGE SAISIE */
					p = strtok(NULL, d);
					while(p != NULL)
					{
						strcat(first_arg, p);
						strcat(first_arg, " ");
					    p = strtok(NULL, d);
					}
					
					int pseudo_id = 0;
					for (;pseudo_id < nb_client;++pseudo_id){
						if(strcmp(users[pseudo_id].pseudo, second_arg)==0){
							clientID = pseudo_id;
						}
					}
					/* SI LE PSEUDO RECU EST "all" (ON ENVOI LE MESSAGE À TOUT LES CLIENTS) */
					if(strcmp(second_arg,"all")==0){

						pseudo_id = 0;
						for (;pseudo_id < nb_client;pseudo_id++){

							/* CREATION DU MESSAGE A ENVOYER */
							strcpy(buffer, c->pseudo);
							strcat(buffer, " : ");
							strcat(buffer, first_arg);
							strcat(buffer, "\n");

							/* ENVOI DU MESSAGE */
							int mes = envoi(users[pseudo_id].dSC, buffer);

						
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

						/* CREATION DU MESSAGE A ENVOYER */
						strcpy(buffer, c->pseudo);
						strcat(buffer, " : ");
						strcat(buffer, first_arg);
						strcat(buffer, "\n");
						printf("%s",buffer);
						/* ENVOI DU MESSAGE */
						int mes = envoi(users[clientID].dSC, buffer);

						/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					} else {

						/* CREATION DU MESSAGE A ENVOYER */
						strcpy(buffer, pseudo_serveur);
						strcat(buffer, " : ");
						strcat(buffer, "Message non distribué. L'utilisateur '");
						strcat(buffer, second_arg);
						strcat(buffer, "' n'est pas connecté.");
						strcat(buffer, "\n");

						/* ENVOI DU MESSAGE */
						int mes = envoi(c->dSC, buffer);

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
				}else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}
				break;

			case 1: /* LISTE LES PERSONNES PRESENTES DANS LE SERVEUR AVEC /whoishere */

				/* ENVOIE DES PSEUDOS AU CLIENT */
				strcpy(second_arg, "[");
				int pseudo_id = 0;
				for (;pseudo_id < nb_client;pseudo_id++){
					strcpy(third_arg, users[pseudo_id].pseudo);
					strcat(second_arg, strcat(third_arg, "] [")); 
				}
				second_arg[strlen(second_arg) - 1] = ' ';

				strcpy(buffer, pseudo_serveur);
				strcat(buffer, " : ");
				strcat(buffer, second_arg);
				strcat(buffer, "\n");

				mes = envoi(c->dSC, buffer);

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée transmission mot C1vC2\n");
					pthread_exit(NULL);
				}

				clientID = -1;

				printf("LISTE PSEUDO TRANSMISE\n\n");
				break;

			case 2: /* COUPE LA CONNEXION DU CLIENT QUI LE DEMANDE AVEC /fin */
				remove_from_salon(c,c->salon);
				supprimer_client(c);
				pthread_exit(NULL);
				break;

			case 3: /* TRANSMET LE FICHIER DE LA SOURCE VERS LE DESTINATAIRE AVEC /file */
				if(arg_minimum(message_copy,3)==1){
					/* RECUPERATION DU PSEUDO DU DESTINATAIRE */
					p = strtok(NULL, d);
					strcpy(first_arg, p);

					/* RECUPERATION DU NOM DU FICHIER */
					p = strtok(NULL, d);
					strcpy(second_arg, p);

					pseudo_id = 0;
					for (;pseudo_id < nb_client;++pseudo_id){
						if(strcmp(users[pseudo_id].pseudo, first_arg)==0){
							clientID = pseudo_id;
						}
					}

					if(clientID != -1){

						strcpy(fourth_arg,"/file ");
						strcat(fourth_arg, c->pseudo);
						strcat(fourth_arg, " ");
						strcat(fourth_arg, second_arg);

						transmit_file(c, &users[clientID], second_arg, fourth_arg);
					} else {
						char error[200];
						strcpy(error, pseudo_serveur);
						strcat(error, " : Fichier non distribué. L'utilisateur '");
						strcat(error, first_arg);
						strcat(error, "' n'est pas connecté.");

						/* ENVOIE ERREUR À LA SOURCE */
						int mes = envoi(c->dSC, error);

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
				} else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}

				break;

			case 4: /* TRANSMET LA LISTE DES FICHIERS VERS LE DEMANDEUR AVEC LA COMANDE /getfile 
				ET LE FICHIER VERS LE DEMANDEUR AVEC /getFile nomfichier */
				/* RECUPERATION DU NOM DU FICHIER */
				p = strtok(NULL, d);

				if (p == NULL){

					//printf("wshhhh 1\n");

					/* ENVOIE LISTE DES FICHIERS A LA SOURCE */
					char file_list_to_send[TMAX];

					get_file(file_list_to_send);

					strcpy(buffer, pseudo_serveur);
					strcat(buffer, " : ");
					strcat(buffer, file_list_to_send);

					int mes = envoi(c->dSC, buffer);

					/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
					if (mes<0){
						perror("Erreur transmission mot vers C1\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission vers C1\n");
						pthread_exit(NULL);
					}

				} else {

					int fd;
					int count_r, count_w;
					char* bufptr;
					char buf[MAX_BUF];
					char filename[MAX_BUF];

					struct sockaddr_in aCF;
					socklen_t lg = sizeof(struct sockaddr_in);

					if(c->dSF == -1){
						c->dSF = accept(dSE_f, (struct sockaddr*)&aCF, &lg);
					}
					
					strcpy(filename, path_folder_serv);
					strcat(filename, p);
					printf("Lecture du fichier à envoyer : %s\n", filename);

					fd = open(filename, O_RDONLY); 
					if (fd == -1)
					{
						perror("File open error");
						exit(1);
					}
					while((count_r = read(fd, buf, MAX_BUF))>0)
					{
						count_w = 0;
						bufptr = buf;
						while (count_w < count_r)
						{
							count_r -= count_w;
							bufptr += count_w;
							count_w = write(c->dSF, bufptr, count_r);
							if (count_w == -1) 
							{
								perror("Socket read error");
								exit(1);
							}
						}
						//write(c->dSF, "EOF", 3);
					}
					close(c->dSF);
					c->dSF = -1;
				}
				break;

			case 5: /* CREER UN NOUVEAU SALON AVEC /newSalon nomsalon capacite description */

				if(arg_minimum(message_copy,4)==1){
					/* RECUPERATION DU NOM DU SALON (arg 1)*/
					p = strtok(NULL, d);
					strcpy(first_arg, p);	

					/* RECUPERATION DE LA CAPACITE DU SALON (arg 2) */
					p = strtok(NULL, d);
					strcpy(second_arg, p);
					int capa = atoi(second_arg);

					/* RECUPERATION DE LA DESCRIPTION DU SALON (arg 3) */
					p = strtok(NULL, d);
					while(p != NULL)
					{
						strcat(third_arg, p);
						strcat(third_arg, " ");
					    p = strtok(NULL, d);
					}
					third_arg [strlen(third_arg)-1] = 0;

					/* ON CREE LE NOUVEAU SALON */
					int res = nouveau_salon(first_arg,capa,third_arg,1);
					if(res == 0){ // SI TOUT EST BON
						/* ON ENREGISTRE LE NOUVEAU SALON */
						save_salon(first_arg);

						/* ON REJOINT LE NOUVEAU SALON */
		    			mes = rejoindre_salon(c,first_arg);

						/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
						strcpy(fourth_arg, pseudo_serveur);
						strcat(fourth_arg, " : Le salon ");
						strtok(first_arg, "\n");
						strcat(fourth_arg, first_arg);
						strcat(fourth_arg, " a été crée !\n");
						int mes = send(c->dSC, fourth_arg, sizeof(fourth_arg), 0);

						/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);						
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else if(res == -1){ // SI IL N'Y A PLUS DE PLACE SUR LE SALON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Le salon existe déjà\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else{ // SI LE SALON  N'EXISTE PAS
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Il y a trop de salons\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
					}
				}else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}
				break; 
			
			case 6: /* DONNE TOUTE INFOS SUR LE SALON EN QUESTION AINSI QUE LES 10 DERNIERS MESSAGES AVEC /salonInfo */

				id_salon = recherche_tab_salon(c->salon);
				sprintf(first_arg, "%d", id_salon);
				sprintf(second_arg, "%d", salons[id_salon].nb_connecte);
				sprintf(third_arg, "%d", salons[id_salon].capacite);

				strcpy(buffer, pseudo_serveur);
				strcat(buffer, " : Numéro du salon: ");
				strcat(buffer, first_arg);
				strcat(buffer, "\n");
				strcat(buffer, pseudo_serveur);
				strcat(buffer, " : Nom du salon: ");
				strcat(buffer, salons[id_salon].nom_salon);
				strcat(buffer, "\n");
				strcat(buffer, pseudo_serveur);
				strcat(buffer, " : ");
				strcat(buffer, second_arg);
				strcat(buffer, " clients sur ");
				strcat(buffer, third_arg);
				strcat(buffer, " dans ");
				strcat(buffer, salons[id_salon].nom_salon);
				strcat(buffer, "\n[ 10 DERNIERS MESSAGES DANS LE SALON ]\n");

				char* last_messages[10];
				int nb_messages = get_last_messages(last_messages, salons[id_salon].nom_salon);

				int i = 0;
				while(i < nb_messages){
					strcat(buffer, last_messages[i]);
					strcat(buffer, "\n");
					i++;
				}

				mes = envoi(c->dSC, buffer);

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur envoie fichier\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée envoie fichier\n");
					pthread_exit(NULL);
				}

				break;
			
			case 7: /* LISTE TOUT LES SALON PRESENT SUR LE SERVEUR AVEC /listSalon */

				for(int j=0; j<nb_salon; j++){
					sprintf(second_arg, "\n%d", j);
			    	strcat(first_arg, second_arg);
			    	strcat(first_arg, " - ");
			    	strcat(first_arg, salons[j].nom_salon);
			    	strcat(first_arg, " [");
			    	strcat(first_arg, salons[j].description);
			    	strcat(first_arg, "] ");
			    	memset (second_arg, 0, sizeof (second_arg));
			    	sprintf(second_arg, "%d", salons[j].nb_connecte);
			    	strcat(first_arg, second_arg);
			    	strcat(first_arg, "/");
			    	memset (second_arg, 0, sizeof (second_arg));
			    	sprintf(second_arg, "%d", salons[j].capacite);
			    	strcat(first_arg, second_arg);
				}
				strcat(first_arg, "\n");

				strcpy(buffer, pseudo_serveur);
				strcat(buffer, " : ");
				strcat(buffer, first_arg);

				/* ENVOIE DU MESSAGE */
				mes = envoi(c->dSC, buffer); 

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur envoie fichier\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée envoie fichier\n");
					pthread_exit(NULL);
				}
				break;

			case 8: /* MODIFIER LE SALON VOULU avec /modifSalon nomsalon newnomsalon newcapacite newdescription */
				if(arg_minimum(message_copy,5)==1){
					/* RECUPERATION DU NOM DU SALON */
					p = strtok(NULL, d);
					strcpy(first_arg, p);

					/* RECUPERATION DU NOUVEAU NOM DE SALON */
					p = strtok(NULL, d);
					strcpy(second_arg, p);		

					/* RECUPERATION DU NOM DU SALON */
					p = strtok(NULL, d);
					strcpy(third_arg, p);
					int capa = atoi(third_arg);

					/* RECUPERATION DE LA DESCRIPTION DU SALON */
					p = strtok(NULL, d);
					while(p != NULL)
					{
						strcat(fourth_arg, p);
						strcat(fourth_arg, " ");
					    p = strtok(NULL, d);
					}
					fourth_arg[strlen(fourth_arg)-1]= '\0';

					int res = modif_salon(first_arg,second_arg,capa,fourth_arg);
					if(res == 0){ // SI TOUT EST BON

						/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
						strcpy(buffer, pseudo_serveur);
						strcat(buffer, " : Le salon ");
						strtok(first_arg, "\n");
						strcat(buffer, first_arg);
						strcat(buffer, " a été modifié !\n");
						mes = envoi(c->dSC, buffer);

						/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);						
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else if(res == -1){ // SI ON A PAS LES DROITS POUR MODIFIER LE SALON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Vous n'avez pas les droits pour modifier ce salon");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else if(res == -2) { // SI LE SALON  N'EXISTE PAS
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Le salon n'existe pas");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
					}
					else{
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Le nom du salon est déjà pris");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
					}
				}else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}
				break;

			case 9: /* ENVOYER UN MESSAGE A QUELQU'UN DE SON SALON AVEC /salon msg */

				/* RECUPERATION DU MESSAGE SAISIE */
				p = strtok(NULL, d);
				while(p != NULL)
				{
					strcat(first_arg, p);
					strcat(first_arg, " ");
				    p = strtok(NULL, d);
				}
				
				pseudo_id = 0;
				for (;pseudo_id < nb_client;pseudo_id++){
					if(strcmp(users[pseudo_id].salon,c->salon)==0){
						int dSC = users[pseudo_id].dSC;

						strcpy(buffer, c->pseudo);
						strcat(buffer, " : ");
						strcat(buffer, first_arg);

						/* ENVOIE DU MESSAGE DU CLIENT 1 VERS LE CLIENT 2*/
						int mes = envoi(dSC, buffer);

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
				save_last_messages(buffer, c->salon);

				break;
			
			case 10: /* REJOINDRE UN SALON AVEC /joinSalon nomsalon */
				if(arg_minimum(message_copy,2)==1){
					/* RECUPERATION DU NOM DU SALON */
					p = strtok(NULL, d);
					strcpy(first_arg, p);

					int res = rejoindre_salon(c,first_arg);
					if(res == 0){ // SI TOUT EST BON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Vous avez rejoint le salon ");
						strcat(second_arg, first_arg);

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else if(res == -1){ // SI IL N'Y A PLUS DE PLACE SUR LE SALON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Le salon n'existe pas\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else{ // SI LE SALON  N'EXISTE PAS
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Plus de place dans le salon\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
					}
				}else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}
				break;

			case 11: /* SUPPRIMER UN SALON AVEC /delSalon nomsalon */
				if(arg_minimum(message_copy,2)==1){
					/* RECUPERATION DU NOM DU SALON */
					p = strtok(NULL, d);
					strcpy(first_arg, p);
					int res = supprime_salon(first_arg);

					if(res == 0){ // SI TOUT EST BON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Vous avez supprimé le salon ");
						strcat(second_arg, first_arg);

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else if(res == -1){ // SI IL N'Y A PLUS DE PLACE SUR LE SALON
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Vous n'avez pas les droits pour supprimer ce salon\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}

					}else{ // SI LE SALON  N'EXISTE PAS
						/* CREATION DU MESSAGE A ENVOYER*/
						strcpy(second_arg, pseudo_serveur);
						strcat(second_arg, " : Le salon n'existe pas\n");

						/* ENVOIE DU MESSAGE */
						mes = envoi(c->dSC, second_arg); 

						if (mes<0){
							perror("Erreur transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
						if (mes==0){
							perror("Socket fermée transmission mot C1vC2\n");
							pthread_exit(NULL);
						}
					}
				}else{
					/* CREATION DU MESSAGE A ENVOYER*/
					strcpy(second_arg, pseudo_serveur);
					strcat(second_arg, " : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");

					/* ENVOIE DU MESSAGE */
					mes = envoi(c->dSC, second_arg); 

					if (mes<0){
						perror("Erreur transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
					if (mes==0){
						perror("Socket fermée transmission mot C1vC2\n");
						pthread_exit(NULL);
					}
				}
				break;

			case 12:

				open_readme(buffer);

				/* ENVOIE DU MESSAGE */
				mes = envoi(c->dSC, buffer); 

				if (mes<0){
					perror("Erreur transmission mot C1vC2\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée transmission mot C1vC2\n");
					pthread_exit(NULL);
				}

				break;

			default:
				printf(" ");
		}
		
	}
	
	printf("La discussion est terminée\n");
	printf("En attente des clients\n");

	pthread_exit(NULL);
}

void init_salon(){

	char d[] = "\n";
	char *channel_list[10];
	int nb_channel = 0;

	FILE *fps = fopen(path_folder_channel_list, "r");
	if (fps == NULL){
		printf("Ne peux pas ouvrir le fichier suivant : %s", path_folder_channel_list);
	}
	else {
		printf("Fichier ouvert : %s\n", path_folder_channel_list);
		char file_content[TMAX] = "";
		char str[1000] = "";

		/*RECUPERER LE CONTENU DU FICHIER*/
		while (fgets(str, 1000, fps) != NULL) {
			strcat(file_content, str);
		}	

		char *p = strtok(file_content, d);
		while(p != NULL){
			channel_list[nb_channel] = p;
			printf("channel_list[%d] : %s\n", nb_channel, channel_list[nb_channel]);
			p = strtok(NULL, d);
			nb_channel++;
		}

		/* CREATION DES SALONS */
		int id_channel = 0;
		for(;id_channel < nb_channel;id_channel++){
			int admin = 1;
			if (strcmp(channel_list[id_channel], "General")){
				admin = 0;
			}
 			nouveau_salon(channel_list[id_channel],100,"Salon récupéré par sauvegarde", admin);
		}
	}
	fclose(fps);
}

int main(int argc, char* argv[]){

	strcpy(port, argv[1]);

	/* INITIALISATION DU SÉMAPHORE  */
	int sem = sem_init(&semaphore, 0, NB_CLIENT_MAX);
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

	/* CREATTION DU SOCKET FICHIER ET VERIFICATION */
	dSE_f = socket(PF_INET, SOCK_STREAM, 0);
	if (dSE_f<0){
		perror("Erreur à la création du socket d'écoute pour les fichiers");
		return -1;
	}

	/* NOMMAGE DU SOCKET FICHIER ET VERIFICATION */
	struct sockaddr_in adServF;
	adServF.sin_family = AF_INET;
	adServF.sin_addr.s_addr = INADDR_ANY;
	adServF.sin_port = htons((short)atoi(argv[1])+1);
	verif = bind(dSE_f, (struct sockaddr*)&adServF, sizeof(adServF));
	if (verif<0){
		perror("Erreur au nommage du socket d'écoute pour les fichiers");
		return -1;
	}

	/* MISE EN ECOUTE DES CONNEXIONS ENTRANTES ET VERIFICATION SOCKET FICHIER */
	verif = listen(dSE_f,7);
	if (verif<0){
		perror("Erreur lors de la mise à l'écoute du socket pour les fichiers");
		return -1;
	}
	struct sockaddr_in aCF;

	users = (CLIENT *) malloc(sizeof(CLIENT)*2);

	init_salon();

 	printf("En attente des clients\n");
	while(1){

		int dSC;
		int dSF;

		dSC = accept(dSE, (struct sockaddr*) &aC, &lg);
		dSF = accept(dSE_f, (struct sockaddr*)&aCF, &lg);

		if (nb_client > 0){
			users = (CLIENT *) realloc(users, sizeof(CLIENT)*(nb_client+1));
		} else {
			users = (CLIENT *) malloc(sizeof(CLIENT)*2);
		}

		/* CONNEXION AVEC UN CLIENT */
		users[nb_client].dSC = dSC;
		users[nb_client].dSF = dSF;

		if (users[nb_client].dSC<0 || users[nb_client].dSF<0){
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
		if( pthread_create(&users[nb_client].thread, NULL, transmission, (void *) &users[nb_client])){
			perror("Erreur à la création du thread de transmission entre le client 1 et le client 2 ");
			return EXIT_FAILURE;
		}

		nb_client++;
	}
	close(dSE);
	close(dSE_f);
	printf("Fin du programme\n");
	free(users);
	return 0;
}
