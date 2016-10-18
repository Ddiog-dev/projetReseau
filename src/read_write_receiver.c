#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

#include "read_write_receiver.h"
#include "packet_interface.h"

#define PKT_MAX_PAYLOAD 512


size_t read_buf(const int sfd, char *buffer, const size_t buf_length) {

        fd_set read_fd;
		int err;
		int size = 0;

		FD_ZERO(&read_fd);
		FD_SET(sfd, &read_fd);
	
		int i = select(sfd+1, &read_fd, NULL, NULL, NULL);
		
		if(i >= 0) {

		  	if(FD_ISSET(sfd, &read_fd)){
			
			  err = read(sfd, (void*) buffer, buf_length);
			  
			  if(err == -1){
			    perror("read");
			  }
				size = err;	
			}
			return size;
		}
		return -1;
}


int send_ack(const int sfd, const ptypes_t ptype) {
	if(ptype != PTYPE_ACK && ptype != PTYPE_NACK) {
		return E_TYPE;	
	}

  char* message = (char*) malloc(sizeof(char)*PKT_MAX_PAYLOAD);
  
  pkt_t* pkt = pkt_new();
  if(pkt == NULL){
    perror("malloc");
    return E_NOMEM;
  }
  size_t size = 0;
  int err;
  
  if(ptype == PTYPE_NACK) {
	  uint8_t mod = window_max % 2;
	  last_congestion = window_max;
	  if(mod != 0) {
		  window_max = window_max - mod;
	  }
	  
	  //Si congestion, on divise la window en deux 
	  window_max = window_max/2; 
	  
	  //Si trop de packet dans le buffer par rapport à la nouvelle window max, la window max prend la valeur
	  //du nombre de pkt dans le buffer et window_actual est donc mis à 0 
	  if(last_congestion - window_actual > window_max) {
		  
		  window_max = last_congestion - window_actual;
		  window_actual = 0; 
		
	  }
	  
	  //Calcul de la nouvelle window_actual
	  else {
		window_actual = window_max - (last_congestion - window_actual);
	  }  
  }
	//Lorsqu'on envoie un ack, on augmente la taille max de la window de 1
	// jusqu'en dessous	de la dernière valeur ou il y a eu congestion 
	if(ptype == PTYPE_ACK && window_max < last_congestion - 1) {
		window_max = window_max + 1;
		window_actual = window_actual + 1; 
	}
	
  err = pkt_set_type(pkt, ptype);
  if(err != PKT_OK) fprintf(stderr, "Send_ack : erreur de set_type\n");
  pkt_set_window(pkt, (uint8_t) window_actual);
  if(err != PKT_OK) fprintf(stderr, "Send_ack : erreur de set_window\n");
  pkt_set_length(pkt, (uint16_t) size);
  if(err != PKT_OK) fprintf(stderr, "Send_ack : erreur de set_length\n");
  pkt_set_payload(pkt, NULL, (uint16_t)size);
  if(err != PKT_OK) fprintf(stderr, "Send_ack : erreur de set_payload\n");
  pkt_set_seqnum(pkt, seq_actual);
  if(err != PKT_OK) fprintf(stderr, "Send_ack : erreur de set_seqnum\n");
					
  size = 524;

  pkt_encode(pkt, message, (size_t*)&size);				
  err = write(sfd, (void*) message, size);
	if(err<0) { 
		perror("Write");
		return -1 ;
}	

	free(message);
	pkt_del(pkt);
  
	return 0;
}


pkt_status_code write_buf(const int sfd, pkt_t *pkt, pkt_t_node **list_head,  pkt_t_node **list_tail) {
	
  int err;
  
  if(pkt_get_type(pkt) == PTYPE_DATA) {
	  
	int cond = 0;
	
	if((uint8_t) (seq_actual + window_max) < window_max) {
		cond = (pkt_get_seqnum(pkt) > (uint8_t) (seq_actual + window_max) && pkt_get_seqnum(pkt) < seq_actual);
	}
	else {
		cond = pkt_get_seqnum(pkt) < seq_actual && pkt_get_seqnum(pkt) > seq_actual + window_max;
	}	
  
    if(cond){
    	return E_SEQNUM;
    }
    // C'est le bon packet 
    if(pkt_get_seqnum(pkt) == seq_actual) {

      //ecriture sur le stdout
	  //on vide la pile jusqu'à ce qu'on atteigne la fin de la liste, on envoie un ack avec le num de packet attendu.
      
	  err = write(fd, (void*) pkt_get_payload(pkt), pkt_get_length(pkt));
      if(err < 0) fprintf(stderr, "write_buf : write stdout\n");
      if(err == -1) perror("write");
  
      fflush(stdout);

      seq_actual += 1;
     
      pkt_t *pkt_pop = pop(list_head, list_tail, (int) seq_actual); 

      //Parcours de la liste
      
      while(pkt_pop != NULL) {
			//Enleve un slot vide
			window_actual += 1;
			//Augmente le seq attendu
			seq_actual += 1;
			err = write(fd, (void*) pkt_get_payload(pkt_pop),  pkt_get_length(pkt_pop));
			
			if(err == -1){
				perror("write");
			}
			fflush(stdout);
			pkt_del(pkt_pop);
			pkt_pop = pop(list_head, list_tail, (int) seq_actual);
      }
 
      pkt_del(pkt);
      
      //Renvoier un ack avec le packet attendu
      err = send_ack(sfd, PTYPE_ACK);
      if(err == E_NOMEM) fprintf(stderr, "send_ack : erreur de mémoire\n");
	  if(err == E_TYPE) fprintf(stderr, "send_ack : erreur de type de packet reçu\n"); 
      
      return PKT_OK;
		   
    }


    //Stocker le packet dans la liste chainée
    
    if(window_actual == 0) return E_NOMEM;
    
    if(!is_in_buffer(list_head, list_tail, pkt_get_seqnum(pkt))) {
		err = push(list_tail, list_head, pkt);
		if(err == -1) {
			fprintf(stderr, "write on stack error %d\n", err);
		}
		window_actual -= 1;
		//Renvoier un ack avec le packet attendu
		err = send_ack(sfd, PTYPE_ACK);
		if(err == E_NOMEM) fprintf(stderr, "send_ack : erreur de mémoire\n");
		if(err == E_TYPE) fprintf(stderr, "send_ack : erreur de type de packet reçu\n"); 
	}
    
	
	else pkt_del(pkt);
	return PKT_OK;

  }

  err = send_ack(sfd, PTYPE_ACK);
  //Si différent de PTYPE_DATA
  return E_TYPE;


}

//retourne le descripteur de fichier 
int open_file(const char* path_name)  {
	if(path_name == NULL) return -1;
	return open(path_name, O_WRONLY|O_CREAT|O_CLOEXEC, S_IRUSR|S_IWUSR);	
}
