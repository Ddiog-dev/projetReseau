#include "packet_interface.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <endian.h>


/* Extra #includes */
/* Your code will be inserted here */

struct __attribute__((__packed__)) pkt {
  	ptypes_t type : 3;
  	uint8_t window : 5;
	uint8_t seq;
  	uint16_t length;
    uint32_t timestamp;
	uint32_t crc;
	char* payload;
};

  
/* Extra code */
/* Your code will be inserted here */


// Error case?
pkt_t* pkt_new(){
	pkt_t* pkt = (pkt_t*) malloc(sizeof(pkt_t));
	return pkt;
}


void pkt_del(pkt_t *pkt){
  if(pkt->type == PTYPE_DATA) free(pkt->payload);
   	free(pkt);
}



pkt_status_code pkt_decode(const char *data, const size_t len, pkt_t *pkt){
	if(len <= 8) {
		return E_NOHEADER;
	}
	size_t payload_size;

	memcpy(&pkt->seq, data+1, 1);
	memcpy(&pkt->length, data+2, 2);
    memcpy(&pkt->timestamp, data+4, 4);
	pkt->length = be16toh(pkt->length);


	payload_size = pkt->length;	
	


	pkt_set_payload(pkt, data+8, pkt->length);

	unsigned char tmp;
	memcpy(&tmp, data, 1);

	pkt->type = tmp >> 5;
	
	pkt->window = tmp;
	uint32_t crc1 = crc32(0L, (const unsigned char*) data, len-4);
	uint32_t crc2;
	memcpy(&crc2, &data[payload_size + 8], sizeof(uint32_t));
	crc2 = be32toh(crc2);
	if(crc1 != crc2){
		return E_CRC;
	}

	return PKT_OK;
}


/*
 * Convert a struct pkt into a set of bytes ready to be sent over the wires,
 * (thus in network byte-order)
 * including the CRC32 of the header & payload of the packet
 *
 * @pkt: The struct that contains the info about the packet to send
 * @buf: A buffer to store the resulting set of bytes
 * @len: The number of bytes that can be written in buf.
 * @len-POST: The number of bytes written in the buffer by the function.
 * @return: A status code indicating the success or E_NOMEM if the buffer is
 * 		too small
 * */
pkt_status_code pkt_encode(const pkt_t* pkt, char *buf, size_t *len){
	// 1. check size
	size_t header_size = 8;
	size_t crc_size = 4;
	size_t payload_size;


	/*if(pkt->length % 4 != 0){
		payload_size = pkt->length + 4-(pkt->length % 4);
		bytes += 4;
	} else {*/
	payload_size = pkt->length;
	//}
	if(pkt->type != PTYPE_DATA){
		payload_size = 0;
	}
	if(pkt->length > MAX_PAYLOAD_SIZE ||
	   header_size + crc_size + payload_size > (*len)) {
		return E_LENGTH;
	}

	// 2. group type and window in one char, and cat it
	
	unsigned char b;

	b = pkt->type << 5;
	b += pkt -> window;
   	       
	*buf = b;


	// 3. cat seq
	uint8_t seq = pkt->seq;
	b = seq;
	*(buf+1) = b;

	// 4. cat length (2 bytes)

	uint16_t length;
	  
	if(pkt->type == PTYPE_DATA) {
	length = pkt->length;
	}
	else {
	  length = 0;
	}
	length = htobe16(length);
	memcpy(buf+2, &length, (size_t) 2); // copy length
    
    // 4.5 cat timestamp (4 bytes)
	
    uint32_t timestamp = pkt->timestamp;
    //timestamp = htobe32(timestamp);
    memcpy(buf+4, &timestamp, (size_t) 4); // Copy timestamp

	// 5. payload

	if(pkt->type == PTYPE_DATA){ 
		memcpy(buf+8, pkt->payload, payload_size);
	}

	// 6. CRC

	uLong crc = crc32(0L, (unsigned char*) buf, (size_t) payload_size + header_size);
    char *a = (char*) malloc(sizeof(char) *4);
	crc = htobe32(crc);
	memcpy(a, &crc, 4);

	memcpy(buf + payload_size + header_size, a, (size_t) 4);


	free(a);

	*len = payload_size + header_size + 4;
  	return PKT_OK;
}

ptypes_t pkt_get_type  (const pkt_t* pkt){
	return pkt->type;
}

uint8_t  pkt_get_window(const pkt_t* pkt){
	return pkt->window;
}

uint8_t  pkt_get_seqnum(const pkt_t* pkt){
	return pkt->seq;
}

uint16_t pkt_get_length(const pkt_t* pkt){
	return pkt->length;
}

uint32_t pkt_get_crc   (const pkt_t* pkt){
	return pkt->crc;
}

const char* pkt_get_payload(const pkt_t* pkt){
	return pkt->payload;
}


pkt_status_code pkt_set_type(pkt_t *pkt, const ptypes_t type){
	pkt->type = type;
	return PKT_OK;
}

pkt_status_code pkt_set_window(pkt_t *pkt, const uint8_t window){
	pkt->window = window;
	return PKT_OK;
}

pkt_status_code pkt_set_seqnum(pkt_t *pkt, const uint8_t seqnum){
	pkt->seq = seqnum;
	return PKT_OK;
}

pkt_status_code pkt_set_length(pkt_t *pkt, const uint16_t length){
	pkt->length = length;
	return PKT_OK;
}

pkt_status_code pkt_set_crc(pkt_t *pkt, const uint32_t crc){
	pkt->crc = crc;
	return PKT_OK;
}

pkt_status_code pkt_set_payload(pkt_t *pkt, const char *data, const uint16_t length){

  pkt -> payload  = (char*) malloc(sizeof(char) * length);

  memcpy(pkt->payload, data, length);
  
  pkt_set_length(pkt, length);
  
  return PKT_OK;	
}


pkt_status_code pkt_set_timestamp(pkt_t *pkt, const uint32_t timestamp){
	pkt->timestamp = timestamp;
    return PKT_OK;
}

uint32_t pkt_get_timestamp(const pkt_t* pkt){
	return pkt->timestamp;
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
