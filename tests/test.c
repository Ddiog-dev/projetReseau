
#include "CUnit/Headers/CUnit.h"
#include "CUnit/Headers/Basic.h"

#include "../src/packet_interface.h"
#include "../src/create_socket.h"
#include "../src/real_address.h"
#include "../src/read_write_sender.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define MAX_SIZE 524
// Tests de packet_implem

void test_pkt_new(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	CU_ASSERT_PTR_NOT_NULL(pkt); 
	
	pkt_del(pkt);
}

void test_pkt_set_type(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_type(pkt, PTYPE_DATA);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK); 
	
	
	CU_ASSERT_EQUAL(pkt_get_type(pkt), PTYPE_DATA);
	
	pkt_del(pkt);
}

void test_pkt_set_window(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_window(pkt, (uint8_t) 26);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);  
	
	CU_ASSERT_EQUAL(pkt_get_window(pkt), (uint8_t) 26);
	
	pkt_del(pkt);
}


void test_pkt_set_seqnum(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_seqnum(pkt, (uint8_t) 26);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);  
	
	CU_ASSERT_EQUAL(pkt_get_seqnum(pkt), (uint8_t) 26);
	
	pkt_del(pkt);
}
	
	
void test_pkt_set_crc(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_crc(pkt, (uint32_t) 2651646);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);  
	
	CU_ASSERT_EQUAL(pkt_get_crc(pkt), (uint32_t) 2651646);
	
	pkt_del(pkt);
}	

void test_pkt_set_length(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_length(pkt, (uint16_t) 400);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);  
	
	CU_ASSERT_EQUAL(pkt_get_length(pkt), (uint16_t) 400);
	
	pkt_del(pkt);
}

void test_pkt_set_payload(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	ptypes_t type; 
	
	type = pkt_set_payload(pkt, "Test payload avec valeur", (uint16_t) 26);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);  
	
	pkt_del(pkt);
}

void test_pkt_encode(void) { 
	
	pkt_t *pkt = pkt_new(); 
	
	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_window(pkt, (uint8_t) 31);
	pkt_set_length(pkt,  600);
	pkt_set_payload(pkt, "AAAAAAAAAAA", (uint16_t)600);
	pkt_set_seqnum(pkt, (uint8_t) 16);
	
	char *buffer = (char*) malloc(sizeof(char)*512);
	
	ptypes_t type; 
	
	size_t size = 600;
	
	type = pkt_encode(pkt, buffer, &size);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) E_LENGTH);  
	
	pkt_set_length(pkt,  12);
	
	pkt_set_payload(pkt, "AAAAAAAAAAA", (uint16_t)12);
	
	size = 512;
	
	type = pkt_encode(pkt, buffer, &size);
    
	CU_ASSERT_EQUAL(type, (ptypes_t) PKT_OK);
	
	pkt_del(pkt);
	
	free(buffer);
}

void test_pkt_decode(void) { 
	
	pkt_t *pkt = pkt_new();
	pkt_t *pkt2 = pkt_new();
	
	pkt_set_type(pkt, PTYPE_DATA);
	pkt_set_window(pkt, (uint8_t) 30);
	pkt_set_length(pkt,  7);
	pkt_set_payload(pkt, "aaaaaa", (uint16_t)7);
	pkt_set_seqnum(pkt, (uint8_t) 16);
	
	char *buffer = (char*) malloc(sizeof(char)*MAX_SIZE);
	
	ptypes_t type; 
	
	size_t size = MAX_SIZE;
	
	pkt_encode(pkt, buffer, &size);
	
	type = pkt_decode(buffer, MAX_SIZE, pkt2);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) E_CRC);   
	
	type = pkt_decode(buffer, 2, pkt2);
	
	CU_ASSERT_EQUAL(type, (ptypes_t) E_NOHEADER);
	
	type = pkt_decode(buffer, (size_t) 8, pkt2);
	
	pkt_del(pkt);
	pkt_del(pkt2);
		
	free(buffer);
}


void test_stack(void){
	pkt_t_node * head = NULL;
	pkt_t_node * tail = NULL;

// Creating dull packets
	pkt_t * elem1 = pkt_new();
	pkt_set_seqnum(elem1, 0);

	pkt_t * elem2 = pkt_new();
	pkt_set_seqnum(elem2, 1);

	pkt_t * elem3 = pkt_new();
	pkt_set_seqnum(elem3, 2);

	push(&tail, &head, elem1);

	CU_ASSERT_PTR_EQUAL(head, tail);

	CU_ASSERT_EQUAL(head->pkt, elem1);

	CU_ASSERT_EQUAL(0, peek_seqnum(&head));
	CU_ASSERT_EQUAL(0, peek_seqnum(&tail));

	push(&tail, &head, elem2);

	CU_ASSERT_EQUAL(0, peek_seqnum(&head));

	CU_ASSERT_EQUAL(1, peek_seqnum(&tail));

	push(&tail, &head, elem3);

	CU_ASSERT_EQUAL(2, peek_seqnum(&tail));

	pop(&head, &tail, 0);
	pop(&head, &tail, 1);
	pop(&head, &tail, 2);

	pkt_del(elem1);
	pkt_del(elem2);
	pkt_del(elem3);
	
	free_buffer(&head, &tail);
    
}
void test_create_socket(){

	int src_port=65001;
	int dst_port=65001;
	struct sockaddr_in6 source_addr;
	struct sockaddr_in6 dest_addr;
	int err=0;
	const char* charErr;
	//test real_address
	charErr=real_address("::1",&source_addr);
	CU_ASSERT_EQUAL(charErr,NULL);
	charErr=real_address("::1",&dest_addr);
	CU_ASSERT_EQUAL(charErr,NULL);
	// Test creation de socket
	err=create_socket(&source_addr,src_port,NULL,0);
	CU_ASSERT_NOT_EQUAL(err,-1);
	err=create_socket(NULL,0,&dest_addr,dst_port);
	CU_ASSERT_NOT_EQUAL(err,-1);

}
/*
void test_read_write_sender(){
	int fd=open("filetest.txt",O_RDONLY);
	int src_port=65001;
	int dst_port=65001;
	int sfd_sender;
	int sfd_receiver;
	struct sockaddr_in6 source_addr;
	struct sockaddr_in6 dest_addr;
	sfd_sender=create_socket(&source_addr,src_port,NULL,0);
	sfd_receiver=create_socket(NULL,0,&dest_addr,dst_port);
} */

void test_manip_timeCheck(){

    timeCheck* buffer_head=NULL;
    timeCheck* buffer_tail=NULL;
    // test 1
    pkt_t *test1=pkt_new();
    pkt_set_seqnum(test1,0);
    printf(" test1 %p \n",test1);
    // test 2
    pkt_t *test2=pkt_new();
    pkt_set_seqnum(test2,1);
    printf(" test2 %p \n",test2);
    // test 3
    pkt_t *test3=pkt_new();
    pkt_set_seqnum(test3,2);
    printf(" test3 %p \n",test3);
    // test 4
    pkt_t *test4=pkt_new();
    pkt_set_seqnum(test4,3);
    printf(" test4 %p \n",test4);
    push_time_check(&buffer_head,&buffer_tail,test1);
    CU_ASSERT_EQUAL(buffer_head,buffer_tail);
    CU_ASSERT_EQUAL(buffer_head->pkt->seq,0);

    push_time_check(&buffer_head,&buffer_tail,test2);
    printf(" buffer_head->pkt 2 %p \n",buffer_head->pkt);
    printf(" buffer_tail-> pkt 2 %p \n",buffer_tail->pkt);
    CU_ASSERT_EQUAL(buffer_head->pkt->seq,0);
    CU_ASSERT_EQUAL(buffer_head->next->pkt->seq,1);
    CU_ASSERT_EQUAL(buffer_tail->pkt->seq,1);
    push_time_check(&buffer_head,&buffer_tail,test3);
    printf(" buffer_head->pkt 3 %p \n",buffer_head->pkt);
    printf(" buffer_tail-> pkt 3 %p \n",buffer_tail->pkt);
    CU_ASSERT_EQUAL(buffer_head->pkt->seq,0);
    CU_ASSERT_EQUAL(buffer_head->next->pkt->seq,1);
    CU_ASSERT_EQUAL(buffer_head->next->next->pkt->seq,2);
    CU_ASSERT_EQUAL(buffer_tail->pkt->seq,2);

    remove_from_buffer(&buffer_head,&buffer_tail,1);
    printf(" buffer_head->pkt 4 %p \n",buffer_head->pkt);
    printf(" buffer_tail-> pkt 4 %p \n",buffer_tail->pkt);
    CU_ASSERT_EQUAL(buffer_head,buffer_tail);
    CU_ASSERT_EQUAL(buffer_tail->pkt->seq,2);

    push_time_check(&buffer_head,&buffer_tail,test4);

    CU_ASSERT_EQUAL(buffer_head->pkt->seq,2);

    CU_ASSERT_EQUAL(buffer_head->next->pkt->seq,3);
    printf(" CA VA PETER\n");
    CU_ASSERT_EQUAL(buffer_tail->pkt->seq,3);

}

int main(void){
	
	// [1] initialisation du catalogue 
	
	if( CUE_SUCCESS != CU_initialize_registry() ) return CU_get_error(); 
	
	// [2] ajouter les suites de tests au catalogue et ajouter les tests à la suite de test 
	
	CU_pSuite pSuite = NULL; 
	
	// (a) suite ... 
	
	pSuite = CU_add_suite("suite_implem", NULL, NULL); 
	
	if( NULL == pSuite ) { CU_cleanup_registry(); return CU_get_error(); }
	
	
	if ((NULL == CU_add_test(pSuite, "Test manip timeCheck", test_manip_timeCheck)) ||(NULL == CU_add_test(pSuite, "Test ptr not null", test_pkt_new)) || (NULL == CU_add_test(pSuite, "Test set_type", test_pkt_set_type)) || (NULL == CU_add_test(pSuite, "Test set_window", test_pkt_set_window)) || (NULL == CU_add_test(pSuite, "Test set_length", test_pkt_set_length)) || (NULL == CU_add_test(pSuite, "Test set_crc", test_pkt_set_crc)) || (NULL == CU_add_test(pSuite, "Test set_seqnum", test_pkt_set_seqnum)) || (NULL == CU_add_test(pSuite, "Test set_payload", test_pkt_set_payload)) || (NULL == CU_add_test(pSuite, "Test encode", test_pkt_encode)) || (NULL == CU_add_test(pSuite, "Test decode", test_pkt_decode)) || (NULL == CU_add_test(pSuite, "Test buffer", test_stack))|| (NULL == CU_add_test(pSuite, "Test create_socket", test_create_socket))) {
		
		
		CU_cleanup_registry(); return CU_get_error();
		
	}
	
	// [4] exécuter les tests 
	
	CU_basic_run_tests(); 
	
	CU_basic_show_failures(CU_get_failure_list()); 
	
	// [5] libérer les ressources 
	
	CU_cleanup_registry(); 
	
	printf("\n\n>>> EOF test.c. <<<\n"); 
	
	return CU_get_error();


}
