CC  		= gcc
OBJS 		= ../../afl-pt-proxy ../../pt-proxy-fast 
LIBS		= 
CFLAGS	= -O3 ${LIBS} -g -DKAFL_MODE #-DDEBUGMSG
LDFLAGS		= -lpthread -lcapstone

all: $(OBJS)

../../afl-pt-proxy: pt_proxy.c disassembler.c
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

../../pt-proxy-fast: pt_proxy_fast.c 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) *.o
