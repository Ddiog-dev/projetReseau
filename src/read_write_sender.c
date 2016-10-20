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
#include <string.h>
#include "packet_interface.h"
#include "read_write_sender.h"
#define PKT_MAX_PAYLOAD 512
int window = 31; // Window size
uint8_t seq_eof; 
int eof_ack = 0;
struct timeval rtt;

int wait_for_ack = 1;

/* Variables utilisées pour le sending buffer */
pkt_t_node *buffer_head = NULL;
pkt_t_node *buffer_tail = NULL;
/* variables du timeout buffer */
timeCheck *timeCheck_head = NULL;
timeCheck *timeCheck_tail = NULL;

int buffer_items = 0;
/*
 * retourne le maximum entre a et b
 */
int max(int a, int b) {
	
	if(a>b) return a; 
	else return b;
	
}
/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_sender(const int sfd, const int fd){
    rtt.tv_usec=3;
	int window_flag = 1; // Sets to 0 when window runs out of free slots
	fd_set read_fd;
	int eof = 0;
	int err;
    //TODO varibale buf timeCheck
    int time_err;
	ssize_t size;
	char* message = (char*) malloc(sizeof(char)*PKT_MAX_PAYLOAD+12);
	uint8_t seqnum = 0;
	last_ack = 0;
	FD_ZERO(&read_fd);
	while(!eof || !eof_ack){
        check_time_out(&timeCheck_head,&timeCheck_tail,&buffer_head,&buffer_tail,sfd);
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
                        // TODO ligne time out

                        time_err=remove_from_buffer(&timeCheck_head,&timeCheck_tail,rseq);
                        if(time_err!=0)printf("Erreur dans le remove du buffer timecheck (ligne 126 \n");

						while(pkt_pop != NULL){
							buffer_items --;
							window_flag = 1;
							pkt_del(pkt_pop);
                            //TODO ligne time out
							pkt_pop = pop_s(&buffer_head, &buffer_tail, rseq);
                            time_err=remove_from_buffer(&timeCheck_head,&timeCheck_tail,rseq);
                            if(time_err!=0)printf("Erreur dans le remove du buffer timecheck (ligne 126 \n");
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
					if(pkt_set_timestamp(p, (const uint32_t) 0x0fffffff) != PKT_OK){
						fprintf(stderr, "error in pkt_set_timestamp\n");
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
                    printf("sizeOfMessage %zu \n",sizeof(message));
                    printf("err= %d \n",err);
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
                    //TODO Push dans le buffe timecheck
                    perr=push_time_check(&timeCheck_head,&timeCheck_tail,p);
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
/*
 * crée une nouvelle structure timeCheck et initialise les champs a timestamp et seqnum
 */
timeCheck* init(pkt_t* pkt){
	timeCheck* newCheck=(timeCheck*)malloc(sizeof(timeCheck));
    if(newCheck==NULL)return NULL;
    newCheck->pkt=pkt_new();
    if(newCheck->pkt==NULL)return NULL;
	newCheck->pkt=memcpy(newCheck->pkt,pkt,sizeof(pkt_t));
	return newCheck;
}
/*
 * retire l'élément timeCheck possédant le numéro de séquence seq_wanted
 * renvoi -1 en cas d'erreur, 0 si on réussi a retirer l'élement voulu et 1 si il n'est pas dans le buffer
 */
int remove_from_buffer(timeCheck **list_head,timeCheck **list_tail, uint8_t seq_wanted){

	timeCheck* iter=*list_head;//on initialise le pointeur qui va nous permettre de parcourir la liste
    printf("remove 1 \n");
    printf(" pointeur iter= %p\n",iter);
	if(iter==NULL)return -1; // si la head est NULL
	while(iter->next!=NULL){ // tant qu'on est pas au dernier element du buffer
        printf(" seq num actu : %i \n",pkt_get_seqnum(iter->pkt));
        printf("remove 1.2 \n");
		if(seq_wanted>=pkt_get_seqnum((const pkt_t*)iter->pkt)){ // si le num de séquence fournis est supérieur ou égale au numéro courant
            if(iter==*list_head)*list_head=iter->next; // si le noueud courant est la tête on met la tête a jour
			if(iter->prev!=NULL)iter->prev->next=iter->next; // si iter -> prev ne pointe pas vers NULL
			iter->next->prev=iter->prev;// on met les pointeur a jour
			free(iter);
		}
		iter=iter->next;
	}
    printf("remove 2 \n");
	if(seq_wanted>=pkt_get_seqnum((const pkt_t*)iter->pkt)){ // si le num de séquence fournis est supérieur ou égale au numéro courant
		if((*list_head)==(*list_tail)){// si il y a 1 seul élément dans la liste
			free(iter);//
            printf("remove 2.1 \n");
			return 0;

		}
        printf("remove 2.2 \n");
        *list_tail=iter->prev; // on redirige la queue de la liste vers l'élement précédent
		free(iter);
		return 0;
	}
    printf("remove 3 \n");
	return 0;
}
/*
 * ajoute un l'élément elem au buffer de timeCheck
 * retourne 0 en cas de succès et -1 si elem pointe vers NULL
 */
int push_time_check(timeCheck **list_head,timeCheck **list_tail,pkt_t* pkt){
    printf("push 1 \n");
	if(pkt==NULL)return -1;
    timeCheck* elem=init(pkt);
    printf("push 2 \n");
	if(*list_head==NULL){ //si y a rien dans la liste
		*list_head=elem;
		*list_tail=elem;
		elem->next=NULL;
		elem->prev=NULL;
        printf("push 2.5 \n");
		return 0;
	}
    printf("push 3 \n");
	(*list_tail)->next=elem;//tout les autres cas
	elem->prev=(*list_tail);
	elem->next=NULL;
	(*list_tail)=elem;
    printf("push 4 \n");
	return 0;
}
/*
 * pre: list_head != null
 * renvoie la valeur timestamp du plus ancien élement du buffer
 */
uint32_t get_head_timestamp(timeCheck ** list_head){
	return pkt_get_timestamp((*list_head)->pkt);
}
/*
 * pre: list_head != null
 * renvoie la valeur seqnum du plus ancien élement du buffer
 */
uint8_t get_head_seqnum(timeCheck **list_head){
	return pkt_get_seqnum((*list_head)->pkt);
}
/*
 * set elem->timestamp à timestamp
 */
pkt_status_code set__timeCheck_timestamp(timeCheck* elem, time_t timestamp){
    pkt_status_code err;
    if(elem==NULL)return E_UNCONSISTENT;
    err=pkt_set_timestamp(elem->pkt,(uint32_t)timestamp);
    return err;
}
/*
 *renvoie tout les paquets qui ont time out et renvoie 0 si tout a bien été, retourne -1 en cas d'erreur
 */
int check_time_out(timeCheck** list_head, timeCheck** list_tail,pkt_t_node** buff_head,pkt_t_node** buff_tail,int sfd){
    //TODO changer ca plus tard
    time_t now;
    list_tail=list_tail;
    printf("checkTimeOut 1 \n");
    timeCheck* iter=*list_head;//on initialise le pointeur qui va nous permettre de parcourir la liste
    if(iter==NULL||buff_head==NULL)return 0; // si les listes sont vides
    uint32_t timestamp;
    printf("checkTimeOut 2 \n");
    int err;
    ssize_t size;
    char* message = (char*) malloc(sizeof(char)*PKT_MAX_PAYLOAD+12);// on alloue une zone message de 524 bytes
    while(iter->next!=NULL){
        printf("checkTimeOut 3 \n");
        timestamp=pkt_get_timestamp(iter->pkt);//on récupère le timestamp du pkt de iter
        if((time(&now)-timestamp)>=rtt.tv_usec){// on vérifie si le packet a time out
            printf("checkTimeOut 4 \n");
            pop_s(buff_head,buff_tail,pkt_get_seqnum(iter->pkt));// on retire le paquet du buffer sender qui possède le numéro de séquence de pkt
            //TODO On renvoit le paquet et on met le timestamp a jour
            set__timeCheck_timestamp(iter,time(&now));// on met le timestamp du packet timeCheck a jour
            pkt_status_code status = pkt_encode(iter->pkt, message, (size_t*)&size);// on encode pkt
            if(status != PKT_OK){
                fprintf(stderr, "Encoding failed (status code %d)\n", status);
                break;
            }
            printf("checkTimeOut 5 \n");
            // Envoi du paquet sur le socket
            err = write(sfd, (void*) message, size); // on envoie pkt
            printf("sizeOfMessage %zu \n",sizeof(message));
            printf("err= %d \n",err);
            if(err == -1){
                perror("write");
                break;
            }
            printf("checkTimeOut 6 \n");
            // Mise en buffer du paquet envoyé

            int perr = push(&buffer_tail, &buffer_head, iter->pkt);// on le remet dans le buffers des senders
            if(perr == -1){
                fprintf(stderr, "Error buffering packet\n");
                break;
            }
        }
        iter=iter->next;
    }
    timestamp=pkt_get_timestamp(iter->pkt);// on récupère le timestamp  de iter->pkt
    if(time(&now)-timestamp>=rtt.tv_usec){ // on vérifie si ca a time out
        printf("checkTimeOut 7 \n");
        pop_s(buff_head,buff_tail,pkt_get_seqnum(iter->pkt)); // on retire le paquet du buffer sender qui possède le numéro de séquence de pkt
        //TODO On renvoit le paquet et on met le timestamp a jour
        pkt_set_timestamp(iter->pkt,time(&now));
        pkt_status_code status = pkt_encode(iter->pkt, message, (size_t*)&size);
        if(status != PKT_OK){
            fprintf(stderr, "Encoding failed (status code %d)\n", status);
        }
        printf("checkTimeOut 8 \n");
        // Envoi du paquet sur le socket
        err = write(sfd, (void*) message, size);
        printf("sizeOfMessage %zu \n",sizeof(message));
        printf("err= %d \n",err);
        if(err == -1){
            perror("write");
        }
        printf("checkTimeOut 9 \n");
        // Mise en buffer du paquet envoyé

        int perr = push(&buffer_tail, &buffer_head, iter->pkt);
        if(perr == -1) {
            fprintf(stderr, "Error buffering packet\n");
        }
    }
    printf("checkTimeOut 10 \n");
    return 0;
}