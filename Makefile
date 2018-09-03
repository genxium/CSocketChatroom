CXX = g++
#CFLAGS = -Wall
CFLAGS = -Wall -g 

all: server client
AUXILIARIES = commons.cpp messages.cpp
TCP_AUXILIARIES = tcputils.cpp $(AUXILIARIES)
EPOLL_TCP_AUXILIARIES = epollutils.cpp $(TCP_AUXILIARIES)

CLEANFILES = server client 

server: tcp-epoll-server.cpp
	$(CXX) $(CFLAGS) -o server tcp-epoll-server.cpp $(EPOLL_TCP_AUXILIARIES) 

client: tcp-select-client.cpp 
	$(CXX) $(CFLAGS) -o client tcp-select-client.cpp $(TCP_AUXILIARIES) 

# ----------------------------- 
clean :
	rm -f $(CLEANFILES)
