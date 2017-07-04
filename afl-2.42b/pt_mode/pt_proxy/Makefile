CC  		= gcc
OBJS 		= ../../afl-pt-proxy 
LIBS		= 
CFLAGS	=-O3 ${LIBS}
LDFLAGS		= 

all: $(OBJS)

../../afl-pt-proxy: pt_proxy.c 
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	$(RM) $(OBJS) *.o
