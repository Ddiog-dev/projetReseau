#include "real_address.h"
#include "create_socket.h"
#include "packet_interface.h"
#include "read_write_sender.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define PKT_MAX_PAYLOAD 512
#define SDR_WIN 31 /* window of the sender */

int f = 0;

int main(int argc, char ** argv){
	
	if(argc < 2){
		fprintf(stderr, "Too few arguments\n"
						"     Usage : \n"
						"     $ receiver [-f] [filename] <hostname> <port>\n");
		return 0;
	}
	
	int port;
	char* host = (char *) malloc(1024);
	char* filename;
	
	int i =1;
	int host_ok = 0; 
	
	//Lecture des arguments
	while (i < argc) {
		if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0){
		 filename = (char*) malloc(1024);
		 f = 1;
		 strcpy(filename, argv[i+1]);
		 i +=2;
		}
		else {
			if(!host_ok) {
				strcpy(host, argv[i]);
				host_ok = 1;
			}
			else port = atoi(argv[i]);
			i += 1; 
		}	
	}
	
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}
	
	// Sender is a client to the receiver
	int sfd = create_socket(NULL, -1, &addr, port);
	
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	
	int fd = 0;
	if(f){
		fd = open(filename, O_RDONLY);
		if(fd == -1){
			perror("open");
			return EXIT_FAILURE;	
		}
	}
	
	last_ack = 0;
	read_write_sender(sfd, fd);

	close(sfd);
	int err2;
	
	if(fd!= STDIN_FILENO && fd != -1) {
		err2 = close(fd); 
		if(err2 == -1) {
			perror("Close file");	
		}
	}
	
	free(host);
	if(f){
		free(filename);
	}
	fprintf(stderr, "Operation terminated\n");
	return 0;
}
