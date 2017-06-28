#define bdflush RENAMED_bdflush
#include <errno.h>
#include <shlib-compat.h>
#undef bdflush
long int _no_syscall (void)
{ __set_errno (ENOSYS); return -1L; }
weak_alias (_no_syscall, bdflush)
stub_warning (bdflush)
weak_alias (_no_syscall, __GI_bdflush)
