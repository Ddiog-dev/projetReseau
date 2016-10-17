#include "real_address.h"
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>  
#include <stdio.h>  
#include <string.h>

const char * real_address(const char *address, struct sockaddr_in6 *rval) {


  struct addrinfo *res;
  struct addrinfo hints;

  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = 0;    /* For wildcard IP address */
  hints.ai_protocol = 0;          /* Any protocol */
  hints.ai_canonname = NULL;
  hints.ai_addr = 0;
  hints.ai_next = 0;

  int status = getaddrinfo(address, NULL, &hints, &res);

  if ( status != 0) {
    return gai_strerror(status);
  }
    
  memcpy(rval, res->ai_addr, res->ai_addrlen);
  
  free(res);
  

  return NULL;


}
