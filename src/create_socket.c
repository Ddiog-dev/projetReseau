#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int create_socket(struct sockaddr_in6 *source_addr, int src_port, struct sockaddr_in6 *dest_addr, int dst_port){

	int quelquechose = socket(AF_INET6, SOCK_DGRAM, 0);
	int err;
	if(quelquechose == -1){
		perror("socket error in create_socket");
		return -1;
	}
	if(src_port > 0){
		// Server
		if(source_addr != NULL){
			source_addr->sin6_port = htons(src_port);
			err = bind(quelquechose, (struct sockaddr*) source_addr, sizeof(struct sockaddr_in6));
			if(err != 0){
				perror("bind(2)");
				return -1;
			}
		}
	} else {
		// Client
		if(dest_addr != NULL){
			dest_addr->sin6_port = htons(dst_port);
			err = connect(quelquechose, (struct sockaddr*) dest_addr, sizeof(struct sockaddr_in6));
			if(err != 0){
				perror("connect(2)");
				return -1;
			}
		}
	}
	return quelquechose;
}