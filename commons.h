#ifndef _COMMONSH_
#define _COMMONSH_

#include "libs/cJSON.h"
#include "messages.h"
#include <sys/fcntl.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

/**
 * data structures begin 
 * */

/**
 * easy_addr 
 * */
struct easy_addr {
	char host[NI_MAXHOST];
	char port[NI_MAXSERV];
};   

struct easy_addr get_easy_addr(struct sockaddr &remote_addr);
struct easy_addr get_easy_addr_from_storage(struct sockaddr_storage &remote_addr);

/**
 * message_list_node
 * */

typedef struct message_list_node {
	int mtype;
	void* message;
	struct message_list_node* prev;
	struct message_list_node* next;
} message_list_node;

message_list_node* create_message_list_node(int mtype, void* message);
bool delete_message_list_node(message_list_node* node, bool will_delete_message);

/**
 * message_list
 * */

typedef struct message_list {
	message_list_node *front, *rear;
	int size;
} message_list;

message_list* create_message_list();
bool delete_message_list(message_list* list, message_list_node* node, bool will_delete_message); 
bool delete_whole_message_list(message_list* list); 
bool push_message_list(message_list* list, message_list_node* node); 
bool pop_message_list(message_list* list, void** p_message, int* p_mtype, bool use_clone);
 
/**
 * data structures end
 * */

bool lshift(char* buff, int buff_size, int* p_buff_content_length, int nbytes_to_shift);

int set_non_blocking(int fd);
int clean_and_read(int fd, char* p_buf, const int buf_size);
int clean_and_recvfrom(int fd, char* p_buf, const int buf_size, struct sockaddr &src_addr);
int bind_socket(const char* port, const bool is_blocking, const int sock_type);
void set_socket_timeout_sec(int sockfd, int recv_timeout_sec, int send_timeout_sec);
void set_socket_timeout_usec(int sockfd, int recv_timeout_sec, int recv_timeout_usec, int send_timeout_sec, int send_timeout_usec);

#endif
