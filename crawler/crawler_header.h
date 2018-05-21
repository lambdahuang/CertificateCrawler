/*
example include file
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <arpa/inet.h>	// you may include this, ohterwise return value of inet_ntoa would be a int

#include <htmlstreamparser.h>
#include <curl/curl.h>


#define MAX_CERTIFICATE_STRING_LENGTH 20 * 1024
#define MAX_FIRST_LEVEL_THREAD_COUNT 500
#define MAX_THREAD_ALIVE_TIME 1000


#define THREAD_ACTIVE 1
#define THREAD_READY 0


// sample distribution structure
typedef struct _SAMPLE_STRUCTURE
{
	char url[256];
	long index;
} sample_element;

typedef struct _DATABASE_RECORD_STRUCTURE
{
	/* data */
	char* 	task_label;
	char* 	server_name;
	char* 	server_ip;
	char* 	target_website_top_domain;
	char* 	target_website_source_domain;
	char* 	target_website_domain;
	char* 	target_website_ip;
	int 	target_website_depth;
	int 	target_website_rtt;
	int 	target_website_type;
	char*	target_website_cert;
	char*	extended_field;
} database_record_structure;

typedef struct _THREAD_CONTROL_STRUCTURE
{
	pthread_t 		thread_id;
	int 			status;	
	void*			memory[32];
	int 			mem_count;
	struct timeval 	time_stamp;
	
	sample_element* se;
} thread_control_element;

void myPrintHelloMake(void);

// openssl certificate interface
int initialize_openssl_environment();
int unload_openssl();
int get_certificate(char* url, char* website_ip, char* pem_string, int i_pem_stirng_length);


// database interface
int 	insert_database_record(database_record_structure * drs);

// sample related function
int 	update_state(char * crawling_label, long new_windows_index);
int 	load_sample(char* sample_file, char* crawling_label);
int 	get_next_sample(sample_element * sample_item);
int 	update_sample_progess(long index);


// basic function
void	heap_heapify(long* heap_array, int index, int heap_size, int array_size);
void 	heap_build_heap(long * heap_array, int heap_size, int array_size);
long 	heap_pop_min(long * heap_array, int* heap_size, int array_size);
void 	heap_insert(long * heap_array, int* heap_size, int array_size, long value);
long 	heap_peek_min(long * heap_array, int heap_size, int array_size);
int 	heap_find(long * heap_array, int heap_size, int array_size, long value);

// crawling function
int		start_crawling(sample_element * se);

// task distribution function
int 	initialize_multitask_environment();
int 	unload_multitask_environment();
int 	dispatch_task(sample_element* se);
