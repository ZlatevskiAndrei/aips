#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <arpa/inet.h>

#define ERROR_IF(cond,msg) \
    if (cond) { \
        if (errno != 0) { \
            fprintf(stderr, "%s:%d [%s()] -> %s (%s)\n", \
                    __FILE__, __LINE__, __func__, msg, strerror(errno)); \
        } else { \
            fprintf(stderr, "%s:%d [%s()] -> %s\n", \
                    __FILE__, __LINE__, __func__, msg); \
        } \
        exit(EXIT_FAILURE); \
    }
enum gateaway_function {
	UNDEFINED,	
	SOURCE,
	PUITS
};
enum transport_protocol {
	UNDEFINED,
	UDP,
	TCP
};

enum gateaway_function gf = UNDEFINED;
enum transport_protocol protocol = UNDEFINED;
int nb_message = -1; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
int longueur = 30; 	//longueur = 30 octets par defaut pour puits et source

struct hostent* hp;
struct sockaddr_in dest_addr;
//memset((char*)&dest_addr,0,sizeof(dest_addr));
struct sockaddr_in local_addr;
//memset((char*)&local_addr,0,sizeof(local_addr));


void usage() {
	printf("usage: cmd [-p|-s][-n ##]\n");
}

//tsock -p [-options] port
//tsock -s [-options] host port
/*
options: 
-u UDP
-l ##
-n ##
*/

void affect_sockaddr(struct sockaddr_in* sockaddr, int port, char* dest_addr, struct hostent* hp){
	ERROR_IF(sockaddr == NULL, "sockaddr is NULL");
	memset((char*)sockaddr, 0, sizeof(sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = port;
	if(dest_addr == NULL){ // -> sockaddr local
		sockaddr->sin_addr.s_addr = INADDR_ANY;
	}
	else { // -> sockaddr destinataire
		ERROR_IF(hp == NULL, "hostent is NULL");
		struct in_addr addr;
		int ret = inet_pton(AF_INET, dest_addr, &addr);
		ERROR_IF(ret != 1, "Invalid IPv4 address format");
		hp = gethostbyaddr(&addr, sizeof(addr), AF_INET);
		ERROR_IF(hp == NULL, "hostent after gethostaddr is NULL");
		memcpy((char*) sockaddr->sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
	}
}

void main (int argc, char **argv) // "p:n:s:"
{
	int c;
	extern char *optarg;
	extern int optind;
	while ((c = getopt(argc, argv, "supl:n:")) != -1) {
		switch (c) {
		case 'p':
			if (gf == SOURCE) {
				usage();
				exit(1);
			}
			gf = PUITS;
			break;

		case 's':
			if (gf == PUITS) {
				usage();
				exit(1) ;
			}
			gf = SOURCE;
			break;
		
		case 'u':
			protocol = UDP;
			break;

		case 'n':
			nb_message = atoi(optarg);
			break;

		default:
			usage();
			break;
		}
	}

	if (gf == UNDEFINED) {
		usage();
		exit(EXIT_FAILURE) ;
	}

	if(protocol == UNDEFINED)
		protocol = TCP;

	
	//tsock -p [-options] port
	//tsock -s [-options] host port

	if (gf == SOURCE){
		if(protocol == TCP){
			
		}
	}
	else {
		
	}

	if (nb_message == -1) {
		if (gf == SOURCE)
			nb_message = 10;
		else 
			nb_message = INT_MAX;	
	}
}


/*
Ecrire une version v1 du programme tsock_v0.c :
-​ en ajoutant au programme la prise en compte de l’option -u ;
-​ basée sur l'utilisation du service fourni par UDP ;
-​ permettant l'échange de données de format le plus simple possible : chaînes de caractères de taille
fixe construite et affichée à l’aide des fonctions suivantes :
void construire_message(char *message, char motif, int lg) {
int i;
for (i=0;i<lg;i++) message[i] = motif;}.
void afficher_message(char *message, int lg) {
int i;
printf("message construit : ");
for (i=0;i<lg;i++) printf("%c", message[i]); printf("\n");}
*/

/*
*/