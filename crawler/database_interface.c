#include <crawler_header.h>

int initialize_database_environment();
int unload_database();
int insert_database_record(database_record_structure * drs);
static size_t write_callback_db(void *buffer, size_t size, size_t nmemb, void *e);

CURL * curl_db;
pthread_mutex_t concurrent_control;
char destination_url[256];
/*
 * Initialize the data pipeline to store the data
 */ 
int initialize_database_environment(char * des_url, int des_url_length)
{
    curl_db = curl_easy_init();
    if(!curl_db)
        return 0;
    if(des_url_length < 256)
        memcpy(destination_url, des_url, des_url_length);
    else
        return 0;
    // create the mutex for concurrent control
    pthread_mutex_init(&concurrent_control, NULL);

    curl_easy_setopt(curl_db, CURLOPT_WRITEFUNCTION, write_callback_db);
    curl_easy_setopt(curl_db, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:15.0) Gecko/20100101 Firefox/15.0");
    curl_easy_setopt(curl_db, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_db, CURLOPT_MAXREDIRS, 6L);
    curl_easy_setopt(curl_db, CURLOPT_TIMEOUT, 50L);
    curl_easy_setopt(curl_db, CURLOPT_CONNECTTIMEOUT, 50L);
    curl_easy_setopt(curl_db, CURLOPT_LOW_SPEED_LIMIT, 1024000L);
    curl_easy_setopt(curl_db, CURLOPT_LOW_SPEED_TIME , 3L);
    curl_easy_setopt(curl_db, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl_db, CURLOPT_NOSIGNAL, 1L);    //for multithread purpose

    return 1;
}


/*
 * unload the data pipeline
 */ 
int unload_database()
{
    if(curl_db != NULL)
        curl_easy_cleanup(curl_db);
}

/*
 * Callback func to receive feedback from curl
 * In our project, we don't keep state for each data submission, so just leave it here.
 */ 
static size_t write_callback_db(void *buffer, size_t size, size_t nmemb, void *e)
{
    size_t realsize = size * nmemb;
    // TODO: add check and see whether the data is successfully delivered
    return realsize;
}

/* -- insert a record into database -- */
int insert_database_record(database_record_structure * drs)
{
    char url[256] = {0};
    // recode the string to POST method acceptable
    char* post_pem_string = curl_easy_escape(
        curl_db, 
        drs->target_website_cert, 
        strlen(drs->target_website_cert) + 1);
    if(post_pem_string == NULL)
        return 0;
    char* post_extended_field = curl_easy_escape(
        curl_db, 
        drs->extended_field, 
        strlen(drs->extended_field) + 1);

    if(post_extended_field == NULL || post_pem_string == NULL)
    {
        if(post_extended_field != NULL)
            curl_free(post_extended_field);
        if(post_pem_string != NULL)
            curl_free(post_pem_string);
        printf("[ERR]   Failed to converte string in HTML format.\n");
        return 0;
    }
    // lock the area
    pthread_mutex_lock(&concurrent_control);

    int record_length = strlen(drs->task_label) + 
        strlen(drs->server_name) + 
        strlen(drs->server_ip) + 
        strlen(drs->target_website_top_domain) + 
        strlen(drs->target_website_source_domain) + 
        strlen(drs->target_website_domain) + 
        strlen(drs->target_website_ip) + 
        strlen(post_pem_string) + 
        strlen(post_extended_field) +
        200;
    char*data = (char*)malloc(record_length);
    memset(data, 0, record_length);
    // the database interface url
    sprintf(url, destination_url);
    sprintf(data, "tasklabel=%s\
&servername=%s\
&serverip=%s\
&webtopdomain=%s\
&websourcedomain=%s\
&webdomain=%s\
&webip=%s\
&webdepth=%d\
&webrtt=%d\
&webtype=%d\
&cert=%s\
&exten=%s"
        ,
        drs->task_label, 
        drs->server_name, 
        drs->server_ip, 
        drs->target_website_top_domain,
        drs->target_website_source_domain,
        drs->target_website_domain,
        drs->target_website_ip,
        drs->target_website_depth,
        drs->target_website_rtt,
        drs->target_website_type,
        post_pem_string,
        post_extended_field
        );
    //printf("%s", data);
    curl_easy_setopt(curl_db, CURLOPT_URL, url);
    curl_easy_setopt(curl_db, CURLOPT_POSTFIELDSIZE, record_length);
    curl_easy_setopt(curl_db, CURLOPT_POSTFIELDS, data);
    curl_easy_perform(curl_db);

    free(data);
    curl_free(post_pem_string);
    curl_free(post_extended_field);
    pthread_mutex_unlock(&concurrent_control);
    return 1;
}