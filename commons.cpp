#include "commons.h"

struct easy_addr get_easy_addr(struct sockaddr &remote_addr) {
        easy_addr ret;
        int rc = getnameinfo(&remote_addr, sizeof(remote_addr), ret.host, sizeof(ret.host), ret.port, sizeof(ret.port), NI_NUMERICHOST | NI_NUMERICSERV);
        if (rc != 0) {
                perror("get_easy_addr");
        }       
        return ret; 
}

struct easy_addr get_easy_addr_from_storage(struct sockaddr_storage &remote_addr) {
        struct sockaddr* casted_addr = (struct sockaddr*) (&remote_addr); 
        return get_easy_addr(*casted_addr);
}

message_list_node* create_message_list_node(int mtype, void* message) {

        message_list_node* ret = (message_list_node*)malloc(sizeof(message_list_node));         

        ret->mtype = mtype;
        ret->message = message;

        ret->prev = NULL;
        ret->next = NULL;

        return ret;
        
}

bool delete_message_list_node(message_list_node* node, bool will_delete_message = true) {
        if (!node) return true;
        // setting `will_delete_message` might result in memory leak, make sure that node->message was kept elsewhere beforehand  
        if (node->message && will_delete_message)       delete_message(node->mtype, node->message);
        free(node);
        return true;
}

message_list* create_message_list() {
        message_list* ret = (message_list*)malloc(sizeof(message_list));        
        ret->front = NULL;
        ret->rear = NULL;
        ret->size = 0;
        return ret;
}

bool delete_message_list(message_list* list, message_list_node* node, bool will_delete_message = true) {
        if (!list || !node) return false;       
        --list->size;
        if (node->prev && node->next) {
                node->prev->next = node->next;
                node->next->prev = node->prev;
        } else if (node->next) {
                list->front = node->next;
                list->front->prev = NULL;
        } else if (node->prev) {
                list->rear = node->prev;
                list->rear->next = NULL;
        } else {
                list->front = NULL;
                list->rear = NULL;
        }
        return delete_message_list_node(node, will_delete_message);
}

bool push_message_list(message_list* list, message_list_node* node) {
        if (!node || !list) return false;       
        ++list->size;
        if (!list->rear) {
                list->front = list->rear = node;
                return true;
        }  
        list->rear->next = node;
        node->prev = list->rear;
        list->rear = node;
        return true;
}

bool pop_message_list(message_list* list, void** p_message, int* p_mtype, bool use_clone = true) {
        if (!list || !list->front) return false;
        message_list_node* front = list->front; 

        *(p_mtype) = front->mtype;
        if (use_clone) {
                *(p_message) = clone_message(front->mtype, front->message);
        } else {
                *(p_message) = front->message;
        }

        bool will_delete_message = use_clone;
        return delete_message_list(list, front, will_delete_message);   
}

bool delete_whole_message_list(message_list* list) {
        if (!list) return false;
        message_list_node* it = list->front;
        while (it) {
                delete_message_list(list, it);
                it = it->next;
        }
        free(list);
        return true;
}

bool lshift(char* buff, int buff_size, int* p_buff_content_length, int nbytes_to_shift) {
        int buff_content_length = *(p_buff_content_length);
        if (buff_content_length < nbytes_to_shift) {
                *(p_buff_content_length) = 0;           
                bzero(buff, buff_size * sizeof(char));  
                return true; 
        }

        int nbytes_to_copy = (buff_content_length - nbytes_to_shift);

        // Kindly note that `void * memcpy ( void * dst, const void * src, size_t num )` SHOULDN'T be used when `src == dst`.
        memmove(buff, buff + nbytes_to_shift, nbytes_to_copy * sizeof(char));
        memset(buff + nbytes_to_copy, '\0', (buff_size - nbytes_to_copy) * sizeof(char));

        *(p_buff_content_length) -= nbytes_to_shift;            
        return true;
}

int set_non_blocking(int fd) {
        return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int clean_and_read(int fd, char* p_buf, const int buf_size) {
        bzero(p_buf, buf_size);
        return read(fd, p_buf, buf_size);
}

int clean_and_recvfrom(int fd, char* p_buf, const int buf_size, struct sockaddr &src_addr) {
        bzero(p_buf, buf_size);
        socklen_t addr_len = (socklen_t)(sizeof src_addr);
        return recvfrom(fd, p_buf, buf_size, 0, &src_addr, &addr_len);
}

int bind_socket(const char* port, const bool is_blocking, const int sock_type) {

        int sockfd = 0;

        // get us a socket and bind it
        struct addrinfo hints, *ai, *p;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = sock_type;
        hints.ai_flags = AI_PASSIVE;
        int rv = getaddrinfo(NULL, port, &hints, &ai);
        if (rv != 0) {
                fprintf(stderr, "error: %s\n", gai_strerror(rv));
                exit(1);
        }

        for(p = ai; p != NULL; p = p->ai_next) {
                sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (sockfd < 0) {
                        perror("socket");
                        continue;
                }

                // lose the pesky "address already in use" error message
                int yes = 1;        // for setsockopt() SO_REUSEADDR, below
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
                        perror("bind");
                        close(sockfd);
                        continue;
                }

                break;
        }

        if (p == NULL) {
                perror("bind_socket");
                sockfd = (-1);
        }

        freeaddrinfo(ai);

        if (0 <= sockfd && !is_blocking) set_non_blocking(sockfd); 

        return sockfd;
}

void set_socket_timeout_sec(int sockfd, int recv_timeout_sec, int send_timeout_sec) {
        struct timeval recv_timeout_val;
        recv_timeout_val.tv_sec = recv_timeout_sec;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_val, sizeof recv_timeout_val);
        struct timeval send_timeout_val;
        send_timeout_val.tv_sec = send_timeout_sec;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout_val, sizeof send_timeout_val);
}

void set_socket_timeout_usec(int sockfd, int recv_timeout_sec, int recv_timeout_usec, int send_timeout_sec, int send_timeout_usec) {
        struct timeval recv_timeout_val;
        recv_timeout_val.tv_sec = recv_timeout_sec;
        recv_timeout_val.tv_usec = recv_timeout_usec;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout_val, sizeof recv_timeout_val);
        struct timeval send_timeout_val;
        send_timeout_val.tv_sec = send_timeout_sec;
        send_timeout_val.tv_usec = send_timeout_usec;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout_val, sizeof send_timeout_val);
}
