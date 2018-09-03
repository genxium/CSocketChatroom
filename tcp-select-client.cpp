#include "tcputils.h"
#include <map>

void close_socket(fd_set* fdset, int fd, int nbytes, int* p_fdmax, std::map<int, tcp_conn_map_val*> *p_client_connections) {
	printf("Closing socket fd %d\n", fd);
	if (nbytes == 0)	printf("socket %d hung up\n", fd);
	else	perror("recv");

	std::map<int, tcp_conn_map_val*>::iterator it = (*p_client_connections).find(fd);	
	if (it != (*p_client_connections).end()) {
		delete_tcp_conn_map_val(it->second);
		(*p_client_connections).erase(fd);
	}	

	close(fd);
	FD_CLR(fd, fdset);
	if (fd == (*p_fdmax)) --(*p_fdmax);
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		fprintf(stderr,"usage %s <hostname> <port>\n", argv[0]);
		exit(0);
	}
	char* port = argv[2];
	char* host = argv[1];

	int client_fd_tcp = connect_host(host, port, false); 
	if (client_fd_tcp < 0) return 0; 

  printf("Connected to %s:%s.\n", host, port);

	// creaate and store tcp_conn_map_val for client_fd_tcp
	int nbytes_indicator = 2;
	int nbytes_codec = 2;
	int buff_size = 1024; 		
	tcp_conn_map_val* tmp_map_val = create_tcp_conn_map_val(nbytes_indicator, nbytes_codec, buff_size);

	std::map<int, tcp_conn_map_val*> client_connections;
	client_connections.insert(std::pair<int, tcp_conn_map_val*>(client_fd_tcp, tmp_map_val));

	// fdmax:  maximum file descriptor number
	int fdmax = client_fd_tcp;
	fd_set master_fd_set;    // master fd set for maintenance
	FD_ZERO(&master_fd_set);    // clear the master fd set

	// add the client fd and stdin fd to the master_fd_set set
	FD_SET(client_fd_tcp, &master_fd_set);
	FD_SET(STDIN_FILENO, &master_fd_set);

	// main loop
	bool terminate = false; 
	char input_buff[MEDIUM_BUFF_SIZE]; 
	char* p_input_buff = input_buff;

	do {
		if (terminate) break;
		fd_set read_fd_set = master_fd_set; // temp fd set for select()
		if (select(fdmax + 1, &read_fd_set, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}

		for (int i = 0; i <= fdmax; i++) {
			if (!FD_ISSET(i, &read_fd_set))	continue;
			if (i != STDIN_FILENO) {
				std::map<int, tcp_conn_map_val*>::iterator it = client_connections.find(i);
				if (it == client_connections.end()) continue;	
				tcp_conn_map_val* map_val = it->second;
				int res = recv_tcp_conn_map_val(i, map_val);
				if (res <= 0)	{
					if (i == client_fd_tcp)	printf("Server hung up.\n");
					if (i == client_fd_tcp)	terminate = true;
					if (i == client_fd_tcp) break;
					close_socket(&master_fd_set, i, res, &fdmax, &client_connections);
				} else {
					while (try_popping_tcp_conn_map_val(map_val)) {
						if (map_val->message_buff->size == 0) continue; 	

						void* message = NULL;
						int mtype = PLAINTEXT;
						pop_message_list(map_val->message_buff, &message, &mtype, true);
						if (message == NULL) continue;

						if (mtype == PLAINTEXT) {
							message_plaintext* casted_message = (message_plaintext*)message;
							fprintf(stdout, "Received: %s\n", casted_message->content);
						}
						// delete message
						delete_message(mtype, message);
					}	
				}
				continue;
			} 

			int nbytes = clean_and_read(i, p_input_buff, MEDIUM_BUFF_SIZE);
			if (nbytes == 0) {
				perror("clean_and_read");
				terminate = true;
				break;
			}

			std::map<int, tcp_conn_map_val*>::iterator it = client_connections.find(client_fd_tcp);	
			if (it == client_connections.end()) continue;
			tcp_conn_map_val* map_val = it->second;

			message_type mtype = PLAINTEXT;
			codec_type ctype = NOCODEC;
			message_plaintext* message = create_message_plaintext(MEDIUM_BUFF_SIZE);	

      // This is a dirty fix to remove the `new line` symbol.
			update_message_plaintext(message, p_input_buff, strlen(input_buff) - 1); 
			int nbytes_sent = send_message(client_fd_tcp, message, mtype, ctype, map_val->nbytes_indicator, map_val->nbytes_codec);
			if (nbytes_sent <= 0)	perror("send_message");				
			if (nbytes_sent <= 0)	terminate = true;
			if (nbytes_sent <= 0)	break;

      printf("Sent: %s\n", p_input_buff);
		}
	} while(true);

	close(client_fd_tcp);
	FD_ZERO(&master_fd_set);

	return 0;
}
