CC=gcc
CFLAGS=-I.
LIBS=-lpthread
DEPS = pfcp_common.h
COBJ = pfcp_common.o
SOBJ = pfcp_udp_client.o
UOBJ = pfcp_udp_server.o
NOBJ = pfcp_n4_test.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

smf_sim: $(SOBJ) $(COBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

upf_sim: $(UOBJ) $(COBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

pfcp_n4_test: $(NOBJ) $(COBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

all: smf_sim upf_sim pfcp_n4_test

.PHONY: clean

clean:
	rm -f *.o
