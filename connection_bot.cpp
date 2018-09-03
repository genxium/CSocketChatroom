#include "tcputils.h"
#include <set>
using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3) {
       fprintf(stderr,"usage %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    char* port = argv[2];
    char* host = argv[1];

	set<int> successful_fds;
	for (int i = 0; i < 4096; ++i) {
		int client_fd_tcp = connect_host(host, port, false); 
		if (client_fd_tcp < 0) {
			break;
		}
		successful_fds.insert(client_fd_tcp);
		printf("i = %d\n", i);
	}
	for (set<int>::iterator it = successful_fds.begin(); it != successful_fds.end(); ++it) {
		int fd = *it;
		close(fd);
	}
	
	printf("max connections: %ld\n", successful_fds.size());	
	
	return 0;
}
