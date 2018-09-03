#include "tcputils.h"
#include "epollutils.h"
#include <set>
#include <map>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE 255

using namespace std;

void broadcast_message(int ep_fd, set<int> &monitored_fds, int server_fd, int from_fd, void* message, message_type mtype, codec_type ctype, map<int, tcp_conn_map_val*> *p_client_connections) {
        map<int, tcp_conn_map_val*> client_connections = *(p_client_connections);

        for(set<int>::iterator it = monitored_fds.begin(); it != monitored_fds.end(); ++it) {
                int fd = (*it);
                if (fd == server_fd || fd == from_fd) continue;

                map<int, tcp_conn_map_val*>::iterator it_map_val = client_connections.find(fd);	
                if (it_map_val == client_connections.end()) continue;
                tcp_conn_map_val* map_val = it_map_val->second;
                int res = send_message(fd, message, mtype, ctype, map_val->nbytes_indicator, map_val->nbytes_codec);
                if (res > 0) continue;
                perror("send_message");
        }
}

void broadcast_connection(int ep_fd, set<int> &monitored_fds, int server_fd, int from_fd, struct sockaddr_storage remote_addr, map<int, tcp_conn_map_val*> *p_client_connections) {

        map<int, tcp_conn_map_val*> client_connections = *(p_client_connections);

        struct easy_addr that_addr = get_easy_addr_from_storage(remote_addr);	

        char addr[NI_MAXHOST + NI_MAXSERV + 1];	bzero(addr, sizeof addr);
        int host_len = strlen(that_addr.host);
        int port_len = strlen(that_addr.port);

        memcpy(addr, that_addr.host, host_len);
        memcpy(&addr[host_len], ":", 1);
        memcpy(&addr[host_len + 1], that_addr.port, port_len);

        char content[MEDIUM_BUFF_SIZE];
        bzero(content, sizeof content);

        sprintf(content, "New connection from %s.", addr);

        printf("%s\n", content);

        message_type mtype = PLAINTEXT;
        codec_type ctype = NOCODEC;
        message_plaintext* message = create_message_plaintext(MEDIUM_BUFF_SIZE);
        update_message_plaintext(message, content, strlen(content));

        for(set<int>::iterator it = monitored_fds.begin(); it != monitored_fds.end(); ++it) {
                int fd = (*it);
                if (fd == server_fd || fd == from_fd) continue;

                map<int, tcp_conn_map_val*>::iterator it_map_val = client_connections.find(fd);	
                if (it_map_val == client_connections.end()) continue;
                tcp_conn_map_val* map_val = it_map_val->second;
                int res = send_message(fd, message, mtype, ctype, map_val->nbytes_indicator, map_val->nbytes_codec);
                if (res > 0) continue;
                perror("send_message");
        }

        delete_message_plaintext(message);
}

void close_socket(int ep_fd, set<int> &monitored_fds, int fd, int nbytes, map<int, tcp_conn_map_val*> *p_client_connections) {
        printf("Closing socket fd %d\n", fd);
        if (nbytes == 0)	printf("socket %d hung up\n", fd);
        else perror("close_socket");

        // once closed, fd will be automatically removed from all epoll sets
        map<int, tcp_conn_map_val*>::iterator it = (*p_client_connections).find(fd);	
        if (it != (*p_client_connections).end()) {
                delete_tcp_conn_map_val(it->second);
                (*p_client_connections).erase(fd);
        }	
        close(fd);
        delete_fd(ep_fd, monitored_fds, fd);
}

int main(int argc, char* argv[])
{
        if (argc < 2) {
                fprintf(stderr, "usage %s <port>\n", argv[0]);
                exit(0);
        }
        set<int> monitored_fds;

        const char* port = argv[1]; 
        int server_fd = bind_and_listen(port, false, 10); 
        if (server_fd < 0) {
                exit(1);
        }

        int ep_fd = epoll_create1(0);
        if (ep_fd < 0) {
                perror("epoll_create");
                abort();
        }

        epoll_event ev_server;
        ev_server.data.fd = server_fd;	

        // Listens on input event in an edge-triggered manner.
        ev_server.events = (EPOLLIN | EPOLLET); 	
        int res = add_fd(ep_fd, monitored_fds, server_fd, ev_server);
        if (res == -1) {
                perror ("add_fd");
                abort ();
        }				

        epoll_event *evs = (epoll_event*)calloc(MAXEVENTS, sizeof(epoll_event));	

        map<int, tcp_conn_map_val*> client_connections;

        pid_t pid = getpid();
        long pidl = (long) pid;
        printf("Listening TCP::%s.\n\nThe pid of current process is %ld, use one of the followings to show information of it.\nshell> cat /proc/%ld/status\nshell> ps -o user,pid,pcpu,pmem,vsize,rss,tname,stat,start,utime,cmd,priority,nlwp --pid %ld\n\n", port, pidl, pidl, pidl);

        do {
                int n = epoll_wait(ep_fd, evs, MAXEVENTS, -1);
                for (int i = 0; i < n; ++i) {
                        if ((evs[i].events & EPOLLERR) ||
                                        (evs[i].events & EPOLLHUP) ||
                                        (!(evs[i].events & EPOLLIN))) {
                                fprintf (stderr, "epoll error\n");
                                int target_fd = evs[i].data.fd;
                                int nbytes = -1;
                                close_socket(ep_fd, monitored_fds, target_fd, nbytes, &client_connections);
                                continue;
                        }
                        if (evs[i].data.fd == server_fd) {
                                // start handling server_fd event   
                                struct sockaddr_storage remote_addr; // client address
                                socklen_t addrlen = sizeof(remote_addr);
                                int client_fd = accept(server_fd, (struct sockaddr *)&remote_addr, &addrlen);

                                if (0 > client_fd) {
                                        if (ECONNABORTED == errno) {
                                                perror("accept, connection aborted.");
                                        }
                                        if (EMFILE == errno) {
                                                perror("accept, process-scope open file limit exceeded.");
                                        }
                                        if (ENFILE == errno) {
                                                perror("accept, system-scope open file limit exceeded.");
                                        }
                                        if (ENOBUFS == errno || ENOMEM == errno) {
                                                perror("accept, not enough memory.");
                                        }
                                        continue;
                                }

                                set_non_blocking(client_fd);

                                epoll_event ev;
                                ev.data.fd = client_fd;
                                ev.events = (EPOLLIN | EPOLLET);
                                add_fd(ep_fd, monitored_fds, client_fd, ev);
                                broadcast_connection(ep_fd, monitored_fds, server_fd, client_fd, remote_addr, &client_connections);

                                int nbytes_indicator = 2;
                                int nbytes_codec = 2;
                                int buff_size = 1024; 		
                                tcp_conn_map_val* map_val = create_tcp_conn_map_val(nbytes_indicator, nbytes_codec, buff_size);
                                client_connections.insert(pair<int, tcp_conn_map_val*>(client_fd, map_val));
                                // finished handling server_fd event
                        } else {
                                // start handling client_fd events
                                int client_fd = evs[i].data.fd;

                                map<int, tcp_conn_map_val*>::iterator it = client_connections.find(client_fd);
                                if (it == client_connections.end()) continue;	
                                tcp_conn_map_val* map_val = it->second;
                                int nbytes = recv_tcp_conn_map_val(client_fd, map_val);
                                        
                                if (0 >= nbytes) {
                                        close_socket(ep_fd, monitored_fds, client_fd, nbytes, &client_connections);
                                } else {
                                        while (try_popping_tcp_conn_map_val(map_val)) {
                                                if (0 == map_val->message_buff->size) continue; 	
                                                void* message = NULL;
                                                int mtype = PLAINTEXT;
                                                pop_message_list(map_val->message_buff, &message, &mtype, true);
                                                if (NULL == message) continue;
                                                if (PLAINTEXT == mtype) {
                                                        message_plaintext* casted_message = (message_plaintext*)message;
                                                        broadcast_message(ep_fd, monitored_fds, server_fd, client_fd, casted_message, PLAINTEXT, NOCODEC, &client_connections);
                                                }
                                                delete_message(mtype, message);
                                        }	
                                }
                                // finished handling client_fd events
                        }
                }
        } while(true);

        // clean up 
        for (map<int, tcp_conn_map_val*>::iterator it = client_connections.begin(); it != client_connections.end(); ++it) {
                delete_tcp_conn_map_val(it->second);
        }
        client_connections.clear();

        close(server_fd); 
        close(ep_fd);
        free(evs);
        return 0;
}
