#include <crawler_header.h>


int MAX_THREAD_COUNT 		= 25;	//maximum thread count
int MAX_CRAWLING_DEPTH 		= 0;	//maximum crawling depth
int MAX_SAMPLE_SIZE 		= 0;	//maximum sample size 
char CRAWLING_LABEL[256];			//crawling label
char SAMPLE_FILE_PATH[256];
char LOCAL_HOST_NAME[256];
char LOCAL_HOST_IP[32];
char DATAPIPE_URL[256];

int initialize_global_environment()
{

	// initialize the global environment for curl,
	// it is important because it's unsafe multithread property
	curl_global_init(CURL_GLOBAL_ALL);
	
	if(!initialize_database_environment(DATAPIPE_URL, strlen(DATAPIPE_URL)))
		printf("[ERR]	Initialize database interface failed.\n");
	else
		printf("[SYS]	Successfully load the database interface.\n");

	if(!initialize_openssl_environment())
		printf("[ERR]	Initialize openssl environment failed.\n");
	else
		printf("[SYS]	Successfully load the openssl environment.\n");

	if(!initialize_multitask_environment())
		printf("[ERR]	Initialize openssl environment failed.\n");
	else
		printf("[SYS]	Successfully load the openssl environment.\n");

	if(!initialize_crawler_environment())
		printf("[ERR]	Initialize crawler environment failed.\n");
	else
		printf("[SYS]	Successfully load the crawler environment.\n");
}
int main(int argc, char *argv[]) {
	
	// initialize the options
	memset(CRAWLING_LABEL, 0, 256);
	memset(SAMPLE_FILE_PATH, 0, 256);
	MAX_THREAD_COUNT 	= 25;
	MAX_CRAWLING_DEPTH 	= 0;
	int pid;
	int status;

	// call a function in another file
	myPrintHelloMake();
	if(argc >= 7)
	{	
		sprintf(CRAWLING_LABEL, "STATE_LOG_%s", argv[1]);
		sprintf(SAMPLE_FILE_PATH, "%s", argv[2]);

		sscanf(argv[3], "%d", &MAX_CRAWLING_DEPTH);
		sscanf(argv[4], "%d", &MAX_SAMPLE_SIZE);
		sscanf(argv[5], "%d", &MAX_THREAD_COUNT);
		sscanf(argv[6], "%s", &DATAPIPE_URL);

		printf("[OPT]	max thread count: %d\n", MAX_THREAD_COUNT);
		printf("[OPT]	max crawling depth: %d\n", MAX_CRAWLING_DEPTH);
		printf("[OPT]	crawling label: %s\n", CRAWLING_LABEL);
		printf("[OPT]	sample file: %s\n", SAMPLE_FILE_PATH);
		printf("[OPT]	sample size: %d\n", MAX_SAMPLE_SIZE);
		printf("[OPT]	data pipeline: %s\n", DATAPIPE_URL);

		gethostname(LOCAL_HOST_NAME, 256);
		get_primary_ip(LOCAL_HOST_IP, 32);

		printf("[SYS]	Server Name: %s\t Server IP: %s\n", LOCAL_HOST_NAME, LOCAL_HOST_IP);
		while(1)
		{
			if ((pid = fork()) < 0) {
				
				exit(1);
			}
			if(pid != 0)
			{ 
				usleep(1000*2000);
				wait(&status);
			}	//if i'm parent
			else
			{									//if i'm child
				initialize_global_environment();		
				if(!load_sample(SAMPLE_FILE_PATH, CRAWLING_LABEL))
					printf("[ERR]	Initialize sample failed.\n");
				else
					printf("[SYS]	Successfully load the sample.\n");
				sample_element se;
	
				int i = 0;
				for(i = 0; i < MAX_SAMPLE_SIZE; i++)
				{
					if(get_next_sample(&se) == -1)
					{
						printf("[INFO]\tCrawling finished.\n");
						while(1)
							usleep(1000*2000);
					}
					//start_crawling(&se);

					printf("[INFO] distributing task: {%ld %s}\n", se.index, se.url);
					dispatch_task(&se);
				}
			}
		}

		//test();
		//release the resource
		printf("Crawling finished!\n");
		while(1)
			usleep(1000*2000);
		curl_global_cleanup();

	}
	else
	{
		printf("[ERR]	There are not enough options.\n");
		return 0;
	}
	return(0);
}
