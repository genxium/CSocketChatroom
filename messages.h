#ifndef _MESSAGESH_
#define _MESSAGESH_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#define MAX_BUFF_SIZE 1024
#define LARGE_BUFF_SIZE 512 
#define MEDIUM_BUFF_SIZE 256
#define SMALL_BUFF_SIZE 128

/**
 * data structures begin 
 * */

/**
 * codec_type 
 **/
enum codec_type {
	NOCODEC = 0,
	JSON = 1,
	MSGPACK = 2,
	PROTOBUF = 3
}; 

/**
 * message_type
 * */
enum message_type {
	PLAINTEXT = 0, // codec_type == NOCODEC
	CJSON = 1, //  codec_type == JSON
	MSGPACK_OBJECT = 2, // codec_type == MSGPACK
	OTHER // codec_type == PROTOBUF, to be added
};

bool encode_message_to_payload(void* in_message, message_type mtype, codec_type ctype, char* out_payload);
void* decode_payload_to_message(int nbytes_codec, char* in_payload, int in_payload_length, int* p_mtype);
int send_message(int to_sockfd, void* message, message_type mtype, codec_type ctype, int nbytes_indicator, int nbytes_codec);

/**
 * message_plaintext
 * */
typedef struct message_plaintext {
	char* content;
	int content_length;
	int max_content_length;
} message_plaintext;

message_plaintext* create_message_plaintext(int max_content_length);
bool update_message_plaintext(message_plaintext* message, char* content, int content_length);
bool delete_message_plaintext(message_plaintext* message);

/**
 * data structures end
 * */

void* clone_message(int mtype, void* original_message);
bool delete_message(int mtype, void* message);

#endif 
