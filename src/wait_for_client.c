#include "wait_for_client.h"
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>  
#include <stdio.h>



/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd)  {

  struct sockaddr_in6 src_addr;
  //char t[1024];
  socklen_t addrlen = sizeof(src_addr);
  //printf("Waiting for client\n");
  if( recvfrom(sfd, NULL, 0, MSG_PEEK,
	       (struct sockaddr*) &src_addr, &addrlen) < 0) {

    perror("recvfrom()");

    return -1;
  }


  if( connect(sfd,(struct sockaddr*)  &src_addr, addrlen) < 0) {

     perror("connect()");

     return -1;

  }


    
  return 0;
  
  
}
