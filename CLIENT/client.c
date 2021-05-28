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

#define TMAX 65000 /* TAILLE MAXIMALE D'UN MESSAGE EN OCTET */
#define ASCII_ESC 27

#define MAX_BUF 1048576
#define S_PORT  6398

const char path_folder_send[20] = "./file_to_send/";
const char path_folder_recv[20] = "./file_received/";
const char d[] = " ";

char port[5];
char addr[15];
char pseudo[100];

/* SOCKET COTE SERVEUR */
int dS;
int dSF;

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
void *envoi_file(char file_name[]){

	if (dSF == -1){
		/* create a socket */
		dSF = socket(PF_INET, SOCK_STREAM, 0);
		if (dSF<0){
			perror("Erreur à la création du socket pour les fichiers\n");
			exit(-1);
		}

		/* server address */ 
		struct sockaddr_in serv_name;
		serv_name.sin_family = AF_INET;
		inet_aton(addr, &serv_name.sin_addr);
		serv_name.sin_port = htons((short)atoi(port)+1);

		/* connect to the server */
		int res = connect(dSF, (struct sockaddr*)&serv_name, sizeof(serv_name));
		if (res<0){ 
			perror("Erreur de connexion au serveur\n");
			exit(-1);
		}
	}
	
	char file[100];
	strcpy(file, path_folder_send);
	strcat(file, file_name);
	strtok(file, "\n");
	
	int fd, count_r, count_w;
	char buf[MAX_BUF];
	char* bufptr;

	fd = open(file, O_RDONLY); 
	if (fd == -1){
		puts("\033[1m");
		printf("Le fichier à envoyer \"%s\" n'est pas présent dans le dossier : %s\n", file_name, path_folder_send);;
		puts("\033[0m");
		close(dSF);
		dSF = -1;
	} else {
		while((count_r = read(fd, buf, MAX_BUF))>0){
			count_w = 0;
			bufptr = buf;
			while (count_w < count_r){
				count_r -= count_w;
				bufptr += count_w;
				count_w = write(dSF, bufptr, count_r);
				if (count_w == -1){
					perror("Socket read error");
					exit(1);
				}
			}
		}
		close(dSF);
		dSF = -1;
	}
}

/* FONCTION RECEPTION DE FICHIER */
void *recevoir_file(char file_name[]){

	if (dSF == -1){
		/* create a socket */
		dSF = socket(PF_INET, SOCK_STREAM, 0);
		if (dSF<0){
			perror("Erreur à la création du socket pour les fichiers\n");
			exit(-1);
		}

		/* server address */ 
		struct sockaddr_in serv_name;
		serv_name.sin_family = AF_INET;
		inet_aton(addr, &serv_name.sin_addr);
		serv_name.sin_port = htons((short)atoi(port)+1);

		/* connect to the server */
		int res = connect(dSF, (struct sockaddr*)&serv_name, sizeof(serv_name));
		if (res<0){ 
			perror("Erreur de connexion au serveur\n");
			exit(-1);
		}
	}

	int count = 1;
	int count_max = 0;
	char buf[TMAX];

	char file[100];
	strcpy(file, path_folder_recv);
	strcat(file, file_name);
	strtok(file_name, "\n");
	strtok(file, "\n");
	
	int fd;

	puts("\033[1m");
	fd = open(file, O_WRONLY | O_CREAT, 0666);
	while (count > 0){
		count = read(dSF, buf, TMAX);
		write(fd, buf, count);
		if (count > count_max){
			count_max = count;
		}
	}
	strcpy(buf, "");
	write(dSF, buf, 0);
	if (count == -1)
	{
		printf("Ne peux pas créer le fichier à l'emplacement suivante : %s\n", file);
		perror("Read error");
		exit(1);
	} else if (count_max > 0){
		printf("Fichier créé et placé à l'emplacement suivant : %s\n", file);
	} else {
		remove(file);
	}
	puts("\033[0m");
	close(dSF);
	dSF = -1;
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
		else{
			int send_message = 0;
			if (strcmp(p,"/file")==0){

				char pseudo_client[100];
				p = strtok(NULL, d);
				strcpy(pseudo_client, p);

				if (strcmp(pseudo_client, pseudo) == 0){
					send_message = 1;
					puts("\033[1m");
					printf( "%c[2K", ASCII_ESC );
					printf( "%c[A", ASCII_ESC );
					printf( "%c[2K", ASCII_ESC );
					printf( "%c[A", ASCII_ESC );
					printf("\033[1;31m[SERVEUR]\033[1;37m : Vous ne pouvez pas envoyer un fichier à vous même\n");
					puts("\033[0m");
				} else {
					p = strtok(NULL, d);
					if (p != NULL){
						char file_name[TMAX];
						strcpy(file_name, p);
						strtok(file_name,"\n");
						envoi_file(file_name);
					} else {
						send_message = 1;
						puts("\033[1m");
						printf( "%c[2K", ASCII_ESC );
						printf( "%c[A", ASCII_ESC );
						printf( "%c[2K", ASCII_ESC );
						printf( "%c[A", ASCII_ESC );
						printf("\033[1;31m[SERVEUR]\033[1;37m : Pas assez d'arguments, veuillez consulter le /man pour voir comment utiliser la commande\n");
						puts("\033[0m");
					}
				}
			}
			if(send_message == 0){

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
				if(strcmp(p,"/getfile")==0){
					p = strtok(NULL, d);
					if (p != NULL){
						recevoir_file(p);
					}
				}
			}
		}
	}
	pthread_exit(NULL);
}

/* FONCTION DE RECEPTION DE MESSAGES */
void *recoie(void* args){

	char mot[TMAX];
	char mot_to_print[TMAX];
	int fin = 0;
	int nb_octets;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(fin==0){

		char mot[TMAX] = "";

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

				puts("\033[1m");
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				recevoir_file(file_name);
				puts("\033[0m");

			} else {

				/* AFFICHAGE DU MESSAGE RECU */
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				printf( "%c[2K", ASCII_ESC );
				printf( "%c[A", ASCII_ESC );
				puts("\033[1m");
				if (strcmp(mot_to_print,"")!=0){
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

	strcpy(port, argv[2]);
	strcpy(addr, argv[1]);

	/* ENTREE DU PSEUDO PAR LE CLIENT */
	char buffer[100];

	printf("Bienvenue dans le chat !\n");

	/* create a socket */
	dSF = socket(PF_INET, SOCK_STREAM, 0);
	if (dSF<0){
		perror("Erreur à la création du socket pour les fichiers\n");
		exit(-1);
	}

	/* server address */ 
	struct sockaddr_in serv_name;
	serv_name.sin_family = AF_INET;
	inet_aton(argv[1], &serv_name.sin_addr);
	serv_name.sin_port = htons((short)atoi(argv[2])+1);

	/* connect to the server */
	int res = connect(dSF, (struct sockaddr*)&serv_name, sizeof(serv_name));
	if (res<0){ 
		perror("Erreur de connexion au serveur\n");
		exit(-1);
	}
	
	/* CREATTION DU SOCKET ET VERIFICATION */
	dS = socket(PF_INET, SOCK_STREAM, 0);
	if (dS<0){
		perror("Erreur à la création du socket\n");
		return -1;
	}

	/* ADRESSAGE DU SOCKET ET VERIFICATION */
	struct sockaddr_in as;
	as.sin_family = AF_INET;
	res = inet_pton(AF_INET, argv[1], &(as.sin_addr));
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
	close(dSF);

	return 0;

}
