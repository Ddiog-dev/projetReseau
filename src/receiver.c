#include "real_address.h"
#include "create_socket.h"
#include "packet_interface.h"
#include "wait_for_client.h"
#include "read_write_receiver.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#define MSG_LEN 520
#define WINDOW_MAX 31
#define PKT_MAX_PAYLOAD 512

uint8_t window_max;
uint8_t window_actual;
uint8_t seq_actual;
int eof_seq = 300;



int main(int argc, char ** argv){
	
	last_congestion = MAX_WINDOW_SIZE +1; 
	
	if(argc < 2){
		fprintf(stderr, "Too few arguments\n"
						"     Usage : \n"
						"     $ receiver [-f] [filename] <hostname> <port>\n");
		return 0;
	}
	
	int port;
	char* host = (char *) malloc(1024);
	char* filename = (char*) malloc(1024);
	
	int i =1;
	int host_ok = 0; 
	
	//Lecture des arguments
	while (i < argc) {
		if(strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--filename") == 0){
		 fd = 1;
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
	
	if(fd){
		fd = open_file(filename);
		if(fd == -1) {
			perror("Open file"); 
			return -1 ; 
		}
		
	} else {
		fd = STDOUT_FILENO; 
	}
	
	struct sockaddr_in6 addr;
	const char *err = real_address(host, &addr);
	if (err) {
		fprintf(stderr, "Could not resolve hostname %s: %s\n", host, err);
		return EXIT_FAILURE;
	}
	
	
	// Receiver is a server to the sender
	int sfd = create_socket(&addr, port, NULL, -1); /* Bound */
	if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */
		fprintf(stderr,
				"Could not connect the socket after the first message.\n");
		close(sfd);
		return EXIT_FAILURE;
	}
	if (sfd < 0) {
		fprintf(stderr, "Failed to create the socket!\n");
		return EXIT_FAILURE;
	}
	


	//Création du buffer
	char *buffer = (char*) malloc(sizeof(char)*MSG_LEN);
	size_t length_buf = MSG_LEN;
	pkt_status_code pkt_stat;


	pkt_t_node **list_head = (pkt_t_node **) malloc(sizeof(pkt_t_node**)); 
	*list_head = NULL;
	pkt_t_node **list_tail = (pkt_t_node **) malloc(sizeof(pkt_t_node**));
	*list_tail = NULL;
	
	window_max = WINDOW_MAX;
	window_actual = window_max;
	seq_actual = 0;
	
	int err2;

	//Variable pour endoffile 
	int eof = 0;
	

	while((!eof && seq_actual != eof_seq + 1) || !eof) {
		
		
	  //Lecture du socket en attendant que quelques chose arrive 
	  size_t size_read = read_buf(sfd, buffer, length_buf);
	  

	  //Création d'un nouveau packet
	  pkt_t *pkt = pkt_new();
	
	  //Décoder ce qui arrive sur le buffer
	  pkt_stat = pkt_decode(buffer, size_read, pkt);
	  
	  if(pkt_stat == E_LENGTH)  fprintf(stderr, "decode : erreur de taille de pkt_t\n");	

	  //Endof file
	  if(pkt_get_type(pkt) == PTYPE_DATA && pkt_get_length(pkt) == 0) {
			if(pkt_get_seqnum(pkt) == seq_actual) {
				seq_actual += 1;
				err2 = send_ack(sfd, PTYPE_ACK); 
				if(err2 == E_NOMEM) fprintf(stderr, "send_ack : erreur de mémoire\n");
				if(err2 == E_TYPE) fprintf(stderr, "send_ack : erreur de type de packet reçu\n");
				eof = 1; 
				eof_seq = pkt_get_seqnum(pkt);
				pkt_del(pkt);
			}
			
			eof_seq = pkt_get_seqnum(pkt);
	  }
	  
	  if(seq_actual > eof_seq) eof = 1;
	 
	
	  if(!eof) {
		//Si CRC ne correspondent pas ou pas de header, envoie un ptype_ack 
		if(pkt_stat == E_CRC || pkt_stat == E_NOHEADER || pkt_stat == E_TYPE || pkt_stat == E_WINDOW) {
			if(pkt_stat == E_NOHEADER) fprintf(stderr, "decode : pas de header\n");
			if(pkt_stat == E_CRC) fprintf(stderr, "decode : erreur de CRC, sqwait : %d\n", seq_actual); 
			if(pkt_stat == E_WINDOW) fprintf(stderr, "decode : erreur de window\n"); 
			if(pkt_stat == E_TYPE) fprintf(stderr, "decode : erreur de type de pkt_t\n"); 
			err2 = send_ack(sfd, PTYPE_ACK);
			if(err2 == E_NOMEM) fprintf(stderr, "send_ack : erreur de mémoire\n");
			if(err2 == E_TYPE) fprintf(stderr, "send_ack : erreur de type de packet reçu\n"); 
		}


		//Le packet décodé est valide, écriture sur la sortie ou bien dans la pile SI fenêtre correspond 
		if(pkt_stat == PKT_OK) {
			err2 = write_buf(sfd ,pkt, list_head,  list_tail);
			if(err2 == E_TYPE) fprintf(stderr, "write_buf : erreur de type de pkt_t attendu\n"); 
			if(err2 == E_SEQNUM) fprintf(stderr, "write_buf : pkt_t pas dans la window\n");
		}
	  
	}

       
	}
	
	//Fermeture du fichier si écriture sur fichier
	if(fd!= STDOUT_FILENO && fd != -1) {
		err2 = close(fd); 
		if(err2 == -1) {
			perror("Close file");	
		}
	}
		
	// Free de toutes les instances buffers + pile	
	free(list_head);
	free(list_tail);
	free(buffer);
	free(host);
	free(filename);
	
	int time = clock();
	
	while((clock() - time)/CLOCKS_PER_SEC < 1){ }
	
	//Fermeture du socket
	err2 = close(sfd);
	if(err2 == -1) perror("Close socket"); 
	
	return 0;
	
}
