#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */
#include <signal.h>
#include <time.h>
#include "packet_interface.h"
#include "read_write_sender.h"
#define PKT_MAX_PAYLOAD 512

int window = 31; // Window size
uint8_t seq_eof; 
int eof_ack = 0; 

int wait_for_ack = 1;

/* Variables utilisées pour le sending buffer */
pkt_t_node *buffer_head = NULL;
pkt_t_node *buffer_tail = NULL;

int buffer_items = 0;

int max(int a, int b) {
	
	if(a>b) return a; 
	else return b;
	
}

void read_write_sender(const int sfd, const int fd){

	int window_flag = 1; // Sets to 0 when window runs out of free slots
	fd_set read_fd;
	int eof = 0;
	int err;
	ssize_t size;
	char* message = (char*) malloc(sizeof(char)*PKT_MAX_PAYLOAD+12);
	uint8_t seqnum = 0;
	last_ack = 0;
	FD_ZERO(&read_fd);
	while(!eof || !eof_ack){
		fflush(stdout);
		FD_ZERO(&read_fd);
		FD_SET(sfd, &read_fd);
		
		if(window_flag && seqnum <= last_ack + 31){
			FD_SET(fd, &read_fd);
		}
		
		struct timeval tv = {1,0};

		int i;
		
		i = select(max(sfd, fd) +1, &read_fd, NULL, NULL, &tv);
		


		//Detected something
		if(i > 0){ 
			/*
			*
			*	RECEIVING PACKETS
			*	
			*/
			if(FD_ISSET(sfd, &read_fd)){
				err = read(sfd, (void*) message, PKT_MAX_PAYLOAD+12);

				if(err == -1){
					perror("read 1");
					break;
				}
				size = (ssize_t)err;
				pkt_t * pkt = pkt_new();
				if(pkt == NULL) {
					perror("malloc");
					break;
				}


				size = (ssize_t) err;

				pkt_status_code status = pkt_decode(message, size, pkt);

				window = pkt_get_window(pkt);

				if(status != PKT_OK){
					fprintf(stderr, "Error in decode() - status code %d\n", status);
				}

					if(pkt_get_type(pkt) == PTYPE_ACK){

						// Manage seqnum of EOF packet
						uint8_t rseq = pkt_get_seqnum(pkt);
						if(seq_eof +1 == rseq && eof) eof_ack = 1;
						if(eof) wait_for_ack = 0;
						
						// Not an EOF
						last_ack = rseq;

						pkt_t *pkt_pop = pop_s(&buffer_head, &buffer_tail, rseq);

						while(pkt_pop != NULL){
							buffer_items --;
							window_flag = 1;

	// TODO This call segfaults; but why?
	// TODO Memleaks inside
							pkt_del(pkt_pop);

							pkt_pop = pop_s(&buffer_head, &buffer_tail, rseq);

						};
						pkt_del(pkt);
						if(is_in_buffer(&buffer_head, &buffer_tail, rseq +1) && eof){
							wait_for_ack = 0;
						}
					}
					 else {
						if(pkt_get_type(pkt) == PTYPE_DATA){
							fprintf(stderr, "Unexpected packet type (PTYPE_DATA)\n");
						}
					}
			}

			/*
			*
			*	SENDING PACKETS
			*
			*/
			if(FD_ISSET(fd, &read_fd) && !eof){
				err = read(fd, (void*)message, PKT_MAX_PAYLOAD);
				if(err == 0){
					// Read 0 bytes of data -> EOF reached. Send DATA packet with length 0.
					eof = 1;
					size = 524;
					char * dull = "";
					pkt_t * eofpkt = pkt_new();

					pkt_set_type(eofpkt, PTYPE_DATA);
					pkt_set_seqnum(eofpkt, seqnum);
					pkt_set_payload(eofpkt, dull, 0);
					pkt_status_code encode_error = pkt_encode(eofpkt, message, (size_t*) &size);
					if(encode_error != PKT_OK){
						fprintf(stderr, "Error encoding EOF packet\n");
					}
					err = write(sfd, (void*) message, size);
					if(err == -1){
						fprintf(stderr, "Error sending EOF packet on socket");
					}
					seq_eof = seqnum;

					int perr = push(&buffer_tail, &buffer_head, eofpkt);

					if(perr == -1){
						fprintf(stderr, "Error buffering packet\n");
						break;
					}
					buffer_items ++;
				} else {
					if(err == -1){
						perror("read 2");
						break;
					}
					
					pkt_t* p = pkt_new();
					if(p == NULL){
						perror("malloc");
						break;
					}

					size = (size_t)err;

					if(pkt_set_type(p, PTYPE_DATA) != PKT_OK){
						fprintf(stderr, "error in pkt_set_type()\n");
					}
					if(pkt_set_window(p, 0) != PKT_OK){
						fprintf(stderr, "error in pkt_set_window\n");
					}
					if(pkt_set_length(p, (uint16_t)size) != PKT_OK){
						fprintf(stderr, "error in pkt_set_length()\n");
					}
					if(pkt_set_payload(p, message, (uint16_t)size)!= PKT_OK){
						fprintf(stderr, "error in pkt_set_payload()\n");
					}
					if(pkt_set_seqnum(p, seqnum) != PKT_OK){
						fprintf(stderr, "error in pkt_set_seqnum()\n");
					}
					
					size = 524;
					
					pkt_status_code status = pkt_encode(p, message, (size_t*)&size);
					
					if(status != PKT_OK){
						fprintf(stderr, "Encoding failed (status code %d)\n", status);
						break;
					}
					// Envoi du paquet sur le socket
					err = write(sfd, (void*) message, size);
					if(err == -1){
						perror("write");
						break;
					}
					// Mise en buffer du paquet envoyé
					int perr = push(&buffer_tail, &buffer_head, p);
					if(perr == -1){
						fprintf(stderr, "Error buffering packet\n");
						break;
					}
					buffer_items ++;

					// Last buffer slot is used, setting appropriate flag
					if(buffer_items >= window){
						window_flag = 0;
					}
					seqnum++;
				}
			
			}

		}
		
		
	}
	free(message);
	
}


