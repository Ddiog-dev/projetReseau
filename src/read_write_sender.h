#ifndef __READ_WRITE_SENDER_H_
#define __READ_WRITE_SENDER_H_

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

uint8_t last_ack;
typedef struct timeCheck{
    pkt_t* pkt;
    struct timeCheck* next;
    struct timeCheck* prev;
}timeCheck;

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_sender(const int sfd, const int fd);
/*
 * crée une nouvelle structure timeCheck et initialise les champs a timestamp et seqnum
 */
timeCheck* init(pkt_t* pkt);
/*
 * retire l'élément timeCheck possédant le numéro de séquence seq_wanted
 * renvoi -1 en cas d'erreur, 0 si on réussi a retirer l'élement voulu et 1 si il n'est pas dans le buffer
 */
int remove_from_buffer(timeCheck **list_head,timeCheck **list_tail, uint8_t seq_wanted);
/*
 * ajoute un l'élément elem au buffer de timeCheck
 * retourne 0 en cas de succès et -1 si elem pointe vers NULL
 */
int push_time_check(timeCheck **list_head,timeCheck **list_tail,pkt_t* pkt);
/*
 * pre: list_head != null
 * renvoie la valeur timestamp du plus ancien élement du buffer
 */
uint32_t get_head_timestamp(timeCheck ** list_head);
/*
 * pre: list_head != null
 * renvoie la valeur seqnum du plus ancien élement du buffer
 */
uint8_t get_head_seqnum(timeCheck **list_head);
/*
 * set elem->timestamp à timestamp
 */
pkt_status_code set__timeCheck_timestamp(timeCheck* elem, uint32_t timestamp);
/*
 *renvoie tout les paquets qui ont time out et renvoie 0 si tout a bien été, retourne -1 en cas d'erreur
 */
int check_time_out(timeCheck** list_head, timeCheck** list_tail,pkt_t_node** buff_head,pkt_t_node** buff_tail,int sfd);
#endif
