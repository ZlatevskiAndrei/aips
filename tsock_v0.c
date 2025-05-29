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
#include<assert.h>

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
	UNKNOWN,
	UDP,
	TCP
};

enum gateaway_function gf = UNDEFINED;
enum transport_protocol protocol = UNKNOWN;
int nb_message = -1; /* Nb de messages à envoyer ou à recevoir, par défaut : 10 en émission, infini en réception */
int longueur = 30; 	//longueur = 30 octets par defaut pour puits et source

struct sockaddr_in dest_addr;
struct sockaddr_in local_addr;


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

void affect_sockaddr(struct sockaddr_in* sockaddr, int port, char* dest_addr){
	ERROR_IF(sockaddr == NULL, "sockaddr is NULL");
	memset((char*)sockaddr, 0, sizeof(sockaddr));
	sockaddr->sin_family = AF_INET;
	sockaddr->sin_port = port;
	if(dest_addr == NULL){ // -> sockaddr local
		sockaddr->sin_addr.s_addr = INADDR_ANY;
	}
	else { // -> sockaddr destinataire
		struct hostent* hp;
		ERROR_IF(hp == NULL, "hostent is NULL");
		struct in_addr addr;
		int ret = inet_pton(AF_INET, dest_addr, &addr);
		ERROR_IF(ret != 1, "Invalid IPv4 address format");
		hp = gethostbyaddr(&addr, sizeof(addr), AF_INET);
		ERROR_IF(hp == NULL, "hostent after gethostaddr is NULL");
		memcpy((char*) &sockaddr->sin_addr.s_addr, hp->h_addr_list[0], hp->h_length);
	}
}

int main (int argc, char **argv){
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

	if(protocol == UNKNOWN)
		protocol = TCP;


	if (protocol == UDP) {
		if (gf == SOURCE) {
			char* parsed_dest_addr = argv[optind];
			int port = atoi(argv[optind + 1]);
			printf("Addresse: %s\n", parsed_dest_addr);
			printf("Port: %d\n", port);
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating socket");
			affect_sockaddr(&dest_addr, htons(port), parsed_dest_addr);
			int lg_dest_addr = sizeof(dest_addr);
			char* msg = "message from local MKD";
			int sent = sendto(sock, msg, strlen(msg), 0, (struct sockaddr*) &dest_addr, lg_dest_addr);
			ERROR_IF(sent == -1, "Unable to send message");
		}
		else {
			int lg_adr_local = sizeof(local_addr);
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating local socket");
			int port = atoi(argv[optind]);
			affect_sockaddr(&local_addr, htons(port), NULL);
			ERROR_IF(bind(sock, (struct sockaddr*) &local_addr, lg_adr_local) == -1, "Error whilst binding the local socket to the address");
			char* recv_msg = malloc(23); // +1 for null-terminator
			assert(recv_msg != NULL);
			while (1) {
				socklen_t addrlen = sizeof(dest_addr);
				int recv = recvfrom(sock, recv_msg, longueur, 0, (struct sockaddr*) &dest_addr, &addrlen);
				ERROR_IF(recv == -1, "Error receiving UDP message");
				recv_msg[recv] = '\0'; 
				if(strcmp("message from local MKD", recv_msg) == 0){
					printf("Received message: %s\n", recv_msg);
					break;
				}
				sleep(1);
			}
			free(recv_msg);
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

	return 0;
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