#include "packet_implem.h"
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
    if(pkt==NULL || buf==NULL || *len<=0)return E_UNCONSISTENT; // TODO COND LEN<12

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
int main( int argc, char** argv){
    argc++;
    argv[0] = NULL;
    //init of the packet
    pkt_t *pkt = pkt_new();
    pkt->type = PTYPE_DATA;
    pkt->window = 0b10001;
    pkt->seqnum = 0b0010001;
    pkt_set_timestamp(pkt, 1);
    pkt_set_payload(pkt, "Hello world !", 13);
    //actual tests - 1st packet
    printf(" ==================== First packet :\n");
    printf("Type: \t\t\t\t%s\n", pkt_get_type(pkt) == PTYPE_DATA ? "data" : "ack");
    printf("Windows: \t\t\t%#02x\n", pkt_get_window(pkt));
    printf("Sequence number: \t\t%#02x\n", pkt_get_seqnum(pkt));
    printf("Timestamp:  \t\t\t%#08x\n", pkt_get_timestamp(pkt));
    printf("Length: \t\t\t%#04x - %d\n", pkt_get_length(pkt), pkt_get_length(pkt));
    printf("CRC: \t\t\t\t%#08x\n", pkt_get_crc(pkt));
    printf("Payload: \t\t\t%s\n", pkt_get_payload(pkt));
    //transfer to the seccond
    pkt_t * seccond = pkt_new();
    char * buff = (char*)malloc(sizeof(char)*25);
    size_t size = 25;
    pkt_status_code ret = pkt_encode(pkt, buff, &size);
    int i;
    for(i=0;i<25;i++){
        printf("%s",buff+i);
    }
    printf("\n");
    ret++;
    printf("test here \n");
    ret = pkt_decode(buff, size, seccond);
    
    printf("===================== Retransmitted packet :\n");
    printf("Type: \t\t\t\t%s\n", pkt_get_type(seccond) == PTYPE_DATA ? "data" : "ack");
    printf("Windows: \t\t\t%#02x\n", pkt_get_window(seccond));
    printf("Sequence number: \t\t%#02x\n", pkt_get_seqnum(seccond));
    printf("Timestamp:  \t\t\t%#08x\n", pkt_get_timestamp(seccond));
    printf("Length: \t\t\t%#04x - %d\n", pkt_get_length(pkt), pkt_get_length(seccond));
    printf("CRC: \t\t\t\t%#08x\n", pkt_get_crc(seccond));
    printf("Payload: \t\t\t%s\n", pkt_get_payload(seccond));
    free(buff);
    //removing of the packet
    pkt_del(pkt);
    return 0;
}
