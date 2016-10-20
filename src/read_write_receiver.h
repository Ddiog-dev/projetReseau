#ifndef __READ_WRITE_RECEIVER_H_
#define __READ_WRITE_RECEIVER_H_

#include "packet_interface.h"
#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

//File descriptor pour le fichier ou écrire
int fd;

uint8_t window_max;
uint8_t window_actual;
unsigned int last_congestion;
uint8_t seq_actual;




/* Lis sur le descripteur de socket sfd et copie le pkt recu dans le buffer 
 * 
 * @pre sfd est un descripteur de socket valide, le pointeur buffer est non null, buf_length est la taille restante sur le buffer 
 * 
 * @post renvoie la taille du pkt lu sur sfd, -1 si erreur
 */ 
size_t read_buf(const int sfd, char *buffer, const size_t buf_length);

/* Envoie un pkt_t de type ack ou nack
 * 
 * @pre ptype est un type de pkt différent de null, sfd est un socket qui a été bind
 * 
 * @post retourne 0 si réussite et -1 si il y a une erreur
 */
int send_ack(const int sfd, const ptypes_t ptype);

/* Vérifie si le packet recu est dans la fenêtre et l'écris dans le fichier (ou stdout) ou bien le stocke dans le buffer
 * 
 * @pre sfd est un descripteur de socket, pkt est un pkt_t non null contenant le packet recu, le pointeur 
 *      listhead pointe sur le début de la liste chainée qui sert de buffer et list_tail point sur la fin. 
 * 
 * @post retourn PKT_OK si le packet a bien été stocké ou une erreur de type PTYPE si il y a une erreur, si 
 *       le fichier est hors de la window, il l'ignore, sinon il le stocke dans un buffer. 
 */
pkt_status_code write_buf(const int sfd, pkt_t *pkt, pkt_t_node **list_head,  pkt_t_node **list_tail);

/* Ouvre le fichier spécifié en écriture seule en spécifiant les autorisations classiques
 * 
 * @pre path_name est un nom de fichier ou null
 * 
 * @post retourne le numéro du descripteur de fichier ou -1 si erreur 
 */
int open_file(const char* path_name);

/*
 * crée une nouvelle structure timeCheck et initialise les champs a timestamp et seqnum
 */



#endif
