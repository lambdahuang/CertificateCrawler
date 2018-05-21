#include <crawler_header.h>

extern MAX_THREAD_COUNT;
thread_control_element* tcl;
int available_resource = 0;
pthread_mutex_t concurrent_control;
pthread_t thread_monitoring;

void * thread_execute_func( void *ptr );
int initialize_multitask_environment();
int unload_multitask_environment();
int dispatch_task(sample_element* se);
void thread_resource_release(thread_control_element* tce);
void * thread_monitoring_func(void *ptr);


int initialize_multitask_environment()
{
	// allocate the memory
	tcl = malloc(MAX_THREAD_COUNT * sizeof(thread_control_element));
	memset(tcl, 0, MAX_THREAD_COUNT * sizeof(thread_control_element));

	if(tcl == 0)
		return 0;

	available_resource = 0;
	
	if(pthread_create(&thread_monitoring, NULL, thread_monitoring_func, NULL))
	{
		printf("[ERR]	Failed to create the monitoring thread.\n");
	}
	
	
	return 1;
}

int unload_multitask_environment()
{
	pthread_kill(thread_monitoring, NULL);
	free(tcl);
}

int find_available()
{
	//pthread_mutex_lock(&concurrent_control);
	int i = 0;
	for(i = 0; i < MAX_THREAD_COUNT; i++)
	{
		if((tcl + i)->status == THREAD_READY)
		{
			//pthread_mutex_unlock(&concurrent_control);
			return i;
		}
	}
	//pthread_mutex_unlock(&concurrent_control);
	return -1;
}

int dispatch_task(sample_element* se)
{
	int available_index = -1;
	int thread_ret;
	//printf("haha\n");
	// find available resource
	while(1)
	{
		pthread_mutex_lock(&concurrent_control);
		//printf("haha\n");
		available_index = find_available();

		if(available_resource != -1 &&
		 available_resource < MAX_THREAD_COUNT)
			break;
		pthread_mutex_unlock(&concurrent_control);

		usleep(1000 * 500);	//sleep 50 ms
	}


	(tcl + available_index)->status 	= THREAD_ACTIVE;
	(tcl + available_index)->mem_count 	= 0;
	(tcl + available_index)->se 		= malloc(sizeof(sample_element));

	strcpy((tcl + available_index)->se->url, se->url);
	(tcl + available_index)->se->index = se->index;

	gettimeofday(&(tcl + available_index)->time_stamp, NULL);
	available_resource ++;

	if(thread_ret = pthread_create(&((tcl+available_index)->thread_id), NULL, thread_execute_func, (void*) (tcl + available_index)))
	{
		printf("[ERR]	Failed to create the thread %ld.\n", se->index);
		update_sample_progess(se->index);
		pthread_mutex_unlock(&concurrent_control);
		thread_resource_release((tcl + available_index));
		return 0;
	}

	// increase resource count
	pthread_mutex_unlock(&concurrent_control);
	return 1;
}

void * thread_monitoring_func(void *ptr)
{
	struct timeval 	time_stamp;
	printf("[STAT]	Monitoring thread initialized.\n");

	int i = 0;
	int time_sub_ret = 0;
	int current_min = MAX_THREAD_ALIVE_TIME;
	int running_thread = 0;
	int pointer[MAX_THREAD_ALIVE_TIME] = {0};
	while(1)
	{
		gettimeofday(&time_stamp, NULL);
		printf("Current running threads: %d\n", available_resource);
		current_min 	= MAX_THREAD_ALIVE_TIME;
		running_thread 	= 0;
		for(i = 0; i < MAX_THREAD_COUNT; i++)
		{
			//pthread_mutex_lock(&concurrent_control);
	
			if((tcl + i)->status == THREAD_ACTIVE )
			{
				printf("[THREAD]\tid: %d running time: %d seconds\n", 
					i, 
					((int)(time_stamp.tv_sec) - (int)((tcl + i)->time_stamp.tv_sec))
				);
				if(((int)(time_stamp.tv_sec) - (int)((tcl + i)->time_stamp.tv_sec)) < current_min)
					current_min = ((int)(time_stamp.tv_sec) - (int)((tcl + i)->time_stamp.tv_sec));
				pointer[running_thread++] = i;
				//thread_resource_release((tcl + i));
			}else{
				//pthread_mutex_unlock(&concurrent_control);
			}	
		}

		printf("Min time: %d\tRunning Thread: %d\n", current_min, running_thread);
		if(current_min >= MAX_THREAD_ALIVE_TIME && running_thread != 0)
		{
			for(i = 0; i < running_thread; i++)
			{
				printf("[WARN]	Thread timeout.\n");
				// terminate task
				pthread_kill((tcl + pointer[i])->thread_id, NULL);
				//usleep(1000 * 50);
				pthread_mutex_unlock(&concurrent_control);

				// task time out 
				update_sample_progess((tcl + pointer[i])->se->index);

				// release the memory
			}
			exit(0);
		}
		usleep(10000 * 1000);	//10 s
	}
	return 0;
}

void * thread_execute_func( void *ptr )
{
	thread_control_element* tce = (thread_control_element*)ptr;
/*
	char* cert 						= malloc(MAX_CERTIFICATE_STRING_LENGTH);
	char* website_ip 				= malloc(32);
	database_record_structure *drs 	= malloc(sizeof (database_record_structure));
	drs->task_label 				= malloc(256);
	drs->server_name 				= malloc(256);
	drs->server_ip 					= malloc(256);
	drs->target_website_top_domain 	= malloc(512);
	drs->target_website_source_domain = malloc(512);
	drs->target_website_domain 		= malloc(512);
	drs->target_website_ip 			= malloc(32);
	drs->target_website_depth 		= 0;
	drs->target_website_rtt	 		= 1;
	drs->target_website_type	 	= 2;
	drs->target_website_cert 		= cert;
	drs->extended_field				= malloc(16);	
	memset(drs->extended_field, 0, 16);
	memset(drs->target_website_ip, 0, 32);
	memset(drs->target_website_domain, 0, 512);
	memset(drs->target_website_source_domain, 0, 512);
	memset(drs->target_website_top_domain, 0, 512);
	memset(drs->server_ip, 0, 256);
	memset(drs->server_name, 0, 256);
	memset(drs->task_label, 0, 256);
	memset(website_ip, 0, 32);
	memset(cert, 0, MAX_CERTIFICATE_STRING_LENGTH);


	pthread_mutex_lock(&concurrent_control);
	tce->memory[0] 	= cert;
	tce->memory[1] 	= website_ip;
	tce->memory[2] 	= drs;
	tce->memory[3] 	= drs->task_label;
	tce->memory[4] 	= drs->server_name;
	tce->memory[5] 	= drs->server_ip;
	tce->memory[6] 	= drs->target_website_top_domain;
	tce->memory[7] 	= drs->target_website_source_domain;
	tce->memory[8] 	= drs->target_website_domain;
	tce->memory[9] 	= drs->target_website_ip;
	tce->memory[10] = drs->extended_field;
	tce->mem_count 	= 11;
	pthread_mutex_unlock(&concurrent_control);
*/

	printf("[STAT]	Processing: {%ld %s}\n", tce->se->index, tce->se->url);

/*
	if(get_certificate(tce->se->url, website_ip, cert, MAX_CERTIFICATE_STRING_LENGTH - 1))
	{
		strcpy(drs->target_website_top_domain, tce->se->url);
		strcpy(drs->target_website_source_domain, tce->se->url);
		strcpy(drs->target_website_domain, tce->se->url);
		memcpy(drs->target_website_ip, website_ip, 32);
		printf("website ip: %s|\n", drs->target_website_ip);
		
		insert_database_record(drs);
	}
*/
	sample_element tmp_se;
	memcpy(&tmp_se, tce->se, sizeof(sample_element));

	start_crawling(&tmp_se);
	
	update_sample_progess(tce->se->index);
	printf("[STAT]	Finishing: {%ld %s}\n", tce->se->index, tce->se->url);

	thread_resource_release(tce);
/*
	free(drs->task_label);
	free(drs->server_name);
	free(drs->server_ip);
	free(drs->target_website_top_domain);
	free(drs->target_website_source_domain);
	free(drs->target_website_domain);
	free(drs->target_website_ip);
	free(drs->extended_field);
	free(drs->target_website_cert);
	free(website_ip);
	free(drs);

	// release the thread
	pthread_mutex_lock(&concurrent_control);
	tce->mem_count 	= 0;
	tce->status 	= THREAD_READY;
	free(tce->se);
	//pthread_detach((tce)->thread_id);
	available_resource --;
	pthread_mutex_unlock(&concurrent_control);
*/
	return 0;
}

void thread_resource_release(thread_control_element* tce)
{
	int i = 0;
	//printf("dispatch lock1\n");
	pthread_mutex_lock(&concurrent_control);
	for(i = 0; i < tce->mem_count; i++)
		if(tce->memory[i] != NULL)
		{	
			free(tce->memory[i]);
			tce->memory[i] = NULL;
		}
	
	// release the thread
	tce->mem_count 	= 0;
	tce->status 	= THREAD_READY;
	if(tce->se)
	{
		free(tce->se);
		tce->se 	= NULL;
	}
	if((tce)->thread_id)
		pthread_detach((tce)->thread_id);
	available_resource --;
	pthread_mutex_unlock(&concurrent_control);
}




