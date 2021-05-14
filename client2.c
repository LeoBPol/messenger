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

/* SOCKET COTE SERVEUR */
int dS;

/* VARIABLE TEST POUR VOIR L'ETAT DU MESSAGE */
int etat_message = 0; 

/* DERNIER PSEUDO (pour affichage) */
char last_pseudo[100];

/* FONCTION DE SAISIE DU PSEUDO DU DESTINATAIRE */
void saisie_destinataire(char *pseudo){
	printf("\nSaisissez le pseudo du destinataire: \n");
	fgets(pseudo,100,stdin);
	strcpy(last_pseudo,pseudo);
	strtok(last_pseudo, "\n");
}

/* FONCTION DE SAISIE DE MESSAGES A ENVOYER */
void saisie(char *mot){
	printf("\nEcrivez votre message\n");
	fgets(mot,TMAX,stdin); 
}

/* FONCTION DE RÉCUPÉRATION DES  */
int getFile(){
    struct dirent *dir;
    /*opendir() renvoie un pointeur de type DIR.*/

    DIR *d = opendir("./file_to_send"); 
    if (d)
    {
        while ((dir = readdir(d)) != NULL)
        {  
            if(strcmp(dir->d_name,".")!=0  && strcmp(dir->d_name,"..") != 0){
                printf("%s\n", dir->d_name);
            }
        }
        closedir(d);
    }
    return 0;
}

/* COMMANDE WHO */
void *whoishere(){

	int nbPseudos;

	recv(dS, &nbPseudos, sizeof(int), 0);

	printf("%d\n", nbPseudos);

	char pseudos[100][100];

	recv(dS, &pseudos, sizeof(pseudos), 0);
	printf("pseudos recus");
	int i = 0;
	for(;i<nbPseudos;i++){
		if(strcmp(pseudos[i], "") != 0){
			printf("[%s] ", pseudos[i]);
		}
	}
	printf("\n");

}

/* FONCTION D'ENVOIE DE MESSAGES */
void *envoie(void *args){
	char mot[TMAX];

	while(strcmp(mot,"/fin\n")!=0){
		saisie(mot);
		printf("mot : %s", mot);
		/* ENVOIE DU MESSAGE */
		int mes = send(dS, mot, sizeof(mot), 0);
		printf("sended\n");

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
	pthread_exit(NULL);
}

/* FONCTION DE RECEPTION DE MESSAGES */
void *recoie(void* args){

	char pseudoOther[100];

	char mot[TMAX];
	int fin = 0;
	int nb_octets;

	/* BOUCLE TANT QUE LES MSG SONT DIFFÉRENTS DE "fin" */
	while(fin==0){

		char char_nb_octets[10];

		/* RECEPTION DU PSEUDO DU CLIENT */
		recv(dS, &pseudoOther, sizeof(pseudoOther), 0);

		//printf("Pseudo reçu : %s\n", pseudoOther);

		/* RECEPTION DU MESSAGE */
		int mes = recv(dS, &mot, sizeof(mot), 0); 

		/* GESTION DES ERREURS DE LA RECEPTION DU MESSAGE */
		if (mes<0){
			perror("Erreur reception mot\n");
			pthread_exit(NULL);
		}
		if (mes==0){
			perror("Socket fermée reception mot\n");
			pthread_exit(NULL);
		}

		if(strcmp(mot,"/file\n")==0){
			printf("test");
		}

		/* AFFICHAGE DU MESSAGE RECU */
		printf( "%c[2K", ASCII_ESC );
		printf( "%c[A", ASCII_ESC );
		printf( "%c[2K", ASCII_ESC );
		printf( "%c[A", ASCII_ESC );
		puts("\033[1m");
		if (strcmp(pseudoOther,"")!=0)
		{
			printf("%s : %s", pseudoOther, mot);
		}
		puts("\033[0m");


		if(strcmp(mot,"/fin\n")==0){
			fin = 1;
			pthread_exit(NULL);
		} else {
			if(etat_message == 1){
				printf("Saisissez le pseudo du destinataire: \n");
			}
			else{
				printf("Ecrivez votre message \n");
			}
			
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

	/* DÉCLARATION DES THREADS */
	pthread_t threadR; /* RECEPTION */
	pthread_t threadS; /* ENVOIE */

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
