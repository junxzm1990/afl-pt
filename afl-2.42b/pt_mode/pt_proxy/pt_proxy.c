#define _GNU_SOURCE
#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <dirent.h>
#include <sched.h>

#include <fcntl.h>


#include "../../config.h"
#include "../../types.h"
#include "../../debug.h"
#include "../../alloc-inl.h"
#include "pt_proxy.h"

/* using global data because afl will start one proxy instance per target                 */

#ifndef HAVE_AFFINITY
#define HAVE_AFFINITY
#endif

/* NETLINK data structures */
#define NETLINK_USER 31                                /* self-define netlink number      */
#define MAX_PAYLOAD 256                                /* maximum payload size            */
int sock_fd;                                           /* sock fd connect to pt_m         */
struct sockaddr_nl src_addr,                           /* sock src structure              */
                   dest_addr;                          /* sock dest structure             */
struct msghdr nl_msg;                                  /* contain iovec pointer           */
struct iovec iov;                                      /* contain nlmsghdr pointer        */
struct nlmsghdr *nlh = NULL;                           /* contain data payload pointer    */


/* AFL data structures */
u8  __afl_area_initial[MAP_SIZE];                      /* trace_bits map                  */
u8* __afl_area_ptr = __afl_area_initial;
static s32 proxy_ctl_fd,                               /* fork server control pipe(write) */
           proxy_st_fd;                                /* fork server status pipe(read)   */
static u8 *target_path;                                /* target program file path        */
static s32 forksrv_pid;                                /* fork server process id          */

/* we use this additional AFLPT_# AFLPT_#+1 pair to communicate between proxy and forksrv */
#define AFLPT_FORKSRV_FD (FORKSRV_FD - 3)

/* ptm-proxy-worker data structures */
#define PT_START "START"                               /* start msg to pt_module          */
#define PT_TARGET "TARGET"                             /* target msg to pt_module         */
#define PT_BUF_NEXT "NEXT"                             /* next msg to get new pt_buf_off  */
#define DEM ":"                                        /* delimeter for pt messeges       */
#define PT_TARGET_CONFIRM "TCONFIRM"                   /* confirm msg from pt_module      */
#define PT_START_CONFIRM "SCONFIRM"                    /* start confirm from pt_module    */
#define PT_TOPA_READY "TOPA"                           /* topa msg from pt_module         */
enum proxy_status proxy_cur_state = PROXY_SLEEP;       /* global proxy state              */
s64 pt_trace_buf = 0;                                  /* address of the pt trace buffer  */
s64 pt_trace_buf_size = 0;                             /* size of the pt trace buffer     */
s64 pt_trace_off_bound = 0;                            /* boundary of trace buffer        */
s64 *p_pt_trace_off = 0;                               /* last off to the buf pt update   */
volatile u8  worker_done,                              /* for syncing worker and proxy    */
            worker_not_done;
#define ONE_BYTE_ENTRIES (1<<8)                        /* num of keys in the rand_map     */
#define TWO_BYTE_ENTRIES (1<<16)                       /* num of keys in the rand_map     */
#define MAX_64K (1<<16) 
u64 rand_map[TWO_BYTE_ENTRIES];                        /* maps u8 val to random value UR()*/
static u32 rand_cnt;                                   /* Random number counter           */
static s32 dev_urandom_fd = -1;                        /* Persistent fd for /dev/urandom  */
u64 curr_ip = 0;                                       /* current ip used by parse worker */
u64 last_ip = 0;                                       /* prev ip used by parse worker    */
u16 curr_tnt_prod = 0;                                 /* tmp tnt product used by worker  */


/* Generate a random number (from 0 to limit - 1). This may have slight bias. */
static inline u32 UR(u32 limit) {

    if (unlikely(!rand_cnt--)) {

        u32 seed[2];

        ck_read(dev_urandom_fd, &seed, sizeof(seed), "/dev/urandom");

        srandom(seed[0]);
        rand_cnt = (RESEED_RNG / 2) + (seed[1] % RESEED_RNG);

    }

    return random() % limit;

}

/* Populate a new rand_map for fuzzing */
static inline void
gen_rand_map(u32 entries, u32 max){
    int i;
    if(sizeof(rand_map)/sizeof(u64) < entries)
        PFATAL("randmap size is too small!");
    for (i=0; i<entries; ++i){
        rand_map[i] = UR(max); 
    }
}

#ifdef HAVE_AFFINITY

/* Build a list of processes bound to specific cores. Returns -1 if nothing
   can be found. Assumes an upper bound of 4k CPUs. */

static void bind_to_free_core(void) {

  DIR* d;
  struct dirent* de;
  cpu_set_t c;
  s32 cpu_aff = -1;  
  u32 cpu_core_count = sysconf(_SC_NPROCESSORS_ONLN);


  u8 cpu_used[4096] = { 0 };
  u32 i;

  if (cpu_core_count < 2) return;

  if (getenv("AFL_NO_AFFINITY")) {

    WARNF("Not binding to a CPU core (AFL_NO_AFFINITY set).");
    return;

  }

  d = opendir("/proc");

  if (!d) {

    WARNF("Unable to access /proc - can't scan for free CPU cores.");
    return;

  }

  ACTF("Checking CPU core loadout...");

  /* Introduce some jitter, in case multiple AFL tasks are doing the same
     thing at the same time... */

  usleep(R(1000) * 250);

  /* Scan all /proc/<pid>/status entries, checking for Cpus_allowed_list.
     Flag all processes bound to a specific CPU using cpu_used[]. This will
     fail for some exotic binding setups, but is likely good enough in almost
     all real-world use cases. */

  while ((de = readdir(d))) {

    u8* fn;
    FILE* f;
    u8 tmp[MAX_LINE];
    u8 has_vmsize = 0;

    if (!isdigit(de->d_name[0])) continue;

    fn = alloc_printf("/proc/%s/status", de->d_name);

    if (!(f = fopen(fn, "r"))) {
      ck_free(fn);
      continue;
    }

    while (fgets(tmp, MAX_LINE, f)) {

      u32 hval;

      /* Processes without VmSize are probably kernel tasks. */

      if (!strncmp(tmp, "VmSize:\t", 8)) has_vmsize = 1;

      if (!strncmp(tmp, "Cpus_allowed_list:\t", 19) &&
          !strchr(tmp, '-') && !strchr(tmp, ',') &&
          sscanf(tmp + 19, "%u", &hval) == 1 && hval < sizeof(cpu_used) &&
          has_vmsize) {

        cpu_used[hval] = 1;
        break;

      }

    }

    ck_free(fn);
    fclose(f);

  }

  closedir(d);

  for (i = 0; i < cpu_core_count; i++) if (!cpu_used[i]) break;

  if (i == cpu_core_count) {

    SAYF("\n" cLRD "[-] " cRST
         "Uh-oh, looks like all %u CPU cores on your system are allocated to\n"
         "    other instances of afl-fuzz (or similar CPU-locked tasks). Starting\n"
         "    another fuzzer on this machine is probably a bad plan, but if you are\n"
         "    absolutely sure, you can set AFL_NO_AFFINITY and try again.\n",
         cpu_core_count);

    FATAL("No more free CPU cores");

  }

  OKF("Found a free CPU core, binding to #%u.", i);

  cpu_aff = i;

  CPU_ZERO(&c);
  CPU_SET(i, &c);

  if (sched_setaffinity(0, sizeof(c), &c))
    PFATAL("sched_setaffinity failed");

}
#endif

inline s64 req_next(s64 cur_boundary){
    char sendstr[MAX_PAYLOAD];
    snprintf(sendstr, MAX_PAYLOAD, "NEXT:0x%lx", cur_boundary); 
    proxy_send_msg(sendstr);
    proxy_recv_msg();
    return pt_trace_off_bound;
}

s32 packet_fd = -1;
s32 off_fd = -1;
/* this function run in a thread until the whole fuzzing is done */
static void *pt_parse_worker(void *arg)
{
    u64 cursor_pos = 0;
    u64 bound_snapshot = 0;
    s64 next;

#ifdef HAVE_AFFINITY
    bind_to_free_core();
#endif

/* #define DEBUG */
#ifdef DEBUG
    char msg[256];
    off_fd = open("/tmp/test.log", O_RDWR);
#endif

/* #define DEBUG_PACKET */

#ifdef DEBUG_PACKET
    packet_fd = open("/tmp/packet.log", O_RDWR);
#endif

    /* while(!pt_trace_buf)pthread_yield(); */

    while(1){
        bound_snapshot = *p_pt_trace_off;
        if(proxy_cur_state == PROXY_FUZZ_STOP && cursor_pos >= bound_snapshot){
            //only when worker_done is 0, the atomic return false
            if(!__atomic_test_and_set(&worker_done, __ATOMIC_SEQ_CST)){ //when proxy is really waiting for us
/* #ifdef DEBUG */
/*                 snprintf(msg, 256, "Current offset %lx\n", (unsigned long)bound_snapshot); */
/*                 write(off_fd, msg, strlen(msg)); */
/* #endif */
                /* write(packet_fd,(void *)pt_trace_buf, bound_snapshot); */
                /* *p_pt_trace_off = 0; */
                /* while(write(packet_fd, "\n=======\n", 9)!=9)break; */
                 /* memset(pt_trace_buf, 0,1<<12);  */
                cursor_pos = 0;
                curr_ip = 0;
                last_ip = 0;
                curr_tnt_prod = 0;
                __atomic_clear(&worker_not_done, __ATOMIC_SEQ_CST);
            }
        }else{
            if(bound_snapshot < cursor_pos){
#ifdef DEBUG
                snprintf(msg, 256, "Current offset %d:%lx:%lx:%lx:%lx\n", proxy_cur_state, (unsigned long)pt_trace_buf, (unsigned long)cursor_pos,(unsigned long)*p_pt_trace_off, (unsigned long)bound_snapshot);
                write(off_fd, msg, strlen(msg));
#endif
                /* PFATAL("not good");    */
            }
            pt_parse_packet((char*)(pt_trace_buf+cursor_pos), bound_snapshot-cursor_pos, packet_fd, off_fd);
            cursor_pos = bound_snapshot;
        }
    }
#ifdef DEBUG
    close(off_fd);
#endif
#ifdef DEBUG_PACKET
    close(packet_fd);
#endif
}


void start_pt_parser(){
    pthread_t worker;
    pthread_create(&worker, NULL, pt_parse_worker, NULL);
}


/*possible types of msg received from PT module*/
static enum msg_type msg_type(char * msg){

	if(strstr(msg, PT_START_CONFIRM))
		return START_CONFIRM;

	if(strstr(msg, PT_TARGET_CONFIRM))
		return TARGET_CONFIRM;

	if(strstr(msg, PT_TOPA_READY))
		return TOPA_RDY;

	if(strstr(msg, PT_BUF_NEXT))
      return PTNEXT;

	return ERROR;
}


void netlink_init(){
  sock_fd=socket(PF_NETLINK, SOCK_RAW, NETLINK_USER);
  if(sock_fd<0)
    PFATAL("netlink start failed");

  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = getpid(); /* self pid */

  bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	
  memset(&dest_addr, 0, sizeof(dest_addr));
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0; /* for kernel */
  dest_addr.nl_groups = 0; /* unicast */
}

void proxy_send_msg(char *msg){

  /* printf("Sent message payload: %s\n", msg); */
  nlh = (struct nlmsghdr *)malloc(NLMSG_LENGTH(MAX_PAYLOAD));
  memset(nlh, 0, NLMSG_LENGTH(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_LENGTH(MAX_PAYLOAD);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  strncpy(NLMSG_DATA(nlh), msg, MAX_PAYLOAD);

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  nl_msg.msg_name = (void *)&dest_addr;
  nl_msg.msg_namelen = sizeof(dest_addr);
  nl_msg.msg_iov = &iov;
  nl_msg.msg_iovlen = 1;

  /* printf("Sending message to kernel\n"); */
  sendmsg(sock_fd,&nl_msg,0);

}

void proxy_recv_msg(){
  char msg[MAX_PAYLOAD];
  /* printf("Waiting for message from kernel\n"); */
  /* Read message from kernel */
  recvmsg(sock_fd, &nl_msg, 0);
  printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));

  strncpy(msg, (const char *) NLMSG_DATA(nlh), MAX_PAYLOAD);

  switch(msg_type(msg)){

    case START_CONFIRM:
      if(proxy_cur_state != PROXY_SLEEP)
        PFATAL("proxy is not on sleep state");
      proxy_cur_state = PROXY_START;
      break;

    case TARGET_CONFIRM:
      if(proxy_cur_state != PROXY_START)
        PFATAL("proxy is not on start state");
      proxy_cur_state = PROXY_FORKSRV;
      break;

    case TOPA_RDY:
      if(proxy_cur_state != PROXY_FORKSRV)
        PFATAL("proxy is not on forksrv state");

      char *tmp = strstr(msg, DEM)+1;//tmp->addr:size:addr
      p_pt_trace_off = (s64 *)strtol(strstr(strstr(tmp,DEM)+1, DEM)+1, NULL, 16);
      strstr(strstr(tmp, DEM)+1, DEM)[0] = '\0';//tmp->addr:size
      pt_trace_buf_size = strtol(strstr(tmp, DEM)+1, NULL, 16);
      strstr(tmp, DEM)[0] = '\0';//tmp->addr
      pt_trace_buf = strtol(tmp, NULL, 16);
      proxy_cur_state = PROXY_FUZZ_RDY;
      assert((pt_trace_buf > 0 && pt_trace_buf_size > 0 && p_pt_trace_off > 0)
             &&"invalid trace buffer and size" );
      break;

    case PTNEXT:
     if(proxy_cur_state != PROXY_FUZZ_ING)
         PFATAL("proxy is not on fuzzing state");

      pt_trace_off_bound = strtol(strstr(msg, DEM)+1, NULL, 16);
      break;


    case ERROR:
      WARNF("got error message from pt-module");
      break;

    default:
      PFATAL("unknown netlink message");
  }
}


void netlink_close(){
  close(sock_fd);
}


static void __afl_map_shm(void) {

  u8 *id_str = getenv(SHM_ENV_VAR);

  /* If we're running under AFL, attach to the appropriate region, replacing the
     early-stage __afl_area_initial region that is needed to allow some really
     hacky .init code to work correctly in projects such as OpenSSL. */

  if (id_str) {

    u32 shm_id = atoi(id_str);

    __afl_area_ptr = shmat(shm_id, NULL, 0);

    /* Whooooops. */

    if (__afl_area_ptr == (void *)-1) _exit(1);

  }

}

static void __afl_proxy_loop(void) {

  static u8 tmp[4];
  s32 child_pid;


  packet_fd = open("/tmp/packet.log", O_RDWR);
  off_fd = open("/tmp/test.log", O_RDWR);
  while (1) {

    u32 was_killed;
    int status;

    /* Wait for parent by reading from the pipe. Abort if read fails. */
    if (read(FORKSRV_FD, &was_killed, 4) != 4) _exit(1);
    else{
      /* state transition: PROXY_FUZZ_STOP -> PROXY_FUZZ_RDY */
      if (proxy_cur_state == PROXY_FUZZ_STOP)
        proxy_cur_state = PROXY_FUZZ_RDY;
    }
    /* write to target about was_killed*/
    if (write(proxy_ctl_fd, &was_killed, 4) != 4) _exit(1);


    /* one-time state transition: PROXY_FORKSRV -> PROXY_FUZZ_RDY */
    if (proxy_cur_state == PROXY_FORKSRV){
        proxy_recv_msg();
        __atomic_test_and_set(&worker_done, __ATOMIC_SEQ_CST);
        __atomic_test_and_set(&worker_not_done, __ATOMIC_SEQ_CST);
        /* start_pt_parser(); */
    }


    /* Wait for target  by reading from the pipe. Abort if read fails. */
    if (read(proxy_st_fd, &child_pid, 4) != 4) _exit(1);
    else{
      /* state transition: PROXY_FUZZ_RDY -> PROXY_FUZZ_ING */
        if (proxy_cur_state != PROXY_FUZZ_RDY)
            PFATAL("proxy is not on fuzz_ready state");
        proxy_cur_state = PROXY_FUZZ_ING;
    }
    /* write to parent about child_pid*/
    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4) _exit(1);




    /* in PROXY_FUZZ_ING state, decode pt buffer and write to trace_bits */
    u64 cursor_pos = 0;
    u64 bound_snapshot = 0;
    cursor_pos = 0;
    curr_ip = 0;
    last_ip = 0;
    curr_tnt_prod = 0;

    /* Wait for target to report child status. Abort if read fails. */
    if (read(proxy_st_fd, &status, 4) != 4) _exit(1);
    else{
      /* state transition: PROXY_FUZZ_ING -> PROXY_FUZZ_STOP */
        if (proxy_cur_state != PROXY_FUZZ_ING)
         PFATAL("proxy is not on fuzzing state");
        proxy_cur_state = PROXY_FUZZ_STOP;
        /* __atomic_clear(&worker_done, __ATOMIC_SEQ_CST); */
        //blocking here until worker is done
        //only when worker_not_done is 0, the atomic return false
        /* while(__atomic_test_and_set(&worker_not_done, __ATOMIC_SEQ_CST)); */
    }

    /* we can parse the pt packet and present it to the trace_bits here*/
    bound_snapshot = *p_pt_trace_off;
    pt_parse_packet((char*)(pt_trace_buf), bound_snapshot, packet_fd, off_fd);
    /* __afl_area_ptr[2424] = 1; */
    /* __afl_area_ptr[2433] = 1; */
    /* __afl_area_ptr[2429] = 1; */

    /* Relay wait status to parent, then loop back. */
    if (write(FORKSRV_FD + 1, &status, 4) != 4) _exit(1);

  }
  close(packet_fd);

}

int main(int argc, char *argv[])
{

  static u8 tmp[4];
  int proxy_st_pipe[2], proxy_ctl_pipe[2];

  /* Generally useful file descriptor. */

  dev_urandom_fd = open("/dev/urandom", O_RDONLY);
  if (dev_urandom_fd < 0) PFATAL("Unable to open /dev/urandom");

  gen_rand_map(TWO_BYTE_ENTRIES, MAP_SIZE);

  /* setting up share memory bitmap */
  __afl_map_shm();

  char **new_argv = ck_alloc(sizeof(char *) * argc);
  memcpy(new_argv, argv + 1, sizeof(char *) * (argc - 1));
  new_argv[argc-1] = 0;
  target_path = new_argv[0];


  /* initialize netlink communication channel */
  netlink_init();

  /* state transition: PROXY_SLEEP -> PROXY_START */
  proxy_send_msg(PT_START);
  proxy_recv_msg();


  /* state transition: PROXY_START -> PROXY_FORKSRV */
  char target_prefix[] = PT_TARGET DEM;
  char *target_msg = (char *)malloc(strlen(target_prefix) + strlen(target_path) +1);
  strcpy(target_msg, target_prefix);
  strcat(target_msg, target_path);
  proxy_send_msg(target_msg);
  proxy_recv_msg();



  printf("proxy starting %s", new_argv[0]);


  /*setup fds, fork and exec target program*/
  if (pipe(proxy_st_pipe) || pipe(proxy_ctl_pipe)) PFATAL("pipe() failed");

  forksrv_pid = fork();
  if(forksrv_pid < 0) PFATAL("fork() failed");
  if(!forksrv_pid){

    /*child will have ctl[0] to read and st[1] to write*/
    if (dup2(proxy_ctl_pipe[0], AFLPT_FORKSRV_FD) < 0) PFATAL("dup2() failed");
    if (dup2(proxy_st_pipe[1], AFLPT_FORKSRV_FD + 1) < 0) PFATAL("dup2() failed");
    close(proxy_ctl_pipe[0]);
    close(proxy_ctl_pipe[1]);
    close(proxy_st_pipe[0]);
    close(proxy_st_pipe[1]);
    close(dev_urandom_fd);
    netlink_close();

    execv(target_path, new_argv);
    exit(0);

  }

  /*parent will have ctl[1] to write and st[0] to read*/
  close(proxy_ctl_pipe[0]);
  close(proxy_st_pipe[1]);
  proxy_ctl_fd = proxy_ctl_pipe[1];
  proxy_st_fd = proxy_st_pipe[0];


  /*wait for target's call to make sure it is online*/
  if (read(proxy_st_fd, tmp, 4) != 4) _exit(1);

  /* Phone home and tell the parent that we're OK. If parent isn't there,
     proxy just simply exits. */
  if (write(FORKSRV_FD + 1, tmp, 4) != 4) _exit(1);

  __afl_proxy_loop();

  return 0;
}

