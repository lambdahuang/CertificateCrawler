#include "crawler_header.h"

extern MAX_THREAD_COUNT;
extern MAX_SAMPLE_SIZE;
long windows_start 		= 0;
long dispatched_index 	= 0;
long windows_size;

int initialize_sample_environment(char* crawling_label);
int load_sample(char* sample_file, char* crawling_label);
int load_state(char* crawling_label);
int get_next_sample(sample_element * sample_item);
int update_state(char * crawling_label, long new_windows_index);
int update_sample_progess(long index);


char current_label[256];
FILE * fp_sample;
pthread_mutex_t concurrent_control;
long* priority_queue;
int queue_maximum_length;
int queue_size;

/* -- update the progress after thread finished-- */
int update_sample_progess(long index)
{
	printf("[STAT]	Task: %ld finished\n", index);
	pthread_mutex_lock(&concurrent_control);
	if(index >= windows_start && index <=dispatched_index)
		heap_insert(priority_queue, &queue_size, queue_maximum_length, index);

	long peek_r = 0;
	while(queue_size > 0 && ((peek_r = heap_peek_min(priority_queue, queue_size, queue_maximum_length)) <= windows_start))
	{
		heap_pop_min(priority_queue, &queue_size, queue_maximum_length);
		printf("[INFO]	record: %ld finished\n", windows_start);
		if(peek_r == windows_start)
			windows_start++;
	}

	// update the status
	update_state(current_label, windows_start);
	pthread_mutex_unlock(&concurrent_control);
	return 1;
}

/* -- get each sample -- */
/* -- multi thread safely -- */
int get_next_sample(sample_element* sample_item)
{
	if(fp_sample == NULL || dispatched_index >= MAX_SAMPLE_SIZE)
	{
		printf("[ERR]	sample or file not exists.\n");
		return -1;
	}
	else
	{
		char* line = NULL;
		size_t len = 0;
		ssize_t read;

		//int tmp_index = dispatched_index;

		// jump if the index already in heap
		while(1)
		{
			//wait until the resource released
			while(1)
			{
				//printf("haha1\n");
				pthread_mutex_lock(&concurrent_control);
				if(dispatched_index - windows_start < windows_size)
				{
					//pthread_mutex_unlock(&concurrent_control);
					break;
				}
				pthread_mutex_unlock(&concurrent_control);
				usleep(1000 * 5000);	//sleep 50 ms
				printf("waiting: %ld - %ld - %ld\n", windows_start, dispatched_index, windows_size);
			}
			//printf("haha2\n");
			//pthread_mutex_lock(&concurrent_control);
			read = getline(&line, &len, fp_sample);
			//tmp_index++;
			//dispatched_index = tmp_index;
			dispatched_index ++;
			if(read == -1){
				printf("[ERR]	Failed to read the sample.\n");
				pthread_mutex_unlock(&concurrent_control);
				return -1;
			}
			if(!heap_find(priority_queue, queue_size, queue_maximum_length, dispatched_index - 1))
			{
				//pthread_mutex_unlock(&concurrent_control);
				break;
			}
			//printf("haha2-unlock1\n");
			pthread_mutex_unlock(&concurrent_control);

		}

		memset(sample_item->url, 0, 256);
		memcpy(sample_item->url, line, read);
		*(sample_item->url + read - 1) = '\0';
		sample_item->index = dispatched_index - 1;

		//dispatched_index = tmp_index;
		//printf("[INFO]	record: %d has dispatched: %d\n", dispatched_index, read);
		//dispatched_index ++;

		free(line);
		//printf("haha2-unlock2\n");
		pthread_mutex_unlock(&concurrent_control);
	}
	return dispatched_index;
}

/* -- initialize the variable and other things -- */
int initialize_sample_environment(char* crawling_label)
{
	// windows size should be equal to maximum thread count.
	windows_size = MAX_THREAD_COUNT;

	// load the break point of current label.
	strcpy(current_label, crawling_label);

	// make the func concurrent safely.
	pthread_mutex_init(&concurrent_control, NULL);

	// initialize a heap to maintain windows
	queue_maximum_length 	= MAX_FIRST_LEVEL_THREAD_COUNT;
	queue_size 				= 0;
	priority_queue = malloc(queue_maximum_length * sizeof(long));
	if(priority_queue == NULL)
		return -1;

	// load the heap from file
	dispatched_index = windows_start;
	load_state(crawling_label);
	printf("[STT]	Dispatched index: %ld\n", dispatched_index);
	printf("[STT]	Windows start index: %ld\n", windows_start);
	printf("[STT]	Queue size: %d\n", queue_size);
	heap_build_heap(priority_queue, queue_size, queue_maximum_length);

	return 0;
}

int recycle_memory()
{
	if(priority_queue != NULL)
		free(priority_queue);
	if(fp_sample != NULL)
		fclose(fp_sample);
}

/* -- load the sample and recorver the break point -- */
int load_sample(char* sample_file, char* crawling_label)
{
	//initialize the environment
	initialize_sample_environment(crawling_label);

	// output status
	printf("[SYS]	Last crawlered record id: %ld\n", windows_start);
	
	fp_sample = fopen(sample_file, "r");
	if(fp_sample == NULL)
	{
		printf("[ERR]	sample file not exists.\n");
		return 0;
	}
	else
	{
		int n = 0;
		char* line = NULL;
		size_t len = 0;
		ssize_t read;

		// recover the pointer to the break point
		while(windows_start != 0 && n < windows_start)
		{
			line = NULL;
			read = getline(&line, &len, fp_sample);
			if(read == -1)
				break;
			free(line);
			n++;
		}
	}
	return 1;
}

/* -- load the crawling progress by label --*/
int load_state(char * crawling_label)
{
	FILE * fp;
	if((access(crawling_label, F_OK) != -1))
		fp = fopen(crawling_label, "r+");
	else
		fp = fopen(crawling_label, "w+");

	if(fp != NULL)
	{
		fscanf(fp, "%ld %ld %d", &windows_start, &dispatched_index, &queue_size);
		int i = 0;
		for(i = 0; i < queue_size; i++)
		{
			fscanf(fp, "%ld", (priority_queue + i));
			//printf("%ld\n", *(priority_queue + i));
		}
		// even we keep the track of dispatched_index but we still set it equals to dispatched_index
		dispatched_index = windows_start;
	}
	else
	{
		windows_start 		= 0;
		dispatched_index 	= 0;
	}
	fclose(fp);
	return windows_start;
}

/* -- update the crawling progress --*/
int update_state(char * crawling_label, long new_windows_index)
{
	FILE * fp;
	
	fp = fopen(crawling_label, "w+");

	rewind(fp);
	//fprintf(fp, "%d ", new_windows_index);
	fprintf(fp, "%ld %ld %d ", windows_start, dispatched_index, queue_size);
	int i = 0;
	for(i = 0; i < queue_size; i++)
	{
		fprintf(fp, "%ld ", *(priority_queue + i));
	}
	fclose(fp);
	return windows_start;
}