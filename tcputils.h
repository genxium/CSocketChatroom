#ifndef _TCPUTILSH_
#define _TCPUTILSH_

#include "commons.h"

/**
 * data structures begin 
 * */

/**
 * length_indicator_algo_state
 * */
enum length_indicator_algo_state {
        EXPECTING_LENGTH_INDICATOR = 0,
        EXPECTING_PAYLOAD = 1
};

/**
 * tcp_conn_map_val
 * */
typedef struct tcp_conn_map_val {
        // a map of {sockfd => tcp_conn_map_val} is maintained  
        length_indicator_algo_state state;

        char* buff;
        int buff_content_length;        
        int buff_size;

        int nbytes_indicator;   
        int payload_length;
        int expecting_length;

        int nbytes_codec;

        message_list* message_buff;

        bool should_pop;
} tcp_conn_map_val;

tcp_conn_map_val* create_tcp_conn_map_val(int nbytes_indicator, int nbytes_codec, int buff_size);
bool delete_tcp_conn_map_val(tcp_conn_map_val* map_val);
bool lshift_tcp_conn_map_val(tcp_conn_map_val* map_val, int nbytes_to_shift);

bool try_popping_tcp_conn_map_val(tcp_conn_map_val* map_val);
int recv_tcp_conn_map_val(int sockfd, tcp_conn_map_val* map_val);

/**
 * data structures end
 * */

int bind_and_listen(const char* port, const bool is_blocking, const int backlog);
int connect_host(const char* host, const char* port, const bool is_blocking);


#endif
