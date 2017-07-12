CC  		= gcc
OBJS 		= ../../afl-pt-proxy 
LIBS		= 
CFLAGS	=-O3 ${LIBS}
LDFLAGS		= -lpthread

all: $(OBJS)

../../afl-pt-proxy: pt_parser.c pt_proxy.c 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) *.o
