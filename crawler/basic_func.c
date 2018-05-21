#include "crawler_header.h"
#include "limits.h"
void 	heap_heapify(long* heap_array, int index, int heap_size, int array_size);
void 	heap_build_heap(long * heap_array, int heap_size, int array_size);
long 	heap_pop_min(long * heap_array, int* heap_size, int array_size);
void 	heap_insert(long * heap_array, int* heap_size, int array_size, long value);
long 	heap_peek_min(long * heap_array, int heap_size, int array_size);
int 	heap_find(long * heap_array, int heap_size, int array_size, long value);

void 	heap_decrease_key(long * heap_array, int heap_size, int array_size, int index, long new_value);
int 	heap_parent(int n);
int 	heap_left_child(int n);
int 	heap_right_child(int n);


void get_primary_ip(char* buffer, size_t buflen) 
{
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1) return;
	const char* kGoogleDnsIp = "8.8.8.8";
	uint16_t kDnsPort = 53;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
	serv.sin_port = htons(kDnsPort);

	int err = connect(sock, (const struct sockaddr*) &serv, sizeof(serv));
	if(err == -1) return;

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr*) &name, &namelen);
	if(err == -1) {close(sock);return;}

	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, buflen);

	close(sock);
}

void print_func(char* tag, char* message)
{
	printf("[%s]	%s\n", tag, message);
}

int heap_find(long * heap_array, int heap_size, int array_size, long value)
{
	int i = 0;
	for(i = 0; i < heap_size; i++)
	{
		if(*(heap_array + i) == value)
			return 1;
	}
	return 0;
}

/* -- heap insert func -- */
void heap_insert(long * heap_array, int* heap_size, int array_size, long value)
{
	// exchange the head with the tail
	*(heap_size) = *(heap_size) + 1;
	*(heap_array + *heap_size - 1) = LONG_MAX;
	// heapify the the top element
	heap_decrease_key(heap_array, *heap_size, array_size, *heap_size - 1, value);
	return;
}

/* -- heap decrease func -- */
void heap_decrease_key(long * heap_array, int heap_size, int array_size, int index, long new_value)
{
	if(*(heap_array + index) < new_value)	//error
		return;
	*(heap_array + index) = new_value;
	
	int tmp = 0;
	while(index >= 0 && *(heap_array + index) < *(heap_array + heap_parent(index)))
	{
		tmp 	= *(heap_array + index);
		*(heap_array + index) = *(heap_array + heap_parent(index));
		*(heap_array + heap_parent(index)) = tmp;
		if(index != 0)
			index 	= heap_parent(index);
		else
			break;
	}
}

/* -- peak heap top -- */
long heap_peek_min(long * heap_array, int heap_size, int array_size)
{
	if(heap_size > 0)
		return *(heap_array);
	else
		return -1;
}

/* -- heap pop minimum element func -- */
long heap_pop_min(long * heap_array, int* heap_size, int array_size)
{
	// exchange the head with the tail
	int tmp = *(heap_array);
	*(heap_array) 	= *(heap_array + *heap_size - 1);
	*(heap_size)	= *(heap_size) - 1;

	// heapify the the top element
	heap_heapify(heap_array, 0, *heap_size, array_size);
	
	return tmp;
}

/* -- heap construct func -- */
void heap_build_heap(long * heap_array, int heap_size, int array_size)
{
	int i = 0;
	for(i = heap_size / 2; i >= 0; i --)
	{
		heap_heapify(heap_array, i, heap_size, array_size);
	}
	return;
}

/* -- heap heapify func -- */
void heap_heapify(long* heap_array, int index, int heap_size, int array_size)
{
	int min = index;
	if(heap_left_child(min) < heap_size && 
		*(heap_array + heap_left_child(index)) < *(heap_array + min))
	{
		min = heap_left_child(index);
	}
	if(heap_right_child(index) < heap_size && 
		*(heap_array + heap_right_child(index)) < *(heap_array + min))
	{
		min = heap_right_child(index);
	}
	if(min != index)
	{
		int tmp = *(heap_array + min);
		*(heap_array + min) 	= *(heap_array + index);
		*(heap_array + index) 	= tmp;
		heap_heapify(heap_array, min, heap_size, array_size);
	}
	return;
}

int heap_parent(int n)
{
	return n / 2;
}

int heap_left_child(int n)
{
	return n * 2;
}

int heap_right_child(int n)
{
	return n * 2 + 1;
}