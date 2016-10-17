#include "create_socket.h"
int create_socket(struct sockaddr_in6 *source_addr,int src_port,struct sockaddr_in6 *dest_addr,int dst_port){
	int err;
	printf("ola 0 \n");
	int sfd=socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
	printf("ola 1 \n");
	if(sfd==-1){
    printf("ca marche paaaaas");
	perror("socket error in create_socket");
	return -1;
	}
	printf("ola 2 \n");
	if(src_port>0){
		printf("ola 2.1 \n");
        if(source_addr!=NULL){
		printf("ola 2.2 \n");
		source_addr->sin6_port = htons(src_port);
		err=bind(sfd,(struct sockaddr*)source_addr,sizeof(struct 	sockaddr_in6));
		printf("ola 2.3 \n");
		if(err!=0){
			perror("bind(2)");
			return -1;
			}
       }
       printf("ola 2.4 \n");
	}
	else{
        if(dest_addr!=NULL){
			dest_addr->sin6_port = htons(dst_port);
			err=connect(sfd,(struct sockaddr*)dest_addr, sizeof(struct sockaddr_in6));
			if(err!=0){
			perror("connect(2)");
			return -1;
			}
		}
	}
	printf("ola 3\n");
	return sfd;
}
