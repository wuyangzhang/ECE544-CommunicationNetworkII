PROGRAM = mf_click_mon

BINDIR = /usr/local/bin/

OBJS = click_mon.o packet_stats.o routing_stats.o link_stats.o control_socket.o
CC = g++
INCLUDEDIRS = 
#CFLAGS = -O0 -Wall -c $(INCLUDEDIRS) -DDEBUG=1 -g 
CFLAGS = -O2 -Wall -c $(INCLUDEDIRS)
LIBS = -loml2 -locomm #-lpopt
LDFLAGS = -Wall  $(LIBS)

all: build
build: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

click_mon.o: click_mon.cpp packet_stats.h routing_stats.h link_stats.h config.h
	$(CC) $(CFLAGS) click_mon.cpp

packet_stats.o: packet_stats.cpp packet_stats.h control_socket.h
	$(CC) $(CFLAGS) packet_stats.cpp

routing_stats.o: routing_stats.cpp routing_stats.h control_socket.h
	$(CC) $(CFLAGS) routing_stats.cpp

link_stats.o: link_stats.cpp link_stats.h control_socket.h
	$(CC) $(CFLAGS) link_stats.cpp

control_socket.o: control_socket.cpp control_socket.h
	$(CC) $(CFLAGS) control_socket.cpp

install: $(PROGRAM)
	install -m 755 $(PROGRAM) $(BINDIR)

clean:
	rm -f a.out *.o *~ mf_click_mon
