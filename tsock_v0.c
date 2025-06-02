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
#include <assert.h>
#include <sys/time.h>
#include <signal.h>

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
socklen_t lg_dest_addr = sizeof(dest_addr);
socklen_t lg_adr_local = sizeof(local_addr);

int port;
char* parsed_dest_addr;

struct timeval tv;



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

char* prompt_and_build_message(int longueur) {
    char c;
    printf("Please type 1 character and press ENTER: ");
    int is_one = scanf(" %c", &c);
    ERROR_IF(is_one != 1, "Error whilst scanning char");
    char* msg = malloc(longueur);
    ERROR_IF(msg == NULL, "Error whilst allocating mem");
    memset(msg, c, (size_t)longueur);
    return msg;
}

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

void affect_ip_components(char* parsed_dest_addr, int* port, char* argument1, char* argument2){
	if(argument1 == NULL)
		parsed_dest_addr = NULL;
	else 
		parsed_dest_addr = argument1;
	*port = atoi(argument2);
	printf("Address: %s\n", parsed_dest_addr);
	printf("Port: %d\n", *port);
}

int main (int argc, char **argv){
	tv.tv_sec = 0; 
	signal(SIGCHLD, SIG_IGN);
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
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating socket");
			affect_ip_components(parsed_dest_addr, &port, argv[optind], argv[optind + 1]);
			affect_sockaddr(&dest_addr, htons(port), parsed_dest_addr);
			char* msg = prompt_and_build_message(longueur);
			int sent = sendto(sock, msg, longueur, 0, (struct sockaddr*) &dest_addr, lg_dest_addr);
			ERROR_IF(sent == -1, "Unable to send message");
			free(msg);
			close(sock);
		}
		else {
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating local socket");
			affect_ip_components(parsed_dest_addr, &port, NULL, argv[optind]);
			affect_sockaddr(&local_addr, htons(port), NULL);
			ERROR_IF(bind(sock, (struct sockaddr*) &local_addr, lg_adr_local) == -1, "Error whilst binding the local socket to the address");
			char* recv_msg = malloc(longueur + 1);
			ERROR_IF(recv_msg == NULL, "Error whilst allocating mem" );
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
			while (1) {
				int recv = recvfrom(sock, recv_msg, longueur, 0, (struct sockaddr*) &dest_addr, &lg_dest_addr);
				recv_msg[recv] = '\0'; 
				if(strlen(recv_msg) == longueur){
					printf("Received message: %s\n", recv_msg);
					break;
				}
				printf("Waiting for a message...\n");
				sleep(1);
			}
			free(recv_msg);
			close(sock);
		}
	}
	else {
		if(gf == SOURCE) {
			affect_ip_components(parsed_dest_addr, &port, argv[optind], argv[optind + 1]);
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating socket");
			affect_sockaddr(&dest_addr, htons(port), parsed_dest_addr);
			int conn = connect(sock, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
			ERROR_IF(conn == -1, "Can't connect to server");
			char* msg = prompt_and_build_message(longueur);
			int sent = write(sock, msg, longueur);
			ERROR_IF(sent == -1, "Unable to send message");
			free(msg);
			int shutdown_ret = shutdown(sock, 2);
			ERROR_IF(shutdown_ret == -1, "Can't shutdown connection");
			close(sock);
		}
		else {
			int sock, sock_bis;
			sock =  socket(AF_INET, SOCK_STREAM, 0);
			affect_ip_components(parsed_dest_addr, &port, NULL, argv[optind]);
			affect_sockaddr(&local_addr, htons(port), NULL);
			ERROR_IF(bind(sock, (struct sockaddr*) &local_addr, lg_adr_local) == -1, "Error whilst binding the local socket to the address");
			listen(sock, 1);
			while(1){
				printf("Waiting for a new connection...\n");
				sock_bis = accept(sock, (struct sockaddr*) &dest_addr, &lg_dest_addr);
				ERROR_IF(sock_bis == -1, "Error whilst accepting new connection");
				switch(fork()){
					case -1:
						fprintf(stderr, "Fork error\n");
						exit(EXIT_FAILURE);
					case 0:
						close(sock);
						pid_t process = getpid();
						char* recv_msg = malloc(longueur + 1);
						ERROR_IF(recv_msg == NULL, "Error whilst allocating mem" );
						setsockopt(sock_bis, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
						while(1){
							printf("Waiting a message for process %d\n", (int)process);
							int recv = read(sock_bis, recv_msg, (size_t)longueur);
							if (recv == 0) {
            					printf("Client disconnected\n");
            					break;
        					}
							ERROR_IF (recv == -1, "Error whilst reading TCP data");
							recv_msg[recv] = '\0'; 
							if(strlen(recv_msg) == longueur){
								printf("Received message: %s\n", recv_msg);
								break;
							}
						}
						int shutdown_ret = shutdown(sock_bis, 2);
						close(sock_bis);
						ERROR_IF(shutdown_ret == -1, "Error whilst disconnecting");
						free(recv_msg);
						exit(EXIT_SUCCESS);
					default:
						close(sock_bis);
				}
			}
		}
	}

	if (nb_message == -1) {
		if (gf == SOURCE)
			nb_message = 10;
		else 
			nb_message = INT_MAX;	
	}

	return 0;
}