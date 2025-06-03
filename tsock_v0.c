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
int nb_message = -1; 
int longueur = 30; 	

struct sockaddr_in dest_addr;
struct sockaddr_in local_addr;
socklen_t lg_dest_addr = sizeof(dest_addr);
socklen_t lg_adr_local = sizeof(local_addr);
char alphabet[] = {
    'a','b','c','d','e','f','g','h','i','j','k','l','m',
    'n','o','p','q','r','s','t','u','v','w','x','y','z'
};
int port;
char* parsed_dest_addr;

void usage() {
	printf("usage: cmd [-p|-s][-n ##]\n");
}

int count_digits(int num){
	if(num == 0) return 1;
	int digits = 0;
	while(num != 0){
		digits++;
		num /= 10;
	}
	return digits;
}

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
	if(dest_addr == NULL){ 
		sockaddr->sin_addr.s_addr = INADDR_ANY;
	}
	else { 
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

void affect_ip_components(char **parsed_dest_addr, int* port, char* argument1, char* argument2){
    if(argument1 == NULL)
        *parsed_dest_addr = NULL;
    else {
        *parsed_dest_addr = malloc(strlen(argument1) + 1);
        ERROR_IF(*parsed_dest_addr == NULL, "Error allocating memory for dest addr");
        strcpy(*parsed_dest_addr, argument1);
    }
    *port = atoi(argument2);
    printf("Address: %s\n", *parsed_dest_addr);
    printf("Port: %d\n", *port);
}

void send_messages(enum transport_protocol protocol, int sock){
	printf("SOURCE: size_msg= %d, port=%d, nb_messages=%d, TP=%d, dest=%s\n", longueur, port, nb_message, protocol, parsed_dest_addr);
	char* msg = malloc(longueur);
	ERROR_IF(msg == NULL, "Can't allocate mem");
	for (int i = 1; i <= nb_message; i++) {
		int num_dashes = 5 - count_digits(i);
		if (num_dashes < 0 || i > 99999) {
			fprintf(stderr, "Limit amount of messages sent exceeded, exiting...\n");
			exit(EXIT_FAILURE);
		}
		memset(msg, '-', num_dashes); 
		int n = snprintf(msg + num_dashes, longueur - num_dashes, "%d", i); 
		char fill_char = alphabet[(i-1) % 26];
		int fill_start = num_dashes + n;
		if (fill_start < longueur) {
			memset(msg + fill_start, fill_char, longueur - fill_start);
		}
		int sent;
		if(protocol == UDP){
			sent = sendto(sock, msg, longueur, 0, (struct sockaddr*) &dest_addr, lg_dest_addr);
		}
		else {
			sent = write(sock, msg, longueur);
		}
		ERROR_IF(sent == -1, "Unable to send message");
		printf("SOURCE: Message #%d (%d) [%.*s]\n", i, longueur, longueur, msg);
	}
	printf("End\n");
	free(msg);
}

void receive_messages(enum transport_protocol protocol, int sock){
	char* recv_msg = malloc(longueur + 1);
	ERROR_IF(recv_msg == NULL, "Error whilst allocating mem" );
	printf("PUITS: size_msg=%d, port=%d, nb_messages_allowed=%d, TP=%d\n", longueur, port, nb_message, protocol);
	for (int i = 1; i <= nb_message; i++) {
		int recv;
		if(protocol == TCP){
			recv = read(sock, recv_msg, (size_t)longueur);
			if (recv == 0) {
            	printf("Client disconnected\n");
            	break;
        	}
		}
		else {
			recv = recvfrom(sock, recv_msg, longueur, 0, (struct sockaddr*) &dest_addr, &lg_dest_addr);
		}
		ERROR_IF (recv == -1, "Error whilst reading data");
		recv_msg[recv] = '\0'; 
		printf("PUITS: Received message #%d (%d) [%.*s]\n", i, longueur, longueur, recv_msg);
	}
	free(recv_msg);
}

int main (int argc, char **argv){
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

		case 'l':
			longueur = atoi(optarg);
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

	if (nb_message == -1) {
		if (gf == SOURCE)
			nb_message = 10;
		else 
			nb_message = INT_MAX;	
	}

	if(protocol == UNKNOWN)
		protocol = TCP;


	if (protocol == UDP) { 
		if (gf == SOURCE) {
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating socket");
			affect_ip_components(&parsed_dest_addr, &port, argv[optind], argv[optind + 1]);
			affect_sockaddr(&dest_addr, htons(port), parsed_dest_addr);
			send_messages(protocol, sock);
			free(parsed_dest_addr);
			close(sock);
		}
		else {
			int sock = socket(AF_INET, SOCK_DGRAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating local socket");
			affect_ip_components(&parsed_dest_addr, &port, NULL, argv[optind]);
			affect_sockaddr(&local_addr, htons(port), NULL);
			ERROR_IF(bind(sock, (struct sockaddr*) &local_addr, lg_adr_local) == -1, "Error whilst binding the local socket to the address");
			receive_messages(protocol, sock);
			free(parsed_dest_addr);
			close(sock);
		}
	}
	else {
		if(gf == SOURCE) {
			affect_ip_components(&parsed_dest_addr, &port, argv[optind], argv[optind + 1]);
			int sock = socket(AF_INET, SOCK_STREAM, 0);
			ERROR_IF(sock == -1, "Error whilst creating socket");
			affect_sockaddr(&dest_addr, htons(port), parsed_dest_addr);
			int conn = connect(sock, (struct sockaddr*) &dest_addr, sizeof(dest_addr));
			ERROR_IF(conn == -1, "Can't connect to server");
			send_messages(protocol, sock);
			int shutdown_ret = shutdown(sock, 2);
			ERROR_IF(shutdown_ret == -1, "Can't shutdown connection");
			free(parsed_dest_addr);
			close(sock);
		}
		else {
			int sock, sock_bis;
			sock =  socket(AF_INET, SOCK_STREAM, 0);
			affect_ip_components(&parsed_dest_addr, &port, NULL, argv[optind]);
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
						printf("We are in child process with PID %d\n", process);
						receive_messages(protocol, sock_bis);
						free(parsed_dest_addr);
						int shutdown_ret = shutdown(sock_bis, 2);
						ERROR_IF(shutdown_ret == -1, "Error whilst disconnecting");
						close(sock_bis);
						exit(EXIT_SUCCESS);	
					default:
						close(sock_bis);
				}
			}
		}
	}
	return 0;
}