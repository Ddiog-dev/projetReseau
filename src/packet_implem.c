#include "packet_interface.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <zlib.h>
/* Extra #includes */
/* Your code will be inserted here */
struct __attribute__((__packed__)) pkt {
    uint8_t	window:5;
    ptypes_t type:3;
    uint8_t seqnum;
    uint16_t length;
    uint32_t Timestamp;
    char * payload;
    uint32_t CRC;
};
/* Extra code */
/* Your code will be inserted here */
pkt_t* pkt_new()
{
    pkt_t* newStruct=(pkt_t*)malloc(sizeof(pkt_t));
    if (newStruct==NULL)return NULL;
    return newStruct;
}
void pkt_del(pkt_t *pkt)
{
    if(pkt==NULL)return;
    if(pkt->payload!=NULL)free(pkt->payload);
    free(pkt);
}
uint32_t getCRC(const pkt_t * pkt){
    unsigned long int crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, (unsigned char*)pkt, 8);
    crc = crc32(crc, (unsigned char*)(pkt->payload), pkt_get_length(pkt));
    return (uint32_t)crc;
}

pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt)
{
    pkt_status_code  err;
    //on décode window + type + seqnum + timestamp
    memcpy((void*)pkt,(const void*)data,sizeof(uint64_t));
    //si payload est NULL on fait un malloc
    if(pkt->payload==NULL)pkt->payload=(char*)malloc(pkt_get_length(pkt));
    //on copie le contenue du payload dans la zone payload
    memcpy((void*)pkt->payload,(const void*)data+(sizeof(uint64_t)),pkt_get_length(pkt));
    //on récupère l'ancien CRC, on calcule un nouveau CRC et on enregistre l'ancien dans pkt
    uint32_t new_crc =getCRC(pkt);
    uint32_t old_crc=*(uint32_t*)(data+8+pkt_get_length(pkt));
    old_crc =ntohl(old_crc);
    err=pkt_set_crc(pkt,new_crc);// on set le crc
    if(err!=PKT_OK){
        return E_UNCONSISTENT;
    }
    //on verifie que les valeurs sont corrects
    if(pkt_get_crc(pkt)!=old_crc)return E_CRC;
    if((size_t)pkt_get_length(pkt)+12!=len)return E_LENGTH;
    if(pkt->type!=PTYPE_ACK&&pkt->type!=PTYPE_DATA)return E_TYPE;
    return PKT_OK;


}
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len)
{
    if(pkt==NULL || buf==NULL)return E_UNCONSISTENT;
    if(*len<12)return E_LENGTH;
    int size= pkt_get_length(pkt)+3*sizeof(uint32_t);
    if(12+pkt_get_length(pkt) > (uint16_t )(*len))
        return E_NOMEM;
    uint16_t sizeLen=pkt->length;// on récupère pkt->length en network byte order
    //encodage type + window +seqnum
    memcpy((void*)buf,(const void*)pkt,sizeof(uint16_t));
    //encodage length
    memcpy((void*)buf+sizeof(uint16_t),(const void*)&sizeLen,sizeof(uint16_t));
    //encodage TimeStamp
    memcpy((void*)buf+sizeof(uint32_t),(const void*)&pkt->Timestamp,sizeof(uint32_t));
    //encodage payload
    if(pkt->payload!=NULL)strncpy(buf+sizeof(uint64_t),(const char*)pkt->payload,pkt_get_length(pkt));
    //calcule CRC + encodement
    uint32_t new_crc = getCRC(pkt);
    new_crc=htonl(new_crc);
    memcpy((void*)buf+sizeof(uint64_t)+pkt_get_length(pkt),(const void*)&new_crc,sizeof(uint32_t));
    *len=size;
    return PKT_OK;

}
ptypes_t pkt_get_type  (const pkt_t* pkt)
{
    return pkt->type;
}
uint8_t  pkt_get_window(const pkt_t* pkt)
{
    return pkt->window;
}
uint8_t  pkt_get_seqnum(const pkt_t* pkt)
{
    return pkt->seqnum;
}
uint16_t pkt_get_length(const pkt_t* pkt)
{
    return ntohs(pkt->length);
}
uint32_t pkt_get_timestamp   (const pkt_t* pkt)
{
    return pkt->Timestamp;
}
uint32_t pkt_get_crc   (const pkt_t* pkt)
{
    return ntohl(pkt->CRC);
}
const char* pkt_get_payload(const pkt_t* pkt)
{
    if(pkt->payload==NULL)return NULL;
    return pkt->payload;
}
pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    if(type==PTYPE_DATA || type==PTYPE_ACK){
        pkt->type=type;
        return PKT_OK;
    }
    return E_TYPE;
}
pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    if(window>MAX_WINDOW_SIZE)return E_WINDOW;
    pkt->window=window;
    return PKT_OK;
}
pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    pkt->seqnum=seqnum;
    return PKT_OK;
}
pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    pkt->length=htons(length);
    return PKT_OK;
}
pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    pkt->Timestamp=timestamp;
    return PKT_OK;
}
pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    pkt->CRC=htonl(crc);
    return PKT_OK;
}
pkt_status_code pkt_set_payload(pkt_t *pkt,const char *data,const uint16_t length)
{
    if(pkt==NULL)return E_UNCONSISTENT;
    if(pkt->payload==NULL)pkt->payload=(char*)malloc(sizeof(char)*length);
    if(pkt->payload==NULL)return E_NOMEM;
    memcpy((void*)pkt->payload,data,length);
    pkt_set_length(pkt,length);
    return PKT_OK;
}




// Enqueues the pacekt *pkt onto the buffer, on the TAIL side
int push(pkt_t_node **list_tail, pkt_t_node **list_head, pkt_t *pkt) {
	
   if((*list_head) == NULL) *list_tail = NULL;
   if((*list_tail) == NULL) *list_head = NULL; 

   pkt_t_node *new = (pkt_t_node *) malloc(sizeof(pkt_t_node));
   
   new->pkt = pkt;
   new->next = NULL;
   new->stime = clock();
   
	if(new == NULL)  {
	  perror("malloc");
	  return -1;
	}

  //si la liste est vide, pointeur vers le premier élément
  if(*list_tail == NULL) {
     *list_tail = new;
     *list_head = new;
     
     return 0;
   }

  (*list_tail)->next = new;

  (*list_tail) = (*list_tail)->next;
  
  (*list_tail)-> next = NULL;

  return 0;
  
}

// Dequeues and returns the HEAD packet of the buffer, for the receiver
// @see read_write_receiver.c
pkt_t *pop(pkt_t_node **list_head, pkt_t_node **list_tail, uint8_t seq_waited) {

  if(*list_head == NULL) return NULL;
  
   pkt_t_node *prev = *list_head;
   pkt_t_node *actual = *list_head;
   
   if(actual == *list_tail && (actual -> pkt) -> seq != seq_waited) {
			return NULL;
   }
   
   pkt_t *pkt;

  if((actual -> pkt) -> seq == seq_waited) { 
	
    *list_head = (*list_head) -> next;
 
  
  } else {
    while((actual -> pkt) -> seq != seq_waited ) {
      prev = actual;
      actual = actual-> next;
		
		if(actual == *list_tail && (actual -> pkt) -> seq != seq_waited) {
			return NULL;
		}
    }

    prev->next = actual -> next;
  }
  
  actual -> next = NULL;
   
  if((*list_tail) == actual && (*list_head) != NULL) {
	  
	  (*list_tail) = prev;   
  }
  if((*list_head) == NULL) *list_tail = NULL; 

  pkt = actual->pkt;

  free(actual);

  return pkt;  
   
}

// Dequeues the HEAD packet of the buffer, for the sender
// @see read_write_sender.c
pkt_t *pop_s(pkt_t_node **list_head, pkt_t_node **list_tail, uint8_t seq_ack){

	if(*list_head == NULL || *list_tail == NULL) return NULL; 
	
	pkt_t_node *prev = *list_head;
    pkt_t_node *actual = *list_head;
    
    uint8_t seq = (actual -> pkt) -> seq;
    pkt_t *pkt;
    
    if(actual == *list_tail && seq >= seq_ack && (uint8_t) (seq_ack-32) <= seq)  {
			return NULL;
    }
    
    if(actual == *list_tail && actual->pkt->seq < seq_ack) {
		pkt = actual->pkt;
		free(actual);
		*list_tail = NULL;
		*list_head = *list_tail;
		return pkt;
    }

    while(actual != *list_tail) {
	  int cond = 0; 
	  uint8_t window_max = 32;
	  uint8_t seq_ack_min = seq_ack - window_max;
	  
		if(seq_ack_min > seq_ack) {
	  		cond = (seq < seq_ack || seq_ack_min < seq);
		} else {
	 		cond = (seq < seq_ack && seq > seq_ack - 31); 
		}
	  
	  if(cond) {
		  if(actual == (*list_head)) (*list_head) = (*list_head)->next;
		  pkt = actual->pkt;
		  prev->next = actual -> next;
		  free(actual);
		  return pkt;
		  
	  }
      else {
		prev = actual;
		actual = actual-> next;
		}
		if(actual != NULL) 	{
		seq = (actual -> pkt) -> seq;}
		else {
		actual = *list_tail;
		return NULL;
		}
    }
    
    if((*list_tail) == actual && (*list_head) != NULL && seq < seq_ack  && (uint8_t) (seq_ack-32) < seq) {
	   pkt = actual->pkt;
	   (*list_tail) = prev;
	   free(actual);
	   return pkt;   
    }
    if((*list_head) == NULL) *list_tail = NULL; 
    return NULL;
}

// Checks whether or not the packet with seqnul seq is already buffered
int is_in_buffer(pkt_t_node **list_head, pkt_t_node **list_tail, uint8_t seq) {
	
	if(*list_tail == NULL) return 0;
	pkt_t_node *actual = *list_head; 

	while (actual->next != NULL) {
			
			if(pkt_get_seqnum(actual->pkt) == seq) return 1;
			actual = actual -> next;
	}	
	
	if(pkt_get_seqnum(actual->pkt) == seq) return 1;
	
	return 0; 	
}

// Frees the memory used for the buffer
int free_buffer(pkt_t_node **list_head, pkt_t_node **list_tail) {
	
	if(*list_tail == NULL) return 0;
	
	pkt_t_node *prev = *list_head; 
	pkt_t_node *actual = *list_head; 

	while (actual->next != NULL) {	
			prev = actual -> next;
			pkt_del(prev->pkt);
			free(prev);
	}
	return 0; 	
}

// Peeks the seqnum of the first packet to be dequeued
uint8_t peek_seqnum(pkt_t_node ** list_tail){
	if((*list_tail) == NULL){
		return 0;
	}
		return pkt_get_seqnum((*list_tail) -> pkt);
}


// Peeks the timeout value of the first packet to be dequeued
clock_t peek_stime(pkt_t_node ** list_tail){
	if(list_tail == NULL){
		return (clock_t)0;
	}
		return (*list_tail)->stime;
}
