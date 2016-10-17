#include "read_write_loop.h"
void read_write_loop(int sfd){
	fd_set rfds;
	
    struct timeval timeout;
    timeout.tv_sec=0;
    timeout.tv_usec=4;
    int eof=0;
    int err,errIO;
    char* texte=(char*)malloc(1024);
    while(!eof){
		FD_ZERO(&rfds);
		FD_SET(sfd, &rfds);
		FD_SET(STDIN_FILENO, &rfds);
		err=select(sfd+1,&rfds,NULL,NULL,&timeout);
		if(err >= 0){
			if(FD_ISSET(sfd,&rfds)){
				errIO=read(sfd, (void*) texte, 1024);
				if(errIO==-1){
				printf("erreur read");
				break;
				}
				errIO=write(STDOUT_FILENO, (void*) texte, errIO);
				if(errIO==-1){
				printf("erreur write");
				break;
				}
			}
			if(FD_ISSET(STDIN_FILENO, &rfds)){
				errIO=read(STDIN_FILENO, (void*)texte, 1024);
				if(errIO == 0){
						eof = 1;
						break;
					}
				if(errIO== -1){
					perror("read");
					break;
				}
				errIO=write(sfd, (void*) texte, errIO);
				if(errIO== -1){
					perror("read");
					break;
				}
			}
	}	
  }
	free(texte);
}
