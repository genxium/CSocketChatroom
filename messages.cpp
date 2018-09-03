#include "messages.h"

bool encode_message_to_payload(void* in_message, message_type mtype, codec_type ctype, int nbytes_codec, char* out_payload) {
        if (NULL == out_payload) {
                perror("encode_message_to_payload, out_payload cannot be NULL.");
                return false;
        }
        if (NULL == in_message) {
                perror("encode_message_to_payload, in_message cannot be NULL.");
                return false;
        }
        if (PLAINTEXT == mtype) {
                message_plaintext* casted_message = (message_plaintext*) in_message;
                sprintf(out_payload, "%0*d%s", nbytes_codec, (int)ctype, casted_message->content);
                return true;
        } else {
                return false;
        }
}

void* decode_payload_to_message(int nbytes_codec, char* in_payload, int in_payload_length, int* p_mtype) {
        if (in_payload_length < nbytes_codec) return NULL;

        // store ctype
        char codec[SMALL_BUFF_SIZE];
        bzero(codec, SMALL_BUFF_SIZE * sizeof(char));
        memcpy(codec, in_payload, nbytes_codec * sizeof(char));
        int ctype = atoi(codec); 

        // create message
        int content_length = (in_payload_length - nbytes_codec);
        char content[MEDIUM_BUFF_SIZE];
        bzero(content, MEDIUM_BUFF_SIZE * sizeof(char));
        memcpy(content, in_payload + nbytes_codec, content_length * sizeof(char));

        if (NOCODEC == ctype) {
                *(p_mtype) = PLAINTEXT;
                message_plaintext* message = create_message_plaintext(LARGE_BUFF_SIZE); 
                update_message_plaintext(message, content, content_length);
                return message;
        } else {
                return NULL;
        }
}

int send_message(int to_sockfd, void* message, message_type mtype, codec_type ctype, int nbytes_indicator, int nbytes_codec) {
        char payload[LARGE_BUFF_SIZE];
        bzero(payload, LARGE_BUFF_SIZE * sizeof(char));
        encode_message_to_payload(message, mtype, ctype, nbytes_codec, payload);

        char to_send[LARGE_BUFF_SIZE]; 
        bzero(to_send, LARGE_BUFF_SIZE * sizeof(char));

        sprintf(to_send, "%0*d%s", nbytes_indicator, (int)strlen(payload), payload);
        return send(to_sockfd, to_send, strlen(to_send), 0);
}

message_plaintext* create_message_plaintext(int max_content_length) {
        message_plaintext* ret = (message_plaintext*)malloc(sizeof(message_plaintext));
        ret->content = (char*)calloc(max_content_length, sizeof(char)); 
        ret->max_content_length = max_content_length;
        ret->content_length = 0;
        return ret;
}

bool update_message_plaintext(message_plaintext* message, char* content, int content_length) {
        if (message == NULL) return false;

        bzero(message->content, message->max_content_length * sizeof(char));

        message->content_length = content_length;
        memcpy(message->content, content, content_length * sizeof(char));
        return true;
}

bool delete_message_plaintext(message_plaintext* message) {
        if (message == NULL) return true;
        if (message->content) free(message->content);
        free(message);
        return true;
}

void* clone_message(int mtype, void* original_message) {
        switch (mtype) {
                case PLAINTEXT: {
                        message_plaintext* casted_message = ((message_plaintext*)original_message);
                        message_plaintext* message = create_message_plaintext(casted_message->max_content_length);
                        update_message_plaintext(message, casted_message->content, casted_message->content_length);
                        return message;
                }
                default:
                        return NULL;
        }
}

bool delete_message(int mtype, void* message) {
        switch (mtype) {
                case PLAINTEXT: { 
                        message_plaintext* casted_message = ((message_plaintext*)message);
                        return delete_message_plaintext(casted_message);
                }
                default:
                        return false;
        }
}
