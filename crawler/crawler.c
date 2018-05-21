#include <stdio.h>
#include <crawler_header.h>
#define THREAD_SAFE_MODE 0
typedef struct {
	size_t len;
	char *url;
	int depth;
	int type;
	int mark;
	void *next;
} URL;


typedef struct {
	char *tag;
	char *attr;
	char *val;
	int domain_type;
	URL *urls;
	URL *next;
	CURL *curl;
	HTMLSTREAMPARSER *hsp;
} EXTR;

int start_crawling(sample_element * se);
EXTR * create_crawling_task(sample_element * se);
int initialize_crawler_environment();
static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *e);
char *find_host_end(char *url);
CURLcode extr_add_url(EXTR *extr, int record_type);
CURLcode extr_perform(EXTR *extr);
void extr_cleanup(EXTR *extr);

int domain_find_type(char* url);

extern MAX_CRAWLING_DEPTH;
extern char CRAWLING_LABEL[256];			//crawling label
extern char SAMPLE_FILE_PATH[256];
extern char LOCAL_HOST_NAME[256];
extern char LOCAL_HOST_IP[32];
pthread_mutex_t concurrent_control;

void myPrintHelloMake(void) {

  printf("Hello makefiles!\n");

  return;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param t [description]
 * @param L [description]
 * 
 * @return [description]
 */
int initialize_crawler_environment()
{
	pthread_mutex_init(&concurrent_control, NULL);

	return 1;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param se [description]
 * @return [description]
 */
int start_crawling(sample_element * se) {
	CURLcode res;
	if (THREAD_SAFE_MODE) pthread_mutex_lock(&concurrent_control);
	EXTR *extr = create_crawling_task(se);
	if (THREAD_SAFE_MODE) pthread_mutex_unlock(&concurrent_control);

	if(!extr)
		return 0;

	/* ------------------ CRAW ROOT DOMAIN ------------------ */

	database_record_structure *drs 	= malloc(sizeof (database_record_structure));
	if(!drs)
	{
		return 0;
	}

	char* cert 						= malloc(MAX_CERTIFICATE_STRING_LENGTH);
	char* website_ip 				= malloc(32);

	drs->task_label 				= malloc(256);
	drs->server_name 				= malloc(256);
	drs->server_ip 					= malloc(256);
	drs->target_website_top_domain 	= malloc(512);
	drs->target_website_source_domain = malloc(512);
	drs->target_website_domain 		= malloc(512);
	drs->target_website_ip 			= malloc(32);
	drs->target_website_depth 		= 0;
	drs->target_website_rtt	 		= 0;
	drs->target_website_type	 	= 0;
	drs->target_website_cert 		= cert;
	drs->extended_field				= malloc(16);	


	if(cert &&  website_ip && website_ip && drs && drs->task_label && drs->server_name &&
		drs->server_ip && drs->target_website_top_domain && drs->target_website_source_domain &&
		drs->target_website_domain && drs->target_website_ip && drs->extended_field)
	{
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

		strcpy(drs->task_label, (char*)CRAWLING_LABEL);
		strcpy(drs->server_name, (char*)LOCAL_HOST_NAME);
		strcpy(drs->server_ip, (char*)LOCAL_HOST_IP);

		if(get_certificate(se->url, website_ip, cert, MAX_CERTIFICATE_STRING_LENGTH - 1))
		{

			strcpy(drs->target_website_top_domain, se->url);
			strcpy(drs->target_website_source_domain, se->url);
			strcpy(drs->target_website_domain, se->url);
			memcpy(drs->target_website_ip, website_ip, 32);
			printf("website ip: %s|\n", drs->target_website_ip);
			
			insert_database_record(drs);
		}
	}
	if(drs->task_label) free(drs->task_label);
	if(drs->server_name) free(drs->server_name);
	if(drs->server_ip) free(drs->server_ip);
	if(drs->target_website_top_domain) free(drs->target_website_top_domain);
	if(drs->target_website_source_domain) free(drs->target_website_source_domain);
	if(drs->target_website_domain) free(drs->target_website_domain);
	if(drs->target_website_ip) free(drs->target_website_ip);
	if(drs->extended_field) free(drs->extended_field);
	if(drs->target_website_cert) free(drs->target_website_cert);
	if(website_ip) free(website_ip);
	if(drs) free(drs);
	printf("start crawling.\n");
	/* ------------------ CRAW ROOT DOMAIN ------------------ */
	if (extr) {
		while ((res = extr_perform(extr)) != 100) {
			if (res != CURLE_OK) printf("[ERR]%s\n", curl_easy_strerror(res));

		}
		if (THREAD_SAFE_MODE) pthread_mutex_lock(&concurrent_control);
		extr_cleanup(extr);
		if (THREAD_SAFE_MODE) pthread_mutex_unlock(&concurrent_control);

	}
	return EXIT_SUCCESS;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param se [description]
 * @return [description]
 */
EXTR * create_crawling_task(sample_element * se)
{
	char* root_url 		= se->url;
	size_t max_url_len 	= 128;
	char *tag 			= malloc(10), 
		*attr 			= malloc(10), 
		*val 			= malloc(max_url_len);

	URL *urls 			= malloc(sizeof(URL));
	size_t len 			= strlen(root_url);
	char* url 			= malloc(len+1);
	CURL *curl 			= curl_easy_init();
	HTMLSTREAMPARSER *hsp = html_parser_init();
	EXTR *extr 			= (EXTR *) malloc(sizeof(EXTR)); 
	
	if (tag && attr && val && urls && url && curl && hsp && extr) {
		html_parser_set_tag_to_lower(hsp, 1);
		html_parser_set_attr_to_lower(hsp, 1);

		html_parser_set_tag_buffer(hsp, tag, 10);
		html_parser_set_attr_buffer(hsp, attr, 10);
		html_parser_set_val_buffer(hsp, val, max_url_len);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
		curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:15.0) Gecko/20100101 Firefox/15.0");
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 6L);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 800L);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 130L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1024000L);
		curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME , 3L);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, extr);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);	//for multithread purpose

		extr->tag 	= tag;
		extr->attr 	= attr;
		extr->val 	= val;

		urls->len 	= len;
		urls->depth = 0;
		urls->type 	= 0;
		urls->mark 	= 1;


		memcpy(url, root_url, len + 1);
		urls->url = url;

		urls->next 	= NULL;
		extr->urls 	= urls;
		extr->next 	= urls;
		extr->hsp 	= hsp;
		extr->curl 	= curl;
		extr->domain_type = domain_find_type(url);
		return extr;
	} else {
		printf("[ERR]	out of memory.\n");
		if (tag) 	free(tag);
		if (attr) 	free(attr);
		if (val) 	free(val);
		if (urls) 	free(urls);
		if (url)	free(url);
		if (curl) 	curl_easy_cleanup(curl);
		if (hsp) 	html_parser_cleanup(hsp);
		if (extr) 	free(extr);
	}
	return NULL;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param url [description]
 * @return [description]
 */
char *find_host_end(char *url) {
	char *p = url;
	while (*p != '\0') {
		if (*p == '/' || *p == '#' || *p == '?') break;
		else p++;
	}
	return p;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param buffer [description]
 * @param size [description]
 * @param nmemb [description]
 * @param e [description]
 * @return [description]
 */
static size_t write_callback(void *buffer, size_t size, size_t nmemb, void *e) {
	size_t realsize = size * nmemb, p;
	
	EXTR *extr = (EXTR *) e;
	HTMLSTREAMPARSER *hsp = extr->hsp;
	for (p = 0; p < realsize && p < 1500; p++) {
		//pthread_mutex_lock(&concurrent_control);
		html_parser_char_parse(hsp, ((char *)buffer)[p]);
		//pthread_mutex_unlock(&concurrent_control);
		if (html_parser_cmp_tag(hsp, "a", 1))
			if (html_parser_cmp_attr(hsp, "href", 4))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 0)) return p;

		if (html_parser_cmp_tag(hsp, "iframe", 6))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 1)) return p;

		if (html_parser_cmp_tag(hsp, "img", 3))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 2)) return p;

		if (html_parser_cmp_tag(hsp, "script", 6))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 3)) return p;

		if (html_parser_cmp_tag(hsp, "link", 4))
			if (html_parser_cmp_attr(hsp, "href", 4))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 4)) return p;

		if (html_parser_cmp_tag(hsp, "base", 4))
			if (html_parser_cmp_attr(hsp, "href", 4))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 5)) return p;

		if (html_parser_cmp_tag(hsp, "input", 5))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 6)) return p;

		if (html_parser_cmp_tag(hsp, "embed", 5))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 7)) return p;

		if (html_parser_cmp_tag(hsp, "frame", 5))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 8)) return p;

		if (html_parser_cmp_tag(hsp, "object", 6))
			if (html_parser_cmp_attr(hsp, "data", 4))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 9)) return p;

		if (html_parser_cmp_tag(hsp, "source", 6))
			if (html_parser_cmp_attr(hsp, "src", 3))
				if (html_parser_is_in(hsp, HTML_VALUE_ENDED))
					if (extr_add_url(extr, 10)) return p;

	}
	//printf("%s\n", (char*)buffer);
	
	return realsize;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param url [description]
 * @return [description]
 */
int domain_find_type(char* url)
{
	int len = strlen(url);
	if(len <= 4)
		return 0;
//	printf("%s\n", url + len - 4 );
	if(strcmp(url + len - 4, ".com") == 0 || strcmp(url + len - 4, ".net")  == 0||
		strcmp(url + len - 4, ".edu") == 0|| strcmp(url + len - 4, ".org")  == 0||
		strcmp(url + len - 4, ".int") == 0|| strcmp(url + len - 4, ".mil") == 0
		)
		return 2;
	else
		return 3;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param url1 [description]
 * @param url2 [description]
 * @param max [description]
 * @return [description]
 */
int domain_comparison(char * url1, char * url2, int max)
{
	int len1 = strlen(url1);
	int len2 = strlen(url2);
	int i = 0, n = 0;
	int offset1 = 0;
	int offset2 = 0;
	for(i = len1, n = 0; i >= 0; i--)
		if(*(url1 + i) == '.' || *(url1 + i) == '/'){
			n++;
			if(n == max)
			{
				offset1 = i + 1;
				//printf("%s\n", (url1 + offset1));
				break;
			}
		}
	for(i = len2, n = 0; i >= 0; i--)
		if(*(url2 + i) == '.' || *(url2 + i) == '/'){
			n++;
			if(n == max)
			{
				offset2 = i + 1;
				//printf("%s\n", (url2 + offset2));
				break;
			}
		}
	if((len1 - offset1) == (len2 - offset2) &&
		strcmp(url1 + offset1, url2 + offset2) == 0)
		return 1;
	else
		return 0;

}
/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param extr [description]
 */
void extr_cleanup(EXTR *extr) {
	free(extr->tag);
	free(extr->attr);
	free(extr->val);

	URL *last;
	while (extr->urls != NULL) {
		last = extr->urls;
		extr->urls = extr->urls->next;
		free(last->url);
		free(last);
	}
	curl_easy_cleanup(extr->curl);
	html_parser_cleanup(extr->hsp);
	free(extr);
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param extr [description]
 * @param record_type [description]
 * 
 * @return [description]
 */
CURLcode extr_add_url(EXTR *extr, int record_type) {
	HTMLSTREAMPARSER *hsp = extr->hsp;
	size_t count = 1, len = html_parser_val_length(hsp);
	char *h, *s, *url = html_parser_val(hsp);

	URL *urls = extr->urls, *next;
	url[len] = '\0';
	// pick up url start with https://
	//printf("[%d] %s\n", record_type, url);
	if (len > 8) if (strncmp("https://", url, 8) == 0) {
		len = find_host_end(&(url[8])) - url;
		if (len > 11) {
			url[len] = '\0';
			// prevent loop caused by duplicate
			while (urls->next != NULL) {
				if (len == urls->len) if (strcmp(url, urls->url) == 0) return 0;
				urls = urls->next;
				count++;
			}

			if (len == urls->len) if (strcmp(url, urls->url) == 0) return 0;

			// add current url into url list
			next = malloc(sizeof(URL));
			if (next == NULL) return CURLE_OUT_OF_MEMORY;
			next->url = malloc(len+1);
			if (next->url == NULL) {
				free(next);
				return CURLE_OUT_OF_MEMORY;}
			urls->next 	= next;
			next->len 	= len;
			next->depth = extr->next == NULL? 1:extr->next->depth + 1;	//depth increase by one
			next->mark 	= domain_comparison(url, extr->urls->url, extr->domain_type);
			next->type 	= record_type;
			next->next 	= NULL;
			memcpy(next->url, url, len+1);
			if(next->depth >= MAX_CRAWLING_DEPTH)
				return 100;			// return an error if exceed max depth
			printf("[%ld]\t[%d]\t[%d]\t[%d]%s\n", count, next->depth, record_type, next->mark, url);


			// top domain 		:extr->urls->url
			// source domain 	:extr->next->url
			// domain 			:url

			database_record_structure *drs 	= malloc(sizeof (database_record_structure));
			if(!drs)
			{
				return CURLE_OUT_OF_MEMORY;
			}

			char* cert 						= malloc(MAX_CERTIFICATE_STRING_LENGTH);
			char* website_ip 				= malloc(32);

			drs->task_label 				= malloc(256);
			drs->server_name 				= malloc(256);
			drs->server_ip 					= malloc(256);
			drs->target_website_top_domain 	= malloc(512);
			drs->target_website_source_domain = malloc(512);
			drs->target_website_domain 		= malloc(512);
			drs->target_website_ip 			= malloc(32);
			drs->target_website_depth 		= next->depth;
			drs->target_website_rtt	 		= 1;
			drs->target_website_type	 	= record_type;
			drs->target_website_cert 		= cert;
			drs->extended_field				= malloc(16);	

			if(cert &&  website_ip && website_ip && drs && drs->task_label && drs->server_name &&
				drs->server_ip && drs->target_website_top_domain && drs->target_website_source_domain &&
				drs->target_website_domain && drs->target_website_ip && drs->extended_field)
			{
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


				strcpy(drs->task_label, (char*)CRAWLING_LABEL);
				strcpy(drs->server_name, (char*)LOCAL_HOST_NAME);
				strcpy(drs->server_ip, (char*)LOCAL_HOST_IP);

				if (THREAD_SAFE_MODE) pthread_mutex_unlock(&concurrent_control);
				if(get_certificate(url, website_ip, cert, MAX_CERTIFICATE_STRING_LENGTH - 1))
				{
					strcpy(drs->target_website_top_domain, extr->urls->url);
					strcpy(drs->target_website_source_domain, extr->next->url);
					strcpy(drs->target_website_domain, url);
					strcpy(drs->target_website_ip, website_ip);
					printf("[INFO]\twebsite ip: %s\t%s|\n", url, drs->target_website_ip);
					
					insert_database_record(drs);
				}
				if (THREAD_SAFE_MODE) pthread_mutex_lock(&concurrent_control);
			}
			if(drs) {
				if(drs->task_label) free(drs->task_label);
				if(drs->server_name) free(drs->server_name);
				if(drs->server_ip) free(drs->server_ip);
				if(drs->target_website_top_domain) free(drs->target_website_top_domain);
				if(drs->target_website_source_domain) free(drs->target_website_source_domain);
				if(drs->target_website_domain) free(drs->target_website_domain);
				if(drs->target_website_ip) free(drs->target_website_ip);
				if(drs->extended_field) free(drs->extended_field);
				if(drs->target_website_cert) free(drs->target_website_cert);
				free(drs);
			}
			if(website_ip) free(website_ip);
			//else
			//	printf("%s\t%s\n", extr->urls->url, url);

		}
	}
	return 0;
}

/**
 * @brief [brief description]
 * @details [long description]
 * 
 * @param extr [description]
 * @return [description]
 */
CURLcode extr_perform(EXTR *extr) {
	if (extr->next == NULL) return 100;
	if (extr->next->mark == 0) {
		extr->next = extr->next->next;
		return CURLE_OK;
	}	// 0 MEANS THAT WE'RE NOT GOING TO CRAWLING THIS URL

	// i have no idea why we need to protect this, but it seems that program will fail if we don't
	//
	
	curl_easy_setopt(extr->curl, CURLOPT_URL, extr->next->url);
	if (THREAD_SAFE_MODE) pthread_mutex_lock(&concurrent_control);
	//printf("[======s]\t %s\n",  extr->next->url);

	CURLcode res = curl_easy_perform(extr->curl);
	//printf("[======e]\t %d %s\n", res,  extr->next->url);

	if (THREAD_SAFE_MODE) pthread_mutex_unlock(&concurrent_control);

	extr->next = extr->next->next;
	return res;
}