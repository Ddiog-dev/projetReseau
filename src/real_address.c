#include "real_address.h"
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>  
#include <stdio.h>  
#include <string.h>

const char * real_address(const char *address, struct sockaddr_in6 *rval) {


  struct addrinfo *res;
  struct addrinfo hints;

  memset(&hints, 0 , sizeof(hints));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  int status = getaddrinfo(address, NULL, &hints, &res);

  if ( status != 0) {
    return gai_strerror(status);
  }
    
  memcpy(rval, res->ai_addr, res->ai_addrlen);
  
  free(res);
  

  return NULL;


}
