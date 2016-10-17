#ifndef __READ_WRITE_SENDER_H_
#define __READ_WRITE_SENDER_H_

#include <stddef.h> /* size_t */
#include <stdint.h> /* uintx_t */

uint8_t last_ack;

/* Loop reading a socket and printing to stdout,
 * while reading stdin and writing to the socket
 * @sfd: The socket file descriptor. It is both bound and connected.
 * @return: as soon as stdin signals EOF
 */
void read_write_sender(const int sfd, const int fd);

#endif