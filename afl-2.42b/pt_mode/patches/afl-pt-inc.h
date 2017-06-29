#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include "../../config.h"
/*
This header should be included in proxy and rtld.c(ld.so)
 */

/*we use this additional AFLPT_# AFLPT_#+1 pair to communicate with proxy*/
#define AFLPT_FORKSRV_FD (FORKSRV_FD - 3)

/*only programs started by proxy will have this envp set*/
#define AFLPT_EN "__AFLPT_ENABLE"


#define AFLPT_RTLD_SNIPPET do {                   \
      __pt_start_forkserver();                    \
}while(0)


/* Fork server logic, invoked before we return from _dl_start. */
static void __pt_start_forkserver(void) {

  static u8 tmp[4];

  /* Tell the parent that we're alive. If the parent is not ready,
     we should exit the program */

  pid_t child_pid;
  if (write(AFLPT_FORKSRV_FD + 1, tmp, 4) != 4) _exit(1);

  /* All right, let's await orders... */

  while (1) {

    pid_t child_pid;
    int status;

    /* Whoops, parent dead? */

    if (read(AFLPT_FORKSRV_FD, tmp, 4) != 4) _exit(2);

    // child_pid = INLINE_SYSCALL (clone, 5,						      \
    //                 CLONE_CHILD_SETTID | CLONE_CHILD_CLEARTID | SIGCHLD, 0,     \
    //                 NULL, NULL, &THREAD_SELF->tid);
    child_pid = INLINE_SYSCALL (fork, 0);
    if (child_pid < 0) _exit(4);

    if (!child_pid) {

      /* Child process. Close descriptors and run free. */

      close(AFLPT_FORKSRV_FD);
      close(AFLPT_FORKSRV_FD + 1);
      return;

    }

    /* Parent. */

    if (write(AFLPT_FORKSRV_FD + 1, &child_pid, 4) != 4) _exit(5);

    /* Get and relay exit status to proxy. */

    if (waitpid(child_pid, &status, 0) < 0) _exit(6);
    if (write(AFLPT_FORKSRV_FD + 1, &status, 4) != 4) _exit(7);

  }

}
