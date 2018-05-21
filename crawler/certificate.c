#include <crawler_header.h>
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
int initialize_openssl_environment();
int unload_openssl();
int get_certificate(char* url, char* website_ip, char* pem_string, int i_pem_stirng_length);

int create_openssl_socket(char url_str[], char* website_ip, char* host_name);

pthread_mutex_t concurrent_control;

int initialize_openssl_environment()
{
	if(SSL_library_init() < 0){
		printf("Initialization failed.\n");
		return 0;
	}
//	pthread_mutex_init(&concurrent_control, NULL);

	// load related resources
	OpenSSL_add_all_algorithms();
	ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();
	return 1;
}

int unload_openssl()
{
	ERR_free_strings();
	EVP_cleanup();
}

int create_openssl_socket(char url_str[], char* website_ip, char* host_name)
{
	int sockfd;
	char hostname[256]	= "";
	char portnum[6]		= "443";
	char proto[6]		= "";
	char* tmp_ptr 		= NULL;
	int port;
	struct hostent *host;
	struct sockaddr_in dest_addr;

	/* Remove the final / from url_str, if there is one */
	if(url_str[strlen(url_str)] == '/')
		url_str[strlen(url_str)] = '\0';

	/* the first : ends the protocol string, i.e. http */
	strncpy(proto, url_str, (strchr(url_str, ':')-url_str));

	/* the hostname starts after the "://" part */
	strncpy(hostname, strstr(url_str, "://")+3, sizeof(hostname));

	/* if the hostname contains a colon :, we got a port number */
	if(strchr(hostname, ':')) {
		tmp_ptr = strchr(hostname, ':');
		/* the last : starts the port number, if avail, i.e. 8443 */
		strncpy(portnum, tmp_ptr+1,  sizeof(portnum));
		*tmp_ptr = '\0';
	}

	strcpy(host_name, hostname);
//	pthread_mutex_lock(&concurrent_control);
	port = atoi(portnum);
	if((host = gethostbyname(hostname)) == NULL)
	{
		printf("[ERR]	Cannot resolve hostname %s\n", hostname);
//		pthread_mutex_unlock(&concurrent_control);
		return 0;
	}

	/* create the basic TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	dest_addr.sin_family	= AF_INET;
	dest_addr.sin_port		= htons(port);
	dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);
	if(sockfd == -1)
	{
		printf("[ERR]	Cannot create socket %s\n", hostname);
//		pthread_mutex_unlock(&concurrent_control);
		return 0;
	}
	memset(&(dest_addr.sin_zero), '\0', 8);
	tmp_ptr = inet_ntoa(dest_addr.sin_addr);
	if(tmp_ptr)
		strcpy(website_ip, tmp_ptr);
	else
		strcpy(website_ip, "UNKNOWN");

//	pthread_mutex_unlock(&concurrent_control);
	if(connect(sockfd, (struct sockaddr *) &dest_addr, 
		sizeof(struct sockaddr)) == -1 )
	{
		close(sockfd);
		printf("[ERR]	Cannot connect to host %s [%s] on port %d.\n", 
			hostname, tmp_ptr, port);
		return 0;
	} 

	return sockfd;
}		

int get_certificate(char* url, char* website_ip, char* pem_string, int i_pem_stirng_length)
{
	X509 *cert 			= NULL;
	X509_NAME *certname 	= NULL;
	const SSL_METHOD *method;
	SSL_CTX *ctx;
	SSL *ssl;
	BIO *bio_pem;
	int server;
	int result = 0;
	size_t pem_stirng_length = i_pem_stirng_length;
	char hostname[256] = {0};

	/*
	if(SSL_library_init() <= 0)
	{
		printf("[ERR]	Openssl library has not been initialized.\n");
		return 0;
	}
	*/
	
	/* Set SSLv2 client hello, also announce SSLv3 and TLSv1 */
	method = SSLv23_client_method();

	/* Create a new SSL context */
	ctx = SSL_CTX_new(method);
	if(ctx == NULL)
	{
		printf("[ERR]	Unable to create a new SSL context structure.\n");
		return 0;
	}

	/* Disabling SSLv2 will leave v3 and TSLv1 for negotiation */
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

	/* Create new SSL connection */
	ssl = SSL_new(ctx);
	if(ssl == NULL)
	{
		SSL_CTX_free(ctx);
		printf("[ERR]	Failed to make the create SSL to %s.\n", url);
		return 0;
	}
	/* Make underliying TCP socket connection */
	server = create_openssl_socket(url, website_ip, hostname);
	if(server == 0)
	{
		SSL_CTX_free(ctx);
		SSL_free(ssl);
		printf("[ERR]	Failed to make the TCP connection to %s.\n", url);
		return 0;
	}

	/* Set SNI */
	SSL_set_tlsext_host_name(ssl, hostname);

	/* Attach the SSL session to the socket descriptor */

	SSL_set_fd(ssl, server);
	if(SSL_connect(ssl) != 1)
	{
		printf("[ERR]	Could not build a SSL session to %s.\n", url);
		SSL_CTX_free(ctx);
		SSL_free(ssl);
		close(server);
		return 0;
	}

	/* Get the remote certificate into the X509 structure */

	cert = SSL_get_peer_certificate(ssl);
	if(cert == NULL)
	{
		printf("[ERR]	Could not get a certificate from to %s.\n", url);
		SSL_CTX_free(ctx);
		SSL_free(ssl);
		close(server);
		return 0;
	}

	bio_pem = BIO_new(BIO_s_mem());
	if(bio_pem == NULL)
	{
		printf("[ERR]	Could not allocate memory for certificate. %s\n", url);
		SSL_CTX_free(ctx);
		SSL_free(ssl);
		close(server);
		X509_free(cert);
		return 0;
	}

	memset(pem_string, 0, MAX_CERTIFICATE_STRING_LENGTH - 1);

	if(bio_pem)
	{
		result = PEM_write_bio_X509(bio_pem, cert);
		result = BIO_read(bio_pem, pem_string, (int)pem_stirng_length - 1);
	}
	else
	{
		printf("[ERR]	Could not get a certificate from to %s.\n", url);
	}


	SSL_CTX_free(ctx);
	SSL_free(ssl);
	close(server);
	X509_free(cert);
	BIO_free(bio_pem);


	return 1;
}









