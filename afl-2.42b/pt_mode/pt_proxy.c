#include <sys/socket.h>
#include <linux/netlink.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "../config.h"
#include "../types.h"
#include "../debug.h"
#include "../alloc-inl.h"

void netlink_stuff();

/*we use this additional AFLPT_# AFLPT_#+1 pair to communicate with proxy*/
#define AFLPT_FORKSRV_FD (FORKSRV_FD - 3)

/*NETLINK data structures*/
#define NETLINK_USER 31
#define MAX_PAYLOAD 1024 /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;


/*AFL data structures*/
u8  __afl_area_initial[MAP_SIZE];
u8* __afl_area_ptr = __afl_area_initial;
static s32 proxy_ctl_fd, /*target fork server control pipe(write)*/
  proxy_st_fd;            /*target fork server status pipe(read)*/


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

    /* Write something into the bitmap so that even with low AFL_INST_RATIO,
       our parent doesn't give up on us. */

    __afl_area_ptr[0] = 1;

  }

}

static void __afl_proxy_loop(void) {

  static u8 tmp[4];
  s32 child_pid;


  while (1) {

    u32 was_killed;
    int status;

    /* Wait for parent by reading from the pipe. Abort if read fails. */
    if (read(FORKSRV_FD, &was_killed, 4) != 4) _exit(1);
    /* write to target about was_killed*/
    if (write(proxy_ctl_fd, &was_killed, 4) != 4) _exit(1);


    /* Wait for target  by reading from the pipe. Abort if read fails. */
    if (read(proxy_st_fd, &child_pid, 4) != 4) _exit(1);
    /* write to parent about child_pid*/
    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4) _exit(1);


    /* Wait for target to report child status. Abort if read fails. */
    if (read(proxy_st_fd, &status, 4) != 4) _exit(1);

    /* we can parse the pt packet and present it to the trace_bits here*/
    __afl_area_ptr[2424] = 1;
    __afl_area_ptr[2433] = 1;
    __afl_area_ptr[2429] = 1;


    /* Relay wait status to parent, then loop back. */
    if (write(FORKSRV_FD + 1, &status, 4) != 4) _exit(1);

  }

}

int main(int argc, char *argv[])
{

  static u8 tmp[4];
  static u8 *target_path;
  static s32 proxy_pid;
  int proxy_st_pipe[2], proxy_ctl_pipe[2];

  __afl_map_shm();

  char **new_argv = ck_alloc(sizeof(char *) * argc);
  memcpy(new_argv, argv + 1, sizeof(char *) * (argc - 1));
  new_argv[argc-1] = 0;
  target_path = new_argv[0];

  printf("proxy starting %s", new_argv[0]);


  /*setup fds, fork and exec target program*/
  if (pipe(proxy_st_pipe) || pipe(proxy_ctl_pipe)) PFATAL("pipe() failed");

  proxy_pid = fork();
  if(proxy_pid < 0) PFATAL("fork() failed");
  if(!proxy_pid){
    /*child will have ctl[0] to read and st[1] to write*/
    if (dup2(proxy_ctl_pipe[0], AFLPT_FORKSRV_FD) < 0) PFATAL("dup2() failed");
    if (dup2(proxy_st_pipe[1], AFLPT_FORKSRV_FD + 1) < 0) PFATAL("dup2() failed");
    close(proxy_ctl_pipe[0]);
    close(proxy_ctl_pipe[1]);
    close(proxy_st_pipe[0]);
    close(proxy_st_pipe[1]);

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

void netlink_stuff(){

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
  dest_addr.nl_pid = 0; /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

  nlh = (struct nlmsghdr *)malloc(NLMSG_LENGTH(MAX_PAYLOAD));
  memset(nlh, 0, NLMSG_LENGTH(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_LENGTH(MAX_PAYLOAD);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  strcpy(NLMSG_DATA(nlh), "START");

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  printf("Sending message to kernel\n");
  sendmsg(sock_fd,&msg,0);
  printf("Waiting for message from kernel\n");

  /* Read message from kernel */
  recvmsg(sock_fd, &msg, 0);
  printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));

  memset(nlh, 0, NLMSG_LENGTH(MAX_PAYLOAD));
  nlh->nlmsg_len = NLMSG_LENGTH(MAX_PAYLOAD);
  nlh->nlmsg_pid = getpid();
  nlh->nlmsg_flags = 0;

  strcpy(NLMSG_DATA(nlh), "TARGET:/bin/ls");

  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  printf("Sending message to kernel\n");
  sendmsg(sock_fd,&msg,0);
  printf("Waiting for message from kernel\n");

  /* Read message from kernel */
  recvmsg(sock_fd, &msg, 0);
  printf("Received message payload: %s\n", (char *)NLMSG_DATA(nlh));


  close(sock_fd);
}

