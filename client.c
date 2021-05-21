#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>

#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ASCII_ESC 27

const char path_folder_send[20] = "./file_to_send/";
const char path_folder_recv[20] = "./file_received/";
const char d[] = " ";

/* SOCKET COTE SERVEUR */
int dS;

/* DERNIER PSEUDO (pour affichage) */
char last_pseudo[100];

/* DÉCLARATION DES THREADS */
pthread_t threadR; /* RECEPTION */
pthread_t threadS; /* ENVOIE */

/* FONCTION DE SAISIE DE MESSAGES A ENVOYER */
void saisie(char *mot){
	printf("\nEcrivez votre message\n");
	fgets(mot,TMAX,stdin); 
}

/* FONCTION DE RÉCUPÉRATION DES FICHIERS A ENVOYER */
int getFile(){
    struct dirent *dir;
    /*opendir() renvoie un pointeur de type DIR.*/

    DIR *d = opendir(path_folder_send); 
    if (d)
    {
    	puts("\033[1m");
    	printf("Liste des fichiers : ");
        while ((dir = readdir(d)) != NULL)
        {  
            if(strcmp(dir->d_name,".")!=0  && strcmp(dir->d_name,"..") != 0){
                printf("[%s] ", dir->d_name);
            }
        }
        puts("\033[0m");
        closedir(d);
    }
    return 0;
}

/* FONCTION D'ENVOIE DE MESSAGES */
void *envoie(void *args){
	char mot[TMAX];
	char mot_to_send[TMAX];

	while(strcmp(mot,"/fin\n")!=0){
		saisie(mot);
		strcpy(mot_to_send, mot);
		char *p = strtok(mot, d);

		if (strcmp(mot,"/fileList\n")==0){
			getFile();
		}
		else if (strcmp(p,"/file")==0){
			char pseudo[100];
			p = strtok(NULL, d);
			strcpy(pseudo, p);

			p = strtok(NULL, d);

			char file_name[100];
			strcpy(file_name, p);
			strtok(file_name, "\n");

			char path_file[100] = "";
			strcpy(path_file, path_folder_send);
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
				int mes = send(dS, message, strlen(message)+1, 0);

				/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
				if (mes<0){
					perror("Erreur envoie fichier\n");
					pthread_exit(NULL);
				}
				if (mes==0){
					perror("Socket fermée envoie fichier\n");
					pthread_exit(NULL);
				}
			}
		}
		else{
			/* ENVOIE DU MESSAGE */
			int mes = send(dS, mot_to_send, sizeof(mot_to_send), 0);

			/* GESTION DES ERREURS DE L'ENVOIE DU MESSAGE */
			if (mes<0){
				perror("Erreur envoie mot\n");
				pthread_exit(NULL);
			}
			if (mes==0){
				perror("Socket fermée envoie mot\n");
				pthread_exit(NULL);
			}
		}
	}
	pthread_exit(NULL);
}

/* FONCTION DE RECEPTION DE MESSAGES */
void *recoie(void* args){

	//char pseudoOther[100];

	char mot[TMAX];
	char mot_to_print[TMAX];
	int fin = 0;
	int nb_octets;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(fin==0){

		char mot[TMAX] = "";

		//char char_nb_octets[10];

		/* RECEPTION DU PSEUDO DU CLIENT */
		/*int mes = recv(dS, pseudoOther, sizeof(pseudoOther), 0);

		/* GESTION DES ERREURS DE LA RECEPTION DU PSEUDO 
		if (mes<0){
			perror("Erreur reception mot\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			printf("Le serveur a été coupé\n");
			pthread_cancel(threadS);
			pthread_exit(NULL);
		}/*

		/* RECEPTION DU MESSAGE */
		int mes = recv(dS, mot, sizeof(mot), 0);

		/* GESTION DES ERREURS DE LA RECEPTION DU MESSAGE */
		if (mes<0){
			perror("Erreur reception mot\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			printf("Le serveur a été coupé\n");
			pthread_cancel(threadS);
			pthread_exit(NULL);
		}

		strcpy(mot_to_print, mot);
		char *recv = strtok(mot, d);

		if(strcmp(mot,"")!=0){
			if(strcmp(recv,"/file")==0){

				char pseudoOther[100];
				recv = strtok(NULL, d);
				strcpy(pseudoOther, recv);

				char file_name[100];
				recv = strtok(NULL, d);
				strcpy(file_name, recv);

				char content[TMAX] = "";
				recv = strtok(NULL, d);
				while(recv != NULL){
					strcat(content, recv);
					strcat(content, " ");
					recv = strtok(NULL, d);
				}
				content[strlen(content)-1] = '\0';

				char file_path[100];
				strcpy(file_path, path_folder_recv);
				strcat(file_path, file_name);

				FILE* fps = fopen(file_path, "w");

				puts("\033[1m");
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				printf("Fichier de %s reçu nommé : %s\n", pseudoOther, file_name);
				if (fps == NULL){
					printf("Ne peux pas créer le fichier à l'emplacement suivante : %s", file_path);
				} else {
					printf("Fichier créé et placé à l'emplacement suivant : %s\n", file_path);
					fprintf(fps,"%s",content);
				}
				puts("\033[0m");

				fclose(fps);
			} else {

				printf("wshhhh 3\n");
				/* AFFICHAGE DU MESSAGE RECU */
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				puts("\033[1m");
				if (strcmp(mot_to_print,"")!=0)
				{
					printf("%s", mot_to_print);
				}
				puts("\033[0m");
			}
			printf("Ecrivez votre message \n");
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char* argv[]){ 

	/* ENTREE DU PSEUDO PAR LE CLIENT */
	char pseudo[100];
	char buffer[100];

	printf("Bienvenue dans le chat !\n");
	
	/* CREATTION DU SOCKET ET VERIFICATION */
	dS = socket(PF_INET, SOCK_STREAM, 0);
	if (dS<0){
		perror("Erreur à la création du socket\n");
		return -1;
	}

	/* ADRESSAGE DU SOCKET ET VERIFICATION */
	struct sockaddr_in as;
	as.sin_family = AF_INET;
	int res = inet_pton(AF_INET, argv[1], &(as.sin_addr));
	as.sin_port = htons((short)atoi(argv[2]));
	if (res<=0){
		perror("Erreur d'adressage\n");
		return -1;
	}

	/* CONNEXION AU SERVEUR ET VERIFICATION */
	socklen_t lgA = sizeof(struct sockaddr_in);
	res = connect(dS, (struct sockaddr *) &as, lgA);
	if (res<0){ 
		perror("Erreur de connexion au serveur\n");
		return -1;
	}

	/* RECEVOIR LA DEMANDE DE PSEUDO */
	recv(dS, buffer, sizeof(buffer), 0);
	printf("%s", buffer);
	fgets(pseudo,100,stdin);
	strtok(pseudo, "\n");

	printf("Pseudo à envoyer : %s\n", pseudo);

	int taille_pseudo = sizeof(pseudo);
	/* ENVOIE DE LA TAILLE DU PSEUDO AU CLIENT */
	send(dS, &taille_pseudo, sizeof(int), 0);

	/* ENVOIE DU PSEUDO AU SERVEUR */
	send(dS, pseudo, sizeof(pseudo), 0);

	printf("Debut de la discussion\n");

	/* CREATION DES THREADS */
	if( pthread_create(&threadR, NULL, recoie, NULL) ){
		perror("creation threadGet erreur");
		return EXIT_FAILURE;
	}

	if( pthread_create(&threadS, NULL, envoie, NULL ) ){
		perror("creation threadS erreur");
		return EXIT_FAILURE;
	}

	/* ATTENTE DE LA FIN DES THREADS */
	if(pthread_join(threadS, NULL)){ 
		perror("Erreur fermeture threadS");
		return EXIT_FAILURE;
	}

	pthread_cancel(threadS);
	pthread_cancel(threadR);

	printf("Fin de la conversation\n");
	close(dS);

	return 0;

}
