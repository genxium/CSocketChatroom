#include "tcputils.h"

tcp_conn_map_val* create_tcp_conn_map_val(int nbytes_indicator, int nbytes_codec, int buff_size) {
        tcp_conn_map_val* ret = (tcp_conn_map_val*)malloc(sizeof(tcp_conn_map_val));
        ret->state = EXPECTING_LENGTH_INDICATOR;
        ret->nbytes_indicator = nbytes_indicator; 
        ret->nbytes_codec = nbytes_codec;

        ret->buff = (char*) calloc(buff_size, sizeof(char));
        ret->buff_content_length = 0;
        ret->buff_size = buff_size;

        ret->expecting_length = nbytes_indicator;
        ret->payload_length = 0;

        ret->message_buff = create_message_list();
        ret->should_pop = false;

        return ret;
}

bool delete_tcp_conn_map_val(tcp_conn_map_val* map_val) {
        if (map_val == NULL) return true; 

        if (map_val->buff) free(map_val->buff); 
        if (map_val->message_buff) delete_whole_message_list(map_val->message_buff);    
        free(map_val);
        return true;
}

bool lshift_tcp_conn_map_val(tcp_conn_map_val* map_val, int nbytes_to_shift) {
        return lshift(map_val->buff, map_val->buff_size, &(map_val->buff_content_length), nbytes_to_shift);
}

bool try_popping_tcp_conn_map_val(tcp_conn_map_val* map_val) {
        if (NULL == map_val) return false;
        if (!map_val->should_pop) return false;

        if (EXPECTING_LENGTH_INDICATOR == map_val->state) {

                // calculate payload length from indicator
                char indicator[MAX_BUFF_SIZE];
                bzero(indicator, MAX_BUFF_SIZE * sizeof(char));
                memcpy(indicator, map_val->buff, map_val->nbytes_indicator);

                // TODO: User a better codec for length indicator.
                int payload_length = atoi(indicator); 
        
                // shift buffer
                lshift_tcp_conn_map_val(map_val, map_val->nbytes_indicator);

                // state transition
                map_val->state = EXPECTING_PAYLOAD; 
                map_val->expecting_length = payload_length;
                map_val->payload_length = payload_length;
        
        }  else if (EXPECTING_PAYLOAD == map_val->state) {

                // decode message
                int mtype = PLAINTEXT;
                void* message = decode_payload_to_message(map_val->nbytes_codec, map_val->buff, map_val->payload_length, &mtype);       

                // store message
                message_list_node* node = create_message_list_node(mtype, message); 
    push_message_list(map_val->message_buff, node);  

                // shift buffer
                lshift_tcp_conn_map_val(map_val, map_val->payload_length);

                // state transition
                map_val->state = EXPECTING_LENGTH_INDICATOR; 
                map_val->expecting_length = map_val->nbytes_indicator;
                map_val->payload_length = 0;

                map_val->should_pop = false;
        } else;

        
        return true;
}

int recv_tcp_conn_map_val(int sockfd, tcp_conn_map_val* map_val) {
        if (NULL == map_val)    return 0;       

        int existing_length = map_val->buff_content_length;
        int nbytes = recv(sockfd, map_val->buff + existing_length, map_val->buff_size - existing_length, 0);    

        map_val->buff_content_length += nbytes;

        if (map_val->buff_content_length >= map_val->expecting_length) {
                map_val->should_pop = true;
        } 
        
        return nbytes;
}

int bind_and_listen(const char* port, const bool is_blocking, const int backlog) {
        int sockfd = bind_socket(port, is_blocking, SOCK_STREAM);
        if (sockfd < 0) {
                perror("bind_and_listen");
                return (sockfd = -1);           
        }
        if (listen(sockfd, backlog) < 0) {
                perror("listen");
                return (sockfd = -1);
        }
        return sockfd;
}

int connect_host(const char* host, const char* port, const bool is_blocking) {
        /**
         * This function works in a blocking manner.
         */
        int sockfd = 0;

        // get us a socket and bind it
        struct addrinfo hints, *ai, *p;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;
        int rv = getaddrinfo(host, port, &hints, &ai);
        if (rv != 0) {
                fprintf(stderr, "error: %s\n", gai_strerror(rv));
                exit(1);
        }

        for(p = ai; p != NULL; p = p->ai_next) {
                sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
                if (sockfd < 0) continue;

                // lose the pesky "address already in use" error message
                int yes = 1;        // for setsockopt() SO_REUSEADDR, below
                setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

                if (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0) {
                        close(sockfd);
                        continue;
                }

                break;
        }

        if (p == NULL) {
                fprintf(stderr, "error: failed to connect\n");
                sockfd = (-1);
        }

        if (sockfd > 0 && !is_blocking) set_non_blocking(sockfd);

        freeaddrinfo(ai);

        return sockfd;
}
