#include "wait_for_client.h"
int wait_for_client(int sfd)  {
	printf("wfc 1 \n");
  struct sockaddr_in6 src_addr;
  char* message=(char*)malloc(1024);
  memset(&src_addr,0,sizeof(struct sockaddr_in6));
  socklen_t addrlen = sizeof( src_addr);
  printf("wfc 2 \n");
  
  if( recvfrom(sfd, message, 1024, MSG_PEEK,
	       (struct sockaddr*) &src_addr, &addrlen) < 0) {
    perror("recvfrom()");
    printf("wfc 2.5 \n");
    return -1;
  }
  printf("wfc 3 \n");
  if( connect(sfd,(struct sockaddr*)  &src_addr, addrlen) < 0) {
     perror("connect()");
     return -1;
  }
  printf("wfc 4 \n");
  free(message);
  return 0;
}

